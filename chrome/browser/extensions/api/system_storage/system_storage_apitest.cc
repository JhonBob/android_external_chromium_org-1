// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/api/system_storage/storage_api_test_util.h"
#include "chrome/browser/extensions/api/system_storage/storage_info_provider.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "components/storage_monitor/storage_info.h"
#include "components/storage_monitor/storage_monitor.h"
#include "components/storage_monitor/test_storage_monitor.h"

namespace {

using extensions::StorageUnitInfoList;
using extensions::test::TestStorageUnitInfo;
using extensions::test::kRemovableStorageData;

const struct TestStorageUnitInfo kTestingData[] = {
  {"dcim:device:001", "0xbeaf", 4098, 1},
  {"path:device:002", "/home", 4098, 2},
  {"path:device:003", "/data", 10000, 3}
};

}  // namespace

class TestStorageInfoProvider : public extensions::StorageInfoProvider {
 public:
  TestStorageInfoProvider(const struct TestStorageUnitInfo* testing_data,
                          size_t n);

 private:
  virtual ~TestStorageInfoProvider();

  // StorageInfoProvider implementations.
  virtual double GetStorageFreeSpaceFromTransientIdOnFileThread(
      const std::string& transient_id) OVERRIDE;

  std::vector<struct TestStorageUnitInfo> testing_data_;
};

TestStorageInfoProvider::TestStorageInfoProvider(
    const struct TestStorageUnitInfo* testing_data, size_t n)
        : testing_data_(testing_data, testing_data + n) {
}

TestStorageInfoProvider::~TestStorageInfoProvider() {
}

double TestStorageInfoProvider::GetStorageFreeSpaceFromTransientIdOnFileThread(
    const std::string& transient_id) {
  std::string device_id =
      StorageMonitor::GetInstance()->GetDeviceIdForTransientId(
          transient_id);
  for (size_t i = 0; i < testing_data_.size(); ++i) {
    if (testing_data_[i].device_id == device_id) {
      return static_cast<double>(testing_data_[i].available_capacity);
    }
  }
  return -1;
}

class SystemStorageApiTest : public ExtensionApiTest {
 public:
  SystemStorageApiTest() {}
  virtual ~SystemStorageApiTest() {}

  virtual void SetUpOnMainThread() OVERRIDE {
    TestStorageMonitor::CreateForBrowserTests();
  }

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
    message_loop_.reset(new base::MessageLoopForUI);
  }

  void SetUpAllMockStorageDevices() {
    for (size_t i = 0; i < arraysize(kTestingData); ++i) {
      AttachRemovableStorage(kTestingData[i]);
    }
  }

  void AttachRemovableStorage(
      const struct TestStorageUnitInfo& removable_storage_info) {
    StorageMonitor::GetInstance()->receiver()->ProcessAttach(
        extensions::test::BuildStorageInfoFromTestStorageUnitInfo(
            removable_storage_info));
  }

  void DetachRemovableStorage(const std::string& id) {
    StorageMonitor::GetInstance()->receiver()->ProcessDetach(id);
  }

 private:
  scoped_ptr<base::MessageLoop> message_loop_;
};

IN_PROC_BROWSER_TEST_F(SystemStorageApiTest, Storage) {
  SetUpAllMockStorageDevices();
  TestStorageInfoProvider* provider =
      new TestStorageInfoProvider(kTestingData,
                                  arraysize(kTestingData));
  extensions::StorageInfoProvider::InitializeForTesting(provider);
  std::vector<linked_ptr<ExtensionTestMessageListener> > device_ids_listeners;
  for (size_t i = 0; i < arraysize(kTestingData); ++i) {
    linked_ptr<ExtensionTestMessageListener> listener(
        new ExtensionTestMessageListener(
            StorageMonitor::GetInstance()->GetTransientIdForDeviceId(
                kTestingData[i].device_id),
            false));
    device_ids_listeners.push_back(listener);
  }
  ASSERT_TRUE(RunPlatformAppTest("system/storage")) << message_;
  for (size_t i = 0; i < device_ids_listeners.size(); ++i)
    EXPECT_TRUE(device_ids_listeners[i]->WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(SystemStorageApiTest, StorageAttachment) {
  ResultCatcher catcher;
  ExtensionTestMessageListener attach_listener("attach", false);
  ExtensionTestMessageListener detach_listener("detach", false);

  EXPECT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("system/storage_attachment")));
  // Simulate triggering onAttached event.
  ASSERT_TRUE(attach_listener.WaitUntilSatisfied());

  AttachRemovableStorage(kRemovableStorageData);

  std::string removable_storage_transient_id =
      StorageMonitor::GetInstance()->GetTransientIdForDeviceId(
          kRemovableStorageData.device_id);
  ExtensionTestMessageListener detach_device_id_listener(
      removable_storage_transient_id, false);

  // Simulate triggering onDetached event.
  ASSERT_TRUE(detach_listener.WaitUntilSatisfied());
  DetachRemovableStorage(kRemovableStorageData.device_id);

  ASSERT_TRUE(detach_device_id_listener.WaitUntilSatisfied());

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}
