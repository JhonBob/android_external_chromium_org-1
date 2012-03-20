// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_IME_IME_OBSERVER_H_
#define ASH_SYSTEM_IME_IME_OBSERVER_H_
#pragma once

namespace ash {

class IMEObserver {
 public:
  virtual ~IMEObserver() {}

  virtual void OnIMERefresh() = 0;
};

}  // namespace ash

#endif  // ASH_SYSTEM_IME_IME_OBSERVER_H_
