// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "sync/notifier/fake_invalidator.h"
#include "sync/notifier/invalidator_test_template.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

class FakeInvalidatorTestDelegate {
 public:
  FakeInvalidatorTestDelegate() {}

  ~FakeInvalidatorTestDelegate() {
    DestroyInvalidator();
  }

  void CreateInvalidator(
      const std::string& initial_state,
      const base::WeakPtr<InvalidationStateTracker>&
          invalidation_state_tracker) {
    DCHECK(!invalidator_.get());
    invalidator_.reset(new FakeInvalidator());
  }

  FakeInvalidator* GetInvalidator() {
    return invalidator_.get();
  }

  void DestroyInvalidator() {
    invalidator_.reset();
  }

  void WaitForInvalidator() {
    // Do Nothing.
  }

  void TriggerOnNotificationsEnabled() {
    invalidator_->EmitOnNotificationsEnabled();
  }

  void TriggerOnIncomingNotification(const ObjectIdStateMap& id_state_map,
                                     IncomingNotificationSource source) {
    invalidator_->EmitOnIncomingNotification(id_state_map, source);
  }

  void TriggerOnNotificationsDisabled(NotificationsDisabledReason reason) {
    invalidator_->EmitOnNotificationsDisabled(reason);
  }

  static bool InvalidatorHandlesDeprecatedState() {
    return false;
  }

 private:
  scoped_ptr<FakeInvalidator> invalidator_;
};

INSTANTIATE_TYPED_TEST_CASE_P(
    FakeInvalidatorTest, InvalidatorTest,
    FakeInvalidatorTestDelegate);

}  // namespace

}  // namespace syncer
