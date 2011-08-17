// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_observer.h"

#include "base/metrics/histogram.h"
#include "base/string_number_conversions.h"
#include "base/time.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/view_messages.h"

namespace prerender {

namespace {

const int kMinHoverThresholdsMs[] = {
  50,
  75,
  100,
  150,
  200,
  250,
  300,
  400,
  500,
  750,
  1000,
  1500,
  2000,
  3000,
  4000,
  5000
};

// Overview of hover-related histograms:
// -- count of hover start
// -- count of hover too short.
// difference between these is pages prerendered
// -- count of prerender used
// -- time from hover commence until click (subtract hover threshold
// from that value to figure out average savings).

enum HoverEvents {
  HOVER_EVENT_START = 0,
  HOVER_EVENT_TOO_SHORT = 1,
  HOVER_EVENT_REPLACED = 2,
  HOVER_EVENT_CLICK = 3,
  HOVER_EVENT_MAX
};

const int kNumHoverThresholds = arraysize(kMinHoverThresholdsMs);

class PerHoverThresholdHistograms {
 public:
  explicit PerHoverThresholdHistograms(int hover_threshold_ms) {
    std::string prefix = std::string("Prerender.HoverStats_") +
        base::IntToString(hover_threshold_ms) + std::string("_");
    count_hover_events_ = base::LinearHistogram::FactoryGet(
        prefix + std::string("Events"),
        1,
        HOVER_EVENT_MAX,
        HOVER_EVENT_MAX + 1,
        base::Histogram::kUmaTargetedHistogramFlag);
    time_hover_until_click_ = base::Histogram::FactoryTimeGet(
        prefix + std::string("TimeToClick"),
        base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromSeconds(60),
        100,
        base::Histogram::kUmaTargetedHistogramFlag);
  }
  base::Histogram* count_hover_events() { return count_hover_events_; }
  base::Histogram* time_hover_until_click() { return time_hover_until_click_; }

 private:
  base::Histogram* count_hover_events_;
  base::Histogram* time_hover_until_click_;
};

}  // namespace

class PrerenderObserver::HoverData {
 public:
  void SetHoverThreshold(int threshold_ms) {
    hover_threshold_ = base::TimeDelta::FromMilliseconds(threshold_ms);
    histograms_.reset(new PerHoverThresholdHistograms(threshold_ms));
  }

  void RecordHover(const GURL& url) {
    VerifyInitialized();

    MaybeStartedPrerendering();

    // Ignore duplicate hover messages.
    if (new_url_ == url)
      return;

    if (!new_url_.is_empty())
      histograms_->count_hover_events()->Add(HOVER_EVENT_TOO_SHORT);

    if (current_url_ == url) {
      // If we came back to URL currently being prerendered, we don't need
      // to "remember" to start prerendering, because we already have that
      // URL prerendered.
      new_url_ = GURL();
    } else {
      // Otherwise, we remember this new URL as something we might soon
      // start prerendering.
      new_url_ = url;
      new_time_ = base::TimeTicks::Now();
    }

    if (!new_url_.is_empty())
      histograms_->count_hover_events()->Add(HOVER_EVENT_START);
  }

  void RecordNavigation(const GURL& url) {
    VerifyInitialized();

    // Artifically call RecordHover with an empty URL to accomplish two things:
    // -- ensure we update what we are currently prerendering
    // -- record if there is a pending new hover, that we "exited" it,
    //    thereby recording an EVENT_TOO_SHORT for it.
    RecordHover(GURL());

    if (current_url_ == url) {
        // Match.  We would have started prerendering the new URL.
        histograms_->count_hover_events()->Add(HOVER_EVENT_CLICK);
        histograms_->time_hover_until_click()->AddTime(
            base::TimeTicks::Now() - current_time_);
        current_url_ = GURL();
    }
  }

 private:
  void VerifyInitialized() {
    DCHECK(histograms_.get() != NULL);
  }

  void MaybeStartedPrerendering() {
    if (!new_url_.is_empty() &&
        base::TimeTicks::Now() - new_time_ > hover_threshold_) {
      if (!current_url_.is_empty())
        histograms_->count_hover_events()->Add(HOVER_EVENT_REPLACED);
      current_url_ = new_url_;
      current_time_ = new_time_;
      new_url_ = GURL();
    }
  }

  scoped_ptr<PerHoverThresholdHistograms> histograms_;

  // The time & url of the hover that is currently being prerendered.
  base::TimeTicks current_time_;
  GURL current_url_;

  // The time & url of a new hover that does not meet the threshold for
  // prerendering and may replace the prerender currently being prerendered.
  base::TimeTicks new_time_;
  GURL new_url_;

