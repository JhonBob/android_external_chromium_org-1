// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// An InvalidationVersionTracker is an interface that handles getting
// and setting (persisting) max invalidation versions.

#ifndef SYNC_NOTIFIER_INVALIDATION_STATE_TRACKER_H_
#define SYNC_NOTIFIER_INVALIDATION_STATE_TRACKER_H_

#include <map>

#include "base/basictypes.h"
#include "sync/internal_api/public/syncable/model_type.h"

namespace sync_notifier {

typedef std::map<syncable::ModelType, int64> InvalidationVersionMap;

class InvalidationStateTracker {
 public:
  InvalidationStateTracker() {}

  virtual InvalidationVersionMap GetAllMaxVersions() const = 0;

  // |max_version| should be strictly greater than any existing max
  // version for |model_type|.
  virtual void SetMaxVersion(syncable::ModelType model_type,
                             int64 max_version) = 0;

  // Used by InvalidationClient for persistence. |state| is opaque data we can
  // present back to the client (e.g. after a restart) for it to bootstrap
  // itself.
  // |state| is plain old data (not valid UTF8, embedded nulls, etc).
  virtual void SetInvalidationState(const std::string& state) = 0;
  virtual std::string GetInvalidationState() const = 0;

 protected:
  virtual ~InvalidationStateTracker() {}
};

}  // namespace sync_notifier

#endif  // SYNC_NOTIFIER_INVALIDATION_STATE_TRACKER_H_
