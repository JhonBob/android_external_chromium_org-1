// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/command_line.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/prefs/pref_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/browser/notifications/sync_notifier/chrome_notifier_service.h"
#include "chrome/browser/notifications/sync_notifier/sync_notifier_test_utils.h"
#include "chrome/browser/notifications/sync_notifier/synced_notification.h"
#include "chrome/browser/notifications/sync_notifier/synced_notification_app_info.h"
#include "chrome/browser/notifications/sync_notifier/synced_notification_app_info_service.h"
#include "chrome/browser/notifications/sync_notifier/synced_notification_app_info_service_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_pref_service_syncable.h"
#include "chrome/test/base/testing_profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "sync/api/sync_change.h"
#include "sync/api/sync_change_processor.h"
#include "sync/api/sync_change_processor_wrapper_for_test.h"
#include "sync/api/sync_error_factory.h"
#include "sync/api/sync_error_factory_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_pb::SyncedNotificationSpecifics;
using sync_pb::EntitySpecifics;
using syncer::SyncData;
using syncer::SyncChange;
using syncer::SyncChangeList;
using syncer::SyncChangeProcessorWrapperForTest;
using syncer::SyncDataList;
using syncer::SYNCED_NOTIFICATIONS;
using notifier::SyncedNotification;
using notifier::ChromeNotifierService;

namespace {

// Extract notification id from syncer::SyncData.
std::string GetNotificationId(const SyncData& sync_data) {
  SyncedNotificationSpecifics specifics = sync_data.GetSpecifics().
      synced_notification();

  return specifics.coalesced_notification().key();
}

}  // namespace

namespace notifier {

// Dummy SyncChangeProcessor used to help review what SyncChanges are pushed
// back up to Sync.
class TestChangeProcessor : public syncer::SyncChangeProcessor {
 public:
  TestChangeProcessor() { }
  virtual ~TestChangeProcessor() { }

  // Store a copy of all the changes passed in so we can examine them later.
  virtual syncer::SyncError ProcessSyncChanges(
      const tracked_objects::Location& from_here,
      const SyncChangeList& change_list) OVERRIDE {
    change_map_.clear();
    for (SyncChangeList::const_iterator iter = change_list.begin();
        iter != change_list.end(); ++iter) {
      // Put the data into the change tracking map.
      change_map_[GetNotificationId(iter->sync_data())] = *iter;
    }

    return syncer::SyncError();
  }

  virtual syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const
      OVERRIDE {
    return syncer::SyncDataList();
  }

  size_t change_list_size() { return change_map_.size(); }

  bool ContainsId(const std::string& id) {
    return change_map_.find(id) != change_map_.end();
  }

  SyncChange GetChangeById(const std::string& id) {
    EXPECT_TRUE(ContainsId(id));
    return change_map_[id];
  }

 private:
  // Track the changes received in ProcessSyncChanges.
  std::map<std::string, SyncChange> change_map_;

  DISALLOW_COPY_AND_ASSIGN(TestChangeProcessor);
};

class ChromeNotifierServiceTest : public testing::Test {
 public:
  ChromeNotifierServiceTest()
      : sync_processor_(new TestChangeProcessor),
        sync_processor_wrapper_(
            new SyncChangeProcessorWrapperForTest(sync_processor_.get())) {}
  virtual ~ChromeNotifierServiceTest() {}

  // Methods from testing::Test.
  virtual void SetUp() {
    // These tests rely on synced notifications being active.  Some testers
    // report the channel as STABLE so we need to manually enable it.
    // See crbug.com/338426 for details.
    CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableSyncSyncedNotifications);

    // Prevent test code from trying to go to the network.
    ChromeNotifierService::set_avoid_bitmap_fetching_for_test(true);
    notification_manager_.reset(new StubNotificationUIManager(GURL(
        kSyncedNotificationsWelcomeOrigin)));

    // Set up a profile for the unit tests to use.
    profile_.reset(new TestingProfile());

    // Set up the testing SyncedNotificationAppInfoService with some test data.
    AddTestingAppInfos();
  }

  virtual void TearDown() {
    notification_manager_.reset();
  }

  StubNotificationUIManager* notification_manager() {
    return notification_manager_.get();
  }

  TestChangeProcessor* processor() {
    return static_cast<TestChangeProcessor*>(sync_processor_.get());
  }

  scoped_ptr<syncer::SyncChangeProcessor> PassProcessor() {
    return sync_processor_wrapper_.Pass();
  }

