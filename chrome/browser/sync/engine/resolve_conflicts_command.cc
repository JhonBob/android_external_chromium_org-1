// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/engine/resolve_conflicts_command.h"

#include "chrome/browser/sync/engine/conflict_resolver.h"
#include "chrome/browser/sync/sessions/session_state.h"
#include "chrome/browser/sync/sessions/sync_session.h"
#include "chrome/browser/sync/syncable/syncable.h"

namespace browser_sync {

ResolveConflictsCommand::ResolveConflictsCommand() {}
ResolveConflictsCommand::~ResolveConflictsCommand() {}

std::set<ModelSafeGroup> ResolveConflictsCommand::GetGroupsToChange(
    const sessions::SyncSession& session) const {
  return session.GetEnabledGroupsWithConflicts();
}

SyncerError ResolveConflictsCommand::ModelChangingExecuteImpl(
    sessions::SyncSession* session) {
  ConflictResolver* resolver = session->context()->resolver();
  DCHECK(resolver);

  syncable::Directory* dir = session->context()->directory();
  sessions::StatusController* status = session->mutable_status_controller();
  const sessions::ConflictProgress* progress = status->conflict_progress();
  if (!progress)
    return SYNCER_OK;  // Nothing to do.
  syncable::WriteTransaction trans(FROM_HERE, syncable::SYNCER, dir);
  const Cryptographer* cryptographer = dir->GetCryptographer(&trans);
  status->update_conflicts_resolved(
      resolver->ResolveConflicts(&trans, cryptographer, *progress, status));

  return SYNCER_OK;
}

}  // namespace browser_sync
