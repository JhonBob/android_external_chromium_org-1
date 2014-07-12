// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_BROWSERTEST_H_
#define CHROME_BROWSER_MEDIA_MEDIA_BROWSERTEST_H_

#include <utility>
#include <vector>

#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class TitleWatcher;
}

// Class used to automate running media related browser tests. The functions
// assume that media files are located under files/media/ folder known to
// the test http server.
class MediaBrowserTest : public InProcessBrowserTest,
                         public content::WebContentsObserver {
 protected:
  typedef std::pair<std::string, std::string> StringPair;

  // Common test results.
  static const char kEnded[];
  // TODO(xhwang): Report detailed errors, e.g. "ERROR-3".
  static const char kError[];
  static const char kFailed[];
  static const char kPluginCrashed[];

  MediaBrowserTest();
  virtual ~MediaBrowserTest();

  // Runs a html page with a list of URL query parameters.
  // If http is true, the test starts a local http test server to load the test
  // page, otherwise a local file URL is loaded inside the content shell.
  // It uses RunTest() to check for expected test output.
  void RunMediaTestPage(const std::string& html_page,
                        std::vector<StringPair>* query_params,
                        const std::string& expected, bool http);

  // Opens a URL and waits for the document title to match either one of the
  // default strings or the expected string.
  base::string16 RunTest(const GURL& gurl, const std::string& expected);

  virtual void AddWaitForTitles(content::TitleWatcher* title_watcher);

  // Fails test and sets document title to kPluginCrashed when a plugin crashes.
  // If IgnorePluginCrash(true) is called then plugin crash is ignored.
  virtual void PluginCrashed(const base::FilePath& plugin_path,
                             base::ProcessId plugin_pid) OVERRIDE;

  // When called, the test will ignore any plugin crashes and not fail the test.
  void IgnorePluginCrash();

 private:
  bool ignore_plugin_crash_;
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_BROWSERTEST_H_
