// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/window/window_button_order_provider.h"

#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/linux_ui/window_button_order_observer.h"

namespace views {

namespace {

class WindowButtonOrderObserverDelegate : public WindowButtonOrderProvider,
                                          public WindowButtonOrderObserver {
 public:
  WindowButtonOrderObserverDelegate();
  virtual ~WindowButtonOrderObserverDelegate();

  // WindowButtonOrderObserver:
  virtual void OnWindowButtonOrderingChange(
      const std::vector<views::FrameButton>& leading_buttons,
      const std::vector<views::FrameButton>& trailing_buttons) OVERRIDE;
 private:
  DISALLOW_COPY_AND_ASSIGN(WindowButtonOrderObserverDelegate);
};

///////////////////////////////////////////////////////////////////////////////
// WindowButtonOrderObserverDelegate, public:

WindowButtonOrderObserverDelegate::WindowButtonOrderObserverDelegate() {
  views::LinuxUI* ui = views::LinuxUI::instance();
  if (ui)
    ui->AddWindowButtonOrderObserver(this);
}

WindowButtonOrderObserverDelegate::~WindowButtonOrderObserverDelegate() {
  views::LinuxUI* ui = views::LinuxUI::instance();
  if (ui)
    ui->RemoveWindowButtonOrderObserver(this);
}

void WindowButtonOrderObserverDelegate::OnWindowButtonOrderingChange(
    const std::vector<views::FrameButton>& leading_buttons,
    const std::vector<views::FrameButton>& trailing_buttons) {
  SetWindowButtonOrder(leading_buttons, trailing_buttons);
}

}  // namespace

// static
WindowButtonOrderProvider* WindowButtonOrderProvider::instance_ = NULL;

///////////////////////////////////////////////////////////////////////////////
// WindowButtonOrderProvider, public:

// static
WindowButtonOrderProvider* WindowButtonOrderProvider::GetInstance() {
  if (!instance_)
    instance_ = new WindowButtonOrderObserverDelegate;
  return instance_;
}

std::vector<views::FrameButton> const
    WindowButtonOrderProvider::GetLeadingButtons() const {
  return leading_buttons_;
}

std::vector<views::FrameButton> const
    WindowButtonOrderProvider::GetTrailingButtons() const {
  return trailing_buttons_;
}

///////////////////////////////////////////////////////////////////////////////
// WindowButtonOrderProvider, protected:

WindowButtonOrderProvider::WindowButtonOrderProvider() {
  trailing_buttons_.push_back(views::FRAME_BUTTON_MINIMIZE);
  trailing_buttons_.push_back(views::FRAME_BUTTON_MAXIMIZE);
  trailing_buttons_.push_back(views::FRAME_BUTTON_CLOSE);
}

WindowButtonOrderProvider::~WindowButtonOrderProvider() {
}

void WindowButtonOrderProvider::SetWindowButtonOrder(
    const std::vector<views::FrameButton>& leading_buttons,
    const std::vector<views::FrameButton>& trailing_buttons) {
  leading_buttons_ = leading_buttons;
  trailing_buttons_ = trailing_buttons;
}

}  // namespace views
