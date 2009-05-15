// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_FUNCTION_DISPATCHER_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_FUNCTION_DISPATCHER_H_

#include <map>
#include <string>
#include <vector>

#include "base/values.h"

class Browser;
class ExtensionFunction;
class Profile;
class RenderViewHost;
class RenderViewHostDelegate;

// A factory function for creating new ExtensionFunction instances.
typedef ExtensionFunction* (*ExtensionFunctionFactory)();

// ExtensionFunctionDispatcher receives requests to execute functions from
// Chromium extensions running in a RenderViewHost and dispatches them to the
// appropriate handler. It lives entirely on the UI thread.
class ExtensionFunctionDispatcher {
 public:
  class Delegate {
   public:
    virtual Browser* GetBrowser() = 0;
  };

  // Gets a list of all known extension function names.
  static void GetAllFunctionNames(std::vector<std::string>* names);

  // Override a previously registered function. Returns true if successful,
  // false if no such function was registered.
  static bool OverrideFunction(const std::string& name,
                               ExtensionFunctionFactory factory);

  // Resets all functions to their initial implementation.
  static void ResetFunctions();

  ExtensionFunctionDispatcher(RenderViewHost* render_view_host,
                              Delegate* delegate,
                              const std::string& extension_id);

  // Handle a request to execute an extension function.
  void HandleRequest(const std::string& name, const std::string& args,
                     int request_id, bool has_callback);

  // Send a response to a function.
  void SendResponse(ExtensionFunction* api, bool success);

  // Gets the browser extension functions should operate relative to. For
  // example, for positioning windows, or alert boxes, or creating tabs.
  Browser* GetBrowser();

  // Handle a malformed message.  Possibly the result of an attack, so kill
  // the renderer.
  void HandleBadMessage(ExtensionFunction* api);

  // Gets the ID for this extension.
  std::string extension_id() { return extension_id_; }

  // The profile that this dispatcher is associated with.
  Profile* profile();

 private:
  RenderViewHost* render_view_host_;

  Delegate* delegate_;

  std::string extension_id_;

  // AutomationExtensionFunction requires access to the RenderViewHost
  // associated with us.  We make it a friend rather than exposing the
  // RenderViewHost as a public method as we wouldn't want everyone to
  // start assuming a 1:1 relationship between us and RenderViewHost,
  // whereas AutomationExtensionFunction is by necessity "tight" with us.
  friend class AutomationExtensionFunction;
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_FUNCTION_DISPATCHER_H_
