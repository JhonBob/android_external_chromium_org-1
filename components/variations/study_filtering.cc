// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/study_filtering.h"

#include <set>

namespace chrome_variations {

namespace {

Study_Platform GetCurrentPlatform() {
#if defined(OS_WIN)
  return Study_Platform_PLATFORM_WINDOWS;
#elif defined(OS_IOS)
  return Study_Platform_PLATFORM_IOS;
#elif defined(OS_MACOSX)
  return Study_Platform_PLATFORM_MAC;
#elif defined(OS_CHROMEOS)
  return Study_Platform_PLATFORM_CHROMEOS;
#elif defined(OS_ANDROID)
  return Study_Platform_PLATFORM_ANDROID;
#elif defined(OS_LINUX) || defined(OS_BSD) || defined(OS_SOLARIS)
  // Default BSD and SOLARIS to Linux to not break those builds, although these
  // platforms are not officially supported by Chrome.
  return Study_Platform_PLATFORM_LINUX;
#else
#error Unknown platform
#endif
}

// Converts |date_time| in Study date format to base::Time.
base::Time ConvertStudyDateToBaseTime(int64 date_time) {
  return base::Time::UnixEpoch() + base::TimeDelta::FromSeconds(date_time);
}

}  // namespace

namespace internal {

bool CheckStudyChannel(const Study_Filter& filter, Study_Channel channel) {
  // An empty channel list matches all channels.
  if (filter.channel_size() == 0)
    return true;

  for (int i = 0; i < filter.channel_size(); ++i) {
    if (filter.channel(i) == channel)
      return true;
  }
  return false;
}

bool CheckStudyFormFactor(const Study_Filter& filter,
                          Study_FormFactor form_factor) {
  // An empty form factor list matches all form factors.
  if (filter.form_factor_size() == 0)
    return true;

  for (int i = 0; i < filter.form_factor_size(); ++i) {
    if (filter.form_factor(i) == form_factor)
      return true;
  }
  return false;
}

bool CheckStudyLocale(const Study_Filter& filter, const std::string& locale) {
  // An empty locale list matches all locales.
  if (filter.locale_size() == 0)
    return true;

  for (int i = 0; i < filter.locale_size(); ++i) {
    if (filter.locale(i) == locale)
      return true;
  }
  return false;
}

bool CheckStudyPlatform(const Study_Filter& filter, Study_Platform platform) {
  // An empty platform list matches all platforms.
  if (filter.platform_size() == 0)
    return true;

  for (int i = 0; i < filter.platform_size(); ++i) {
    if (filter.platform(i) == platform)
      return true;
  }
  return false;
}

bool CheckStudyStartDate(const Study_Filter& filter,
                         const base::Time& date_time) {
  if (filter.has_start_date()) {
    const base::Time start_date =
        ConvertStudyDateToBaseTime(filter.start_date());
    return date_time >= start_date;
  }

  return true;
}

bool CheckStudyVersion(const Study_Filter& filter,
                       const base::Version& version) {
  if (filter.has_min_version()) {
    if (version.CompareToWildcardString(filter.min_version()) < 0)
      return false;
  }

  if (filter.has_max_version()) {
    if (version.CompareToWildcardString(filter.max_version()) > 0)
      return false;
  }

  return true;
}

bool IsStudyExpired(const Study& study, const base::Time& date_time) {
  if (study.has_expiry_date()) {
    const base::Time expiry_date =
        ConvertStudyDateToBaseTime(study.expiry_date());
    return date_time >= expiry_date;
  }

  return false;
}

bool ShouldAddStudy(
    const Study& study,
    const std::string& locale,
    const base::Time& reference_date,
    const base::Version& version,
    Study_Channel channel,
    Study_FormFactor form_factor) {
  if (study.has_filter()) {
    if (!CheckStudyChannel(study.filter(), channel)) {
      DVLOG(1) << "Filtered out study " << study.name() << " due to channel.";
      return false;
    }

    if (!CheckStudyFormFactor(study.filter(), form_factor)) {
      DVLOG(1) << "Filtered out study " << study.name() <<
                  " due to form factor.";
      return false;
    }

    if (!CheckStudyLocale(study.filter(), locale)) {
      DVLOG(1) << "Filtered out study " << study.name() << " due to locale.";
      return false;
    }

    if (!CheckStudyPlatform(study.filter(), GetCurrentPlatform())) {
      DVLOG(1) << "Filtered out study " << study.name() << " due to platform.";
      return false;
    }

    if (!CheckStudyVersion(study.filter(), version)) {
      DVLOG(1) << "Filtered out study " << study.name() << " due to version.";
      return false;
    }

    if (!CheckStudyStartDate(study.filter(), reference_date)) {
      DVLOG(1) << "Filtered out study " << study.name() <<
                  " due to start date.";
      return false;
    }
  }

  DVLOG(1) << "Kept study " << study.name() << ".";
  return true;
}

}  // namespace internal

void FilterAndValidateStudies(
    const VariationsSeed& seed,
    const std::string& locale,
    const base::Time& reference_date,
    const base::Version& version,
    Study_Channel channel,
    Study_FormFactor form_factor,
    std::vector<ProcessedStudy>* filtered_studies) {
  DCHECK(version.IsValid());

  // Add expired studies (in a disabled state) only after all the non-expired
  // studies have been added (and do not add an expired study if a corresponding
  // non-expired study got added). This way, if there's both an expired and a
  // non-expired study that applies, the non-expired study takes priority.
  std::set<std::string> created_studies;
  std::vector<const Study*> expired_studies;

  for (int i = 0; i < seed.study_size(); ++i) {
    const Study& study = seed.study(i);
    if (!internal::ShouldAddStudy(study, locale, reference_date, version,
                                  channel, form_factor)) {
      continue;
    }

    if (internal::IsStudyExpired(study, reference_date)) {
      expired_studies.push_back(&study);
    } else if (!ContainsKey(created_studies, study.name())) {
      ProcessedStudy::ValidateAndAppendStudy(&study, false, filtered_studies);
      created_studies.insert(study.name());
    }
  }

  for (size_t i = 0; i < expired_studies.size(); ++i) {
    if (!ContainsKey(created_studies, expired_studies[i]->name())) {
      ProcessedStudy::ValidateAndAppendStudy(expired_studies[i], true,
                                             filtered_studies);
    }
  }
}

}  // namespace chrome_variations
