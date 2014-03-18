// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Main entry point called once the page has loaded.
 */
function onLoad() {
  NetExportView.getInstance();
}

document.addEventListener('DOMContentLoaded', onLoad);

/**
 * This class handles the presentation of our profiler view. Used as a
 * singleton.
 */
var NetExportView = (function() {
  'use strict';

  /**
   * Delay in milliseconds between updates of certain browser information.
   */
  /** @const */ var POLL_INTERVAL_MS = 5000;

  // --------------------------------------------------------------------------

  /**
   * @constructor
   */
  function NetExportView() {
    $('export-view-start-data').onclick = this.onStartData_.bind(this);
    $('export-view-stop-data').onclick = this.onStopData_.bind(this);
    $('export-view-send-data').onclick = this.onSendData_.bind(this);

    window.setInterval(function() { chrome.send('getExportNetLogInfo'); },
                       POLL_INTERVAL_MS);

    chrome.send('getExportNetLogInfo');
  }

  cr.addSingletonGetter(NetExportView);

  NetExportView.prototype = {
    /**
     * Starts saving NetLog data to a file.
     */
    onStartData_: function() {
      var stripPrivateData = $('export-view-private-data-toggle').checked;
      chrome.send('startNetLog', [stripPrivateData]);
    },

    /**
     * Stops saving NetLog data to a file.
     */
    onStopData_: function() {
      chrome.send('stopNetLog');
    },

    /**
     * Sends NetLog data via email from browser.
     */
    onSendData_: function() {
      chrome.send('sendNetLog');
    },

    /**
     * Updates the UI to reflect the current state. Displays the path name of
     * the file where NetLog data is collected.
     */
    onExportNetLogInfoChanged: function(exportNetLogInfo) {
      if (exportNetLogInfo.file) {
        var message = '';
        if (exportNetLogInfo.state == 'LOGGING')
          message = 'NetLog data is collected in: ';
        else if (exportNetLogInfo.logType != 'NONE')
          message = 'NetLog data to send is in: ';
        $('export-view-file-path-text').textContent =
            message + exportNetLogInfo.file;
      } else {
        $('export-view-file-path-text').textContent = '';
      }

      $('export-view-private-data-toggle').disabled = true;
      $('export-view-start-data').disabled = true;
      $('export-view-deletes-log-text').hidden = true;
      $('export-view-stop-data').disabled = true;
      $('export-view-send-data').disabled = true;
      $('export-view-private-data-text').hidden = true;
      $('export-view-send-old-log-text').hidden = true;
      if (exportNetLogInfo.state == 'NOT_LOGGING') {
        // Allow making a new log.
        $('export-view-private-data-toggle').disabled = false;
        $('export-view-start-data').disabled = false;

        // If there's an existing log, allow sending it.
        if (exportNetLogInfo.logType != 'NONE') {
          $('export-view-deletes-log-text').hidden = false;
          $('export-view-send-data').disabled = false;
          if (exportNetLogInfo.logType == 'UNKNOWN') {
            $('export-view-send-old-log-text').hidden = false;
          } else if (exportNetLogInfo.logType == 'NORMAL') {
            $('export-view-private-data-text').hidden = false;
          }
        }
      } else if (exportNetLogInfo.state == 'LOGGING') {
        // Only possible to stop logging. Checkbox reflects current state.
        $('export-view-private-data-toggle').checked =
            (exportNetLogInfo.logType == 'STRIP_PRIVATE_DATA');
        $('export-view-stop-data').disabled = false;
      } else if (exportNetLogInfo.state == 'UNINITIALIZED') {
        $('export-view-file-path-text').textContent =
            'Unable to initialize NetLog data file.';
      }
    }
  };

  return NetExportView;
})();
