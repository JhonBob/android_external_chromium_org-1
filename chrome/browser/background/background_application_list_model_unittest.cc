// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(rickcam): Bug 73183: Add unit tests for image loading

#include <cstdlib>
#include <set>

#include "chrome/browser/background/background_application_list_model.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/stl_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_unittest.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_manifest_constants.h"
#include "chrome/common/extensions/permissions/api_permission.h"
#include "chrome/common/extensions/permissions/permission_set.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_types.h"
#include "testing/gtest/include/gtest/gtest.h"

// This value is used to seed the PRNG at the beginning of a sequence of
// operations to produce a repeatable sequence.
#define RANDOM_SEED (0x33F7A7A7)

using extensions::APIPermission;
using extensions::Extension;

// For ExtensionService interface when it requires a path that is not used.
base::FilePath bogus_file_path() {
  return base::FilePath(FILE_PATH_LITERAL("//foobar_nonexistent"));
}

class BackgroundApplicationListModelTest : public ExtensionServiceTestBase {
 public:
  BackgroundApplicationListModelTest() {}
  virtual ~BackgroundApplicationListModelTest() {}

 protected:
  void InitializeAndLoadEmptyExtensionService() {
    InitializeEmptyExtensionService();
    service_->Init(); /* Sends EXTENSIONS_READY */
  }

  bool IsBackgroundApp(const Extension& app) {
    return BackgroundApplicationListModel::IsBackgroundApp(app,
                                                           profile_.get());
  }
};

// Returns a barebones test Extension object with the specified |name|.  The
// returned extension will include background permission iff
// |background_permission| is true.
static scoped_refptr<Extension> CreateExtension(const std::string& name,
                                                bool background_permission) {
  DictionaryValue manifest;
  manifest.SetString(extension_manifest_keys::kVersion, "1.0.0.0");
  manifest.SetString(extension_manifest_keys::kName, name);
  if (background_permission) {
    ListValue* permissions = new ListValue();
    manifest.Set(extension_manifest_keys::kPermissions, permissions);
    permissions->Append(Value::CreateStringValue("background"));
  }
  std::string error;
  scoped_refptr<Extension> extension = Extension::Create(
      bogus_file_path().AppendASCII(name),
      extensions::Manifest::INVALID_LOCATION,
      manifest,
      Extension::NO_FLAGS,
      &error);
  // Cannot ASSERT_* here because that attempts an illegitimate return.
  // Cannot EXPECT_NE here because that assumes non-pointers unlike EXPECT_EQ
  EXPECT_TRUE(extension.get() != NULL) << error;
  return extension;
}

namespace {
std::string GenerateUniqueExtensionName() {
  static int uniqueness = 0;
  std::ostringstream output;
  output << "Unique Named Extension " << uniqueness;
  ++uniqueness;
  return output.str();
}

void AddBackgroundPermission(ExtensionService* service,
                             Extension* extension) {
  if (BackgroundApplicationListModel::IsBackgroundApp(*extension,
                                                      service->profile())) {
    return;
  }

  scoped_refptr<Extension> temporary =
      CreateExtension(GenerateUniqueExtensionName(), true);
  scoped_refptr<const extensions::PermissionSet> permissions =
      temporary->GetActivePermissions();
  extensions::PermissionsUpdater(service->profile()).AddPermissions(
      extension, permissions.get());
}

void RemoveBackgroundPermission(ExtensionService* service,
                                Extension* extension) {
  if (!BackgroundApplicationListModel::IsBackgroundApp(*extension,
                                                       service->profile())) {
    return;
  }
  extensions::PermissionsUpdater(service->profile())
      .RemovePermissions(extension, extension->GetActivePermissions().get());
}
}  // namespace

