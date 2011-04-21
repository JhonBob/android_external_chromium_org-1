// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/gtk/bubble/bubble_accelerators_gtk.h"

#include <gdk/gdkkeysyms.h>
#include <glib.h>

namespace {

// Listing of the accelerators that are either handled or forwarded by
// bubbles. Any accelerators that are not explicitly listed here
// are ignored and silently dropped. This table is expected to change
// after discussion over which accelerators should be addressed in
// bubbles. For a complete listing of accelerators that are used
// in chrome consult accelerators_gtk.cc
struct BubbleAcceleratorGtk BubbleAcceleratorGtkTable[] = {
  // Tab/window controls.
  { GDK_w, GDK_CONTROL_MASK },

  // Navigation / toolbar buttons.
  { GDK_Escape, GdkModifierType(0) }
};

}  // namespace

BubbleAcceleratorGtkList BubbleAcceleratorsGtk::GetList() {
  BubbleAcceleratorGtkList accelerators;
  for (size_t i = 0; i < arraysize(BubbleAcceleratorGtkTable); ++i)
    accelerators.push_back(BubbleAcceleratorGtkTable[i]);

  return accelerators;
}
