// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_SYNC_PROCESS_RUNNER_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_SYNC_PROCESS_RUNNER_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chrome/browser/sync_file_system/sync_callbacks.h"
#include "chrome/browser/sync_file_system/sync_service_state.h"

namespace sync_file_system {

class SyncFileSystemService;

// A base class to schedule a sync.
// Each subclass must implement StartSync().
// An instance of this class is supposed to be owned by SyncFileSystemService.
//
// Note that multiple SyncProcessRunner doesn't coordinate its sync schedule
// with each other.
class SyncProcessRunner {
 public:
  class TimerHelper {
   public:
    virtual ~TimerHelper() {}
    virtual bool IsRunning() = 0;
    virtual void Start(const tracked_objects::Location& from_here,
                       const base::TimeDelta& delay,
                       const base::Closure& closure) = 0;

   protected:
    TimerHelper() {}
  };

  SyncProcessRunner(const std::string& name,
                    SyncFileSystemService* sync_service,
                    scoped_ptr<TimerHelper> timer_helper,
                    int max_parallel_task);
  virtual ~SyncProcessRunner();

  // Subclass must implement this.
  virtual void StartSync(const SyncStatusCallback& callback) = 0;

  // Schedules a new sync.
  void Schedule();
  void ScheduleIfNotRunning();

  int64 pending_changes() const { return pending_changes_; }

 protected:
  void OnChangesUpdated(int64 pending_changes);
  SyncFileSystemService* sync_service() { return sync_service_; }

  // Returns the current service state.  Default implementation returns
  // sync_service()->GetSyncServiceState().
  virtual SyncServiceState GetServiceState();

 private:
  void Finished(const base::TimeTicks& start_time, SyncStatusCode status);
  void Run();
  void ScheduleInternal(int64 delay);

  std::string name_;
  SyncFileSystemService* sync_service_;
  int max_parallel_task_;
  int running_tasks_;
  scoped_ptr<TimerHelper> timer_helper_;
  base::TimeTicks last_scheduled_;
  int64 current_delay_;
  int64 last_delay_;
  int64 pending_changes_;
  base::WeakPtrFactory<SyncProcessRunner> factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncProcessRunner);
};

}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_SYNC_PROCESS_RUNNER_H_