// Crashes on Mac tryslaves.
// http://crbug.com/165458
#if defined(OS_MACOSX) || defined(OS_LINUX)
#define MAYBE_ExplicitTest DISABLED_ExplicitTest
#else
#define MAYBE_ExplicitTest ExplicitTest
#endif
// With minimal test logic, verifies behavior over an explicit set of
// extensions, of which some are Background Apps and others are not.
TEST_F(BackgroundApplicationListModelTest, MAYBE_ExplicitTest) {
  InitializeAndLoadEmptyExtensionService();
  ExtensionService* service = extensions::ExtensionSystem::Get(profile_.get())->
      extension_service();
  ASSERT_TRUE(service);
  ASSERT_TRUE(service->is_ready());
  ASSERT_TRUE(service->extensions());
  ASSERT_TRUE(service->extensions()->is_empty());
  scoped_ptr<BackgroundApplicationListModel> model(
      new BackgroundApplicationListModel(profile_.get()));
  ASSERT_EQ(0U, model->size());

  scoped_refptr<Extension> ext1 = CreateExtension("alpha", false);
  scoped_refptr<Extension> ext2 = CreateExtension("bravo", false);
  scoped_refptr<Extension> ext3 = CreateExtension("charlie", false);
  scoped_refptr<Extension> bgapp1 = CreateExtension("delta", true);
  scoped_refptr<Extension> bgapp2 = CreateExtension("echo", true);
  ASSERT_TRUE(service->extensions() != NULL);
  ASSERT_EQ(0U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());

  // Add alternating Extensions and Background Apps
  ASSERT_FALSE(IsBackgroundApp(*ext1.get()));
  service->AddExtension(ext1.get());
  ASSERT_EQ(1U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());
  ASSERT_TRUE(IsBackgroundApp(*bgapp1.get()));
  service->AddExtension(bgapp1.get());
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());
  ASSERT_FALSE(IsBackgroundApp(*ext2.get()));
  service->AddExtension(ext2.get());
  ASSERT_EQ(3U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());
  ASSERT_TRUE(IsBackgroundApp(*bgapp2.get()));
  service->AddExtension(bgapp2.get());
  ASSERT_EQ(4U, service->extensions()->size());
  ASSERT_EQ(2U, model->size());
  ASSERT_FALSE(IsBackgroundApp(*ext3.get()));
  service->AddExtension(ext3.get());
  ASSERT_EQ(5U, service->extensions()->size());
  ASSERT_EQ(2U, model->size());

  // Remove in FIFO order.
  ASSERT_FALSE(IsBackgroundApp(*ext1.get()));
  service->UninstallExtension(ext1->id(), false, NULL);
  ASSERT_EQ(4U, service->extensions()->size());
  ASSERT_EQ(2U, model->size());
  ASSERT_TRUE(IsBackgroundApp(*bgapp1.get()));
  service->UninstallExtension(bgapp1->id(), false, NULL);
  ASSERT_EQ(3U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());
  ASSERT_FALSE(IsBackgroundApp(*ext2.get()));
  service->UninstallExtension(ext2->id(), false, NULL);
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());
  ASSERT_TRUE(IsBackgroundApp(*bgapp2.get()));
  service->UninstallExtension(bgapp2->id(), false, NULL);
  ASSERT_EQ(1U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());
  ASSERT_FALSE(IsBackgroundApp(*ext3.get()));
  service->UninstallExtension(ext3->id(), false, NULL);
  ASSERT_EQ(0U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());
}

// With minimal test logic, verifies behavior with dynamic permissions.
TEST_F(BackgroundApplicationListModelTest, AddRemovePermissionsTest) {
  InitializeAndLoadEmptyExtensionService();
  ExtensionService* service = extensions::ExtensionSystem::Get(profile_.get())->
      extension_service();
  ASSERT_TRUE(service);
  ASSERT_TRUE(service->is_ready());
  ASSERT_TRUE(service->extensions());
  ASSERT_TRUE(service->extensions()->is_empty());
  scoped_ptr<BackgroundApplicationListModel> model(
      new BackgroundApplicationListModel(profile_.get()));
  ASSERT_EQ(0U, model->size());

  scoped_refptr<Extension> ext = CreateExtension("extension", false);
  ASSERT_FALSE(ext->HasAPIPermission(APIPermission::kBackground));
  scoped_refptr<Extension> bgapp = CreateExtension("application", true);
  ASSERT_TRUE(bgapp->HasAPIPermission(APIPermission::kBackground));
  ASSERT_TRUE(service->extensions() != NULL);
  ASSERT_EQ(0U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());

  // Add one (non-background) extension and one background application
  ASSERT_FALSE(IsBackgroundApp(*ext.get()));
  service->AddExtension(ext.get());
  ASSERT_EQ(1U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());
  ASSERT_TRUE(IsBackgroundApp(*bgapp.get()));
  service->AddExtension(bgapp.get());
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());

  // Change permissions back and forth
  AddBackgroundPermission(service, ext.get());
  ASSERT_TRUE(ext->HasAPIPermission(APIPermission::kBackground));
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(2U, model->size());
  RemoveBackgroundPermission(service, bgapp.get());
  ASSERT_FALSE(bgapp->HasAPIPermission(APIPermission::kBackground));
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());
  RemoveBackgroundPermission(service, ext.get());
  ASSERT_FALSE(ext->HasAPIPermission(APIPermission::kBackground));
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(0U, model->size());
  AddBackgroundPermission(service, bgapp.get());
  ASSERT_TRUE(bgapp->HasAPIPermission(APIPermission::kBackground));
  ASSERT_EQ(2U, service->extensions()->size());
  ASSERT_EQ(1U, model->size());
}

typedef std::set<scoped_refptr<Extension> > ExtensionCollection;

