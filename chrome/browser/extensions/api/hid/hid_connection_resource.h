// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_HID_HID_CONNECTION_RESOURCE_H_
#define CHROME_BROWSER_EXTENSIONS_API_HID_HID_CONNECTION_RESOURCE_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "content/public/browser/browser_thread.h"
#include "device/hid/hid_connection.h"
#include "extensions/browser/api/api_resource.h"
#include "extensions/browser/api/api_resource_manager.h"

namespace extensions {

class HidConnectionResource : public ApiResource {
 public:
  HidConnectionResource(const std::string& owner_extension_id,
                        scoped_refptr<device::HidConnection> connection);
  virtual ~HidConnectionResource();

  scoped_refptr<device::HidConnection> connection() const {
    return connection_;
  }

  static const char* service_name() { return "HidConnectionResourceManager"; }

  static const content::BrowserThread::ID kThreadId =
      content::BrowserThread::FILE;

 private:
  scoped_refptr<device::HidConnection> connection_;

  DISALLOW_COPY_AND_ASSIGN(HidConnectionResource);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_HID_HID_CONNECTION_RESOURCE_H_
