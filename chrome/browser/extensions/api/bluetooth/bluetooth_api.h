// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_BLUETOOTH_BLUETOOTH_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_BLUETOOTH_BLUETOOTH_API_H_
#pragma once

#include "chrome/browser/extensions/extension_function.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/bluetooth/bluetooth_adapter.h"
#endif

namespace extensions {
namespace api {

class BluetoothExtensionFunction : public SyncExtensionFunction {
 public:
  BluetoothExtensionFunction();

 protected:
#if defined(OS_CHROMEOS)
  const chromeos::BluetoothAdapter* adapter_;
#endif
};

class BluetoothIsAvailableFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.isAvailable")
};

class BluetoothIsPoweredFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.isPowered")
};

class BluetoothGetAddressFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.getAddress")
};

class BluetoothGetDevicesWithServiceFunction
    : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.bluetooth.getDevicesWithService")
};

class BluetoothDisconnectFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.disconnect")
};

class BluetoothReadFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.read")
};

class BluetoothSetOutOfBandPairingDataFunction
    : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.bluetooth.setOutOfBandPairingData")
};

class BluetoothGetOutOfBandPairingDataFunction
    : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.bluetooth.getOutOfBandPairingData")
};

class BluetoothWriteFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.write")
};

class BluetoothConnectFunction : public BluetoothExtensionFunction {
 public:
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bluetooth.connect")
};

}  // namespace api
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_BLUETOOTH_BLUETOOTH_API_H_
