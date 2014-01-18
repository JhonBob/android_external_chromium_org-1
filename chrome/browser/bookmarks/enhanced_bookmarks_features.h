// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_ENHANCED_BOOKMARKS_FEATURES_H_
#define CHROME_BROWSER_BOOKMARKS_ENHANCED_BOOKMARKS_FEATURES_H_

#include <string>

#include "extensions/common/extension.h"

// If user not in Finch experiment then check if extension was installed
// manually and then opt-in user into experiment.
// Returns true if user was opt-in.
bool OptInIntoBookmarksExperimentIfHasExtension(
    const extensions::ExtensionIdSet& extension_ids);

// Returns true if enhanced bookmarks experiment is enabled.
bool IsEnhancedBookmarksExperimentEnabled();

// Returns true when flag enable-dom-distiller is set or enabled from Finch.
bool IsEnableDomDistillerSet();

// Returns true when flag enable-sync-articles is set or enabled from Finch.
bool IsEnableSyncArticlesSet();

// Get extension id from Finch EnhancedBookmarks group parameters.
std::string GetEnhancedBookmarksExtensionId();

#endif  // CHROME_BROWSER_BOOKMARKS_ENHANCED_BOOKMARKS_FEATURES_H_
