// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_USER_H_
#define ASH_SYSTEM_TRAY_USER_H_
#pragma once

#include "ash/system/tray/system_tray_item.h"

namespace ash {
namespace internal {

class TrayUser : public SystemTrayItem {
 public:
  TrayUser();
  virtual ~TrayUser();

 private:
  // Overridden from SystemTrayItem
  virtual views::View* CreateTrayView() OVERRIDE;
  virtual views::View* CreateDefaultView() OVERRIDE;
  virtual views::View* CreateDetailedView() OVERRIDE;
  virtual void DestroyTrayView() OVERRIDE;
  virtual void DestroyDefaultView() OVERRIDE;
  virtual void DestroyDetailedView() OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(TrayUser);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_USER_H_
