// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_STATE_HANDLER_OBSERVER_H_
#define CHROMEOS_NETWORK_NETWORK_STATE_HANDLER_OBSERVER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

class NetworkState;

// Observer class for all network state changes, including changes to
// active (connecting or connected) services.
class CHROMEOS_EXPORT NetworkStateHandlerObserver {
 public:
  typedef std::vector<const NetworkState*> NetworkStateList;

  NetworkStateHandlerObserver();
  virtual ~NetworkStateHandlerObserver();

  // Called when one or more network manager properties changes.
  virtual void NetworkManagerChanged();

  // The list of networks changed.
  virtual void NetworkListChanged();

  // The list of devices changed, or a property changed (e.g. scanning).
  virtual void DeviceListChanged();

  // The default network changed (includes VPNs) or its connection state
  // changed. |network| will be NULL if there is no longer a default network.
  virtual void DefaultNetworkChanged(const NetworkState* network);

  // The connection state of |network| changed.
  virtual void NetworkConnectionStateChanged(const NetworkState* network);

  // One or more properties of |network| have been updated. Note: this will get
  // called in *addition* to NetworkConnectionStateChanged() when the
  // connection state property changes. Use this to track properties like
  // wifi strength.
  virtual void NetworkPropertiesUpdated(const NetworkState* network);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkStateHandlerObserver);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_STATE_HANDLER_OBSERVER_H_
