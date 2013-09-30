// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_POSIX)
#include <signal.h>
#endif

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/ui/app_modal_dialogs/javascript_app_modal_dialog.h"
#include "chrome/browser/ui/app_modal_dialogs/native_app_modal_dialog.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/test/net/url_request_mock_http_job.h"
#include "net/url_request/url_request_test_util.h"

#if defined(OS_WIN)
// For version specific disabled tests below (http://crbug.com/267597).
#include "base/win/windows_version.h"
#endif

using base::TimeDelta;
using content::BrowserThread;

const std::string NOLISTENERS_HTML =
    "<html><head><title>nolisteners</title></head><body></body></html>";

const std::string UNLOAD_HTML =
    "<html><head><title>unload</title></head><body>"
    "<script>window.onunload=function(e){}</script></body></html>";

const std::string BEFORE_UNLOAD_HTML =
    "<html><head><title>beforeunload</title></head><body>"
    "<script>window.onbeforeunload=function(e){"
    "setTimeout('document.title=\"cancelled\"', 0);return 'foo'}</script>"
    "</body></html>";

const std::string INNER_FRAME_WITH_FOCUS_HTML =
    "<html><head><title>innerframewithfocus</title></head><body>"
    "<script>window.onbeforeunload=function(e){return 'foo'}</script>"
    "<iframe src=\"data:text/html,<html><head><script>window.onload="
    "function(){document.getElementById('box').focus()}</script>"
    "<body><input id='box'></input></body></html>\"></iframe>"
    "</body></html>";

const std::string TWO_SECOND_BEFORE_UNLOAD_HTML =
    "<html><head><title>twosecondbeforeunload</title></head><body>"
    "<script>window.onbeforeunload=function(e){"
      "var start = new Date().getTime();"
      "while(new Date().getTime() - start < 2000){}"
      "return 'foo';"
    "}</script></body></html>";

const std::string INFINITE_UNLOAD_HTML =
    "<html><head><title>infiniteunload</title></head><body>"
    "<script>window.onunload=function(e){while(true){}}</script>"
    "</body></html>";

const std::string INFINITE_BEFORE_UNLOAD_HTML =
    "<html><head><title>infinitebeforeunload</title></head><body>"
    "<script>window.onbeforeunload=function(e){while(true){}}</script>"
    "</body></html>";

const std::string INFINITE_UNLOAD_ALERT_HTML =
    "<html><head><title>infiniteunloadalert</title></head><body>"
    "<script>window.onunload=function(e){"
      "while(true){}"
      "alert('foo');"
    "}</script></body></html>";

const std::string INFINITE_BEFORE_UNLOAD_ALERT_HTML =
    "<html><head><title>infinitebeforeunloadalert</title></head><body>"
    "<script>window.onbeforeunload=function(e){"
      "while(true){}"
      "alert('foo');"
    "}</script></body></html>";

const std::string TWO_SECOND_UNLOAD_ALERT_HTML =
    "<html><head><title>twosecondunloadalert</title></head><body>"
    "<script>window.onunload=function(e){"
      "var start = new Date().getTime();"
      "while(new Date().getTime() - start < 2000){}"
      "alert('foo');"
    "}</script></body></html>";

const std::string TWO_SECOND_BEFORE_UNLOAD_ALERT_HTML =
    "<html><head><title>twosecondbeforeunloadalert</title></head><body>"
    "<script>window.onbeforeunload=function(e){"
      "var start = new Date().getTime();"
      "while(new Date().getTime() - start < 2000){}"
      "alert('foo');"
    "}</script></body></html>";

const std::string CLOSE_TAB_WHEN_OTHER_TAB_HAS_LISTENER =
    "<html><head><title>only_one_unload</title></head>"
    "<body onclick=\"window.open('data:text/html,"
    "<html><head><title>popup</title></head></body>')\" "
    "onbeforeunload='return;'>"
    "</body></html>";