  // The threshold for hovering.
  base::TimeDelta hover_threshold_;
};

PrerenderObserver::PrerenderObserver(TabContentsWrapper* tab)
    : TabContentsObserver(tab->tab_contents()),
      tab_(tab),
      pplt_load_start_(),
      last_hovers_(new HoverData[kNumHoverThresholds]) {
  for (int i = 0; i < kNumHoverThresholds; i++)
    last_hovers_[i].SetHoverThreshold(kMinHoverThresholdsMs[i]);
}

PrerenderObserver::~PrerenderObserver() {
}

void PrerenderObserver::ProvisionalChangeToMainFrameUrl(const GURL& url,
                                                        bool has_opener_set) {
  if (!tab_->delegate())
    return;  // PrerenderManager needs a delegate to handle the swap.
  PrerenderManager* prerender_manager = MaybeGetPrerenderManager();
  if (!prerender_manager)
    return;
  if (prerender_manager->IsTabContentsPrerendering(tab_contents()))
    return;
  prerender_manager->MarkTabContentsAsNotPrerendered(tab_contents());
  MaybeUsePrerenderedPage(url, has_opener_set);
}

bool PrerenderObserver::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(PrerenderObserver, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DidStartProvisionalLoadForFrame,
                        OnDidStartProvisionalLoadForFrame)
    IPC_MESSAGE_HANDLER(ViewHostMsg_UpdateTargetURL, OnMsgUpdateTargetURL)
  IPC_END_MESSAGE_MAP()
  return false;
}

void PrerenderObserver::OnDidStartProvisionalLoadForFrame(int64 frame_id,
                                                          bool is_main_frame,
                                                          bool has_opener_set,
                                                          const GURL& url) {
  if (is_main_frame) {
    // Record the beginning of a new PPLT navigation.
    pplt_load_start_ = base::TimeTicks::Now();

    // Update hover stats.
    for (int i = 0; i < kNumHoverThresholds; i++)
      last_hovers_[i].RecordNavigation(url);

    MaybeLogCurrentHover(current_hover_url_ == url);
  }
}

void PrerenderObserver::OnMsgUpdateTargetURL(int32 page_id, const GURL& url) {
  for (int i = 0; i < kNumHoverThresholds; i++)
    last_hovers_[i].RecordHover(url);

  if (url != current_hover_url_) {
    MaybeLogCurrentHover(false);
    current_hover_url_ = url;
    current_hover_time_ = base::TimeTicks::Now();
  }
}

void PrerenderObserver::DidStopLoading() {
  // Don't include prerendered pages in the PPLT metric until after they are
  // swapped in.

  // Compute the PPLT metric and report it in a histogram, if needed.
  if (!pplt_load_start_.is_null() && !IsPrerendering()) {
    PrerenderManager::RecordPerceivedPageLoadTime(
        base::TimeTicks::Now() - pplt_load_start_, tab_contents());
  }

  // Reset the PPLT metric.
  pplt_load_start_ = base::TimeTicks();
}

PrerenderManager* PrerenderObserver::MaybeGetPrerenderManager() {
  Profile* profile =
      Profile::FromBrowserContext(tab_contents()->browser_context());
  return profile->GetPrerenderManager();
}

bool PrerenderObserver::MaybeUsePrerenderedPage(const GURL& url,
                                                bool has_opener_set) {
  PrerenderManager* prerender_manager = MaybeGetPrerenderManager();
  if (!prerender_manager)
    return false;
  DCHECK(!prerender_manager->IsTabContentsPrerendering(tab_contents()));
  return prerender_manager->MaybeUsePrerenderedPage(tab_contents(),
                                                    url,
                                                    has_opener_set);
}

bool PrerenderObserver::IsPrerendering() {
  PrerenderManager* prerender_manager = MaybeGetPrerenderManager();
  if (!prerender_manager)
    return false;
  return prerender_manager->IsTabContentsPrerendering(tab_contents());
}

void PrerenderObserver::PrerenderSwappedIn() {
  // Ensure we are not prerendering any more.
  DCHECK(!IsPrerendering());
  if (pplt_load_start_.is_null()) {
    // If we have already finished loading, report a 0 PPLT.
    PrerenderManager::RecordPerceivedPageLoadTime(base::TimeDelta(),
                                                  tab_contents());
  } else {
    // If we have not finished loading yet, rebase the start time to now.
    pplt_load_start_ = base::TimeTicks::Now();
  }
}

void PrerenderObserver::MaybeLogCurrentHover(bool was_used) {
  if (current_hover_url_.is_empty())
    return;

  static const int64 min_ms = 0;
  static const int64 max_ms = 2000;
  static const int num_buckets = 100;

  int64 elapsed_ms =
      (base::TimeTicks::Now() - current_hover_time_).InMilliseconds();

  elapsed_ms = std::min(std::max(elapsed_ms, min_ms), max_ms);
  elapsed_ms /= max_ms / num_buckets;

  if (was_used) {
    UMA_HISTOGRAM_ENUMERATION("Prerender.HoverStats_TimeUntilClicked",
                              elapsed_ms, num_buckets);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Prerender.HoverStats_TimeUntilDiscarded",
                              elapsed_ms, num_buckets);
  }

  current_hover_url_ = GURL();
}

}  // namespace prerender
