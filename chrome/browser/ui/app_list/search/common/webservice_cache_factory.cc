// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/common/webservice_cache_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/ui/app_list/search/common/webservice_cache.h"
#include "components/browser_context_keyed_service/browser_context_dependency_manager.h"

namespace app_list {

// static
WebserviceCacheFactory* WebserviceCacheFactory::GetInstance() {
  return Singleton<WebserviceCacheFactory>::get();
}

// static
WebserviceCache* WebserviceCacheFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<WebserviceCache*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WebserviceCacheFactory::WebserviceCacheFactory()
    : BrowserContextKeyedServiceFactory(
          "app_list::WebserviceCache",
          BrowserContextDependencyManager::GetInstance()) {}

WebserviceCacheFactory::~WebserviceCacheFactory() {}

BrowserContextKeyedService* WebserviceCacheFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new WebserviceCache(context);
}

}  // namespace app_list
