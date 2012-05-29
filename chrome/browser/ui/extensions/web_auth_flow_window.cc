// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/web_auth_flow_window.h"

#include "content/public/browser/browser_context.h"

using content::BrowserContext;
using content::WebContents;

// TODO(munjal): Remove this once we have mac/linux implementations.
#if defined(OS_MACOSX)
// static
WebAuthFlowWindow* WebAuthFlowWindow::Create(
    Delegate* delegate,
    BrowserContext* browser_context,
    WebContents* contents) {
  return NULL;
}
#endif

WebAuthFlowWindow::~WebAuthFlowWindow() {
}

WebAuthFlowWindow::WebAuthFlowWindow(
    Delegate* delegate,
    BrowserContext* browser_context,
    WebContents* contents)
    : delegate_(delegate),
      browser_context_(browser_context),
      contents_(contents) {
}

