// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SYNC_PREFS_H_
#define CHROME_BROWSER_SYNC_SYNC_PREFS_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/non_thread_safe.h"
#include "base/time.h"
#include "chrome/browser/prefs/pref_member.h"
#include "content/public/browser/notification_observer.h"
#include "sync/internal_api/public/base/model_type.h"
#include "sync/notifier/invalidation_state_tracker.h"

class PrefService;

namespace browser_sync {

class SyncPrefObserver {
 public:
  // Called whenever the pref that controls whether sync is managed
  // changes.
  virtual void OnSyncManagedPrefChange(bool is_sync_managed) = 0;

 protected:
  virtual ~SyncPrefObserver();
};

// SyncPrefs is a helper class that manages getting, setting, and
// persisting global sync preferences.  It is not thread-safe, and
// lives on the UI thread.
//
// TODO(akalin): Some classes still read the prefs directly.  Consider
// passing down a pointer to SyncPrefs to them.  A list of files:
//
//   profile_sync_service_startup_unittest.cc
//   profile_sync_service.cc
//   sync_setup_flow.cc
//   sync_setup_wizard.cc
//   sync_setup_wizard_unittest.cc
//   two_client_preferences_sync_test.cc
class SyncPrefs : NON_EXPORTED_BASE(public base::NonThreadSafe),
                  public base::SupportsWeakPtr<SyncPrefs>,
                  public content::NotificationObserver {
 public:
  // |pref_service| may be NULL (for unit tests), but in that case no
  // setter methods should be called.  Does not take ownership of
  // |pref_service|.
  explicit SyncPrefs(PrefService* pref_service);

  virtual ~SyncPrefs();

  void AddSyncPrefObserver(SyncPrefObserver* sync_pref_observer);
  void RemoveSyncPrefObserver(SyncPrefObserver* sync_pref_observer);

  // Clears important sync preferences.
  void ClearPreferences();

  // Getters and setters for global sync prefs.

  bool HasSyncSetupCompleted() const;
  void SetSyncSetupCompleted();

  bool IsStartSuppressed() const;
  void SetStartSuppressed(bool is_suppressed);

  std::string GetGoogleServicesUsername() const;

  base::Time GetLastSyncedTime() const;
  void SetLastSyncedTime(base::Time time);

  bool HasKeepEverythingSynced() const;
  void SetKeepEverythingSynced(bool keep_everything_synced);

  // The returned set is guaranteed to be a subset of
  // |registered_types|.  Returns |registered_types| directly if
  // HasKeepEverythingSynced() is true.
  syncer::ModelTypeSet GetPreferredDataTypes(
      syncer::ModelTypeSet registered_types) const;
  // |preferred_types| should be a subset of |registered_types|.  All
  // types in |preferred_types| are marked preferred, and all types in
  // |registered_types| \ |preferred_types| are marked not preferred.
  // Changes are still made to the prefs even if
  // HasKeepEverythingSynced() is true, but won't be visible until
  // SetKeepEverythingSynced(false) is called.
  void SetPreferredDataTypes(
    syncer::ModelTypeSet registered_types,
    syncer::ModelTypeSet preferred_types);

  // This pref is set outside of sync.
  bool IsManaged() const;

  // Use this encryption bootstrap token if we're using an explicit passphrase.
  std::string GetEncryptionBootstrapToken() const;
  void SetEncryptionBootstrapToken(const std::string& token);

  // Use this keystore bootstrap token if we're not using an explicit
  // passphrase.
  std::string GetKeystoreEncryptionBootstrapToken() const;
  void SetKeystoreEncryptionBootstrapToken(const std::string& token);

  // Maps |data_type| to its corresponding preference name.
  static const char* GetPrefNameForDataType(syncer::ModelType data_type);

#if defined(OS_CHROMEOS)
  // Use this spare bootstrap token only when setting up sync for the first
  // time.
  std::string GetSpareBootstrapToken() const;
  void SetSpareBootstrapToken(const std::string& token);
#endif

  // Merges the given set of types with the set of acknowledged types.
  void AcknowledgeSyncedTypes(syncer::ModelTypeSet types);

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // For testing.

  void SetManagedForTest(bool is_managed);
  syncer::ModelTypeSet GetAcknowledgeSyncedTypesForTest() const;

 private:
  void RegisterPrefGroups();
  void RegisterPreferences();

  void RegisterDataTypePreferredPref(
      syncer::ModelType type, bool is_preferred);
  bool GetDataTypePreferred(syncer::ModelType type) const;
  void SetDataTypePreferred(syncer::ModelType type, bool is_preferred);

  // Returns a ModelTypeSet based on |types| expanded to include pref groups
  // (see |pref_groups_|), but as a subset of |registered_types|.
  syncer::ModelTypeSet ResolvePrefGroups(
      syncer::ModelTypeSet registered_types,
      syncer::ModelTypeSet types) const;

  // May be NULL.
  PrefService* const pref_service_;

  ObserverList<SyncPrefObserver> sync_pref_observers_;

  // The preference that controls whether sync is under control by
  // configuration management.
  BooleanPrefMember pref_sync_managed_;

  // Groups of prefs that always have the same value as a "master" pref.
  // For example, the APPS group has {APP_NOTIFICATIONS, APP_SETTINGS}
  // (as well as APPS, but that is implied), so
  //   pref_groups_[syncer::APPS] =       { syncer::APP_NOTIFICATIONS,
  //                                          syncer::APP_SETTINGS }
  //   pref_groups_[syncer::EXTENSIONS] = { syncer::EXTENSION_SETTINGS }
  // etc.
  typedef std::map<syncer::ModelType, syncer::ModelTypeSet> PrefGroupsMap;
  PrefGroupsMap pref_groups_;

  DISALLOW_COPY_AND_ASSIGN(SyncPrefs);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_SYNC_PREFS_H_
