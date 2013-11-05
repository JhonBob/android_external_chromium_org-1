// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var REMOTE_DEBUGGER_HOST = 'localhost:9222';

function requestUrl(path, callback) {
  var req = new XMLHttpRequest();
  req.open('GET', 'http://' + REMOTE_DEBUGGER_HOST + path, true);
  req.onload = function() {
    if (req.status == 200)
      callback(req.responseText);
    else
      req.onerror();
  };
  req.onerror = function() {
    chrome.test.fail('XHR failed: ' + req.status);
  };
  req.send(null);
}


function checkTarget(targets, url, type, opt_title, opt_faviconUrl) {
  var target =
      targets.filter(function(t) { return t.url == url }) [0];
  if (!target)
    chrome.test.fail('Cannot find a target with url ' + url);

  var wsAddress = REMOTE_DEBUGGER_HOST + '/devtools/page/' + target.id;

  chrome.test.assertEq(
      '/devtools/devtools.html?ws=' + wsAddress,
      target.devtoolsFrontendUrl);
  // On some platforms (e.g. Chrome OS) target.faviconUrl might be empty for
  // a freshly created tab. Ignore the check then.
  if (target.faviconUrl && opt_faviconUrl)
    chrome.test.assertEq(opt_faviconUrl, target.faviconUrl);
  // Sometimes thumbnailUrl is not available for a freshly loaded tab.
  if (target.thumbnailUrl)
    chrome.test.assertEq('/thumb/' + target.id, target.thumbnailUrl);
  chrome.test.assertEq(opt_title || target.url, target.title);
  chrome.test.assertEq(type, target.type);
  chrome.test.assertEq('ws://' + wsAddress, target.webSocketDebuggerUrl);

  return target;
}

function waitForTab(filter, callback) {
  var fired = false;
  function onUpdated(updatedTabId, changeInfo, updatedTab) {
    if (!filter(updatedTab) && !fired)
      return;

    chrome.tabs.onUpdated.removeListener(onUpdated);
    if (!fired) {
      fired = true;
      callback(updatedTab);
    }
  }

  chrome.tabs.onUpdated.addListener(onUpdated);

  chrome.tabs.query({}, function(tabs) {
    if (!fired) {
      for (var i = 0; i < tabs.length; ++i)
        if (filter(tabs[i])) {
          fired = true;
          callback(tabs[i]);
        }
    }
  });
}

function listenOnce(event, func) {
  var listener = function() {
    event.removeListener(listener);
    func.apply(null, arguments)
  };
  event.addListener(listener);
}

function runNewPageTest(devtoolsUrl, expectedUrl) {
  var json;
  var newTab;
  var pendingCount = 2;

  function checkResult() {
    if (--pendingCount)
      return;
    chrome.test.assertEq(newTab.url, expectedUrl);
    chrome.test.assertEq(json.url, expectedUrl);
    chrome.test.assertTrue(newTab.active);
    chrome.test.succeed();
  }

  function onCreated(createdTab) {
    waitForTab(
      function(tab) {
        return tab.id == createdTab.id &&
               tab.active &&
               tab.status == "complete";
      },
      function(tab) {
        newTab = tab;
        checkResult();
      });
  }

  listenOnce(chrome.tabs.onCreated, onCreated);

  requestUrl(devtoolsUrl,
      function(text) {
        json = JSON.parse(text);
        checkResult();
      });
}

var extensionTargetId;
var extensionTabId;

chrome.test.runTests([
  function discoverTargets() {
    var testPageUrl = chrome.extension.getURL('test_page.html');

    function onUpdated(updatedTabId) {
      requestUrl('/json', function(text) {
        var targets = JSON.parse(text);
        waitForTab(
            function(tab) {
              return tab.id == updatedTabId && tab.status == "complete"
            },
            function() {
              checkTarget(targets, 'about:blank', 'page');
              checkTarget(targets,
                  chrome.extension.getURL('_generated_background_page.html'),
                  'background_page',
                  'Remote Debugger Test');
              extensionTargetId = checkTarget(targets,
                  testPageUrl, 'page', 'Test page',
                  chrome.extension.getURL('favicon.png')).id;
              chrome.test.succeed();
            });
        });
    }
    listenOnce(chrome.tabs.onUpdated, onUpdated);
    chrome.tabs.create({url: testPageUrl});
  },

  function versionInfo() {
    requestUrl('/json/version',
        function(text) {
          var versionInfo = JSON.parse(text);
          chrome.test.assertTrue(/^Chrome\//.test(versionInfo["Browser"]));
          chrome.test.assertTrue("Protocol-Version" in versionInfo);
          chrome.test.assertTrue("User-Agent" in versionInfo);
          chrome.test.assertTrue("WebKit-Version" in versionInfo);
          chrome.test.succeed();
        });
  },

  function activatePage() {
    requestUrl('/json/activate/' + extensionTargetId,
        function(text) {
          chrome.test.assertEq(text, "Target activated");
          waitForTab(
              function(tab) {
                return tab.active &&
                       tab.status == "complete" &&
                       tab.title == 'Test page';
              },
              function (tab) {
                extensionTabId = tab.id;
                chrome.test.succeed();
              });
        });
  },

  function closePage() {
    function onRemoved(tabId) {
      chrome.test.assertEq(tabId, extensionTabId);
      chrome.test.succeed();
    }

    listenOnce(chrome.tabs.onRemoved, onRemoved);

    requestUrl('/json/close/' + extensionTargetId, function(text) {
      chrome.test.assertEq(text, "Target is closing");
    });
  },

  function newSpecificPage() {
    runNewPageTest('/json/new?chrome://version/', "chrome://version/");
  },

  function newDefaultPage() {
    runNewPageTest('/json/new', "about:blank");
  }
]);
