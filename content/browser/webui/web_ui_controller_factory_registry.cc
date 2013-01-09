// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/web_ui_controller_factory_registry.h"

#include "base/lazy_instance.h"

namespace content {

base::LazyInstance<std::vector<WebUIControllerFactory*> > g_factories =
    LAZY_INSTANCE_INITIALIZER;

void WebUIControllerFactory::RegisterFactory(WebUIControllerFactory* factory) {
  g_factories.Pointer()->push_back(factory);
}

WebUIControllerFactoryRegistry* WebUIControllerFactoryRegistry::GetInstance() {
  return Singleton<WebUIControllerFactoryRegistry>::get();
}

void WebUIControllerFactoryRegistry::UnregisterFactoryForTesting(
    WebUIControllerFactory* factory) {
  std::vector<WebUIControllerFactory*>* factories = g_factories.Pointer();
  for (size_t i = 0; i < factories->size(); ++i) {
    if ((*factories)[i] == factory) {
      factories->erase(factories->begin() + i);
      return;
    }
  }
  NOTREACHED() << "Tried to unregister a factory but it wasn't found";
}

WebUIController* WebUIControllerFactoryRegistry::CreateWebUIControllerForURL(
    WebUI* web_ui, const GURL& url) const {
  std::vector<WebUIControllerFactory*>* factories = g_factories.Pointer();
  for (size_t i = 0; i < factories->size(); ++i) {
    WebUIController* controller = (*factories)[i]->CreateWebUIControllerForURL(
        web_ui, url);
    if (controller)
      return controller;
  }
  return NULL;
}

WebUI::TypeID WebUIControllerFactoryRegistry::GetWebUIType(
    BrowserContext* browser_context, const GURL& url) const {
  std::vector<WebUIControllerFactory*>* factories = g_factories.Pointer();
  for (size_t i = 0; i < factories->size(); ++i) {
    WebUI::TypeID type = (*factories)[i]->GetWebUIType(browser_context, url);
    if (type != WebUI::kNoWebUI)
      return type;
  }
  return WebUI::kNoWebUI;
}

bool WebUIControllerFactoryRegistry::UseWebUIForURL(
    BrowserContext* browser_context, const GURL& url) const {
  std::vector<WebUIControllerFactory*>* factories = g_factories.Pointer();
  for (size_t i = 0; i < factories->size(); ++i) {
    if ((*factories)[i]->UseWebUIForURL(browser_context, url))
      return true;
  }
  return false;
}

bool WebUIControllerFactoryRegistry::UseWebUIBindingsForURL(
    BrowserContext* browser_context, const GURL& url) const {
  std::vector<WebUIControllerFactory*>* factories = g_factories.Pointer();
  for (size_t i = 0; i < factories->size(); ++i) {
    if ((*factories)[i]->UseWebUIBindingsForURL(browser_context, url))
      return true;
  }
  return false;
}

bool WebUIControllerFactoryRegistry::IsURLAcceptableForWebUI(
    BrowserContext* browser_context,
    const GURL& url,
    bool data_urls_allowed) const {
  std::vector<WebUIControllerFactory*>* factories = g_factories.Pointer();
  for (size_t i = 0; i < factories->size(); ++i) {
    if ((*factories)[i]->IsURLAcceptableForWebUI(
            browser_context, url, data_urls_allowed)) {
      return true;
    }
  }
  return false;
}

WebUIControllerFactoryRegistry::WebUIControllerFactoryRegistry() {
}

WebUIControllerFactoryRegistry::~WebUIControllerFactoryRegistry() {
}

}  // namespace content
