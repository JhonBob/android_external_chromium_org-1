// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NETWORK_CHANGE_NOTIFIER_MAC_H_
#define NET_BASE_NETWORK_CHANGE_NOTIFIER_MAC_H_

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "net/base/network_change_notifier.h"
#include "net/base/network_change_notifier_helper.h"

class MessageLoop;
namespace base {
class Thread;
}  // namespace base

namespace net {

class NetworkChangeNotifierMac : public NetworkChangeNotifier {
 public:
  NetworkChangeNotifierMac();

  void OnIPAddressChanged() { helper_.OnIPAddressChanged(); }

  // NetworkChangeNotifier methods:

  virtual void AddObserver(Observer* observer) {
    helper_.AddObserver(observer);
  }

  virtual void RemoveObserver(Observer* observer) {
    helper_.RemoveObserver(observer);
  }

 private:
  friend class base::RefCounted<NetworkChangeNotifierMac>;
  
  virtual ~NetworkChangeNotifierMac();

  // Initializes the notifier thread.  The SystemConfiguration calls in this
  // function can lead to contention early on, so we invoke this function later
  // on in startup to keep it fast.
  // See http://crbug.com/34926 for details.
  void InitializeNotifierThread(MessageLoop* loop);

  // Receives the OS X network change notifications on this thread.
  scoped_ptr<base::Thread> notifier_thread_;

  internal::NetworkChangeNotifierHelper helper_;

  // Used to initialize the notifier thread.
  ScopedRunnableMethodFactory<NetworkChangeNotifierMac> method_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierMac);
};

}  // namespace net

#endif  // NET_BASE_NETWORK_CHANGE_NOTIFIER_MAC_H_
