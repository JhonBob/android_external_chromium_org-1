// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_TREE_HOST_OZONE_H_
#define UI_AURA_WINDOW_TREE_HOST_OZONE_H_

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_pump_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/event_source.h"
#include "ui/gfx/insets.h"
#include "ui/gfx/rect.h"

namespace aura {

class WindowTreeHostOzone : public WindowTreeHost,
                            public ui::EventSource,
                            public base::MessagePumpDispatcher {
 public:
  explicit WindowTreeHostOzone(const gfx::Rect& bounds);
  virtual ~WindowTreeHostOzone();

 private:
  // Overridden from Dispatcher overrides:
  virtual uint32_t Dispatch(const base::NativeEvent& event) OVERRIDE;

  // WindowTreeHost Overrides.
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual void ToggleFullScreen() OVERRIDE;
  virtual gfx::Rect GetBounds() const OVERRIDE;
  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual gfx::Insets GetInsets() const OVERRIDE;
  virtual void SetInsets(const gfx::Insets& bounds) OVERRIDE;
  virtual gfx::Point GetLocationOnNativeScreen() const OVERRIDE;
  virtual void SetCapture() OVERRIDE;
  virtual void ReleaseCapture() OVERRIDE;
  virtual bool QueryMouseLocation(gfx::Point* location_return) OVERRIDE;
  virtual bool ConfineCursorToRootWindow() OVERRIDE;
  virtual void UnConfineCursor() OVERRIDE;
  virtual void PostNativeEvent(const base::NativeEvent& event) OVERRIDE;
  virtual void OnDeviceScaleFactorChanged(float device_scale_factor) OVERRIDE;
  virtual void PrepareForShutdown() OVERRIDE;
  virtual void SetCursorNative(gfx::NativeCursor cursor_type) OVERRIDE;
  virtual void MoveCursorToNative(const gfx::Point& location) OVERRIDE;
  virtual void OnCursorVisibilityChangedNative(bool show) OVERRIDE;

  // ui::EventSource overrides.
  virtual ui::EventProcessor* GetEventProcessor() OVERRIDE;

  gfx::AcceleratedWidget widget_;
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeHostOzone);
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_TREE_HOST_OZONE_H_
