// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines a helper class for selecting a supported language from a
// set of candidates.

#include "chrome/installer/util/language_selector.h"

#include <algorithm>
#include <functional>

#include "base/logging.h"
#include "base/string_util.h"
#include "base/win/i18n.h"
#include "chrome/installer/util/google_update_settings.h"

#include "installer_util_strings.h"

namespace {

struct LangToOffset {
  const wchar_t* language;
  int offset;
};

// The language we fall back upon when all else fails.
const wchar_t kFallbackLanguage[] = L"en-us";
const int kFallbackLanguageOffset = IDS_L10N_OFFSET_EN_US;

// http://tools.ietf.org/html/rfc5646 Section 2.3.3
const std::wstring::size_type kScriptSubtagLength = 4;

// A sorted array of language identifiers (and their offsets) for which
// translations are available. The contents of the array are generated by
// create_string_rc.py.
const LangToOffset kLanguageOffsetPairs[] = {
#if defined(GOOGLE_CHROME_BUILD)
#define HANDLE_LANGUAGE(l_, o_) { L#l_, o_ },
  DO_LANGUAGES
#undef HANDLE_LANGUAGE
#else  // defined(GOOGLE_CHROME_BUILD)
  { &kFallbackLanguage[0], kFallbackLanguageOffset }
#endif  // !defined(GOOGLE_CHROME_BUILD)
};

// A sorted array of language identifiers that are aliases to other languages
// for which translations are available.
const LangToOffset kLanguageToOffsetExceptions[] = {
#if defined(GOOGLE_CHROME_BUILD)
  // Google web properties use iw for he. Handle both just to be safe.
  { L"he", IDS_L10N_OFFSET_IW },
  // Google web properties use no for nb. Handle both just to be safe.
  { L"nb", IDS_L10N_OFFSET_NO },
  // Some Google web properties use tl for fil. Handle both just to be safe.
  // They're not completely identical, but alias it here.
  { L"tl", IDS_L10N_OFFSET_FIL },
  // Pre-Vista aliases for Chinese w/ script subtag.
  { L"zh-chs", IDS_L10N_OFFSET_ZH_CN },
  { L"zh-cht", IDS_L10N_OFFSET_ZH_TW },
  // Vista+ aliases for Chinese w/ script subtag.
  { L"zh-hans", IDS_L10N_OFFSET_ZH_CN },
  { L"zh-hant", IDS_L10N_OFFSET_ZH_TW },
  // Alias Macau and Hong Kong to Taiwan.
  { L"zh-hk", IDS_L10N_OFFSET_ZH_TW },
  { L"zh-mk", IDS_L10N_OFFSET_ZH_TW },
  // Windows uses "mo" for Macau.
  { L"zh-mo", IDS_L10N_OFFSET_ZH_TW },
  // Although the wildcard entry for zh would result in this, alias zh-sg so
  // that it will win if it precedes another valid tag in a list of candidates.
  { L"zh-sg", IDS_L10N_OFFSET_ZH_CN }
#else  // defined(GOOGLE_CHROME_BUILD)
  // An empty array is no good, so repeat the fallback.
  { &kFallbackLanguage[0], kFallbackLanguageOffset }
#endif  // !defined(GOOGLE_CHROME_BUILD)
};

// A sorted array of neutral language identifiers that are wildcard aliases to
// other languages for which translations are available.
const LangToOffset kLanguageToOffsetWildcards[] = {
  // Use the U.S. region for anything English.
  { L"en", IDS_L10N_OFFSET_EN_US },
#if defined(GOOGLE_CHROME_BUILD)
  // Use the Latin American region for anything Spanish.
  { L"es", IDS_L10N_OFFSET_ES_419 },
  // Use the Brazil region for anything Portugese.
  { L"pt", IDS_L10N_OFFSET_PT_BR },
  // Use the P.R.C. region for anything Chinese.
  { L"zh", IDS_L10N_OFFSET_ZH_CN }
#endif  // defined(GOOGLE_CHROME_BUILD)
};

#if !defined(NDEBUG)
// Returns true if the items in the given range are sorted.  If
// |byNameAndOffset| is true, the items must be sorted by both name and offset.
bool IsArraySorted(const LangToOffset* first, const LangToOffset* last,
                   bool byNameAndOffset) {
  if (last - first > 1) {
    for (--last; first != last; ++first) {
       if (!(std::wstring(first->language) < (first + 1)->language) ||
           byNameAndOffset && !(first->offset < (first + 1)->offset)) {
         return false;
       }
    }
  }
  return true;
}

// Validates that the static read-only mappings are properly sorted.
void ValidateMappings() {
  // Ensure that kLanguageOffsetPairs is sorted.
  DCHECK(IsArraySorted(&kLanguageOffsetPairs[0],
                       &kLanguageOffsetPairs[arraysize(kLanguageOffsetPairs)],
                       true)) << "kOffsetToLanguageId is not sorted";

  // Ensure that kLanguageToOffsetExceptions is sorted.
  DCHECK(IsArraySorted(
           &kLanguageToOffsetExceptions[0],
           &kLanguageToOffsetExceptions[arraysize(kLanguageToOffsetExceptions)],
           false)) << "kLanguageToOffsetExceptions is not sorted";

  // Ensure that kLanguageToOffsetWildcards is sorted.
  DCHECK(IsArraySorted(
            &kLanguageToOffsetWildcards[0],
            &kLanguageToOffsetWildcards[arraysize(kLanguageToOffsetWildcards)],
            false)) << "kLanguageToOffsetWildcards is not sorted";
}
#endif  // !defined(NDEBUG)

// A less-than overload to do slightly more efficient searches in the
// sorted arrays.
bool operator<(const LangToOffset& left, const std::wstring& right) {
  return left.language < right;
}

// A less-than overload to do slightly more efficient searches in the
// sorted arrays.
bool operator<(const std::wstring& left, const LangToOffset& right) {
  return left < right.language;
}

// A not-so-efficient less-than overload for the same uses as above.
bool operator<(const LangToOffset& left, const LangToOffset& right) {
  return std::wstring(left.language) < right.language;
}

// A compare function for searching in a sorted array by offset.
bool IsOffsetLessThan(const LangToOffset& left, const LangToOffset& right) {
  return left.offset < right.offset;
}

// Binary search in one of the sorted arrays to find the offset corresponding to
// a given language |name|.
bool TryFindOffset(const LangToOffset* first, const LangToOffset* last,
                   const std::wstring& name, int* offset) {
  const LangToOffset* search_result = std::lower_bound(first, last, name);
  if (last != search_result && search_result->language == name) {
    *offset = search_result->offset;
    return true;
  }
  return false;
}

// A predicate function for LanguageSelector::SelectIf that searches for the
// offset of a translated language.  The search first tries to find an exact
// match.  Failing that, an exact match with an alias is attempted.
bool GetLanguageOffset(const std::wstring& language, int* offset) {
  // Note: always perform the exact match first so that an alias is never
  // selected in place of a future translation.
  return
      TryFindOffset(
          &kLanguageOffsetPairs[0],
          &kLanguageOffsetPairs[arraysize(kLanguageOffsetPairs)],
          language, offset) ||
      TryFindOffset(
          &kLanguageToOffsetExceptions[0],
          &kLanguageToOffsetExceptions[arraysize(kLanguageToOffsetExceptions)],
          language, offset);
}

// A predicate function for LanguageSelector::SelectIf that searches for a
// wildcard match with |language|'s primary language subtag.
bool MatchLanguageOffset(const std::wstring& language, int* offset) {
  std::wstring primary_language = language.substr(0, language.find(L'-'));

  // Now check for wildcards.
  return
      TryFindOffset(
          &kLanguageToOffsetWildcards[0],
          &kLanguageToOffsetWildcards[arraysize(kLanguageToOffsetWildcards)],
          primary_language, offset);
}

// Adds to |candidates| the eligible languages on the system.  Any language
// setting specified by Omaha takes precedence over the operating system's
// configured languages.
void GetCandidatesFromSystem(std::vector<std::wstring>* candidates) {
  DCHECK(candidates);
  std::wstring language;

  // Omaha gets first pick.
  GoogleUpdateSettings::GetLanguage(&language);
  if (!language.empty()) {
    candidates->push_back(language);
  }

  // Now try the Windows UI languages.  Use the thread preferred since that will
  // kindly return us a list of all kinds of fallbacks.
  base::win::i18n::GetThreadPreferredUILanguageList(candidates);
}

}  // namespace