  SyncedNotification* CreateNotification(
      const std::string& title,
      const std::string& text,
      const std::string& app_icon_url,
      const std::string& image_url,
      const std::string& app_id,
      const std::string& key,
      sync_pb::CoalescedSyncedNotification_ReadState read_state) {
    SyncData sync_data = CreateSyncData(title, text, app_icon_url, image_url,
                                        app_id, key, read_state);
    // Set enough fields in sync_data, including specifics, for our tests
    // to pass.
    return new SyncedNotification(sync_data, NULL, notification_manager_.get());
  }

  // Helper to create syncer::SyncChange.
  static SyncChange CreateSyncChange(
      SyncChange::SyncChangeType type,
      SyncedNotification* notification) {
    // Take control of the notification to clean it up after we create data
    // out of it.
    scoped_ptr<SyncedNotification> scoped_notification(notification);
    return SyncChange(
        FROM_HERE,
        type,
        ChromeNotifierService::CreateSyncDataFromNotification(*notification));
  }

  void AddTestingAppInfos() {
    // Get the SyncedNotificationAppInfoService from the browser object.
    SyncedNotificationAppInfoService* synced_notification_app_info_service =
        SyncedNotificationAppInfoServiceFactory::GetForProfile(
            profile_.get(), Profile::EXPLICIT_ACCESS);

    // Create a notification to add.
    // The sending_service_infos_ list will take ownership of this pointer.
    scoped_ptr<SyncedNotificationAppInfo> test_service1(
        new SyncedNotificationAppInfo(profile_.get(),
                                      std::string(kSendingService1Name),
                                      synced_notification_app_info_service));

    // Add some App IDs.
    test_service1->AddAppId(kAppId1);
    test_service1->AddAppId(kAppId2);

    // Set this icon's GURLs.
    test_service1->SetSettingsURLs(GURL(kTestIconUrl), GURL());

    // Call AddForTest.
    synced_notification_app_info_service->AddForTest(test_service1.Pass());
  }

 protected:
  scoped_ptr<TestingProfile> profile_;

 private:
  scoped_ptr<syncer::SyncChangeProcessor> sync_processor_;
  scoped_ptr<syncer::SyncChangeProcessor> sync_processor_wrapper_;
  scoped_ptr<StubNotificationUIManager> notification_manager_;
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(ChromeNotifierServiceTest);
};

// Create a Notification, convert it to SyncData and convert it back.
TEST_F(ChromeNotifierServiceTest, NotificationToSyncDataToNotification) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());

  scoped_ptr<SyncedNotification> notification1(
      CreateNotification(kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1,
                         kKey1, kUnread));
  SyncData sync_data =
      ChromeNotifierService::CreateSyncDataFromNotification(*notification1);
  scoped_ptr<SyncedNotification> notification2(
      notifier.CreateNotificationFromSyncData(sync_data));
  EXPECT_TRUE(notification2.get());
  EXPECT_TRUE(notification1->EqualsIgnoringReadState(*notification2));
  EXPECT_EQ(notification1->GetReadState(), notification2->GetReadState());
}

// Model assocation:  We have no local data, and no remote data.
TEST_F(ChromeNotifierServiceTest, ModelAssocBothEmpty) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());

  notifier.MergeDataAndStartSyncing(
      SYNCED_NOTIFICATIONS,
      SyncDataList(),  // Empty.
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  EXPECT_EQ(0U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());
  EXPECT_EQ(0U, processor()->change_list_size());
}

// Process sync changes when there is no local data.
TEST_F(ChromeNotifierServiceTest, ProcessSyncChangesEmptyModel) {
  // We initially have no data.
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  notifier.MergeDataAndStartSyncing(
      SYNCED_NOTIFICATIONS,
      SyncDataList(),
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  // Set up a bunch of ADDs.
  SyncChangeList changes;
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_ADD, CreateNotification(
          kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread)));
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_ADD, CreateNotification(
          kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kUnread)));
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_ADD, CreateNotification(
          kTitle3, kText3, kIconUrl3, kImageUrl3, kAppId3, kKey3, kUnread)));

  notifier.ProcessSyncChanges(FROM_HERE, changes);

  EXPECT_EQ(3U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());
  // TODO(petewil): verify that the list entries have expected values to make
  // this test more robust.
}

