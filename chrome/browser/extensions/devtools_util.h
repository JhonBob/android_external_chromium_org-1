// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_DEVTOOLS_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_DEVTOOLS_UTIL_H_

class Profile;

namespace extensions {
class Extension;

namespace devtools_util {

// Open a dev tools window for the background page for the given extension,
// starting the background page first if necessary.
void InspectBackgroundPage(const Extension* extension, Profile* profile);

}  // namespace devtools_util
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_DEVTOOLS_UTIL_H_