namespace installer_util {

LanguageSelector::LanguageSelector()
    : offset_(arraysize(kLanguageOffsetPairs)) {
#if !defined(NDEBUG)
  ValidateMappings();
#endif  // !defined(NDEBUG)
  std::vector<std::wstring> candidates;

  GetCandidatesFromSystem(&candidates);
  DoSelect(candidates);
}

LanguageSelector::LanguageSelector(const std::vector<std::wstring>& candidates)
    : offset_(arraysize(kLanguageOffsetPairs)) {
#if !defined(NDEBUG)
  ValidateMappings();
#endif  // !defined(NDEBUG)
  DoSelect(candidates);
}

LanguageSelector::~LanguageSelector() {
}

// static
std::wstring LanguageSelector::GetLanguageName(int offset) {
  DCHECK_GE(offset, 0);
  DCHECK_LT(static_cast<size_t>(offset), arraysize(kLanguageOffsetPairs));

  LangToOffset value = { NULL, offset };
  const LangToOffset* search_result =
    std::lower_bound(&kLanguageOffsetPairs[0],
                     &kLanguageOffsetPairs[arraysize(kLanguageOffsetPairs)],
                     value, IsOffsetLessThan);
  if (&kLanguageOffsetPairs[arraysize(kLanguageOffsetPairs)] != search_result &&
      search_result->offset == offset) {
    return search_result->language;
  }
  NOTREACHED() << "Unknown language offset.";
  return std::wstring(&kFallbackLanguage[0], arraysize(kFallbackLanguage) - 1);
}

// Runs through the set of candidates, sending their downcased representation
// through |select_predicate|.  Returns true if the predicate selects a
// candidate, in which case |matched_name| is assigned the value of the
// candidate and |matched_offset| is assigned the language offset of the
// selected translation.
// static
bool LanguageSelector::SelectIf(const std::vector<std::wstring>& candidates,
                                SelectPred_Fn select_predicate,
                                std::wstring* matched_name,
                                int* matched_offset) {
  std::wstring candidate;
  for (std::vector<std::wstring>::const_iterator scan = candidates.begin(),
          end = candidates.end(); scan != end; ++scan) {
    candidate.assign(*scan);
    StringToLowerASCII(&candidate);
    if (select_predicate(candidate, matched_offset)) {
      matched_name->assign(*scan);
      return true;
    }
  }

  return false;
}

// Select the best-fit translation from the ordered list |candidates|.
// At the conclusion, this instance's |matched_candidate_| and |offset_| members
// are set to the name of the selected candidate and the offset of the matched
// translation.  If no translation is selected, the fallback's name and offset
// are selected.
void LanguageSelector::DoSelect(const std::vector<std::wstring>& candidates) {
  // Make a pass through the candidates looking for an exact or alias match.
  // Failing that, make another pass looking for a wildcard match.
  if (!SelectIf(candidates, &GetLanguageOffset, &matched_candidate_,
                &offset_) &&
      !SelectIf(candidates, &MatchLanguageOffset, &matched_candidate_,
                &offset_)) {
    VLOG(1) << "No suitable language found for any candidates.";

    // Our fallback is "en-us"
    matched_candidate_.assign(&kFallbackLanguage[0],
                              arraysize(kFallbackLanguage) - 1);
    offset_ = kFallbackLanguageOffset;
  }
}

}  // namespace installer_util