// Process sync changes when there is local data.
TEST_F(ChromeNotifierServiceTest, ProcessSyncChangesNonEmptyModel) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  notifier.AddForTest(n1.Pass());
  scoped_ptr<SyncedNotification> n2(CreateNotification(
      kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kUnread));
  notifier.AddForTest(n2.Pass());
  scoped_ptr<SyncedNotification> n3(CreateNotification(
      kTitle3, kText3, kIconUrl3, kImageUrl3, kAppId3, kKey3, kUnread));
  notifier.AddForTest(n3.Pass());

  notifier.MergeDataAndStartSyncing(
      SYNCED_NOTIFICATIONS,
      SyncDataList(),
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  // Set up some ADDs, some UPDATES, and some DELETEs
  SyncChangeList changes;
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_ADD, CreateNotification(
          kTitle4, kText4, kIconUrl4, kImageUrl4, kAppId4, kKey4, kUnread)));
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_UPDATE, CreateNotification(
          kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kRead)));
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_DELETE, CreateNotification(
          kTitle3, kText3, kIconUrl3, kImageUrl3, kAppId3, kKey3, kDismissed)));

  // Simulate incoming new notifications at runtime.
  notifier.ProcessSyncChanges(FROM_HERE, changes);

  // We should find notifications 1, 2, and 4, but not 3.
  EXPECT_EQ(3U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());
  EXPECT_TRUE(notifier.FindNotificationById(kKey1));
  EXPECT_TRUE(notifier.FindNotificationById(kKey2));
  EXPECT_FALSE(notifier.FindNotificationById(kKey3));
  EXPECT_TRUE(notifier.FindNotificationById(kKey4));

  // Verify that the first run preference is now set to false.
  bool first_run = notifier.profile()->GetPrefs()->GetBoolean(
      prefs::kSyncedNotificationFirstRun);
  EXPECT_NE(true, first_run);
}

// Process sync changes that arrive before the change they are supposed to
// modify.
TEST_F(ChromeNotifierServiceTest, ProcessSyncChangesOutOfOrder) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  notifier.AddForTest(n1.Pass());
  scoped_ptr<SyncedNotification> n2(CreateNotification(
      kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kUnread));
  notifier.AddForTest(n2.Pass());
  scoped_ptr<SyncedNotification> n3(CreateNotification(
      kTitle3, kText3, kIconUrl3, kImageUrl3, kAppId3, kKey3, kUnread));
  notifier.AddForTest(n3.Pass());

  notifier.MergeDataAndStartSyncing(
      SYNCED_NOTIFICATIONS,
      SyncDataList(),
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  SyncChangeList changes;
  // UPDATE a notification we have not seen an add for.
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_UPDATE, CreateNotification(
          kTitle4, kText4, kIconUrl4, kImageUrl4, kAppId4, kKey4, kUnread)));
  // ADD a notification that we already have.
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_ADD, CreateNotification(
          kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kRead)));
  // DELETE a notification we have not seen yet.
  changes.push_back(CreateSyncChange(
      SyncChange::ACTION_DELETE, CreateNotification(
          kTitle5, kText5, kIconUrl5, kImageUrl5, kAppId5, kKey5, kDismissed)));

  // Simulate incoming new notifications at runtime.
  notifier.ProcessSyncChanges(FROM_HERE, changes);

  // We should find notifications 1, 2, 3, and 4, but not 5.
  EXPECT_EQ(4U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());
  EXPECT_TRUE(notifier.FindNotificationById(kKey1));
  EXPECT_TRUE(notifier.FindNotificationById(kKey2));
  EXPECT_TRUE(notifier.FindNotificationById(kKey3));
  EXPECT_TRUE(notifier.FindNotificationById(kKey4));
  EXPECT_FALSE(notifier.FindNotificationById(kKey5));
}


