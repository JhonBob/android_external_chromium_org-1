// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_AURA_H_
#define VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_AURA_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/aura/window_observer.h"
#include "views/controls/native/native_view_host_wrapper.h"

namespace views {

class NativeViewHost;

// Aura implementation of NativeViewHostWrapper.
class NativeViewHostAura : public NativeViewHostWrapper,
                           public aura::WindowObserver {
 public:
  explicit NativeViewHostAura(NativeViewHost* host);
  virtual ~NativeViewHostAura();

  // Overridden from NativeViewHostWrapper:
  virtual void NativeViewAttached() OVERRIDE;
  virtual void NativeViewDetaching(bool destroyed) OVERRIDE;
  virtual void AddedToWidget() OVERRIDE;
  virtual void RemovedFromWidget() OVERRIDE;
  virtual void InstallClip(int x, int y, int w, int h) OVERRIDE;
  virtual bool HasInstalledClip() OVERRIDE;
  virtual void UninstallClip() OVERRIDE;
  virtual void ShowWidget(int x, int y, int w, int h) OVERRIDE;
  virtual void HideWidget() OVERRIDE;
  virtual void SetFocus() OVERRIDE;
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() OVERRIDE;

 private:
  // Overridden from aura::WindowObserver:
  virtual void OnWindowDestroyed(aura::Window* window) OVERRIDE;

  // Our associated NativeViewHost.
  NativeViewHost* host_;

  // Have we installed a clip region?
  bool installed_clip_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewHostAura);
};

}  // namespace views

#endif  // VIEWS_CONTROLS_NATIVE_NATIVE_VIEW_HOST_AURA_H_
