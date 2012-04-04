// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data_appcache_helper.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/browsing_data_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "webkit/appcache/appcache_database.h"
#include "webkit/appcache/appcache_storage.h"

using appcache::AppCacheDatabase;
using content::BrowserContext;
using content::BrowserThread;
using content::ResourceContext;

BrowsingDataAppCacheHelper::BrowsingDataAppCacheHelper(Profile* profile)
    : is_fetching_(false),
      resource_context_(profile->GetResourceContext()) {
}

void BrowsingDataAppCacheHelper::StartFetching(const base::Closure& callback) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    DCHECK(!is_fetching_);
    DCHECK_EQ(false, callback.is_null());
    is_fetching_ = true;
    info_collection_ = new appcache::AppCacheInfoCollection;
    completion_callback_ = callback;
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowsingDataAppCacheHelper::StartFetching, this, callback));
    return;
  }

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  appcache_info_callback_.Reset(
      base::Bind(&BrowsingDataAppCacheHelper::OnFetchComplete,
                 base::Unretained(this)));
  ResourceContext::GetAppCacheService(resource_context_)->
      GetAllAppCacheInfo(info_collection_, appcache_info_callback_.callback());
}

void BrowsingDataAppCacheHelper::CancelNotification() {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    completion_callback_.Reset();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowsingDataAppCacheHelper::CancelNotification, this));
    return;
  }

  appcache_info_callback_.Cancel();
}

void BrowsingDataAppCacheHelper::DeleteAppCacheGroup(
    const GURL& manifest_url) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowsingDataAppCacheHelper::DeleteAppCacheGroup, this,
                   manifest_url));
    return;
  }

  ResourceContext::GetAppCacheService(resource_context_)->DeleteAppCacheGroup(
      manifest_url, net::CompletionCallback());
}

BrowsingDataAppCacheHelper::~BrowsingDataAppCacheHelper() {}

void BrowsingDataAppCacheHelper::OnFetchComplete(int rv) {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    // Filter out appcache info entries for non-websafe schemes. Extension state
    // and DevTools, for example, are not considered browsing data.
    typedef std::map<GURL, appcache::AppCacheInfoVector> InfoByOrigin;
    InfoByOrigin& origin_map = info_collection_->infos_by_origin;
    for (InfoByOrigin::iterator origin = origin_map.begin();
         origin != origin_map.end();) {
      InfoByOrigin::iterator current = origin;
      ++origin;
      if (!BrowsingDataHelper::HasValidScheme(current->first))
        origin_map.erase(current);
    }

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&BrowsingDataAppCacheHelper::OnFetchComplete, this, rv));
    return;
  }

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(is_fetching_);
  is_fetching_ = false;
  if (!completion_callback_.is_null()) {
    completion_callback_.Run();
    completion_callback_.Reset();
  }
}

CannedBrowsingDataAppCacheHelper::CannedBrowsingDataAppCacheHelper(
    Profile* profile)
    : BrowsingDataAppCacheHelper(profile),
      profile_(profile) {
  info_collection_ = new appcache::AppCacheInfoCollection;
}

CannedBrowsingDataAppCacheHelper* CannedBrowsingDataAppCacheHelper::Clone() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CannedBrowsingDataAppCacheHelper* clone =
      new CannedBrowsingDataAppCacheHelper(profile_);

  clone->info_collection_->infos_by_origin = info_collection_->infos_by_origin;
  return clone;
}

void CannedBrowsingDataAppCacheHelper::AddAppCache(const GURL& manifest_url) {
  if (!BrowsingDataHelper::HasValidScheme(manifest_url))
    return;  // Ignore non-websafe schemes.

  typedef std::map<GURL, appcache::AppCacheInfoVector> InfoByOrigin;
  InfoByOrigin& origin_map = info_collection_->infos_by_origin;
  appcache::AppCacheInfoVector& appcache_infos_ =
      origin_map[manifest_url.GetOrigin()];

  for (appcache::AppCacheInfoVector::iterator
       appcache = appcache_infos_.begin(); appcache != appcache_infos_.end();
       ++appcache) {
    if (appcache->manifest_url == manifest_url)
      return;
  }

  appcache::AppCacheInfo info;
  info.manifest_url = manifest_url;
  appcache_infos_.push_back(info);
}

void CannedBrowsingDataAppCacheHelper::Reset() {
  info_collection_->infos_by_origin.clear();
}

bool CannedBrowsingDataAppCacheHelper::empty() const {
  return info_collection_->infos_by_origin.empty();
}

void CannedBrowsingDataAppCacheHelper::StartFetching(
    const base::Closure& completion_callback) {
  completion_callback.Run();
}

CannedBrowsingDataAppCacheHelper::~CannedBrowsingDataAppCacheHelper() {}
