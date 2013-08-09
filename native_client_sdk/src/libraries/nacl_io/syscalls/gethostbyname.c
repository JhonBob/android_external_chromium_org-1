/* Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#include "nacl_io/ossocket.h"

#ifdef PROVIDES_SOCKET_API

#include "nacl_io/kernel_intercept.h"
#include "nacl_io/kernel_wrap.h"

struct hostent* gethostbyname(const char* name) {
  return ki_gethostbyname(name);
}

#endif /* PROVIDES_SOCKET_API */
