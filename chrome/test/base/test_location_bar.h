// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_TEST_LOCATION_BAR_H_
#define CHROME_TEST_BASE_TEST_LOCATION_BAR_H_
#pragma once

#include "base/string16.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/omnibox/location_bar.h"
#include "content/public/common/page_transition_types.h"
#include "webkit/glue/window_open_disposition.h"

class TestLocationBar : public LocationBar {
 public:
  TestLocationBar();
  virtual ~TestLocationBar();

  void set_input_string(const string16& input_string) {
    input_string_ = input_string;
  }
  void set_disposition(WindowOpenDisposition disposition) {
    disposition_ = disposition;
  }
  void set_transition(content::PageTransition transition) {
    transition_ = transition;
  }

  // Overridden from LocationBar:
  virtual void ShowFirstRunBubble(FirstRun::BubbleType bubble_type) OVERRIDE {}
  virtual void SetSuggestedText(const string16& text,
                                InstantCompleteBehavior behavior) OVERRIDE {}
  virtual string16 GetInputString() const OVERRIDE;
  virtual WindowOpenDisposition GetWindowOpenDisposition() const OVERRIDE;
  virtual content::PageTransition GetPageTransition() const OVERRIDE;
  virtual void AcceptInput() OVERRIDE {}
  virtual void FocusLocation(bool select_all) OVERRIDE {}
  virtual void FocusSearch() OVERRIDE {}
  virtual void UpdateContentSettingsIcons() OVERRIDE {}
  virtual void UpdatePageActions() OVERRIDE {}
  virtual void InvalidatePageActions() OVERRIDE {}
  virtual void SaveStateToContents(TabContents* contents) OVERRIDE {}
  virtual void Revert() OVERRIDE {}
  virtual const OmniboxView* location_entry() const OVERRIDE;
  virtual OmniboxView* location_entry() OVERRIDE;
  virtual LocationBarTesting* GetLocationBarForTesting() OVERRIDE;

 private:

  // Test-supplied values that will be returned through the LocationBar
  // interface.
  string16 input_string_;
  WindowOpenDisposition disposition_;
  content::PageTransition transition_;

  DISALLOW_COPY_AND_ASSIGN(TestLocationBar);
};


#endif  // CHROME_TEST_BASE_TEST_LOCATION_BAR_H_
