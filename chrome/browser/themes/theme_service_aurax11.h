// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THEMES_THEME_SERVICE_AURAX11_H_
#define CHROME_BROWSER_THEMES_THEME_SERVICE_AURAX11_H_

#include "chrome/browser/themes/theme_service.h"
#include "ui/views/linux_ui/native_theme_change_observer.h"

// A subclass of ThemeService that manages the CustomThemeSupplier which
// provides the native X11 theme.
class ThemeServiceAuraX11 : public ThemeService,
                            public views::NativeThemeChangeObserver {
 public:
  ThemeServiceAuraX11();
  virtual ~ThemeServiceAuraX11();

  // Overridden from ThemeService:
  virtual bool ShouldInitWithNativeTheme() const OVERRIDE;
  virtual void SetNativeTheme() OVERRIDE;
  virtual bool UsingDefaultTheme() const OVERRIDE;
  virtual bool UsingNativeTheme() const OVERRIDE;

  // Overridden from views::NativeThemeChangeObserver:
  virtual void OnNativeThemeChanged() OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ThemeServiceAuraX11);
};

#endif  // CHROME_BROWSER_THEMES_THEME_SERVICE_AURAX11_H_
