// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "net/base/mock_host_resolver.h"

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, CrossOriginXHR) {
  host_resolver()->AddRule("*.com", "127.0.0.1");
  ASSERT_TRUE(StartHTTPServer());
  ASSERT_TRUE(RunExtensionTest("cross_origin_xhr")) << message_;
}