class UnloadTest : public InProcessBrowserTest {
 public:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    if (strcmp(test_info->name(),
        "BrowserCloseTabWhenOtherTabHasListener") == 0) {
      command_line->AppendSwitch(switches::kDisablePopupBlocking);
    } else if (strcmp(test_info->name(), "BrowserTerminateBeforeUnload") == 0) {
#if defined(OS_POSIX)
      DisableSIGTERMHandling();
#endif
    }
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
  }

  void CheckTitle(const char* expected_title) {
    string16 expected = ASCIIToUTF16(expected_title);
    EXPECT_EQ(expected,
              browser()->tab_strip_model()->GetActiveWebContents()->GetTitle());
  }

  void NavigateToDataURL(const std::string& html_content,
                         const char* expected_title) {
    ui_test_utils::NavigateToURL(browser(),
                                 GURL("data:text/html," + html_content));
    CheckTitle(expected_title);
  }

  void NavigateToNolistenersFileTwice() {
    GURL url(content::URLRequestMockHTTPJob::GetMockUrl(
        base::FilePath(FILE_PATH_LITERAL("title2.html"))));
    ui_test_utils::NavigateToURL(browser(), url);
    CheckTitle("Title Of Awesomeness");
    ui_test_utils::NavigateToURL(browser(), url);
    CheckTitle("Title Of Awesomeness");
  }

  // Navigates to a URL asynchronously, then again synchronously. The first
  // load is purposely async to test the case where the user loads another
  // page without waiting for the first load to complete.
  void NavigateToNolistenersFileTwiceAsync() {
    GURL url(content::URLRequestMockHTTPJob::GetMockUrl(
        base::FilePath(FILE_PATH_LITERAL("title2.html"))));
    ui_test_utils::NavigateToURLWithDisposition(browser(), url, CURRENT_TAB, 0);
    ui_test_utils::NavigateToURL(browser(), url);
    CheckTitle("Title Of Awesomeness");
  }

  void LoadUrlAndQuitBrowser(const std::string& html_content,
                             const char* expected_title) {
    NavigateToDataURL(html_content, expected_title);
    content::WindowedNotificationObserver window_observer(
        chrome::NOTIFICATION_BROWSER_CLOSED,
        content::NotificationService::AllSources());
    chrome::CloseWindow(browser());
    window_observer.Wait();
  }

  // If |accept| is true, simulates user clicking OK, otherwise simulates
  // clicking Cancel.
  void ClickModalDialogButton(bool accept) {
    AppModalDialog* dialog = ui_test_utils::WaitForAppModalDialog();
    ASSERT_TRUE(dialog->IsJavaScriptModalDialog());
    JavaScriptAppModalDialog* js_dialog =
        static_cast<JavaScriptAppModalDialog*>(dialog);
    if (accept)
      js_dialog->native_dialog()->AcceptAppModalDialog();
    else
      js_dialog->native_dialog()->CancelAppModalDialog();
  }
};

// Navigate to a page with an infinite unload handler.
// Then two async crosssite requests to ensure
// we don't get confused and think we're closing the tab.
//
// This test is flaky on the valgrind UI bots. http://crbug.com/39057
IN_PROC_BROWSER_TEST_F(UnloadTest, CrossSiteInfiniteUnloadAsync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_UNLOAD_HTML, "infiniteunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwiceAsync();
}

// Navigate to a page with an infinite unload handler.
// Then two sync crosssite requests to ensure
// we correctly nav to each one.
IN_PROC_BROWSER_TEST_F(UnloadTest, CrossSiteInfiniteUnloadSync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_UNLOAD_HTML, "infiniteunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwice();
}

// Navigate to a page with an infinite beforeunload handler.
// Then two two async crosssite requests to ensure
// we don't get confused and think we're closing the tab.
// This test is flaky on the valgrind UI bots. http://crbug.com/39057 and
// http://crbug.com/86469
IN_PROC_BROWSER_TEST_F(UnloadTest, CrossSiteInfiniteBeforeUnloadAsync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_BEFORE_UNLOAD_HTML, "infinitebeforeunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwiceAsync();
}

