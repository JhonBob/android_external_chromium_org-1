// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_APPCACHE_APPCACHE_DISPATCHER_H_
#define CHROME_COMMON_APPCACHE_APPCACHE_DISPATCHER_H_

#include <vector>
#include "chrome/common/appcache/appcache_backend_proxy.h"
#include "ipc/ipc_message.h"
#include "webkit/appcache/appcache_frontend_impl.h"

// Dispatches appcache related messages sent to a child process from the
// main browser process. There is one instance per child process. Messages
// are dispatched on the main child thread. The ChildThread base class
// creates an instance and delegates calls to it.
class AppCacheDispatcher {
 public:
  explicit AppCacheDispatcher(IPC::Message::Sender* sender)
      : backend_proxy_(sender) {}

  AppCacheBackendProxy* backend_proxy() { return &backend_proxy_; }

  bool OnMessageReceived(const IPC::Message& msg);

 private:
  // Ipc message handlers
  void OnCacheSelected(int host_id, int64 cache_id,
                       appcache::Status status);
  void OnStatusChanged(const std::vector<int>& host_ids,
                       appcache::Status status);
  void OnEventRaised(const std::vector<int>& host_ids,
                     appcache::EventID event_id);

  AppCacheBackendProxy backend_proxy_;
  appcache::AppCacheFrontendImpl frontend_impl_;
};

#endif  // CHROME_COMMON_APPCACHE_APPCACHE_DISPATCHER_H_
