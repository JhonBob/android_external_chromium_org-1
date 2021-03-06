// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/services/gcm/gcm_profile_service.h"

#include "base/logging.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"

#if defined(OS_ANDROID)
#include "components/gcm_driver/gcm_driver_android.h"
#else
#include "base/files/file_path.h"
#include "chrome/browser/services/gcm/gcm_desktop_utils.h"
#include "chrome/browser/signin/profile_identity_provider.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/common/chrome_constants.h"
#include "components/gcm_driver/gcm_client_factory.h"
#include "components/signin/core/browser/signin_manager.h"
#include "google_apis/gaia/identity_provider.h"
#include "net/url_request/url_request_context_getter.h"
#endif

namespace gcm {

#if !defined(OS_ANDROID)
class GCMProfileService::IdentityObserver : public IdentityProvider::Observer {
 public:
  IdentityObserver(Profile* profile, GCMDriver* driver);
  virtual ~IdentityObserver();

  // IdentityProvider::Observer:
  virtual void OnActiveAccountLogin() OVERRIDE;
  virtual void OnActiveAccountLogout() OVERRIDE;

  std::string SignedInUserName() const;

 private:
  GCMDriver* driver_;
  scoped_ptr<IdentityProvider> identity_provider_;

  // The account ID that this service is responsible for. Empty when the service
  // is not running.
  std::string account_id_;

  DISALLOW_COPY_AND_ASSIGN(IdentityObserver);
};

GCMProfileService::IdentityObserver::IdentityObserver(Profile* profile,
                                                      GCMDriver* driver)
    : driver_(driver) {
  identity_provider_.reset(new ProfileIdentityProvider(
      SigninManagerFactory::GetForProfile(profile),
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      LoginUIServiceFactory::GetForProfile(profile)));
  identity_provider_->AddObserver(this);

  OnActiveAccountLogin();
}

GCMProfileService::IdentityObserver::~IdentityObserver() {
  identity_provider_->RemoveObserver(this);
}

void GCMProfileService::IdentityObserver::OnActiveAccountLogin() {
  // This might be called multiple times when the password changes.
  const std::string account_id = identity_provider_->GetActiveAccountId();
  if (account_id == account_id_)
    return;
  account_id_ = account_id;

  driver_->OnSignedIn();
}

void GCMProfileService::IdentityObserver::OnActiveAccountLogout() {
  driver_->Purge();
}

std::string GCMProfileService::IdentityObserver::SignedInUserName() const {
  return driver_->IsStarted() ? account_id_ : std::string();
}
#endif  // !defined(OS_ANDROID)

// static
bool GCMProfileService::IsGCMEnabled(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(prefs::kGCMChannelEnabled);
}

// static
void GCMProfileService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kGCMChannelEnabled,
      true,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  PushMessagingServiceImpl::RegisterProfilePrefs(registry);
}

#if defined(OS_ANDROID)
GCMProfileService::GCMProfileService(Profile* profile)
    : profile_(profile),
      push_messaging_service_(this, profile) {
  DCHECK(!profile->IsOffTheRecord());

  driver_.reset(new GCMDriverAndroid);
}
#else
GCMProfileService::GCMProfileService(
    Profile* profile,
    scoped_ptr<GCMClientFactory> gcm_client_factory)
    : profile_(profile),
      push_messaging_service_(this, profile) {
  DCHECK(!profile->IsOffTheRecord());

  driver_ = CreateGCMDriverDesktop(
      gcm_client_factory.Pass(),
      profile_->GetPath().Append(chrome::kGCMStoreDirname),
      profile_->GetRequestContext());

  identity_observer_.reset(new IdentityObserver(profile, driver_.get()));
}
#endif  // defined(OS_ANDROID)

GCMProfileService::GCMProfileService()
    : profile_(NULL),
      push_messaging_service_(this, NULL) {
}

GCMProfileService::~GCMProfileService() {
}

void GCMProfileService::AddAppHandler(const std::string& app_id,
                                      GCMAppHandler* handler) {
  if (driver_)
    driver_->AddAppHandler(app_id, handler);
}

void GCMProfileService::RemoveAppHandler(const std::string& app_id) {
  if (driver_)
    driver_->RemoveAppHandler(app_id);
}

void GCMProfileService::Register(const std::string& app_id,
                                 const std::vector<std::string>& sender_ids,
                                 const GCMDriver::RegisterCallback& callback) {
  if (driver_)
    driver_->Register(app_id, sender_ids, callback);
}

void GCMProfileService::Shutdown() {
#if !defined(OS_ANDROID)
  identity_observer_.reset();
#endif  // !defined(OS_ANDROID)

  if (driver_) {
    driver_->Shutdown();
    driver_.reset();
  }
}

std::string GCMProfileService::SignedInUserName() const {
#if defined(OS_ANDROID)
  return std::string();
#else
  return identity_observer_ ? identity_observer_->SignedInUserName()
                            : std::string();
#endif  // defined(OS_ANDROID)
}

void GCMProfileService::SetDriverForTesting(GCMDriver* driver) {
  driver_.reset(driver);
}

}  // namespace gcm
