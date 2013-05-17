// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/services/libc_urandom_override.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/rand_util.h"

// Note: this file is used by the zygote and nacl_helper.

namespace sandbox {

static bool g_override_urandom = false;

void InitLibcUrandomOverrides() {
  // Make sure /dev/urandom is open.
  base::GetUrandomFD();
  g_override_urandom = true;
}

// TODO(sergeyu): Currently this code doesn't work properly under ASAN
// - it crashes content_unittests. Make sure it works properly and
// enable it here. http://crbug.com/123263
#if !defined(ADDRESS_SANITIZER)

static const char kUrandomDevPath[] = "/dev/urandom";

typedef FILE* (*FopenFunction)(const char* path, const char* mode);
typedef int (*XstatFunction)(int version, const char *path, struct stat *buf);
typedef int (*Xstat64Function)(int version, const char *path,
                               struct stat64 *buf);

static pthread_once_t g_libc_file_io_funcs_guard = PTHREAD_ONCE_INIT;
static FopenFunction g_libc_fopen;
static FopenFunction g_libc_fopen64;
static XstatFunction g_libc_xstat;
static Xstat64Function g_libc_xstat64;

static void InitLibcFileIOFunctions() {
  g_libc_fopen = reinterpret_cast<FopenFunction>(
      dlsym(RTLD_NEXT, "fopen"));
  g_libc_fopen64 = reinterpret_cast<FopenFunction>(
      dlsym(RTLD_NEXT, "fopen64"));

  if (!g_libc_fopen) {
    LOG(FATAL) << "Failed to get fopen() from libc.";
  } else if (!g_libc_fopen64) {
#if !defined(OS_OPENBSD) && !defined(OS_FREEBSD)
    LOG(WARNING) << "Failed to get fopen64() from libc. Using fopen() instead.";
#endif  // !defined(OS_OPENBSD) && !defined(OS_FREEBSD)
    g_libc_fopen64 = g_libc_fopen;
  }

  // TODO(sergeyu): This works only on systems with glibc. Fix it to
  // work properly on other systems if necessary.
  g_libc_xstat = reinterpret_cast<XstatFunction>(
      dlsym(RTLD_NEXT, "__xstat"));
  g_libc_xstat64 = reinterpret_cast<Xstat64Function>(
      dlsym(RTLD_NEXT, "__xstat64"));

  if (!g_libc_xstat) {
    LOG(FATAL) << "Failed to get __xstat() from libc.";
  }
  if (!g_libc_xstat64) {
    LOG(WARNING) << "Failed to get __xstat64() from libc.";
  }
}

// fopen() and fopen64() are intercepted here so that NSS can open
// /dev/urandom to seed its random number generator. NSS is used by
// remoting in the sendbox.

// fopen() call may be redirected to fopen64() in stdio.h using
// __REDIRECT(), which sets asm name for fopen() to "fopen64". This
// means that we cannot override fopen() directly here. Instead the
// the code below defines fopen_override() function with asm name
// "fopen", so that all references to fopen() will resolve to this
// function.
__attribute__ ((__visibility__("default")))
FILE* fopen_override(const char* path, const char* mode)  __asm__ ("fopen");

__attribute__ ((__visibility__("default")))
FILE* fopen_override(const char* path, const char* mode) {
  if (g_override_urandom && strcmp(path, kUrandomDevPath) == 0) {
    int fd = HANDLE_EINTR(dup(base::GetUrandomFD()));
    if (fd < 0) {
      PLOG(ERROR) << "dup() failed.";
      return NULL;
    }
    return fdopen(fd, mode);
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    return g_libc_fopen(path, mode);
  }
}

__attribute__ ((__visibility__("default")))
FILE* fopen64(const char* path, const char* mode) {
  if (g_override_urandom && strcmp(path, kUrandomDevPath) == 0) {
    int fd = HANDLE_EINTR(dup(base::GetUrandomFD()));
    if (fd < 0) {
      PLOG(ERROR) << "dup() failed.";
      return NULL;
    }
    return fdopen(fd, mode);
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    return g_libc_fopen64(path, mode);
  }
}

// stat() is subject to the same problem as fopen(), so we have to use
// the same trick to override it.
__attribute__ ((__visibility__("default")))
int xstat_override(int version,
                   const char *path,
                   struct stat *buf)  __asm__ ("__xstat");

__attribute__ ((__visibility__("default")))
int xstat_override(int version, const char *path, struct stat *buf) {
  if (g_override_urandom && strcmp(path, kUrandomDevPath) == 0) {
    int result = __fxstat(version, base::GetUrandomFD(), buf);
    return result;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    return g_libc_xstat(version, path, buf);
  }
}

__attribute__ ((__visibility__("default")))
int xstat64_override(int version,
                     const char *path,
                     struct stat64 *buf)  __asm__ ("__xstat64");

__attribute__ ((__visibility__("default")))
int xstat64_override(int version, const char *path, struct stat64 *buf) {
  if (g_override_urandom && strcmp(path, kUrandomDevPath) == 0) {
    int result = __fxstat64(version, base::GetUrandomFD(), buf);
    return result;
  } else {
    CHECK_EQ(0, pthread_once(&g_libc_file_io_funcs_guard,
                             InitLibcFileIOFunctions));
    CHECK(g_libc_xstat64);
    return g_libc_xstat64(version, path, buf);
  }
}

#endif  // !ADDRESS_SANITIZER

}  // namespace content
