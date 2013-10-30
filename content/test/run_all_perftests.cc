// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/unittest_test_suite.h"
#include "content/test/content_test_suite.h"

int main(int argc, char** argv) {
  content::UnitTestTestSuite test_suite(
      new content::ContentTestSuite(argc, argv));

  // Always run the perf tests serially, to avoid distorting
  // perf measurements with randomness resulting from running
  // in parallel.
  return test_suite.Run();
}