// Navigate to a page with an infinite beforeunload handler.
// Then two two sync crosssite requests to ensure
// we correctly nav to each one.
// If this flakes, see bug http://crbug.com/86469.
IN_PROC_BROWSER_TEST_F(UnloadTest, CrossSiteInfiniteBeforeUnloadSync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_BEFORE_UNLOAD_HTML, "infinitebeforeunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwice();
}

// Tests closing the browser on a page with no unload listeners registered.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseNoUnloadListeners) {
  LoadUrlAndQuitBrowser(NOLISTENERS_HTML, "nolisteners");
}

// Tests closing the browser on a page with an unload listener registered.
// Test marked as flaky in http://crbug.com/51698
IN_PROC_BROWSER_TEST_F(UnloadTest, DISABLED_BrowserCloseUnload) {
  LoadUrlAndQuitBrowser(UNLOAD_HTML, "unload");
}

// Tests closing the browser with a beforeunload handler and clicking
// OK in the beforeunload confirm dialog.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseBeforeUnloadOK) {
  NavigateToDataURL(BEFORE_UNLOAD_HTML, "beforeunload");

  content::WindowedNotificationObserver window_observer(
        chrome::NOTIFICATION_BROWSER_CLOSED,
        content::NotificationService::AllSources());
  chrome::CloseWindow(browser());
  ClickModalDialogButton(true);
  window_observer.Wait();
}

// Tests closing the browser with a beforeunload handler and clicking
// CANCEL in the beforeunload confirm dialog.
// If this test flakes, reopen http://crbug.com/123110
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseBeforeUnloadCancel) {
  NavigateToDataURL(BEFORE_UNLOAD_HTML, "beforeunload");
  chrome::CloseWindow(browser());

  // We wait for the title to change after cancelling the popup to ensure that
  // in-flight IPCs from the renderer reach the browser. Otherwise the browser
  // won't put up the beforeunload dialog because it's waiting for an ack from
  // the renderer.
  string16 expected_title = ASCIIToUTF16("cancelled");
  content::TitleWatcher title_watcher(
      browser()->tab_strip_model()->GetActiveWebContents(), expected_title);
  ClickModalDialogButton(false);
  ASSERT_EQ(expected_title, title_watcher.WaitAndGetTitle());

  content::WindowedNotificationObserver window_observer(
        chrome::NOTIFICATION_BROWSER_CLOSED,
        content::NotificationService::AllSources());
  chrome::CloseWindow(browser());
  ClickModalDialogButton(true);
  window_observer.Wait();
}

// Tests terminating the browser with a beforeunload handler.
// Currently only ChromeOS shuts down gracefully.
#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserTerminateBeforeUnload) {
  NavigateToDataURL(BEFORE_UNLOAD_HTML, "beforeunload");
  EXPECT_EQ(kill(base::GetCurrentProcessHandle(), SIGTERM), 0);
}
#endif

// Tests closing the browser and clicking OK in the beforeunload confirm dialog
// if an inner frame has the focus.
// If this flakes, use http://crbug.com/32615 and http://crbug.com/45675
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseWithInnerFocusedFrame) {
  NavigateToDataURL(INNER_FRAME_WITH_FOCUS_HTML, "innerframewithfocus");

  content::WindowedNotificationObserver window_observer(
        chrome::NOTIFICATION_BROWSER_CLOSED,
        content::NotificationService::AllSources());
  chrome::CloseWindow(browser());
  ClickModalDialogButton(true);
  window_observer.Wait();
}

// Tests closing the browser with a beforeunload handler that takes
// two seconds to run.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseTwoSecondBeforeUnload) {
  LoadUrlAndQuitBrowser(TWO_SECOND_BEFORE_UNLOAD_HTML,
                        "twosecondbeforeunload");
}

