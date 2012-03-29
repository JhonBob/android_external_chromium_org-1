// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_BLOCKING_METHOD_CALLER_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_BLOCKING_METHOD_CALLER_H_
#pragma once

#include "base/callback.h"
#include "base/synchronization/waitable_event.h"

namespace dbus {

class Bus;
class ObjectProxy;
class MethodCall;
class Response;

}  // namespace dbus

namespace chromeos {

// A utility class to call D-Bus methods in a synchronous (blocking) way.
// Note: Blocking the thread until it returns is not a good idea in most cases.
//       Avoid using this class as hard as you can.
class BlockingMethodCaller {
 public:
  BlockingMethodCaller(dbus::Bus* bus, dbus::ObjectProxy* proxy);
  virtual ~BlockingMethodCaller();

  // Calls the method and blocks until it returns.
  // The caller is responsible to delete the returned object.
  dbus::Response* CallMethodAndBlock(dbus::MethodCall* method_call);

 private:
  dbus::Bus* bus_;
  dbus::ObjectProxy* proxy_;
  base::WaitableEvent on_blocking_method_call_;

  DISALLOW_COPY_AND_ASSIGN(BlockingMethodCaller);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_BLOCKING_METHOD_CALLER_H_