namespace {
void AddExtension(ExtensionService* service,
                  ExtensionCollection* extensions,
                  BackgroundApplicationListModel* model,
                  size_t* expected,
                  size_t* count) {
  bool create_background = false;
  if (rand() % 2) {
    create_background = true;
    ++*expected;
  }
  scoped_refptr<Extension> extension =
      CreateExtension(GenerateUniqueExtensionName(), create_background);
  ASSERT_EQ(BackgroundApplicationListModel::IsBackgroundApp(*extension.get(),
                                                            service->profile()),
            create_background);
  extensions->insert(extension);
  ++*count;
  ASSERT_EQ(*count, extensions->size());
  service->AddExtension(extension.get());
  ASSERT_EQ(*count, service->extensions()->size());
  ASSERT_EQ(*expected, model->size());
}

void RemoveExtension(ExtensionService* service,
                     ExtensionCollection* extensions,
                     BackgroundApplicationListModel* model,
                     size_t* expected,
                     size_t* count) {  // Maybe remove an extension.
  ExtensionCollection::iterator cursor = extensions->begin();
  if (cursor == extensions->end()) {
    // Nothing to remove.  Just verify accounting.
    ASSERT_EQ(0U, *count);
    ASSERT_EQ(0U, *expected);
    ASSERT_EQ(0U, service->extensions()->size());
    ASSERT_EQ(0U, model->size());
  } else {
    // Randomly select which extension to remove
    if (extensions->size() > 1) {
      int offset = rand() % (extensions->size() - 1);
      for (int index = 0; index < offset; ++index)
        ++cursor;
    }
    scoped_refptr<Extension> extension = cursor->get();
    std::string id = extension->id();
    if (BackgroundApplicationListModel::IsBackgroundApp(*extension.get(),
                                                        service->profile())) {
      --*expected;
    }
    extensions->erase(cursor);
    --*count;
    ASSERT_EQ(*count, extensions->size());
    service->UninstallExtension(extension->id(), false, NULL);
    ASSERT_EQ(*count, service->extensions()->size());
    ASSERT_EQ(*expected, model->size());
  }
}

void TogglePermission(ExtensionService* service,
                      ExtensionCollection* extensions,
                      BackgroundApplicationListModel* model,
                      size_t* expected,
                      size_t* count) {
  ExtensionCollection::iterator cursor = extensions->begin();
  if (cursor == extensions->end()) {
    // Nothing to toggle.  Just verify accounting.
    ASSERT_EQ(0U, *count);
    ASSERT_EQ(0U, *expected);
    ASSERT_EQ(0U, service->extensions()->size());
    ASSERT_EQ(0U, model->size());
  } else {
    // Randomly select which extension to toggle.
    if (extensions->size() > 1) {
      int offset = rand() % (extensions->size() - 1);
      for (int index = 0; index < offset; ++index)
        ++cursor;
    }
    scoped_refptr<Extension> extension = cursor->get();
    std::string id = extension->id();
    if (BackgroundApplicationListModel::IsBackgroundApp(*extension.get(),
                                                        service->profile())) {
      --*expected;
      ASSERT_EQ(*count, extensions->size());
      RemoveBackgroundPermission(service, extension.get());
      ASSERT_EQ(*count, service->extensions()->size());
      ASSERT_EQ(*expected, model->size());
    } else {
      ++*expected;
      ASSERT_EQ(*count, extensions->size());
      AddBackgroundPermission(service, extension.get());
      ASSERT_EQ(*count, service->extensions()->size());
      ASSERT_EQ(*expected, model->size());
    }
  }
}
}  // namespace

// Verifies behavior with a pseudo-randomly generated set of actions: Adding and
// removing extensions, of which some are Background Apps and others are not.
TEST_F(BackgroundApplicationListModelTest, RandomTest) {
  InitializeAndLoadEmptyExtensionService();
  ExtensionService* service = extensions::ExtensionSystem::Get(profile_.get())->
      extension_service();
  ASSERT_TRUE(service);
  ASSERT_TRUE(service->is_ready());
  ASSERT_TRUE(service->extensions());
  ASSERT_TRUE(service->extensions()->is_empty());
  scoped_ptr<BackgroundApplicationListModel> model(
      new BackgroundApplicationListModel(profile_.get()));
  ASSERT_EQ(0U, model->size());

  static const int kIterations = 500;
  ExtensionCollection extensions;
  size_t count = 0;
  size_t expected = 0;
  srand(RANDOM_SEED);
  for (int index = 0; index < kIterations; ++index) {
    switch (rand() % 3) {
      case 0:
        AddExtension(service, &extensions, model.get(), &expected, &count);
        break;
      case 1:
        RemoveExtension(service, &extensions, model.get(), &expected, &count);
        break;
      case 2:
        TogglePermission(service, &extensions, model.get(), &expected, &count);
        break;
      default:
        NOTREACHED();
        break;
    }
  }
}