// Tests closing the browser on a page with an unload listener registered where
// the unload handler has an infinite loop.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseInfiniteUnload) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  LoadUrlAndQuitBrowser(INFINITE_UNLOAD_HTML, "infiniteunload");
}

// Tests closing the browser with a beforeunload handler that hangs.
// If this flakes, use http://crbug.com/78803 and http://crbug.com/86469
IN_PROC_BROWSER_TEST_F(UnloadTest, DISABLED_BrowserCloseInfiniteBeforeUnload) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  LoadUrlAndQuitBrowser(INFINITE_BEFORE_UNLOAD_HTML, "infinitebeforeunload");
}

// Tests closing the browser on a page with an unload listener registered where
// the unload handler has an infinite loop followed by an alert.
// If this flakes, use http://crbug.com/86469
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseInfiniteUnloadAlert) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  LoadUrlAndQuitBrowser(INFINITE_UNLOAD_ALERT_HTML, "infiniteunloadalert");
}

// Tests closing the browser with a beforeunload handler that hangs then
// pops up an alert.
// If this flakes, use http://crbug.com/78803 and http://crbug.com/86469.
IN_PROC_BROWSER_TEST_F(UnloadTest,
                       DISABLED_BrowserCloseInfiniteBeforeUnloadAlert) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  LoadUrlAndQuitBrowser(INFINITE_BEFORE_UNLOAD_ALERT_HTML,
                        "infinitebeforeunloadalert");
}

// Tests closing the browser on a page with an unload listener registered where
// the unload handler has an 2 second long loop followed by an alert.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseTwoSecondUnloadAlert) {
  LoadUrlAndQuitBrowser(TWO_SECOND_UNLOAD_ALERT_HTML, "twosecondunloadalert");
}

// Tests closing the browser with a beforeunload handler that takes
// two seconds to run then pops up an alert.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseTwoSecondBeforeUnloadAlert) {
  LoadUrlAndQuitBrowser(TWO_SECOND_BEFORE_UNLOAD_ALERT_HTML,
                        "twosecondbeforeunloadalert");
}

// Tests that if there's a renderer process with two tabs, one of which has an
// unload handler, and the other doesn't, the tab that doesn't have an unload
// handler can be closed.
// If this flakes, see http://crbug.com/45162, http://crbug.com/45281 and
// http://crbug.com/86769.
IN_PROC_BROWSER_TEST_F(UnloadTest, BrowserCloseTabWhenOtherTabHasListener) {
  NavigateToDataURL(CLOSE_TAB_WHEN_OTHER_TAB_HAS_LISTENER, "only_one_unload");

  // Simulate a click to force user_gesture to true; if we don't, the resulting
  // popup will be constrained, which isn't what we want to test.

  content::WindowedNotificationObserver observer(
        chrome::NOTIFICATION_TAB_ADDED,
        content::NotificationService::AllSources());
  content::WindowedNotificationObserver load_stop_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::NotificationService::AllSources());
  content::SimulateMouseClick(
      browser()->tab_strip_model()->GetActiveWebContents(), 0,
      WebKit::WebMouseEvent::ButtonLeft);
  observer.Wait();
  load_stop_observer.Wait();
  CheckTitle("popup");

  content::WindowedNotificationObserver tab_close_observer(
      content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
      content::NotificationService::AllSources());
  chrome::CloseTab(browser());
  tab_close_observer.Wait();

  CheckTitle("only_one_unload");
}

