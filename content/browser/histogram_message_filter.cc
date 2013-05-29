// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/histogram_message_filter.h"

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/metrics/statistics_recorder.h"
#include "base/process_util.h"
#include "content/browser/histogram_controller.h"
#include "content/browser/tcmalloc_internals_request_job.h"
#include "content/common/child_process_messages.h"
#include "content/public/common/content_switches.h"

namespace content {

HistogramMessageFilter::HistogramMessageFilter() {}

void HistogramMessageFilter::OnChannelConnected(int32 peer_pid) {
  BrowserMessageFilter::OnChannelConnected(peer_pid);
}

bool HistogramMessageFilter::OnMessageReceived(const IPC::Message& message,
                                              bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(HistogramMessageFilter, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(ChildProcessHostMsg_ChildHistogramData,
                        OnChildHistogramData)
    IPC_MESSAGE_HANDLER(ChildProcessHostMsg_GetBrowserHistogram,
                        OnGetBrowserHistogram)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()
  return handled;
}

HistogramMessageFilter::~HistogramMessageFilter() {}

void HistogramMessageFilter::OnChildHistogramData(
    int sequence_number,
    const std::vector<std::string>& pickled_histograms) {
  HistogramController::GetInstance()->OnHistogramDataCollected(
      sequence_number, pickled_histograms);
}

void HistogramMessageFilter::OnGetBrowserHistogram(
    const std::string& name,
    std::string* histogram_json) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  // Security: Only allow access to browser histograms when running in the
  // context of a test.
  bool using_stats_collection_controller =
      CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kStatsCollectionController);
  bool reduced_security =
      CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kReduceSecurityForStatsCollectionTests);

  if (!using_stats_collection_controller || !reduced_security) {
    LOG(ERROR) << "Attempt at reading browser histogram without specifying "
               << "--" << switches::kStatsCollectionController << " and "
               << "--" << switches::kReduceSecurityForStatsCollectionTests
               << " switches.";
    return;
  }
  base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(name);
  if (!histogram) {
    *histogram_json = "{}";
  } else {
    histogram->WriteJSON(histogram_json);
  }
}

}  // namespace content