// Model has some notifications, some of them are local only. Sync has some
// notifications. No items match up.
TEST_F(ChromeNotifierServiceTest, LocalRemoteBothNonEmptyNoOverlap) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  notifier.AddForTest(n1.Pass());
  scoped_ptr<SyncedNotification> n2(CreateNotification(
      kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kUnread));
  notifier.AddForTest(n2.Pass());
  scoped_ptr<SyncedNotification> n3(CreateNotification(
      kTitle3, kText3, kIconUrl3, kImageUrl3, kAppId3, kKey3, kUnread));
  notifier.AddForTest(n3.Pass());

  // Create some remote fake data.
  SyncDataList initial_data;
  initial_data.push_back(CreateSyncData(kTitle4, kText4, kIconUrl4, kImageUrl4,
                                        kAppId4, kKey4, kUnread));
  initial_data.push_back(CreateSyncData(kTitle5, kText5, kIconUrl5, kImageUrl5,
                                        kAppId5, kKey5, kUnread));
  initial_data.push_back(CreateSyncData(kTitle6, kText6, kIconUrl6, kImageUrl6,
                                        kAppId6, kKey6, kUnread));
  initial_data.push_back(CreateSyncData(kTitle7, kText7, kIconUrl7, kImageUrl7,
                                        kAppId7, kKey7, kUnread));

  // Merge the local and remote data.
  notifier.MergeDataAndStartSyncing(
      SYNCED_NOTIFICATIONS,
      initial_data,
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  // Ensure the local store now has all local and remote notifications.
  EXPECT_EQ(7U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());
  EXPECT_TRUE(notifier.FindNotificationById(kKey1));
  EXPECT_TRUE(notifier.FindNotificationById(kKey2));
  EXPECT_TRUE(notifier.FindNotificationById(kKey3));
  EXPECT_TRUE(notifier.FindNotificationById(kKey4));
  EXPECT_TRUE(notifier.FindNotificationById(kKey5));
  EXPECT_TRUE(notifier.FindNotificationById(kKey6));
  EXPECT_TRUE(notifier.FindNotificationById(kKey7));

  // Test the type conversion and construction functions.
  for (SyncDataList::const_iterator iter = initial_data.begin();
      iter != initial_data.end(); ++iter) {
    scoped_ptr<SyncedNotification> notification1(
        notifier.CreateNotificationFromSyncData(*iter));
    // TODO(petewil): Revisit this when we add version info to notifications.
    const std::string& key = notification1->GetKey();
    const SyncedNotification* notification2 =
        notifier.FindNotificationById(key);
    EXPECT_TRUE(NULL != notification2);
    EXPECT_TRUE(notification1->EqualsIgnoringReadState(*notification2));
    EXPECT_EQ(notification1->GetReadState(), notification2->GetReadState());
  }
  EXPECT_TRUE(notifier.FindNotificationById(kKey1));
  EXPECT_TRUE(notifier.FindNotificationById(kKey2));
  EXPECT_TRUE(notifier.FindNotificationById(kKey3));
}

// Test the local store having the read bit unset, the remote store having
// it set.
TEST_F(ChromeNotifierServiceTest, ModelAssocBothNonEmptyReadMismatch1) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  notifier.AddForTest(n1.Pass());
  scoped_ptr<SyncedNotification> n2(CreateNotification(
      kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kUnread));
  notifier.AddForTest(n2.Pass());

  // Create some remote fake data, item 1 matches except for the read state.
  syncer::SyncDataList initial_data;
  initial_data.push_back(CreateSyncData(kTitle1, kText1, kIconUrl1, kImageUrl1,
                                        kAppId1, kKey1, kDismissed));
  // Merge the local and remote data.
  notifier.MergeDataAndStartSyncing(
      syncer::SYNCED_NOTIFICATIONS,
      initial_data,
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  // Ensure the local store still has only two notifications, and the read
  // state of the first is now read.
  EXPECT_EQ(2U, notifier.GetAllSyncData(syncer::SYNCED_NOTIFICATIONS).size());
  SyncedNotification* notification1 =
      notifier.FindNotificationById(kKey1);
  EXPECT_FALSE(NULL == notification1);
  EXPECT_EQ(SyncedNotification::kDismissed, notification1->GetReadState());
  EXPECT_TRUE(notifier.FindNotificationById(kKey2));
  EXPECT_FALSE(notifier.FindNotificationById(kKey3));

  // Make sure that the notification manager was told to dismiss the
  // notification.
  EXPECT_EQ(std::string(kKey1), notification_manager()->dismissed_id());

  // Ensure no new data will be sent to the remote store for notification1.
  EXPECT_EQ(0U, processor()->change_list_size());
  EXPECT_FALSE(processor()->ContainsId(kKey1));
}

