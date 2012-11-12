// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/debugger/remote_debugging_server.h"

#include "chrome/browser/debugger/browser_list_tabcontents_provider.h"
#include "chrome/browser/ui/webui/devtools_ui.h"
#include "content/public/browser/devtools_http_handler.h"
#include "net/base/tcp_listen_socket.h"

RemoteDebuggingServer::RemoteDebuggingServer(Profile* profile,
                                             const std::string& ip,
                                             int port,
                                             const std::string& frontend_url) {
  // Initialize DevTools data source.
  DevToolsUI::RegisterDevToolsDataSource(profile);

  devtools_http_handler_ = content::DevToolsHttpHandler::Start(
      new net::TCPListenSocketFactory(ip, port),
      frontend_url,
      new BrowserListTabContentsProvider(profile));
}

RemoteDebuggingServer::~RemoteDebuggingServer() {
  devtools_http_handler_->Stop();
}
