// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/skia_utils_gtk.h"

#include <gdk/gdkcolor.h>

namespace skia {

const int kSkiaToGDKMultiplier = 257;

// GDK_COLOR_RGB multiplies by 257 (= 0x10001) to distribute the bits evenly
// See: http://www.mindcontrol.org/~hplus/graphics/expand-bits.html
// To get back, we can just right shift by eight
// (or, formulated differently, i == (i*257)/256 for all i < 256).

SkColor GdkColorToSkColor(GdkColor color) {
  return SkColorSetRGB(color.red >> 8, color.green >> 8, color.blue >> 8);
}

GdkColor SkColorToGdkColor(SkColor color) {
  GdkColor gdk_color = {
      0,
      SkColorGetR(color) * kSkiaToGDKMultiplier,
      SkColorGetG(color) * kSkiaToGDKMultiplier,
      SkColorGetB(color) * kSkiaToGDKMultiplier
  };
  return gdk_color;
}

}  // namespace
