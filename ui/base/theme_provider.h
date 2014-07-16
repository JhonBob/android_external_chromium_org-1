// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_THEME_PROVIDER_H_
#define UI_BASE_THEME_PROVIDER_H_

#include "base/basictypes.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/layout.h"
#include "ui/base/ui_base_export.h"

#if defined(OS_MACOSX)
#ifdef __OBJC__
@class NSColor;
@class NSGradient;
@class NSImage;
#else
class NSColor;
class NSGradient;
class NSImage;
#endif  // __OBJC__
#endif  // OS_*

class SkBitmap;

namespace base {
class RefCountedMemory;
}

namespace gfx {
class ImageSkia;
}

namespace ui {

////////////////////////////////////////////////////////////////////////////////
//
// ThemeProvider
//
//   ThemeProvider is an abstract class that defines the API that should be
//   implemented to provide bitmaps and color information for a given theme.
//
////////////////////////////////////////////////////////////////////////////////

class UI_BASE_EXPORT ThemeProvider {
 public:
  virtual ~ThemeProvider();

  // Whether we're using the system theme (which may or may not be the
  // same as the default theme).
  virtual bool UsingSystemTheme() const = 0;

  // Get the image specified by |id|. An implementation of ThemeProvider should
  // have its own source of ids (e.g. an enum, or external resource bundle).
  virtual gfx::ImageSkia* GetImageSkiaNamed(int id) const = 0;

  // Get the color specified by |id|.
  virtual SkColor GetColor(int id) const = 0;

  // Get the property (e.g. an alignment expressed in an enum, or a width or
  // height) specified by |id|.
  virtual int GetDisplayProperty(int id) const = 0;

  // Whether we should use the native system frame (typically Aero glass) or
  // a custom frame.
  virtual bool ShouldUseNativeFrame() const = 0;

  // Whether or not we have a certain image. Used for when the default theme
  // doesn't provide a certain image, but custom themes might (badges, etc).
  virtual bool HasCustomImage(int id) const = 0;

  // Reads the image data from the theme file into the specified vector. Only
  // valid for un-themed resources and the themed IDR_THEME_NTP_* in most
  // implementations of ThemeProvider. Returns NULL on error.
  virtual base::RefCountedMemory* GetRawData(
      int id,
      ui::ScaleFactor scale_factor) const = 0;

#if defined(OS_MACOSX)
  // Gets the NSImage with the specified |id|.
  virtual NSImage* GetNSImageNamed(int id) const = 0;

  // Gets the NSImage that GetNSImageNamed (above) would return, but returns it
  // as a pattern color.
  virtual NSColor* GetNSImageColorNamed(int id) const = 0;

  // Gets the NSColor with the specified |id|.
  virtual NSColor* GetNSColor(int id) const = 0;

  // Gets the NSColor for tinting with the specified |id|.
  virtual NSColor* GetNSColorTint(int id) const = 0;

  // Gets the NSGradient with the specified |id|.
  virtual NSGradient* GetNSGradient(int id) const = 0;
#endif
};

}  // namespace ui

#endif  // UI_BASE_THEME_PROVIDER_H_