class FastUnloadTest : public UnloadTest {
 public:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    UnloadTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableFastUnload);
  }

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    ASSERT_TRUE(test_server()->Start());
  }

  virtual void TearDownInProcessBrowserTestFixture() OVERRIDE {
    test_server()->Stop();
  }

  GURL GetUrl(const std::string& name) {
    return GURL(test_server()->GetURL(
        "files/fast_tab_close/" + name + ".html"));
  }

  void NavigateToPage(const char* name) {
    ui_test_utils::NavigateToURL(browser(), GetUrl(name));
    CheckTitle(name);
  }

  void NavigateToPageInNewTab(const char* name) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), GetUrl(name), NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
    CheckTitle(name);
  }

  std::string GetCookies(const char* name) {
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    return content::GetCookies(contents->GetBrowserContext(), GetUrl(name));
  }
};

class FastTabCloseTabStripModelObserver : public TabStripModelObserver {
 public:
  FastTabCloseTabStripModelObserver(TabStripModel* model,
                                    base::RunLoop* run_loop)
      : model_(model),
        run_loop_(run_loop) {
    model_->AddObserver(this);
  }

  virtual ~FastTabCloseTabStripModelObserver() {
    model_->RemoveObserver(this);
  }

  // TabStripModelObserver:
  virtual void TabDetachedAt(content::WebContents* contents,
                             int index) OVERRIDE {
    run_loop_->Quit();
  }

 private:
  TabStripModel* const model_;
  base::RunLoop* const run_loop_;
};


// Test that fast-tab-close works when closing a tab with an unload handler
// (http://crbug.com/142458).
// Flaky on Windows bots (http://crbug.com/267597).
#if defined(OS_WIN)
#define MAYBE_UnloadHidden \
    DISABLED_UnloadHidden
#else
#define MAYBE_UnloadHidden \
    UnloadHidden
#endif
IN_PROC_BROWSER_TEST_F(FastUnloadTest, MAYBE_UnloadHidden) {
  NavigateToPage("no_listeners");
  NavigateToPageInNewTab("unload_sets_cookie");
  EXPECT_EQ("", GetCookies("no_listeners"));

  {
    base::RunLoop run_loop;
    FastTabCloseTabStripModelObserver observer(
        browser()->tab_strip_model(), &run_loop);
    chrome::CloseTab(browser());
    run_loop.Run();
  }

  // Check that the browser only has the original tab.
  CheckTitle("no_listeners");
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  // Show that the web contents to go away after the was removed.
  // Without unload-detached, this times-out because it happens earlier.
  content::WindowedNotificationObserver contents_destroyed_observer(
      content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
      content::NotificationService::AllSources());
  contents_destroyed_observer.Wait();

  // Browser still has the same tab.
  CheckTitle("no_listeners");
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ("unloaded=ohyeah", GetCookies("no_listeners"));
}

// Test that fast-tab-close does not break a solo tab.
IN_PROC_BROWSER_TEST_F(FastUnloadTest, PRE_ClosingLastTabFinishesUnload) {
  // The unload handler sleeps before setting the cookie to catch cases when
  // unload handlers are not allowed to run to completion. (For example,
  // using the detached handler for the tab and then closing the browser.)
  NavigateToPage("unload_sleep_before_cookie");
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ("", GetCookies("unload_sleep_before_cookie"));

  content::WindowedNotificationObserver window_observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  chrome::CloseTab(browser());
  window_observer.Wait();
}

// Fails on Mac 64 bots (http://crbug.com/301173).
#if defined(OS_MACOSX) && ARCH_CPU_64_BITS
#define MAYBE_ClosingLastTabFinishesUnload DISABLED_ClosingLastTabFinishesUnload
#else
#define MAYBE_ClosingLastTabFinishesUnload ClosingLastTabFinishesUnload
#endif
IN_PROC_BROWSER_TEST_F(FastUnloadTest, MAYBE_ClosingLastTabFinishesUnload) {
#if defined(OS_WIN)
  // Flaky on Win7+ bots (http://crbug.com/267597).
  if (base::win::GetVersion() >= base::win::VERSION_WIN7)
    return;
#endif
  // Check for cookie set in unload handler of PRE_ test.
  NavigateToPage("no_listeners");
  EXPECT_EQ("unloaded=ohyeah", GetCookies("no_listeners"));
}

