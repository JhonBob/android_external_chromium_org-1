// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Helper functions used for the WebRequest API.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DECLARATIVE_WEBREQUEST_WEBREQUEST_HELPERS_H_
#define CHROME_BROWSER_EXTENSIONS_API_DECLARATIVE_WEBREQUEST_WEBREQUEST_HELPERS_H_
#pragma once

#include <string>
#include <vector>

namespace base {
class Value;
}

namespace extensions {
namespace declarative_webrequest_helpers {

// Converts a ValueList |value| of strings into a vector. Returns true if
// successful.
bool GetAsStringVector(const base::Value* value, std::vector<std::string>* out);

}  // namespace declarative_webrequest_helpers
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DECLARATIVE_WEBREQUEST_WEBREQUEST_HELPERS_H_
