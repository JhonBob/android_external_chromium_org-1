// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_MOCK_UPDATE_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_MOCK_UPDATE_SCREEN_H_

#include "chrome/browser/chromeos/login/view_screen.h"
#include "testing/gmock/include/gmock/gmock.h"

class MockUpdateScreen : public UpdateScreen {
 public:
  explicit MockUpdateScreen(WizardScreenDelegate* d) : UpdateScreen(d) {}
  MOCK_METHOD0(StartUpdate, void());
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_MOCK_UPDATE_SCREEN_H_
