// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/navigation_interception/navigation_params_android.h"

#include "base/android/jni_string.h"
#include "jni/NavigationParams_jni.h"

using base::android::ConvertUTF8ToJavaString;

namespace navigation_interception {

base::android::ScopedJavaLocalRef<jobject> CreateJavaNavigationParams(
    JNIEnv* env,
    const NavigationParams& params) {
  ScopedJavaLocalRef<jstring> jstring_url =
      ConvertUTF8ToJavaString(env, params.url().spec());

  return Java_NavigationParams_create(env,
                                      jstring_url.obj(),
                                      params.is_post(),
                                      params.has_user_gesture(),
                                      params.transition_type(),
                                      params.is_redirect());
}

// Register native methods.

bool RegisterNavigationParams(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace navigation_interception
