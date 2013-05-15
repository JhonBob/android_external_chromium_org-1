// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/fullscreen/fullscreen_controller.h"
#include "chrome/browser/ui/fullscreen/fullscreen_controller_state_test.h"
#include "chrome/browser/ui/fullscreen/fullscreen_controller_test.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::kAboutBlankURL;
using content::PAGE_TRANSITION_TYPED;

// Interactive test fixture testing Fullscreen Controller through its states. --
// See documentation at the top of fullscreen_controller_state_unittest.cc.
class FullscreenControllerStateInteractiveTest
    : public InProcessBrowserTest,
      public FullscreenControllerStateTest {
 private:
  // FullscreenControllerStateTest override:
  virtual Browser* GetBrowser() OVERRIDE;
};

Browser* FullscreenControllerStateInteractiveTest::GetBrowser() {
  return InProcessBrowserTest::browser();
}

// Tests -----------------------------------------------------------------------

// Soak tests:

// Tests all states with all permutations of multiple events to detect lingering
// state issues that would bleed over to other states.
// I.E. for each state test all combinations of events E1, E2, E3.
//
// This produces coverage for event sequences that may happen normally but
// would not be exposed by traversing to each state via TransitionToState().
// TransitionToState() always takes the same path even when multiple paths
// exist.
IN_PROC_BROWSER_TEST_F(FullscreenControllerStateInteractiveTest,
                       DISABLED_TransitionsForEachState) {
  // A tab is needed for tab fullscreen.
  AddTabAtIndex(0, GURL(kAboutBlankURL), PAGE_TRANSITION_TYPED);
  TestTransitionsForEachState();
  // Progress of test can be examined via LOG(INFO) << GetAndClearDebugLog();
}


// Individual tests for each pair of state and event:

// An "empty" test is included as part of each "TEST_EVENT" because it makes
// running the entire test suite less flaky on MacOS. All of the tests pass
// when run individually.
#define TEST_EVENT(state, event) \
    IN_PROC_BROWSER_TEST_F(FullscreenControllerStateInteractiveTest, \
                           DISABLED_##state##__##event##__Empty) { \
    } \
    IN_PROC_BROWSER_TEST_F(FullscreenControllerStateInteractiveTest, \
                           DISABLED_##state##__##event) { \
      AddTabAtIndex(0, GURL(kAboutBlankURL), PAGE_TRANSITION_TYPED); \
      ASSERT_NO_FATAL_FAILURE(TestStateAndEvent(state, event)) \
          << GetAndClearDebugLog(); \
    }
    // Progress of tests can be examined by inserting the following line:
    // LOG(INFO) << GetAndClearDebugLog(); }

TEST_EVENT(STATE_NORMAL, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_NORMAL, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_NORMAL, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_NORMAL, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_NORMAL, METRO_SNAP_TRUE);
TEST_EVENT(STATE_NORMAL, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_NORMAL, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_NORMAL, BUBBLE_ALLOW);
TEST_EVENT(STATE_NORMAL, BUBBLE_DENY);
TEST_EVENT(STATE_NORMAL, WINDOW_CHANGE);

TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, METRO_SNAP_TRUE);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, BUBBLE_ALLOW);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, BUBBLE_DENY);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_NO_CHROME, WINDOW_CHANGE);

TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, METRO_SNAP_TRUE);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, BUBBLE_ALLOW);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, BUBBLE_DENY);
TEST_EVENT(STATE_BROWSER_FULLSCREEN_WITH_CHROME, WINDOW_CHANGE);

#if defined(OS_WIN)
TEST_EVENT(STATE_METRO_SNAP, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_METRO_SNAP, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_METRO_SNAP, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_METRO_SNAP, TAB_FULLSCREEN_FALSE);
TEST_EVENT(STATE_METRO_SNAP, METRO_SNAP_TRUE);
TEST_EVENT(STATE_METRO_SNAP, METRO_SNAP_FALSE);
TEST_EVENT(STATE_METRO_SNAP, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_METRO_SNAP, BUBBLE_ALLOW);
TEST_EVENT(STATE_METRO_SNAP, BUBBLE_DENY);
TEST_EVENT(STATE_METRO_SNAP, WINDOW_CHANGE);
#endif

TEST_EVENT(STATE_TAB_FULLSCREEN, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TAB_FULLSCREEN, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TAB_FULLSCREEN, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TAB_FULLSCREEN, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TAB_FULLSCREEN, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TAB_FULLSCREEN, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TAB_FULLSCREEN, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TAB_FULLSCREEN, BUBBLE_ALLOW);
TEST_EVENT(STATE_TAB_FULLSCREEN, BUBBLE_DENY);
TEST_EVENT(STATE_TAB_FULLSCREEN, WINDOW_CHANGE);

TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, BUBBLE_ALLOW);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, BUBBLE_DENY);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN, WINDOW_CHANGE);

TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, BUBBLE_ALLOW);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, BUBBLE_DENY);
TEST_EVENT(STATE_TAB_BROWSER_FULLSCREEN_CHROME, WINDOW_CHANGE);

TEST_EVENT(STATE_TO_NORMAL, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TO_NORMAL, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TO_NORMAL, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TO_NORMAL, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TO_NORMAL, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TO_NORMAL, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TO_NORMAL, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TO_NORMAL, BUBBLE_ALLOW);
TEST_EVENT(STATE_TO_NORMAL, BUBBLE_DENY);
TEST_EVENT(STATE_TO_NORMAL, WINDOW_CHANGE);

TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, BUBBLE_ALLOW);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, BUBBLE_DENY);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_NO_CHROME, WINDOW_CHANGE);

TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, BUBBLE_ALLOW);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, BUBBLE_DENY);
TEST_EVENT(STATE_TO_BROWSER_FULLSCREEN_WITH_CHROME, WINDOW_CHANGE);

TEST_EVENT(STATE_TO_TAB_FULLSCREEN, TOGGLE_FULLSCREEN);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, TOGGLE_FULLSCREEN_CHROME);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, TAB_FULLSCREEN_TRUE);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, TAB_FULLSCREEN_FALSE);
#if defined(OS_WIN)
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, METRO_SNAP_TRUE);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, METRO_SNAP_FALSE);
#endif
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, BUBBLE_EXIT_LINK);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, BUBBLE_ALLOW);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, BUBBLE_DENY);
TEST_EVENT(STATE_TO_TAB_FULLSCREEN, WINDOW_CHANGE);


// Specific one-off tests for known issues:

// Used manually to determine what happens on a platform.
IN_PROC_BROWSER_TEST_F(FullscreenControllerStateInteractiveTest,
                       DISABLED_ManualTest) {
  // A tab is needed for tab fullscreen.
  AddTabAtIndex(0, GURL(kAboutBlankURL), PAGE_TRANSITION_TYPED);
  ASSERT_TRUE(InvokeEvent(TOGGLE_FULLSCREEN)) << GetAndClearDebugLog();
  ASSERT_TRUE(InvokeEvent(WINDOW_CHANGE)) << GetAndClearDebugLog();
  ASSERT_TRUE(InvokeEvent(TAB_FULLSCREEN_TRUE)) << GetAndClearDebugLog();
  ASSERT_TRUE(InvokeEvent(TOGGLE_FULLSCREEN)) << GetAndClearDebugLog();
  ASSERT_TRUE(InvokeEvent(WINDOW_CHANGE)) << GetAndClearDebugLog();

  // Wait, allowing human operator to observe the result.
  scoped_refptr<content::MessageLoopRunner> message_loop;
  message_loop = new content::MessageLoopRunner();
  message_loop->Run();
}

