// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_ANDROID_IMPL_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_ANDROID_IMPL_H_

#include <string>

#include "base/compiler_specific.h"
#include "chrome/test/chromedriver/chrome/chrome_impl.h"

class DevToolsHttpClient;

class ChromeAndroidImpl : public ChromeImpl {
 public:
  ChromeAndroidImpl(
      scoped_ptr<DevToolsHttpClient> client,
      const std::string& version,
      int build_no,
      ScopedVector<DevToolsEventListener>& devtools_event_listeners,
      Log* log);
  virtual ~ChromeAndroidImpl();

  // Overridden from Chrome:
  virtual std::string GetOperatingSystemName() OVERRIDE;
  virtual Status Quit() OVERRIDE;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_ANDROID_IMPL_H_
