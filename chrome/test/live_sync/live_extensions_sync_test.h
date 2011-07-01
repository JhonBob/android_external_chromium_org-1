// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_LIVE_SYNC_LIVE_EXTENSIONS_SYNC_TEST_H_
#define CHROME_TEST_LIVE_SYNC_LIVE_EXTENSIONS_SYNC_TEST_H_
#pragma once

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chrome/test/live_sync/live_sync_extension_helper.h"
#include "chrome/test/live_sync/live_sync_test.h"

class Profile;

class LiveExtensionsSyncTest : public LiveSyncTest {
 public:
  explicit LiveExtensionsSyncTest(TestType test_type);
  virtual ~LiveExtensionsSyncTest();

 protected:
  // Like LiveSyncTest::SetupClients(), but also sets up
  // |extension_helper_|.
  virtual bool SetupClients() OVERRIDE WARN_UNUSED_RESULT;

  // Returns true iff the profile with index |index| has the same extensions
  // as the verifier.
  bool HasSameExtensionsAsVerifier(int index) WARN_UNUSED_RESULT;

  // Returns true iff all existing profiles have the same extensions
  // as the verifier.
  bool AllProfilesHaveSameExtensionsAsVerifier() WARN_UNUSED_RESULT;

  // Returns true iff all existing profiles have the same extensions.
  bool AllProfilesHaveSameExtensions() WARN_UNUSED_RESULT;

  // Installs the extension for the given index to |profile|.
  void InstallExtension(Profile* profile, int index);

  // Uninstalls the extension for the given index from |profile|. Assumes that
  // it was previously installed.
  void UninstallExtension(Profile* profile, int index);

  // Returns a vector containing the indices of all currently installed
  // test extensions on |profile|.
  std::vector<int> GetInstalledExtensions(Profile* profile);

  // Installs all pending synced extensions for |profile|.
  void InstallExtensionsPendingForSync(Profile* profile);

  // Enables the extension for the given index on |profile|.
  void EnableExtension(Profile* profile, int index);

  // Disables the extension for the given index on |profile|.
  void DisableExtension(Profile* profile, int index);

  // Returns true if the extension with index |index| is enabled on |profile|.
  bool IsExtensionEnabled(Profile* profile, int index);

  // Enables the extension for the given index in incognito mode on |profile|.
  void IncognitoEnableExtension(Profile* profile, int index);

  // Disables the extension for the given index in incognito mode on |profile|.
  void IncognitoDisableExtension(Profile* profile, int index);

  // Returns true if the extension with index |index| is enabled in incognito
  // mode on |profile|.
  bool IsIncognitoEnabled(Profile* profile, int index);

  // Returns a unique extension name based in the integer |index|.
  static std::string CreateFakeExtensionName(int index);

  // Converts a fake extension name back into the index used to generate it.
  // Returns true if successful, false on failure.
  static bool ExtensionNameToIndex(const std::string& name, int* index);

 private:
  LiveSyncExtensionHelper extension_helper_;

  DISALLOW_COPY_AND_ASSIGN(LiveExtensionsSyncTest);
};

class SingleClientLiveExtensionsSyncTest : public LiveExtensionsSyncTest {
 public:
  SingleClientLiveExtensionsSyncTest()
      : LiveExtensionsSyncTest(SINGLE_CLIENT) {}

  virtual ~SingleClientLiveExtensionsSyncTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientLiveExtensionsSyncTest);
};

class TwoClientLiveExtensionsSyncTest : public LiveExtensionsSyncTest {
 public:
  TwoClientLiveExtensionsSyncTest()
      : LiveExtensionsSyncTest(TWO_CLIENT) {}

  virtual ~TwoClientLiveExtensionsSyncTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientLiveExtensionsSyncTest);
};


#endif  // CHROME_TEST_LIVE_SYNC_LIVE_EXTENSIONS_SYNC_TEST_H_
