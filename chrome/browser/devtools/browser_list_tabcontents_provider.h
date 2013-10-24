// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_BROWSER_LIST_TABCONTENTS_PROVIDER_H_
#define CHROME_BROWSER_DEVTOOLS_BROWSER_LIST_TABCONTENTS_PROVIDER_H_

#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chrome/browser/ui/host_desktop.h"
#include "content/public/browser/devtools_http_handler_delegate.h"
#include "content/public/browser/worker_service.h"

class BrowserListTabContentsProvider
    : public content::DevToolsHttpHandlerDelegate {
 public:
  explicit BrowserListTabContentsProvider(
      chrome::HostDesktopType host_desktop_type);
  virtual ~BrowserListTabContentsProvider();

  // DevToolsHttpProtocolHandler::Delegate overrides.
  virtual std::string GetDiscoveryPageHTML() OVERRIDE;
  virtual bool BundlesFrontendResources() OVERRIDE;
  virtual base::FilePath GetDebugFrontendDir() OVERRIDE;
  virtual std::string GetPageThumbnailData(const GURL& url) OVERRIDE;
  virtual scoped_ptr<content::DevToolsTarget> CreateNewTarget() OVERRIDE;
  virtual void EnumerateTargets(TargetCallback callback) OVERRIDE;
  virtual scoped_ptr<net::StreamListenSocket> CreateSocketForTethering(
      net::StreamListenSocket::Delegate* delegate,
      std::string* name) OVERRIDE;

 private:
  typedef std::vector<content::WorkerService::WorkerInfo> WorkerInfoList;
  WorkerInfoList GetWorkerInfo();
  void RespondWithTargetList(TargetCallback callback,
                             const WorkerInfoList& worker_info_list);

  chrome::HostDesktopType host_desktop_type_;
  DISALLOW_COPY_AND_ASSIGN(BrowserListTabContentsProvider);
};

#endif  // CHROME_BROWSER_DEVTOOLS_BROWSER_LIST_TABCONTENTS_PROVIDER_H_
