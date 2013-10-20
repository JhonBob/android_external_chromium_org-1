// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/shell/browser/shell_aura.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/default_activation_client.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/gfx/screen.h"

namespace content {

namespace {

class FillLayout : public aura::LayoutManager {
 public:
  explicit FillLayout(aura::RootWindow* root)
      : root_(root) {
  }

  virtual ~FillLayout() {}

 private:
  // aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE {
  }

  virtual void OnWindowAddedToLayout(aura::Window* child) OVERRIDE {
    child->SetBounds(gfx::Rect(root_->GetHostSize()));
  }

  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) OVERRIDE {
  }

  virtual void OnWindowRemovedFromLayout(aura::Window* child) OVERRIDE {
  }

  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visible) OVERRIDE {
  }

  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) OVERRIDE {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::RootWindow* root_;

  DISALLOW_COPY_AND_ASSIGN(FillLayout);
};

class MinimalInputEventFilter : public ui::internal::InputMethodDelegate,
                                public ui::EventHandler {
 public:
  explicit MinimalInputEventFilter(aura::RootWindow* root)
      : root_(root),
        input_method_(ui::CreateInputMethod(this, NULL)) {
    input_method_->Init(true);
    root_->AddPreTargetHandler(this);
    root_->SetProperty(aura::client::kRootWindowInputMethodKey,
                       input_method_.get());
  }

  virtual ~MinimalInputEventFilter() {
    root_->RemovePreTargetHandler(this);
    root_->SetProperty(aura::client::kRootWindowInputMethodKey,
                       static_cast<ui::InputMethod*>(NULL));
  }

 private:
  // ui::EventHandler:
  virtual void OnKeyEvent(ui::KeyEvent* event) OVERRIDE {
    const ui::EventType type = event->type();
    if (type == ui::ET_TRANSLATED_KEY_PRESS ||
        type == ui::ET_TRANSLATED_KEY_RELEASE) {
      // The |event| is already handled by this object, change the type of the
      // event to ui::ET_KEY_* and pass it to the next filter.
      static_cast<ui::TranslatedKeyEvent*>(event)->ConvertToKeyEvent();
    } else {
      bool handled = false;
      if (event->HasNativeEvent())
        handled = input_method_->DispatchKeyEvent(event->native_event());
      else
        handled = input_method_->DispatchFabricatedKeyEvent(*event);
      if (handled)
        event->StopPropagation();
    }
  }

  // ui::InputMethodDelegate:
  virtual bool DispatchKeyEventPostIME(
      const base::NativeEvent& event) OVERRIDE {
    ui::TranslatedKeyEvent aura_event(event, false /* is_char */);
    return root_->AsRootWindowHostDelegate()->OnHostKeyEvent(
        &aura_event);
  }

  virtual bool DispatchFabricatedKeyEventPostIME(ui::EventType type,
                                                 ui::KeyboardCode key_code,
                                                 int flags) OVERRIDE {
    ui::TranslatedKeyEvent aura_event(type == ui::ET_KEY_PRESSED, key_code,
                                      flags);
    return root_->AsRootWindowHostDelegate()->OnHostKeyEvent(
        &aura_event);
  }

  aura::RootWindow* root_;
  scoped_ptr<ui::InputMethod> input_method_;

  DISALLOW_COPY_AND_ASSIGN(MinimalInputEventFilter);
};

}

ShellAuraPlatformData::ShellAuraPlatformData() {
}

ShellAuraPlatformData::~ShellAuraPlatformData() {
}

void ShellAuraPlatformData::CreateWindow(int width, int height) {
  aura::TestScreen* screen = aura::TestScreen::Create();
  gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, screen);
  root_window_.reset(screen->CreateRootWindowForPrimaryDisplay());
  root_window_->SetHostSize(gfx::Size(width, height));
  root_window_->ShowRootWindow();
  root_window_->SetLayoutManager(new FillLayout(root_window_.get()));

  focus_client_.reset(new aura::test::TestFocusClient());
  aura::client::SetFocusClient(root_window_.get(), focus_client_.get());

  activation_client_.reset(
      new aura::client::DefaultActivationClient(root_window_.get()));
  capture_client_.reset(
      new aura::client::DefaultCaptureClient(root_window_.get()));
  ime_filter_.reset(new MinimalInputEventFilter(root_window_.get()));
}

// static
void Shell::PlatformInitialize(const gfx::Size& default_window_size) {
  aura::Env::CreateInstance();
}

void Shell::PlatformExit() {
  aura::Env::DeleteInstance();
}

void Shell::PlatformCleanUp() {
}

void Shell::PlatformEnableUIControl(UIControl control, bool is_enabled) {
}

void Shell::PlatformSetAddressBarURL(const GURL& url) {
}

void Shell::PlatformSetIsLoading(bool loading) {
}

void Shell::PlatformCreateWindow(int width, int height) {
  platform_.reset(new ShellAuraPlatformData());
  platform_->CreateWindow(width, height);
}

void Shell::PlatformSetContents() {
  platform_->window()->AddChild(web_contents_->GetView()->GetNativeView());
  web_contents_->GetView()->GetNativeView()->Show();
}

void Shell::PlatformResizeSubViews() {
}

void Shell::Close() {
}

void Shell::PlatformSetTitle(const string16& title) {
}

}  // namespace content
