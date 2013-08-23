// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/diagnostics/diagnostics_test.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"

namespace diagnostics {

DiagnosticsTest::DiagnosticsTest(DiagnosticsTestId id)
    : id_(id), outcome_code_(-1), result_(DiagnosticsModel::TEST_NOT_RUN) {}

DiagnosticsTest::~DiagnosticsTest() {}

bool DiagnosticsTest::Execute(DiagnosticsModel::Observer* observer,
                              DiagnosticsModel* model,
                              size_t index) {
  start_time_ = base::Time::Now();
  result_ = DiagnosticsModel::TEST_RUNNING;
  bool keep_going = ExecuteImpl(observer);
  if (observer)
    observer->OnTestFinished(index, model);
  return keep_going;
}

bool DiagnosticsTest::Recover(DiagnosticsModel::Observer* observer,
                              DiagnosticsModel* model,
                              size_t index) {
  result_ = DiagnosticsModel::RECOVERY_RUNNING;
  bool keep_going = RecoveryImpl(observer);
  result_ = keep_going ? DiagnosticsModel::RECOVERY_OK
                       : DiagnosticsModel::RECOVERY_FAIL_STOP;
#if defined(OS_CHROMEOS)  // Only collecting UMA stats on ChromeOS
  if (result_ == DiagnosticsModel::RECOVERY_OK) {
    RecordUMARecoveryResult(static_cast<DiagnosticsTestId>(GetId()),
                            RESULT_SUCCESS);
  } else {
    RecordUMARecoveryResult(static_cast<DiagnosticsTestId>(GetId()),
                            RESULT_FAILURE);
  }
#endif
  if (observer)
    observer->OnRecoveryFinished(index, model);
  return keep_going;
}

void DiagnosticsTest::RecordOutcome(int outcome_code,
                                    const std::string& additional_info,
                                    DiagnosticsModel::TestResult result) {
  end_time_ = base::Time::Now();
  outcome_code_ = outcome_code;
  additional_info_ = additional_info;
  result_ = result;
#if defined(OS_CHROMEOS)  // Only collecting UMA stats on ChromeOS
  if (result_ == DiagnosticsModel::TEST_OK) {
    // Record individual test success.
    RecordUMATestResult(static_cast<DiagnosticsTestId>(GetId()),
                        RESULT_SUCCESS);
  } else if (result_ == DiagnosticsModel::TEST_FAIL_CONTINUE ||
             result_ == DiagnosticsModel::TEST_FAIL_STOP) {
    // Record test failure in summary histogram.
    UMA_HISTOGRAM_ENUMERATION(
        "Diagnostics.TestFailures", GetId(), DIAGNOSTICS_TEST_ID_COUNT);
    // Record individual test failure.
    RecordUMATestResult(static_cast<DiagnosticsTestId>(GetId()),
                        RESULT_FAILURE);
  }
#endif
}

// static
base::FilePath DiagnosticsTest::GetUserDefaultProfileDir() {
  base::FilePath path;
  if (!PathService::Get(chrome::DIR_USER_DATA, &path))
    return base::FilePath();
  return path.AppendASCII(chrome::kInitialProfile);
}

int DiagnosticsTest::GetId() const { return id_; }

std::string DiagnosticsTest::GetName() const { return GetTestName(id_); }

std::string DiagnosticsTest::GetTitle() const {
  return GetTestDescription(id_);
}

DiagnosticsModel::TestResult DiagnosticsTest::GetResult() const {
  return result_;
}

int DiagnosticsTest::GetOutcomeCode() const { return outcome_code_; }

std::string DiagnosticsTest::GetAdditionalInfo() const {
  return additional_info_;
}

base::Time DiagnosticsTest::GetStartTime() const { return start_time_; }

base::Time DiagnosticsTest::GetEndTime() const { return end_time_; }

bool DiagnosticsTest::RecoveryImpl(DiagnosticsModel::Observer* observer) {
  return true;
}

}  // namespace diagnostics
