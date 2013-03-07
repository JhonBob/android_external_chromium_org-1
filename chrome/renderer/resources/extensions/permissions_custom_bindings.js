// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom bindings for the Permissions API.

var chromeHidden = requireNative('chrome_hidden').GetChromeHidden();
var sendRequest = require('sendRequest').sendRequest;
var lastError = require('lastError');

// These custom bindings are only necessary because it is not currently
// possible to have a union of types as the type of the items in an array.
// Once that is fixed, this entire file should go away.
// See,
// https://code.google.com/p/chromium/issues/detail?id=162044
// https://code.google.com/p/chromium/issues/detail?id=162042
// TODO(bryeung): delete this file.
chromeHidden.registerCustomHook('permissions', function(api) {
  var apiFunctions = api.apiFunctions;

  function maybeConvertToObject(str) {
    var parts = str.split('|');
    if (parts.length != 2)
      return str;

    var ret = {};
    ret[parts[0]] = JSON.parse(parts[1]);
    return ret;
  }

  function convertObjectPermissionsToStrings() {
    if (arguments.length < 1)
      return arguments;

    var args = arguments[0].permissions;
    if (!args)
      return arguments;

    for (var i = 0; i < args.length; i += 1) {
      if (typeof(args[i]) == 'object') {
        var a = args[i];
        var keys = Object.keys(a);
        if (keys.length != 1) {
          throw new Error("Too many keys in object-style permission.");
        }
        arguments[0].permissions[i] = keys[0] + '|' +
            JSON.stringify(a[keys[0]]);
      }
    }

    return arguments;
  }

  // Convert complex permissions to strings so they validate against the schema
  apiFunctions.setUpdateArgumentsPreValidate(
      'contains', convertObjectPermissionsToStrings);
  apiFunctions.setUpdateArgumentsPreValidate(
      'remove', convertObjectPermissionsToStrings);
  apiFunctions.setUpdateArgumentsPreValidate(
      'request', convertObjectPermissionsToStrings);

  // Convert complex permissions back to objects
  apiFunctions.setCustomCallback('getAll',
      function(name, request, response) {
        for (var i = 0; i < response.permissions.length; i += 1) {
          response.permissions[i] =
              maybeConvertToObject(response.permissions[i]);
        }

        // Since the schema says Permissions.permissions contains strings and
        // not objects, validation will fail after the for-loop above.  This
        // skips validation and calls the callback directly, then clears it so
        // that handleResponse doesn't call it again.
        if (request.callback)
          request.callback.apply(request, [response]);
        delete request.callback;
      });

  // Also convert complex permissions back to objects for events.  The
  // dispatchToListener call happens after argument validation, which works
  // around the problem that Permissions.permissions is supposed to be a list
  // of strings.
  chrome.permissions.onAdded.dispatchToListener = function(callback, args) {
    for (var i = 0; i < args[0].permissions.length; i += 1) {
      args[0].permissions[i] = maybeConvertToObject(args[0].permissions[i]);
    }
    chrome.Event.prototype.dispatchToListener(callback, args);
  };
  chrome.permissions.onRemoved.dispatchToListener =
      chrome.permissions.onAdded.dispatchToListener;
});
