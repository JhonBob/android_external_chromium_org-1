// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OPTIONS2_CHROMEOS_SYSTEM_OPTIONS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_OPTIONS2_CHROMEOS_SYSTEM_OPTIONS_HANDLER_H_
#pragma once

#include "base/memory/weak_ptr.h"
#include "base/compiler_specific.h"
#include "chrome/browser/ui/webui/options2/options_ui2.h"

namespace base {
class DictionaryValue;
}

// ChromeOS system options page UI handler.
class SystemOptionsHandler
  : public OptionsPage2UIHandler,
    public base::SupportsWeakPtr<SystemOptionsHandler> {
 public:
  SystemOptionsHandler();
  virtual ~SystemOptionsHandler();

  // OptionsPage2UIHandler implementation.
  virtual void GetLocalizedValues(
      base::DictionaryValue* localized_strings) OVERRIDE;
  virtual void Initialize() OVERRIDE;

  virtual void RegisterMessages() OVERRIDE;

  // Called when the accessibility checkbox value is changed.
  // |args| will contain the checkbox checked state as a string
  // ("true" or "false").
  void AccessibilityChangeCallback(const base::ListValue* args);

  // Called when the System configuration screen is used to adjust
  // the screen brightness.
  // |args| will be an empty list.
  void DecreaseScreenBrightnessCallback(const base::ListValue* args);
  void IncreaseScreenBrightnessCallback(const base::ListValue* args);

 private:
  // Callback for TouchpadHelper.
  void TouchpadExists(bool* exists);

  DISALLOW_COPY_AND_ASSIGN(SystemOptionsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_OPTIONS2_CHROMEOS_SYSTEM_OPTIONS_HANDLER_H_
