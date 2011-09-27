// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_X_X11_UTIL_INTERNAL_H_
#define UI_BASE_X_X11_UTIL_INTERNAL_H_
#pragma once

// This file declares utility functions for X11 (Linux only).
//
// These functions require the inclusion of the Xlib headers. Since the Xlib
// headers pollute so much of the namespace, this should only be included
// when needed.

extern "C" {
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrender.h>
}

#include "ui/base/ui_export.h"

namespace ui {

// --------------------------------------------------------------------------
// NOTE: these functions cache the results and must be called from the UI
// thread.
// Get the XRENDER format id for ARGB32 (Skia's format).
//
// NOTE:Currently this don't support multiple screens/displays.
XRenderPictFormat* GetRenderARGB32Format(Display* dpy);

// Get the XRENDER format id for the default visual on the first screen. This
// is the format which our GTK window will have.
UI_EXPORT XRenderPictFormat* GetRenderVisualFormat(Display* dpy,
                                                   Visual* visual);

// --------------------------------------------------------------------------
// X11 error handling.
// Sets the X Error Handlers. Passing NULL for either will enable the default
// error handler, which if called will log the error and abort the process.
UI_EXPORT void SetX11ErrorHandlers(XErrorHandler error_handler,
                                   XIOErrorHandler io_error_handler);

// NOTE: This function should not be called directly from the
// X11 Error handler because it queries the server to decode the
// error message, which may trigger other errors. A suitable workaround
// is to post a task in the error handler to call this function.
UI_EXPORT void LogErrorEventDescription(Display* dpy,
                                        const XErrorEvent& error_event);

// LOG(FATAL) if an X11 errors has been reported. Uses XSync to ensure that all
// requests have been received and processed by the X server and uses the X
// display contained in the reported XErrorEvent.
UI_EXPORT void CheckForReportedX11Error();

}  // namespace ui

#endif  // UI_BASE_X_X11_UTIL_INTERNAL_H_
