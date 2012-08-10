// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Interface to the sync notifier, which is an object that receives
// notifications when updates are available for a set of sync types.
// All the observers are notified when such an event happens.

#ifndef SYNC_NOTIFIER_SYNC_NOTIFIER_H_
#define SYNC_NOTIFIER_SYNC_NOTIFIER_H_

#include <string>

#include "sync/internal_api/public/base/model_type.h"
#include "sync/notifier/invalidation_util.h"

namespace syncer {
class SyncNotifierObserver;

class SyncNotifier {
 public:
  SyncNotifier() {}
  virtual ~SyncNotifier() {}

  // Updates the set of ObjectIds associated with a given |handler|.
  // Passing an empty ObjectIdSet will unregister |handler|.
  // There should be at most one handler registered per object id.
  virtual void UpdateRegisteredIds(SyncNotifierObserver* handler,
                                   const ObjectIdSet& ids) = 0;

  // SetUniqueId must be called once, before any call to
  // UpdateCredentials.  |unique_id| should be a non-empty globally
  // unique string.
  virtual void SetUniqueId(const std::string& unique_id) = 0;

  // SetState must be called once, before any call to
  // UpdateCredentials.  |state| may be empty.
  // Deprecated in favour of InvalidationStateTracker persistence.
  virtual void SetStateDeprecated(const std::string& state) = 0;

  // The observers won't be notified of any notifications until
  // UpdateCredentials is called at least once. It can be called more than
  // once.
  virtual void UpdateCredentials(
      const std::string& email, const std::string& token) = 0;

  // This is here only to support the old p2p notification implementation,
  // which is still used by sync integration tests.
  // TODO(akalin): Remove this once we move the integration tests off p2p
  // notifications.
  virtual void SendNotification(ModelTypeSet changed_types) = 0;
};
}  // namespace syncer

#endif  // SYNC_NOTIFIER_SYNC_NOTIFIER_H_
