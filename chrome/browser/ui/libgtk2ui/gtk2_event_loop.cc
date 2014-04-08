// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtk2ui/gtk2_event_loop.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/X.h>

#include "base/memory/singleton.h"
#include "base/message_loop/message_pump_x11.h"

namespace libgtk2ui {

// static
Gtk2EventLoop* Gtk2EventLoop::GetInstance() {
  return Singleton<Gtk2EventLoop>::get();
}

Gtk2EventLoop::Gtk2EventLoop() {
  gdk_event_handler_set(GdkEventTrampoline, this, NULL);
}

Gtk2EventLoop::~Gtk2EventLoop() {
  gdk_event_handler_set(reinterpret_cast<GdkEventFunc>(gtk_main_do_event),
                        NULL, NULL);
}

// static
void Gtk2EventLoop::GdkEventTrampoline(GdkEvent* event, gpointer data) {
  Gtk2EventLoop* loop = reinterpret_cast<Gtk2EventLoop*>(data);
  loop->DispatchGdkEvent(event);
}

void Gtk2EventLoop::DispatchGdkEvent(GdkEvent* gdk_event) {
  switch (gdk_event->type) {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      ProcessGdkEventKey(gdk_event->key);
      break;
    default:
      break;  // Do nothing.
  }

  gtk_main_do_event(gdk_event);
}

void Gtk2EventLoop::ProcessGdkEventKey(const GdkEventKey& gdk_event_key) {
  // This function translates GdkEventKeys into XKeyEvents and puts them to
  // the X event queue.
  //
  // base::MessagePumpX11 is using the X11 event queue and all key events should
  // be processed there.  However, there are cases(*1) that GdkEventKeys are
  // created instead of XKeyEvents.  In these cases, we have to translate
  // GdkEventKeys to XKeyEvents and puts them to the X event queue so our main
  // event loop can handle those key events.
  //
  // (*1) At least ibus-gtk in async mode creates a copy of user's key event and
  // pushes it back to the GDK event queue.  In this case, there is no
  // corresponding key event in the X event queue.  So we have to handle this
  // case.  ibus-gtk is used through gtk-immodule to support IMEs.

  XEvent x_event = {0};
  x_event.xkey.type =
      gdk_event_key.type == GDK_KEY_PRESS ? KeyPress : KeyRelease;
  x_event.xkey.send_event = gdk_event_key.send_event;
  x_event.xkey.display = base::MessagePumpX11::GetDefaultXDisplay();
  x_event.xkey.window = GDK_WINDOW_XID(gdk_event_key.window);
  x_event.xkey.root = DefaultRootWindow(x_event.xkey.display);
  x_event.xkey.time = gdk_event_key.time;
  x_event.xkey.state = gdk_event_key.state;
  x_event.xkey.keycode = gdk_event_key.hardware_keycode;
  x_event.xkey.same_screen = true;

  XPutBackEvent(x_event.xkey.display, &x_event);
}

}  // namespace libgtk2ui
