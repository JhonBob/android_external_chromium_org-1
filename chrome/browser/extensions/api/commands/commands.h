// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_COMMANDS_COMMANDS_H_
#define CHROME_BROWSER_EXTENSIONS_API_COMMANDS_COMMANDS_H_

#include "chrome/browser/extensions/extension_function.h"

class GetAllCommandsFunction : public SyncExtensionFunction {
  virtual ~GetAllCommandsFunction() {}
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("commands.getAll")
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_COMMANDS_COMMANDS_H_
