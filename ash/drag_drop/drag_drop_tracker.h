// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DRAG_DROP_DRAG_DROP_TRACKER_H_
#define ASH_DRAG_DROP_DRAG_DROP_TRACKER_H_

#include "ash/ash_export.h"
#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/base/event.h"

namespace aura {
class RootWindow;
class Window;
}

namespace ash {
namespace internal {

// Provides functions for handling drag events inside and outside the root
// window where drag is started. This internally sets up a capture window for
// tracking drag events outside the root window where drag is initiated.
// ash/wm/coordinate_conversion.h is used internally and only X11 environment
// is supported for now.
class ASH_EXPORT DragDropTracker {
 public:
  explicit DragDropTracker(aura::RootWindow* root_window);
  ~DragDropTracker();

  // Gets the target located at |event| in the coordinates of the root window
  // given to the constructor.
  aura::Window* GetTarget(const ui::LocatedEvent& event);

  // Converts the locations of |event| in the coordinates of the root window
  // given to the constructor to the ones in |target|'s coordinates.
  // Caller takes ownership of the returned object.
  ui::MouseEvent* ConvertMouseEvent(aura::Window* target,
                                    const ui::MouseEvent& event);

 private:
  // A window for capturing drag events while dragging.
  scoped_ptr<aura::Window> capture_window_;

  DISALLOW_COPY_AND_ASSIGN(DragDropTracker);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_DRAG_DROP_DRAG_DROP_TRACKER_H_
