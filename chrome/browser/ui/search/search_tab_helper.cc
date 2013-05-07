// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/search/search_tab_helper.h"

#include "chrome/browser/search/search.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SearchTabHelper);

namespace {

bool IsNTP(const content::WebContents* contents) {
  // We can't use WebContents::GetURL() because that uses the active entry,
  // whereas we want the visible entry.
  const content::NavigationEntry* entry =
      contents->GetController().GetVisibleEntry();
  if (entry && entry->GetVirtualURL() == GURL(chrome::kChromeUINewTabURL))
    return true;

  return chrome::IsInstantNTP(contents);
}

bool IsSearchResults(const content::WebContents* contents) {
  return !chrome::GetSearchTerms(contents).empty();
}

}  // namespace

SearchTabHelper::SearchTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      is_search_enabled_(chrome::IsInstantExtendedAPIEnabled()),
      user_input_in_progress_(false),
      popup_is_open_(false),
      user_text_is_empty_(true),
      web_contents_(web_contents) {
  if (!is_search_enabled_)
    return;

  registrar_.Add(
      this,
      content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::Source<content::NavigationController>(
          &web_contents->GetController()));
}

SearchTabHelper::~SearchTabHelper() {
}

void SearchTabHelper::OmniboxEditModelChanged(bool user_input_in_progress,
                                              bool cancelling,
                                              bool popup_is_open,
                                              bool user_text_is_empty) {
  if (!is_search_enabled_)
    return;

  user_input_in_progress_ = user_input_in_progress;
  popup_is_open_ = popup_is_open;
  user_text_is_empty_ = user_text_is_empty;
  if (!user_input_in_progress && !cancelling)
    return;

  UpdateMode();
}

void SearchTabHelper::NavigationEntryUpdated() {
  if (!is_search_enabled_)
    return;

  UpdateMode();
}

void SearchTabHelper::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_NAV_ENTRY_COMMITTED, type);
  UpdateMode();
}

bool SearchTabHelper::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(SearchTabHelper, message)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_SearchBoxShowBars,
                        OnSearchBoxShowBars)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_SearchBoxHideBars,
                        OnSearchBoxHideBars)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void SearchTabHelper::UpdateMode() {
  SearchMode::Type type = SearchMode::MODE_DEFAULT;
  SearchMode::Origin origin = SearchMode::ORIGIN_DEFAULT;
  if (IsNTP(web_contents_)) {
    type = SearchMode::MODE_NTP;
    origin = SearchMode::ORIGIN_NTP;
  } else if (IsSearchResults(web_contents_)) {
    type = SearchMode::MODE_SEARCH_RESULTS;
    origin = SearchMode::ORIGIN_SEARCH;
  }
  if (user_input_in_progress_)
    type = SearchMode::MODE_SEARCH_SUGGESTIONS;

  if (type == SearchMode::MODE_NTP && origin == SearchMode::ORIGIN_NTP &&
      !popup_is_open_ && !user_text_is_empty_) {
    // We're switching back (|popup_is_open_| is false) to an NTP (type and
    // mode are |NTP|) with suggestions (|user_text_is_empty_| is false), don't
    // modify visibility of top bars.  This specific omnibox state is set when
    // OmniboxEditModelChanged() is called from
    // OmniboxEditModel::SetInputInProgress() which is called from
    // OmniboxEditModel::Revert().
    model_.SetState(SearchModel::State(SearchMode(type, origin),
                                       model_.state().top_bars_visible));
  } else {
    model_.SetMode(SearchMode(type, origin));
  }
}

void SearchTabHelper::OnSearchBoxShowBars(int page_id) {
  if (web_contents()->IsActiveEntry(page_id))
    model_.SetTopBarsVisible(true);
}

void SearchTabHelper::OnSearchBoxHideBars(int page_id) {
  if (web_contents()->IsActiveEntry(page_id)) {
    model_.SetTopBarsVisible(false);
    Send(new ChromeViewMsg_SearchBoxBarsHidden(routing_id()));
  }
}
