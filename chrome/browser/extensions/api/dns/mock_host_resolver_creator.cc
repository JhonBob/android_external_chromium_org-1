// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/dns/mock_host_resolver_creator.h"

#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_thread.h"
#include "net/dns/mock_host_resolver.h"

using content::BrowserThread;

namespace extensions {

const std::string MockHostResolverCreator::kHostname = "www.sowbug.com";
const std::string MockHostResolverCreator::kAddress = "9.8.7.6";

MockHostResolverCreator::MockHostResolverCreator()
  : resolver_event_(true, false),
    mock_host_resolver_(NULL) {
}

MockHostResolverCreator::~MockHostResolverCreator() {
}

net::MockHostResolver* MockHostResolverCreator::CreateMockHostResolver() {
  DCHECK(!mock_host_resolver_);
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  bool result = BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&MockHostResolverCreator::CreateMockHostResolverOnIOThread,
                 this));
  DCHECK(result);

  base::TimeDelta max_time = base::TimeDelta::FromSeconds(5);
  resolver_event_.TimedWait(max_time);

  return mock_host_resolver_;
}

void MockHostResolverCreator::CreateMockHostResolverOnIOThread() {
  mock_host_resolver_ = new net::MockHostResolver();
  mock_host_resolver_->rules()->AddRule(kHostname, kAddress);
  mock_host_resolver_->rules()->AddSimulatedFailure("this.hostname.is.bogus");
  resolver_event_.Signal();
}

void MockHostResolverCreator::DeleteMockHostResolver() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!mock_host_resolver_)
    return;
  resolver_event_.Reset();
  bool result = BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&MockHostResolverCreator::DeleteMockHostResolverOnIOThread,
                 this));
  DCHECK(result);

  base::TimeDelta max_time = base::TimeDelta::FromSeconds(5);
  ASSERT_TRUE(resolver_event_.TimedWait(max_time));
}

void MockHostResolverCreator::DeleteMockHostResolverOnIOThread() {
  delete(mock_host_resolver_);
  mock_host_resolver_ = NULL;
  resolver_event_.Signal();
}

}  // namespace extensions