// Test when the local store has the read bit set, and the remote store has
// it unset.
TEST_F(ChromeNotifierServiceTest, ModelAssocBothNonEmptyReadMismatch2) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kDismissed));
  notifier.AddForTest(n1.Pass());
  scoped_ptr<SyncedNotification> n2(CreateNotification(
      kTitle2, kText2, kIconUrl2, kImageUrl2, kAppId2, kKey2, kUnread));
  notifier.AddForTest(n2.Pass());

  // Create some remote fake data, item 1 matches except for the read state.
  syncer::SyncDataList initial_data;
  initial_data.push_back(CreateSyncData(kTitle1, kText1, kIconUrl1, kImageUrl1,
                                        kAppId1, kKey1, kUnread));
  // Merge the local and remote data.
  notifier.MergeDataAndStartSyncing(
      syncer::SYNCED_NOTIFICATIONS,
      initial_data,
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  // Ensure the local store still has only two notifications, and the read
  // state of the first is now read.
  EXPECT_EQ(2U, notifier.GetAllSyncData(syncer::SYNCED_NOTIFICATIONS).size());
  SyncedNotification* notification1 =
      notifier.FindNotificationById(kKey1);
  EXPECT_FALSE(NULL == notification1);
  EXPECT_EQ(SyncedNotification::kDismissed, notification1->GetReadState());
  EXPECT_TRUE(notifier.FindNotificationById(kKey2));
  EXPECT_FALSE(notifier.FindNotificationById(kKey3));

  // Ensure the new data will be sent to the remote store for notification1.
  EXPECT_EQ(1U, processor()->change_list_size());
  EXPECT_TRUE(processor()->ContainsId(kKey1));
  EXPECT_EQ(SyncChange::ACTION_UPDATE, processor()->GetChangeById(
      kKey1).change_type());
}

// We have a notification in the local store, we get an updated version
// of the same notification remotely, it should take precedence.
TEST_F(ChromeNotifierServiceTest, ModelAssocBothNonEmptyWithUpdate) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kDismissed));
  notifier.AddForTest(n1.Pass());

  // Create some remote fake data, item 1 matches the ID, but has different data
  syncer::SyncDataList initial_data;
  initial_data.push_back(CreateSyncData(kTitle2, kText2, kIconUrl2, kImageUrl2,
                                        kAppId1, kKey1, kUnread));
  // Merge the local and remote data.
  notifier.MergeDataAndStartSyncing(
      syncer::SYNCED_NOTIFICATIONS,
      initial_data,
      PassProcessor(),
      scoped_ptr<syncer::SyncErrorFactory>(new syncer::SyncErrorFactoryMock()));

  // Ensure the local store still has only one notification
  EXPECT_EQ(1U, notifier.GetAllSyncData(syncer::SYNCED_NOTIFICATIONS).size());
  SyncedNotification* notification1 =
      notifier.FindNotificationById(kKey1);

  EXPECT_FALSE(NULL == notification1);
  EXPECT_EQ(SyncedNotification::kUnread, notification1->GetReadState());
  EXPECT_EQ(std::string(kTitle2), notification1->GetTitle());

  // Ensure no new data will be sent to the remote store for notification1.
  EXPECT_EQ(0U, processor()->change_list_size());
  EXPECT_FALSE(processor()->ContainsId(kKey1));
}

TEST_F(ChromeNotifierServiceTest, ServiceEnabledTest) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  std::set<std::string>::iterator iter;
  std::string first_synced_notification_service_id(kSendingService1Name);

  // Create some local fake data.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  notifier.AddForTest(n1.Pass());

  // Enable the service and ensure the service is in the list.
  // Initially the service starts in the disabled state.
  notifier.OnSyncedNotificationServiceEnabled(kSendingService1Name, true);
  iter = find(notifier.enabled_sending_services_.begin(),
              notifier.enabled_sending_services_.end(),
              first_synced_notification_service_id);

  EXPECT_NE(notifier.enabled_sending_services_.end(), iter);

  // TODO(petewil): Verify Display gets called too.
  // Disable the service and ensure it is gone from the list and the
  // notification_manager.
  notifier.OnSyncedNotificationServiceEnabled(kSendingService1Name, false);
  iter = find(notifier.enabled_sending_services_.begin(),
              notifier.enabled_sending_services_.end(),
              first_synced_notification_service_id);

  EXPECT_EQ(notifier.enabled_sending_services_.end(), iter);
  EXPECT_EQ(notification_manager()->dismissed_id(), std::string(kKey1));
}

