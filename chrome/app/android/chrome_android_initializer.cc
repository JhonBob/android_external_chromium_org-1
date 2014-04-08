// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/android/chrome_android_initializer.h"

#include "base/android/jni_android.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/logging.h"
#include "chrome/app/android/chrome_main_delegate_android.h"
#include "chrome/common/chrome_version_info.h"
#include "content/public/app/android_library_loader_hooks.h"
#include "content/public/app/content_main.h"

jint RunChrome(JavaVM* vm, ChromeMainDelegateAndroid* main_delegate) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!base::android::RegisterLibraryLoaderEntryHook(env))
    return -1;

  // Pass the library version number to content so that we can check it from the
  // Java side before continuing initialization
  chrome::VersionInfo vi;
  base::android::SetLibraryLoadedHook(&content::LibraryLoaded);
  base::android::SetVersionNumber(vi.Version().c_str());

  DCHECK(main_delegate);
  content::SetContentMainDelegate(main_delegate);

  return JNI_VERSION_1_4;
}
