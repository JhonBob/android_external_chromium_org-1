// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_SYNCABLE_SYNC_OPERATION_RESULT_H_
#define WEBKIT_FILEAPI_SYNCABLE_SYNC_OPERATION_RESULT_H_

#include "webkit/fileapi/syncable/sync_action.h"

namespace fileapi {

// TODO(kinuko): Deprecate this.
enum SyncOperationResult {
  // Indicates no operation has been made.
  SYNC_OPERATION_NONE = SYNC_ACTION_NONE,

  // Indicates a new file or directory has been added.
  SYNC_OPERATION_ADDED = SYNC_ACTION_ADDED,

  // Indicates an existing file or directory has been updated.
  SYNC_OPERATION_UPDATED = SYNC_ACTION_UPDATED,

  // Indicates a file or directory has been deleted.
  SYNC_OPERATION_DELETED = SYNC_ACTION_DELETED,

  // Indicates a file or directory has conflicting changes.
  SYNC_OPERATION_CONFLICTED,
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_SYNCABLE_SYNC_OPERATION_RESULT_H_