TEST_F(ChromeNotifierServiceTest, AddNewSendingServicesTest) {
  // This test will see if we get a new sending service after the first
  // notification for that service.
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // We initially have no data.
  EXPECT_EQ(0U, notifier.enabled_sending_services_.size());
  EXPECT_EQ(0U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());

  // Set up an ADD.
  SyncChangeList changes;
  changes.push_back(
      CreateSyncChange(SyncChange::ACTION_ADD,
                       CreateNotification(kTitle1,
                                          kText1,
                                          kIconUrl1,
                                          kImageUrl1,
                                          kAppId1,
                                          kKey1,
                                          kUnread)));

  notifier.ProcessSyncChanges(FROM_HERE, changes);

  EXPECT_EQ(1U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());

  // Verify that the first synced notification service is enabled in memory.
  std::set<std::string>::iterator iter;
  std::string first_notification_service_id(kSendingService1Name);
  iter = find(notifier.enabled_sending_services_.begin(),
              notifier.enabled_sending_services_.end(),
              first_notification_service_id);

  EXPECT_NE(notifier.enabled_sending_services_.end(), iter);

  // We should have gotten the synced notification and a welcome notification.
  EXPECT_EQ(2U, notification_manager()->added_notifications());
  EXPECT_TRUE(notification_manager()->welcomed());

  changes.clear();
  changes.push_back(
      CreateSyncChange(SyncChange::ACTION_ADD,
                       CreateNotification(kTitle2,
                                          kText2,
                                          kIconUrl2,
                                          kImageUrl2,
                                          kAppId1,
                                          kKey2,
                                          kUnread)));
  notifier.ProcessSyncChanges(FROM_HERE, changes);

  // But adding another notification should not cause another welcome.
  EXPECT_EQ(3U, notification_manager()->added_notifications());
}

TEST_F(ChromeNotifierServiceTest, CheckInitializedServicesTest) {
  // This test will see if we get a new sending service after the first
  // notification for that service.
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Initialize but do not enable the sending service.
  notifier.initialized_sending_services_.insert(kSendingService1Name);
  ASSERT_EQ(0U, notifier.enabled_sending_services_.size());
  ASSERT_EQ(1U, notifier.initialized_sending_services_.size());

  // We initially have no data.
  EXPECT_EQ(0U, notifier.enabled_sending_services_.size());
  EXPECT_EQ(0U, notifier.GetAllSyncData(SYNCED_NOTIFICATIONS).size());

  // Set up an ADD.
  std::string first_synced_notification_service_id(kSendingService1Name);

  SyncChangeList changes;
  changes.push_back(
      CreateSyncChange(SyncChange::ACTION_ADD,
                       CreateNotification(kTitle1,
                                          kText1,
                                          kIconUrl1,
                                          kImageUrl1,
                                          kAppId1,
                                          kKey1,
                                          kUnread)));

  notifier.ProcessSyncChanges(FROM_HERE, changes);

  // Since we added to |initialized_sending_services_| before receiving the
  // synced notification, we should not have enabled this service while
  // processing the sync change.
  EXPECT_EQ(0U, notifier.enabled_sending_services_.size());
  EXPECT_EQ(0U, notification_manager()->added_notifications());
}

TEST_F(ChromeNotifierServiceTest, SetAddedAppIdsTest) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Add some notifications to our notification list.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  n1->SetNotifierServiceForTest(&notifier);
  notifier.AddForTest(n1.Pass());

  EXPECT_EQ(static_cast<size_t>(0),
            notification_manager()->added_notifications());

  // Call SetAddedAppIds.
  std::vector<std::string> added_app_ids;
  added_app_ids.push_back(std::string(kAppId1));
  notifier.OnAddedAppIds(added_app_ids);

  // Verify the notification was added by the notification_manager.
  // We see one welcome notification and one new notification.
  EXPECT_EQ(static_cast<size_t>(2),
            notification_manager()->added_notifications());
}

TEST_F(ChromeNotifierServiceTest, SetRemovedAppIdsTest) {
  ChromeNotifierService notifier(profile_.get(), notification_manager());
  notifier.set_avoid_bitmap_fetching_for_test(true);

  // Add some notifications to our notification list.
  scoped_ptr<SyncedNotification> n1(CreateNotification(
      kTitle1, kText1, kIconUrl1, kImageUrl1, kAppId1, kKey1, kUnread));
  notifier.AddForTest(n1.Pass());

  // Call SetRemovedAppIds.
  std::vector<std::string> removed_app_ids;
  removed_app_ids.push_back(std::string(kAppId1));
  notifier.OnRemovedAppIds(removed_app_ids);

  // Verify the notification was "removed" in the notification manager.
  EXPECT_EQ(std::string(kKey1), notification_manager()->dismissed_id());
}

// TODO(petewil): Add a test that we do *not* get a welcome dialog unless we
// have a valid app info for the notification.

}  // namespace notifier
