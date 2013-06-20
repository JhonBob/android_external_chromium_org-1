// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/webkit_test_platform_support.h"

#include "third_party/skia/include/ports/SkTypeface_android.h"

namespace {

// The root directory on the device to which resources will be pushed. This
// value needs to be equal to that set in chromium_android.py.
#define DEVICE_SOURCE_ROOT_DIR "/data/local/tmp/content_shell/"

// Primary font configuration file on the device for Skia.
const char kPrimaryFontConfig[] =
    DEVICE_SOURCE_ROOT_DIR "android_main_fonts.xml";

// The file on the device containing the fallback font configuration for Skia.
const char kFallbackFontConfig[] =
    DEVICE_SOURCE_ROOT_DIR "android_fallback_fonts.xml";

// The directory in which fonts will be stored on the Android device.
const char kFontDirectory[] = DEVICE_SOURCE_ROOT_DIR "fonts/";

}  // namespace

namespace content {

bool CheckLayoutSystemDeps() {
  return true;
}

bool WebKitTestPlatformInitialize() {
  // Initialize Skia with the font configuration files crafted for layout tests.
  SkUseTestFontConfigFile(
      kPrimaryFontConfig, kFallbackFontConfig, kFontDirectory);

  return true;
}

}  // namespace content
