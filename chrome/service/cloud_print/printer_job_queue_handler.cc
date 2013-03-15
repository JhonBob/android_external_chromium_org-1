// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/printer_job_queue_handler.h"

#include <math.h>

#include <algorithm>

#include "base/values.h"

namespace cloud_print {

class TimeProviderImpl : public PrinterJobQueueHandler::TimeProvider {
 public:
    virtual base::Time GetNow() OVERRIDE;
};

base::Time TimeProviderImpl::GetNow() {
  return base::Time::Now();
}

JobDetails::JobDetails() {}

JobDetails::~JobDetails() {}

void JobDetails::Clear() {
  job_id_.clear();
  job_title_.clear();
  print_ticket_.clear();
  print_data_mime_type_.clear();
  print_data_file_path_ = base::FilePath();
  print_data_url_.clear();
  print_ticket_url_.clear();
  tags_.clear();
  time_remaining_ = base::TimeDelta();
}

// static
bool JobDetails::ordering(const JobDetails& first, const JobDetails& second) {
  return first.time_remaining_ <= second.time_remaining_;
}

PrinterJobQueueHandler::PrinterJobQueueHandler(TimeProvider* time_provider)
    : time_provider_(time_provider) {}

PrinterJobQueueHandler::PrinterJobQueueHandler()
    : time_provider_(new TimeProviderImpl()) {}

PrinterJobQueueHandler::~PrinterJobQueueHandler() {}

void PrinterJobQueueHandler::ConstructJobDetailsFromJson(
    const DictionaryValue* job_data,
    JobDetails* job_details) {
  job_details->Clear();

  job_data->GetString(kIdValue, &job_details->job_id_);
  job_data->GetString(kTitleValue, &job_details->job_title_);

  job_data->GetString(kTicketUrlValue, &job_details->print_ticket_url_);
  job_data->GetString(kFileUrlValue, &job_details->print_data_url_);

  // Get tags for print job.
  const ListValue* tags = NULL;
  if (job_data->GetList(kTagsValue, &tags)) {
    for (size_t i = 0; i < tags->GetSize(); i++) {
      std::string value;
      if (tags->GetString(i, &value))
        job_details->tags_.push_back(value);
    }
  }
}

base::TimeDelta PrinterJobQueueHandler::ComputeBackoffTime(
    const std::string& job_id) {
  FailedJobMap::const_iterator job_location = failed_job_map_.find(job_id);
  if (job_location == failed_job_map_.end()) {
    return base::TimeDelta();
  }

  base::TimeDelta backoff_time =
      base::TimeDelta::FromSeconds(kJobFirstWaitTimeSecs);
  backoff_time *=
      // casting argument to double and result to uint64 to avoid compilation
      // issues
      static_cast<int64>(pow(
          static_cast<long double>(kJobWaitTimeExponentialMultiplier),
          job_location->second.retries_) + 0.5);
  base::Time scheduled_retry =
      job_location->second.last_retry_ + backoff_time;
  base::Time now = time_provider_->GetNow();
  base::TimeDelta time_remaining;

  if (scheduled_retry < now) {
    return base::TimeDelta();
  }
  return scheduled_retry - now;
}

void PrinterJobQueueHandler::GetJobsFromQueue(const DictionaryValue* json_data,
                                              std::vector<JobDetails>* jobs) {
  std::vector<JobDetails> jobs_with_timeouts;

  jobs->clear();

  const ListValue* job_list = NULL;
  if (!json_data->GetList(kJobListValue, &job_list)) {
    return;
  }

  size_t list_size = job_list->GetSize();
  for (size_t cur_job = 0; cur_job < list_size; cur_job++) {
    const DictionaryValue* job_data = NULL;
    if (job_list->GetDictionary(cur_job, &job_data)) {
      JobDetails job_details_current;
      ConstructJobDetailsFromJson(job_data, &job_details_current);

      job_details_current.time_remaining_ =
          ComputeBackoffTime(job_details_current.job_id_);

      if (job_details_current.time_remaining_ == base::TimeDelta()) {
        jobs->push_back(job_details_current);
      } else {
        jobs_with_timeouts.push_back(job_details_current);
      }
    }
  }

  sort(jobs_with_timeouts.begin(), jobs_with_timeouts.end(),
       &JobDetails::ordering);
  jobs->insert(jobs->end(), jobs_with_timeouts.begin(),
               jobs_with_timeouts.end());
}

void PrinterJobQueueHandler::JobDone(const std::string& job_id) {
  failed_job_map_.erase(job_id);
}

bool PrinterJobQueueHandler::JobFetchFailed(const std::string& job_id) {
  FailedJobMetadata metadata;
  metadata.retries_ = 0;
  metadata.last_retry_ = time_provider_->GetNow();

  std::pair<FailedJobMap::iterator, bool> job_found =
      failed_job_map_.insert(FailedJobPair(job_id, metadata));

  // If the job has already failed once, increment the number of retries.
  // If it has failed too many times, remove it from the map and tell the caller
  // to report a failure.
  if (!job_found.second) {
    if (job_found.first->second.retries_ >= kNumRetriesBeforeAbandonJob) {
      failed_job_map_.erase(job_found.first);
      return false;
    }

    job_found.first->second.retries_ += 1;
    job_found.first->second.last_retry_ = time_provider_->GetNow();
  }

  return true;
}

}  // namespace cloud_print