// Test that fast-tab-close does not break window close.
IN_PROC_BROWSER_TEST_F(FastUnloadTest, PRE_WindowCloseFinishesUnload) {
  NavigateToPage("no_listeners");

  // The unload handler sleeps before setting the cookie to catch cases when
  // unload handlers are not allowed to run to completion. Without the sleep,
  // the cookie can get set even if the browser does not wait for
  // the unload handler to finish.
  NavigateToPageInNewTab("unload_sleep_before_cookie");
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
  EXPECT_EQ("", GetCookies("no_listeners"));

  content::WindowedNotificationObserver window_observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  chrome::CloseWindow(browser());
  window_observer.Wait();
}

// Flaky on Windows bots (http://crbug.com/279267) and fails on Mac 64 bots
// (http://crbug.com/301173).
#if defined(OS_WIN) || (defined(OS_MACOSX) && ARCH_CPU_64_BITS)
#define MAYBE_WindowCloseFinishesUnload DISABLED_WindowCloseFinishesUnload
#else
#define MAYBE_WindowCloseFinishesUnload WindowCloseFinishesUnload
#endif
IN_PROC_BROWSER_TEST_F(FastUnloadTest, MAYBE_WindowCloseFinishesUnload) {
  // Check for cookie set in unload during PRE_ test.
  NavigateToPage("no_listeners");
  EXPECT_EQ("unloaded=ohyeah", GetCookies("no_listeners"));
}

// Test that a tab crash during unload does not break window close.
//
// Hits assertion on Linux and Mac:
//     [FATAL:profile_destroyer.cc(46)] Check failed:
//         hosts.empty() ||
//         profile->IsOffTheRecord() ||
//         content::RenderProcessHost::run_renderer_in_process().
//     More details: The renderer process host matches the closed, crashed tab.
//     The |UnloadController| receives |NOTIFICATION_WEB_CONTENTS_DISCONNECTED|
//     and proceeds with the close.
IN_PROC_BROWSER_TEST_F(FastUnloadTest, DISABLED_WindowCloseAfterUnloadCrash) {
  NavigateToPage("no_listeners");
  NavigateToPageInNewTab("unload_sets_cookie");
  content::WebContents* unload_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ("", GetCookies("no_listeners"));

  {
    base::RunLoop run_loop;
    FastTabCloseTabStripModelObserver observer(
        browser()->tab_strip_model(), &run_loop);
    chrome::CloseTab(browser());
    run_loop.Run();
  }

  // Check that the browser only has the original tab.
  CheckTitle("no_listeners");
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  CrashTab(unload_contents);

  // Check that the browser only has the original tab.
  CheckTitle("no_listeners");
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  content::WindowedNotificationObserver window_observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  chrome::CloseWindow(browser());
  window_observer.Wait();
}

// Times out on Windows and Linux.
#if defined(OS_WIN) || defined(OS_LINUX)
#define MAYBE_WindowCloseAfterBeforeUnloadCrash \
    DISABLED_WindowCloseAfterBeforeUnloadCrash
#else
#define MAYBE_WindowCloseAfterBeforeUnloadCrash \
    WindowCloseAfterBeforeUnloadCrash
#endif
IN_PROC_BROWSER_TEST_F(FastUnloadTest,
                       MAYBE_WindowCloseAfterBeforeUnloadCrash) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(BEFORE_UNLOAD_HTML, "beforeunload");
  content::WebContents* beforeunload_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  content::WindowedNotificationObserver window_observer(
        chrome::NOTIFICATION_BROWSER_CLOSED,
        content::NotificationService::AllSources());
  chrome::CloseWindow(browser());
  CrashTab(beforeunload_contents);
  window_observer.Wait();
}

// TODO(ojan): Add tests for unload/beforeunload that have multiple tabs
// and multiple windows.
