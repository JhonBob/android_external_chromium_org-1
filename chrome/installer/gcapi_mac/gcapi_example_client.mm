// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/gcapi_mac/gcapi.h"

#import <Foundation/Foundation.h>
#include <getopt.h>

#include <string>

void Usage() {
  fprintf(stderr,
"usage: gcapi_example [options]\n"
"\n"
"options:\n"
"  --criteria-check    exit after criteria check\n"
"  --force-reinstall   delete Google Chrome from Applications first\n"
"  --install <path>    copy <path> to /Applications/Google Chrome.app, set up\n"
"  --brand <CODE>      set brandcode to <CODE> during installation\n"
"  --launch            launch Google Chrome when all is done\n"
"  --help              print this message\n"
);
}

int main(int argc, char* argv[]) {
  const option kLongOptions[] = {
    { "criteria-check", no_argument, NULL, 'c' },
    { "force-reinstall", no_argument, NULL, 'r' },
    { "install", required_argument, NULL, 'i' },
    { "brand", required_argument, NULL, 'b' },
    { "launch", no_argument, NULL, 'l' },
    { "help", no_argument, NULL, 'h' },
    { NULL, 0, NULL, 0 }
  };

  std::string source_path;
  std::string brand_code;
  bool check_only = false;
  bool reinstall = false;
  bool launch = false;
  int opt;
  while ((opt = getopt_long(argc, argv, "cri:b:lh", kLongOptions, NULL))
         != -1) {
    switch (opt) {
      case 'c':
        check_only = true;
        break;
      case 'r':
        reinstall = true;
        break;
      case 'i':
        source_path = optarg;
        break;
      case 'b':
        brand_code = optarg;
        break;
      case 'l':
        launch = true;
        break;
      case 'h':
      default:
        Usage();
        return 1;
    }
  }

  if (reinstall) {
    [[NSFileManager defaultManager]
        removeItemAtPath:@"/Applications/Google Chrome.app" error:nil];
  }

  unsigned reasons;
  int can_install = GoogleChromeCompatibilityCheck(&reasons);
  NSLog(@"can_install: %d, reasons %x", can_install, reasons);
  if (check_only)
    return 0;

  if (can_install && !source_path.empty()) {
    int install_result = InstallGoogleChrome(
        source_path.c_str(),
        brand_code.empty() ? NULL : brand_code.c_str(),
        NULL, 0);
    NSLog(@"install result: %d", install_result);
  }

  if (launch)
    LaunchGoogleChrome();
}
