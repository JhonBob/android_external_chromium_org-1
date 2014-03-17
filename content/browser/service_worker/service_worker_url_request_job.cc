// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_url_request_job.h"

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "content/browser/service_worker/service_worker_fetch_dispatcher.h"
#include "content/browser/service_worker/service_worker_provider_host.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"

namespace content {

ServiceWorkerURLRequestJob::ServiceWorkerURLRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    base::WeakPtr<ServiceWorkerProviderHost> provider_host)
    : net::URLRequestJob(request, network_delegate),
      provider_host_(provider_host),
      response_type_(NOT_DETERMINED),
      is_started_(false),
      weak_factory_(this) {
}

void ServiceWorkerURLRequestJob::FallbackToNetwork() {
  DCHECK_EQ(NOT_DETERMINED, response_type_);
  response_type_ = FALLBACK_TO_NETWORK;
  MaybeStartRequest();
}

void ServiceWorkerURLRequestJob::ForwardToServiceWorker() {
  DCHECK_EQ(NOT_DETERMINED, response_type_);
  response_type_ = FORWARD_TO_SERVICE_WORKER;
  MaybeStartRequest();
}

void ServiceWorkerURLRequestJob::Start() {
  is_started_ = true;
  MaybeStartRequest();
}

void ServiceWorkerURLRequestJob::Kill() {
  net::URLRequestJob::Kill();
  fetch_dispatcher_.reset();
  weak_factory_.InvalidateWeakPtrs();
}

net::LoadState ServiceWorkerURLRequestJob::GetLoadState() const {
  // TODO(kinuko): refine this for better debug.
  return net::URLRequestJob::GetLoadState();
}

bool ServiceWorkerURLRequestJob::GetCharset(std::string* charset) {
  if (!http_info())
    return false;
  return http_info()->headers->GetCharset(charset);
}

bool ServiceWorkerURLRequestJob::GetMimeType(std::string* mime_type) const {
  if (!http_info())
    return false;
  return http_info()->headers->GetMimeType(mime_type);
}

void ServiceWorkerURLRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  if (!http_info())
    return;
  *info = *http_info();
}

int ServiceWorkerURLRequestJob::GetResponseCode() const {
  if (!http_info())
    return -1;
  return http_info()->headers->response_code();
}

void ServiceWorkerURLRequestJob::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  std::string range_header;
  std::vector<net::HttpByteRange> ranges;
  if (!headers.GetHeader(net::HttpRequestHeaders::kRange, &range_header) ||
      !net::HttpUtil::ParseRangeHeader(range_header, &ranges)) {
    return;
  }

  // We don't support multiple range requests in one single URL request.
  if (ranges.size() == 1U)
    byte_range_ = ranges[0];
}

bool ServiceWorkerURLRequestJob::ReadRawData(
    net::IOBuffer* buf, int buf_size, int *bytes_read) {
  // TODO(kinuko): Implement this.
  // If the response returned from ServiceWorker had an
  // identifier to on-disk data (e.g. blob or cache entry) we'll need to
  // pull the body from disk.
  NOTIMPLEMENTED();
  *bytes_read = 0;
  return true;
}

const net::HttpResponseInfo* ServiceWorkerURLRequestJob::http_info() const {
  if (!http_response_info_)
    return NULL;
  if (range_response_info_)
    return range_response_info_.get();
  return http_response_info_.get();
}

ServiceWorkerURLRequestJob::~ServiceWorkerURLRequestJob() {
}

void ServiceWorkerURLRequestJob::MaybeStartRequest() {
  if (is_started_ && response_type_ != NOT_DETERMINED) {
    // Start asynchronously.
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ServiceWorkerURLRequestJob::StartRequest,
                   weak_factory_.GetWeakPtr()));
  }
}

void ServiceWorkerURLRequestJob::StartRequest() {
  switch (response_type_) {
    case NOT_DETERMINED:
      NOTREACHED();
      return;

    case FALLBACK_TO_NETWORK:
      // Restart the request to create a new job. Our request handler will
      // return NULL, and the default job (which will hit network) should be
      // created.
      NotifyRestartRequired();
      return;

    case FORWARD_TO_SERVICE_WORKER:
      DCHECK(provider_host_ && provider_host_->associated_version());
      DCHECK(!fetch_dispatcher_);

      // Send a fetch event to the ServiceWorker associated to the
      // provider_host.
      fetch_dispatcher_.reset(new ServiceWorkerFetchDispatcher(
          request(), provider_host_->associated_version(),
          base::Bind(&ServiceWorkerURLRequestJob::DidDispatchFetchEvent,
                     weak_factory_.GetWeakPtr())));
      fetch_dispatcher_->Run();
      return;
  }

  NOTREACHED();
}

void ServiceWorkerURLRequestJob::DidDispatchFetchEvent(
    ServiceWorkerStatusCode status,
    ServiceWorkerFetchEventResult fetch_result,
    const ServiceWorkerResponse& response) {
  fetch_dispatcher_.reset();

  // Check if we're not orphaned.
  if (!request())
    return;

  if (status != SERVICE_WORKER_OK) {
    // Dispatching event has been failed, falling back to the network.
    // (Tentative behavior described on github)
    // TODO(kinuko): consider returning error if we've come here because
    // unexpected worker termination etc (so that we could fix bugs).
    // TODO(kinuko): Would be nice to log the error case.
    response_type_ = FALLBACK_TO_NETWORK;
    NotifyRestartRequired();
  }

  if (fetch_result == SERVICE_WORKER_FETCH_EVENT_RESULT_FALLBACK) {
    // Change the response type and restart the request to fallback to
    // the network.
    response_type_ = FALLBACK_TO_NETWORK;
    NotifyRestartRequired();
    return;
  }

  // We should have response now.
  DCHECK_EQ(SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE, fetch_result);

  CreateResponseHeader(response);
  NotifyHeadersComplete();
}

void ServiceWorkerURLRequestJob::CreateResponseHeader(
    const ServiceWorkerResponse& response) {
  // TODO(kinuko): If the response has an identifier to on-disk cache entry,
  // pull response header from the disk.
  std::string status_line(base::StringPrintf("HTTP/1.1 %d %s",
                                             response.status_code,
                                             response.status_text.c_str()));
  status_line.push_back('\0');
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(status_line));
  for (std::map<std::string, std::string>::const_iterator it =
           response.headers.begin();
       it != response.headers.end(); ++it) {
    std::string header;
    header.reserve(it->first.size() + 2 + it->second.size());
    header.append(it->first);
    header.append(": ");
    header.append(it->second);
    headers->AddHeader(header);
  }

  http_response_info_.reset(new net::HttpResponseInfo());
  http_response_info_->headers = headers;
}

}  // namespace content
