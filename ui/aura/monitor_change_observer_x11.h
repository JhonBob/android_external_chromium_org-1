// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MONITOR_CHANGE_OBSERVER_H
#define UI_AURA_MONITOR_CHANGE_OBSERVER_H
#pragma once

#include <X11/Xlib.h>

// Xlib.h defines RootWindow.
#undef RootWindow

#include "base/basictypes.h"
#include "ui/aura/aura_export.h"

namespace aura {

// An object that observes changes in monitor configuration and
// update MonitorManagers.
class AURA_EXPORT MonitorChangeObserverX11 {
 public:
  MonitorChangeObserverX11();
  ~MonitorChangeObserverX11();

  bool Dispatch(const XEvent* event);

  // Reads monitor configurations from the system and notifies
  // |monitor_manager_| about the change.
  void NotifyMonitorChange();

 private:
  Display* xdisplay_;

  ::Window x_root_window_;

  int xrandr_event_base_;

  DISALLOW_COPY_AND_ASSIGN(MonitorChangeObserverX11);
};

}  // namespace aura

#endif  // UI_AURA_MONITOR_CHANGE_OBSERVER_H
