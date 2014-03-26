// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_LOG_UPLOADER_H_
#define COMPONENTS_RAPPOR_LOG_UPLOADER_H_

#include <queue>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
}

namespace rappor {

// Uploads logs from RapporService.  Logs are passed in via QueueLog(), stored
// internally, and uploaded one at a time.  A queued log will be uploaded at a
// fixed interval after the successful upload of the previous logs.  If an
// upload fails, the uploader will keep retrying the upload with an exponential
// backoff interval.
class LogUploader : public net::URLFetcherDelegate {
 public:
  // Constructor takes the |server_url| that logs should be uploaded to, the
  // |mime_type| of the uploaded data, and |request_context| to create uploads
  // with.
  LogUploader(const GURL& server_url,
              const std::string& mime_type,
              net::URLRequestContextGetter* request_context);

  // If the object is destroyed (or the program terminates) while logs are
  // queued, the logs are lost.
  virtual ~LogUploader();

  // Adds an entry to the queue of logs to be uploaded to the server.  The
  // uploader makes no assumptions about the format of |log| and simply sends
  // it verbatim to the server.
  void QueueLog(const std::string& log);

 protected:
  // Checks if an upload has been scheduled.
  virtual bool IsUploadScheduled() const;

  // Schedules a future call to StartScheduledUpload if one isn't already
  // pending.  Can be overridden for testing.
  virtual void ScheduleNextUpload(base::TimeDelta interval);

  // Starts transmission of the next log. Exposed for tests.
  void StartScheduledUpload();

  // Increases the upload interval each time it's called, to handle the case
  // where the server is having issues. Exposed for tests.
  static base::TimeDelta BackOffUploadInterval(base::TimeDelta);

 private:
  // Implements net::URLFetcherDelegate. Called after transmission completes
  // (whether successful or not).
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;

  // Called when the upload is completed.
  void OnUploadFinished(bool server_is_healthy, bool more_logs_remaining);

  // The server URL to upload logs to.
  const GURL server_url_;

  // The mime type to specify on uploaded logs.
  const std::string mime_type_;

  // The request context used to send uploads.
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // The outstanding transmission that appears as a URL Fetch operation.
  scoped_ptr<net::URLFetcher> current_fetch_;

  // The logs that still need to be uploaded.
  std::queue<std::string> queued_logs_;

  // A timer used to delay before attempting another upload.
  base::OneShotTimer<LogUploader> upload_timer_;

  // Indicates that the last triggered upload hasn't resolved yet.
  bool has_callback_pending_;

  // The interval to wait after an upload's URLFetcher completion before
  // starting the next upload attempt.
  base::TimeDelta upload_interval_;

  DISALLOW_COPY_AND_ASSIGN(LogUploader);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_LOG_UPLOADER_H_
