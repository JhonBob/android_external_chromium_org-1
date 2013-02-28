// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * The deserialized form of the chromoting host as returned by Apiary.
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * Note that the object has more fields than are detailed below--these
 * are just the ones that we refer to directly.
 * @constructor
 */
remoting.Host = function() {
  /** @type {string} */
  this.hostName = '';
  /** @type {string} */
  this.hostId = '';
  /** @type {string} */
  this.status = '';
  /** @type {string} */
  this.jabberId = '';
  /** @type {string} */
  this.publicKey = '';
  /** @type {string} */
  this.hostVersion = '';
};

/**
 * Determine whether a host needs to be manually updated. This is the case if
 * the host's major version number is more than 1 lower than that of the web-
 * app (a difference of 1 is tolerated due to the different update mechanisms)
 * and if the host is on-line (off-line hosts are not expected to auto-update).
 *
 * @param {remoting.Host} host The host information from the directory.
 * @param {string|number} webappVersion The version number of the web-app, in
 *     either dotted-decimal notation notation, or directly represented by the
 *     major version.
 * @return {boolean} True if the host is on-line but out-of-date.
 */
remoting.Host.needsUpdate = function(host, webappVersion) {
  if (host.status != 'ONLINE') {
    return false;
  }
  var hostMajorVersion = parseInt(host.hostVersion, 10);
  if (isNaN(hostMajorVersion)) {
    // Host versions 26 and higher include the version number in heartbeats,
    // so if it's missing then the host is at most version 25.
    hostMajorVersion = 25;
  }
  return (parseInt(webappVersion, 10) - hostMajorVersion) > 1;
};
