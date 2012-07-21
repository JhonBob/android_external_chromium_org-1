// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TEST_MOCK_DEVICES_CHANGED_OBSERVER_H_
#define BASE_TEST_MOCK_DEVICES_CHANGED_OBSERVER_H_

#include <string>

#include "base/system_monitor/system_monitor.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace base {

class MockDevicesChangedObserver
    : public base::SystemMonitor::DevicesChangedObserver {
 public:
  MockDevicesChangedObserver();
  ~MockDevicesChangedObserver();

  MOCK_METHOD0(OnDevicesChanged, void());
  MOCK_METHOD4(OnMediaDeviceAttached,
               void(const std::string& id,
                    const string16& name,
                    base::SystemMonitor::MediaDeviceType type,
                    const FilePath::StringType& location));
  MOCK_METHOD1(OnMediaDeviceDetached, void(const std::string& id));

  DISALLOW_COPY_AND_ASSIGN(MockDevicesChangedObserver);
};

}  // namespace base

#endif  // BASE_TEST_MOCK_DEVICES_CHANGED_OBSERVER_H_
