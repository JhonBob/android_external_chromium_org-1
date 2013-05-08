// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/app/content_main_delegate.h"

#include "content/public/browser/content_browser_client.h"

#if !defined(OS_IOS)
#include "content/public/plugin/content_plugin_client.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/utility/content_utility_client.h"
#endif

namespace content {

bool ContentMainDelegate::BasicStartupComplete(int* exit_code) {
  return false;
}

int ContentMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  return -1;
}

#if defined(OS_MACOSX) && !defined(OS_IOS)

bool ContentMainDelegate::ProcessRegistersWithSystemProcess(
    const std::string& process_type) {
  return false;
}

bool ContentMainDelegate::ShouldSendMachPort(const std::string& process_type) {
  return true;
}

bool ContentMainDelegate::DelaySandboxInitialization(
    const std::string& process_type) {
  return false;
}

#elif defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_IOS)

ZygoteForkDelegate* ContentMainDelegate::ZygoteStarting() {
  return NULL;
}

#endif

ContentBrowserClient* ContentMainDelegate::CreateContentBrowserClient() {
  return new ContentBrowserClient();
}

ContentPluginClient* ContentMainDelegate::CreateContentPluginClient() {
#if defined(OS_IOS)
  return NULL;
#else
  return new ContentPluginClient();
#endif
}

ContentRendererClient* ContentMainDelegate::CreateContentRendererClient() {
  // TODO(scottmg): http://crbug.com/237249 Need split for chrome_child.dll.
#if defined(OS_IOS) || defined(CHROME_SPLIT_DLL)
  return NULL;
#else
  return new ContentRendererClient();
#endif
}

ContentUtilityClient* ContentMainDelegate::CreateContentUtilityClient() {
  // TODO(scottmg): http://crbug.com/237249 Need split for chrome_child.dll.
#if defined(OS_IOS) || defined(CHROME_SPLIT_DLL)
  return NULL;
#else
  return new ContentUtilityClient();
#endif
}

}  // namespace content
