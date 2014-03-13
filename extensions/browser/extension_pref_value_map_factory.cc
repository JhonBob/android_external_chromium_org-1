// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_pref_value_map_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_pref_value_map.h"

ExtensionPrefValueMapFactory::ExtensionPrefValueMapFactory()
    : BrowserContextKeyedServiceFactory(
        "ExtensionPrefValueMap",
        BrowserContextDependencyManager::GetInstance()) {
}

ExtensionPrefValueMapFactory::~ExtensionPrefValueMapFactory() {
}

// static
ExtensionPrefValueMap* ExtensionPrefValueMapFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ExtensionPrefValueMap*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ExtensionPrefValueMapFactory* ExtensionPrefValueMapFactory::GetInstance() {
  return Singleton<ExtensionPrefValueMapFactory>::get();
}

KeyedService* ExtensionPrefValueMapFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ExtensionPrefValueMap();
}
