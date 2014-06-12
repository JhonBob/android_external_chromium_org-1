// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/managed_mode/managed_user_service.h"

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/managed_mode/custodian_profile_downloader_service.h"
#include "chrome/browser/managed_mode/custodian_profile_downloader_service_factory.h"
#include "chrome/browser/managed_mode/managed_mode_site_list.h"
#include "chrome/browser/managed_mode/managed_user_constants.h"
#include "chrome/browser/managed_mode/managed_user_registration_utility.h"
#include "chrome/browser/managed_mode/managed_user_settings_service.h"
#include "chrome/browser/managed_mode/managed_user_settings_service_factory.h"
#include "chrome/browser/managed_mode/managed_user_shared_settings_service_factory.h"
#include "chrome/browser/managed_mode/managed_user_sync_service.h"
#include "chrome/browser/managed_mode/managed_user_sync_service_factory.h"
#include "chrome/browser/managed_mode/permission_request_creator_apiary.h"
#include "chrome/browser/managed_mode/permission_request_creator_sync.h"
#include "chrome/browser/managed_mode/supervised_user_pref_mapping_service.h"
#include "chrome/browser/managed_mode/supervised_user_pref_mapping_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/api/managed_mode_private/managed_mode_handler.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/user_metrics.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension_set.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "grit/generated_resources.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"
#include "chrome/browser/chromeos/login/users/user_manager.h"
#endif

#if defined(ENABLE_THEMES)
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#endif

using base::DictionaryValue;
using base::UserMetricsAction;
using content::BrowserThread;

ManagedUserService::URLFilterContext::URLFilterContext()
    : ui_url_filter_(new ManagedModeURLFilter),
      io_url_filter_(new ManagedModeURLFilter) {}
ManagedUserService::URLFilterContext::~URLFilterContext() {}

ManagedModeURLFilter*
ManagedUserService::URLFilterContext::ui_url_filter() const {
  return ui_url_filter_.get();
}

ManagedModeURLFilter*
ManagedUserService::URLFilterContext::io_url_filter() const {
  return io_url_filter_.get();
}

void ManagedUserService::URLFilterContext::SetDefaultFilteringBehavior(
    ManagedModeURLFilter::FilteringBehavior behavior) {
  ui_url_filter_->SetDefaultFilteringBehavior(behavior);
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&ManagedModeURLFilter::SetDefaultFilteringBehavior,
                 io_url_filter_.get(), behavior));
}

void ManagedUserService::URLFilterContext::LoadWhitelists(
    ScopedVector<ManagedModeSiteList> site_lists) {
  // ManagedModeURLFilter::LoadWhitelists takes ownership of |site_lists|,
  // so we make an additional copy of it.
  /// TODO(bauerb): This is kinda ugly.
  ScopedVector<ManagedModeSiteList> site_lists_copy;
  for (ScopedVector<ManagedModeSiteList>::iterator it = site_lists.begin();
       it != site_lists.end(); ++it) {
    site_lists_copy.push_back((*it)->Clone());
  }
  ui_url_filter_->LoadWhitelists(site_lists.Pass());
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&ManagedModeURLFilter::LoadWhitelists,
                 io_url_filter_, base::Passed(&site_lists_copy)));
}

void ManagedUserService::URLFilterContext::SetManualHosts(
    scoped_ptr<std::map<std::string, bool> > host_map) {
  ui_url_filter_->SetManualHosts(host_map.get());
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&ManagedModeURLFilter::SetManualHosts,
                 io_url_filter_, base::Owned(host_map.release())));
}

void ManagedUserService::URLFilterContext::SetManualURLs(
    scoped_ptr<std::map<GURL, bool> > url_map) {
  ui_url_filter_->SetManualURLs(url_map.get());
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&ManagedModeURLFilter::SetManualURLs,
                 io_url_filter_, base::Owned(url_map.release())));
}

ManagedUserService::ManagedUserService(Profile* profile)
    : profile_(profile),
      active_(false),
      delegate_(NULL),
      extension_registry_observer_(this),
      waiting_for_sync_initialization_(false),
      is_profile_active_(false),
      elevated_for_testing_(false),
      did_shutdown_(false),
      waiting_for_permissions_(false),
      weak_ptr_factory_(this) {
}

ManagedUserService::~ManagedUserService() {
  DCHECK(did_shutdown_);
}

void ManagedUserService::Shutdown() {
  did_shutdown_ = true;
  if (ProfileIsManaged()) {
    content::RecordAction(UserMetricsAction("ManagedUsers_QuitBrowser"));
  }
  SetActive(false);
}

bool ManagedUserService::ProfileIsManaged() const {
  return profile_->IsSupervised();
}

// static
void ManagedUserService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(
      prefs::kSupervisedUserManualHosts,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      prefs::kSupervisedUserManualURLs,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterIntegerPref(
      prefs::kDefaultSupervisedUserFilteringBehavior,
      ManagedModeURLFilter::ALLOW,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterStringPref(
      prefs::kSupervisedUserCustodianEmail, std::string(),
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterStringPref(
      prefs::kSupervisedUserCustodianName, std::string(),
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterBooleanPref(prefs::kSupervisedUserCreationAllowed, true,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
}

// static
void ManagedUserService::MigrateUserPrefs(PrefService* prefs) {
  if (!prefs->HasPrefPath(prefs::kProfileIsSupervised))
    return;

  bool is_managed = prefs->GetBoolean(prefs::kProfileIsSupervised);
  prefs->ClearPref(prefs::kProfileIsSupervised);

  if (!is_managed)
    return;

  std::string managed_user_id = prefs->GetString(prefs::kSupervisedUserId);
  if (!managed_user_id.empty())
    return;

  prefs->SetString(prefs::kSupervisedUserId, "Dummy ID");
}

void ManagedUserService::SetDelegate(Delegate* delegate) {
  if (delegate_ == delegate)
    return;
  // If the delegate changed, deactivate first to give the old delegate a chance
  // to clean up.
  SetActive(false);
  delegate_ = delegate;
}

scoped_refptr<const ManagedModeURLFilter>
ManagedUserService::GetURLFilterForIOThread() {
  return url_filter_context_.io_url_filter();
}

ManagedModeURLFilter* ManagedUserService::GetURLFilterForUIThread() {
  return url_filter_context_.ui_url_filter();
}

// Items not on any list must return -1 (CATEGORY_NOT_ON_LIST in history.js).
// Items on a list, but with no category, must return 0 (CATEGORY_OTHER).
#define CATEGORY_NOT_ON_LIST -1;
#define CATEGORY_OTHER 0;

int ManagedUserService::GetCategory(const GURL& url) {
  std::vector<ManagedModeSiteList::Site*> sites;
  GetURLFilterForUIThread()->GetSites(url, &sites);
  if (sites.empty())
    return CATEGORY_NOT_ON_LIST;

  return (*sites.begin())->category_id;
}

// static
void ManagedUserService::GetCategoryNames(CategoryList* list) {
  ManagedModeSiteList::GetCategoryNames(list);
}

std::string ManagedUserService::GetCustodianEmailAddress() const {
#if defined(OS_CHROMEOS)
  return chromeos::UserManager::Get()->GetSupervisedUserManager()->
      GetManagerDisplayEmail(
          chromeos::UserManager::Get()->GetActiveUser()->email());
#else
  return profile_->GetPrefs()->GetString(prefs::kSupervisedUserCustodianEmail);
#endif
}

std::string ManagedUserService::GetCustodianName() const {
#if defined(OS_CHROMEOS)
  return base::UTF16ToUTF8(chromeos::UserManager::Get()->
      GetSupervisedUserManager()->GetManagerDisplayName(
          chromeos::UserManager::Get()->GetActiveUser()->email()));
#else
  std::string name = profile_->GetPrefs()->GetString(
      prefs::kSupervisedUserCustodianName);
  return name.empty() ? GetCustodianEmailAddress() : name;
#endif
}

void ManagedUserService::AddNavigationBlockedCallback(
    const NavigationBlockedCallback& callback) {
  navigation_blocked_callbacks_.push_back(callback);
}

void ManagedUserService::DidBlockNavigation(
    content::WebContents* web_contents) {
  for (std::vector<NavigationBlockedCallback>::iterator it =
           navigation_blocked_callbacks_.begin();
       it != navigation_blocked_callbacks_.end(); ++it) {
    it->Run(web_contents);
  }
}

std::string ManagedUserService::GetDebugPolicyProviderName() const {
  // Save the string space in official builds.
#ifdef NDEBUG
  NOTREACHED();
  return std::string();
#else
  return "Managed User Service";
#endif
}

bool ManagedUserService::UserMayLoad(const extensions::Extension* extension,
                                     base::string16* error) const {
  base::string16 tmp_error;
  if (ExtensionManagementPolicyImpl(extension, &tmp_error))
    return true;

  // If the extension is already loaded, we allow it, otherwise we'd unload
  // all existing extensions.
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();

  // |extension_service| can be NULL in a unit test.
  if (extension_service &&
      extension_service->GetInstalledExtension(extension->id()))
    return true;

  bool was_installed_by_default = extension->was_installed_by_default();
#if defined(OS_CHROMEOS)
  // On Chrome OS all external sources are controlled by us so it means that
  // they are "default". Method was_installed_by_default returns false because
  // extensions creation flags are ignored in case of default extensions with
  // update URL(the flags aren't passed to OnExternalExtensionUpdateUrlFound).
  // TODO(dpolukhin): remove this Chrome OS specific code as soon as creation
  // flags are not ignored.
  was_installed_by_default =
      extensions::Manifest::IsExternalLocation(extension->location());
#endif
  if (extension->location() == extensions::Manifest::COMPONENT ||
      was_installed_by_default) {
    return true;
  }

  if (error)
    *error = tmp_error;
  return false;
}

bool ManagedUserService::UserMayModifySettings(
    const extensions::Extension* extension,
    base::string16* error) const {
  return ExtensionManagementPolicyImpl(extension, error);
}

void ManagedUserService::OnStateChanged() {
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (waiting_for_sync_initialization_ && service->sync_initialized()) {
    waiting_for_sync_initialization_ = false;
    service->RemoveObserver(this);
    SetupSync();
    return;
  }

  DLOG_IF(ERROR, service->GetAuthError().state() ==
                     GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS)
      << "Credentials rejected";
}

void ManagedUserService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (!extensions::ManagedModeInfo::GetContentPackSiteList(extension).empty()) {
    UpdateSiteLists();
  }
}
void ManagedUserService::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionInfo::Reason reason) {
  if (!extensions::ManagedModeInfo::GetContentPackSiteList(extension).empty()) {
    UpdateSiteLists();
  }
}

void ManagedUserService::SetupSync() {
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  DCHECK(service->sync_initialized());

  bool sync_everything = false;
  syncer::ModelTypeSet synced_datatypes;
  synced_datatypes.Put(syncer::SUPERVISED_USER_SETTINGS);
  service->OnUserChoseDatatypes(sync_everything, synced_datatypes);

  // Notify ProfileSyncService that we are done with configuration.
  service->SetSetupInProgress(false);
  service->SetSyncSetupCompleted();
}

bool ManagedUserService::ExtensionManagementPolicyImpl(
    const extensions::Extension* extension,
    base::string16* error) const {
  // |extension| can be NULL in unit_tests.
  if (!ProfileIsManaged() || (extension && extension->is_theme()))
    return true;

  if (elevated_for_testing_)
    return true;

  if (error)
    *error = l10n_util::GetStringUTF16(IDS_EXTENSIONS_LOCKED_MANAGED_USER);
  return false;
}

ScopedVector<ManagedModeSiteList> ManagedUserService::GetActiveSiteLists() {
  ScopedVector<ManagedModeSiteList> site_lists;
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  // Can be NULL in unit tests.
  if (!extension_service)
    return site_lists.Pass();

  const extensions::ExtensionSet* extensions = extension_service->extensions();
  for (extensions::ExtensionSet::const_iterator it = extensions->begin();
       it != extensions->end(); ++it) {
    const extensions::Extension* extension = it->get();
    if (!extension_service->IsExtensionEnabled(extension->id()))
      continue;

    extensions::ExtensionResource site_list =
        extensions::ManagedModeInfo::GetContentPackSiteList(extension);
    if (!site_list.empty()) {
      site_lists.push_back(new ManagedModeSiteList(extension->id(),
                                                   site_list.GetFilePath()));
    }
  }

  return site_lists.Pass();
}

ManagedUserSettingsService* ManagedUserService::GetSettingsService() {
  return ManagedUserSettingsServiceFactory::GetForProfile(profile_);
}

void ManagedUserService::OnManagedUserIdChanged() {
  std::string managed_user_id =
      profile_->GetPrefs()->GetString(prefs::kSupervisedUserId);
  SetActive(!managed_user_id.empty());
}

void ManagedUserService::OnDefaultFilteringBehaviorChanged() {
  DCHECK(ProfileIsManaged());

  int behavior_value = profile_->GetPrefs()->GetInteger(
      prefs::kDefaultSupervisedUserFilteringBehavior);
  ManagedModeURLFilter::FilteringBehavior behavior =
      ManagedModeURLFilter::BehaviorFromInt(behavior_value);
  url_filter_context_.SetDefaultFilteringBehavior(behavior);
}

void ManagedUserService::UpdateSiteLists() {
  url_filter_context_.LoadWhitelists(GetActiveSiteLists());
}

bool ManagedUserService::AccessRequestsEnabled() {
  if (waiting_for_permissions_)
    return false;

  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  GoogleServiceAuthError::State state = service->GetAuthError().state();
  // We allow requesting access if Sync is working or has a transient error.
  return (state == GoogleServiceAuthError::NONE ||
          state == GoogleServiceAuthError::CONNECTION_FAILED ||
          state == GoogleServiceAuthError::SERVICE_UNAVAILABLE);
}

void ManagedUserService::OnPermissionRequestIssued() {
  waiting_for_permissions_ = false;
  // TODO(akuegel): Figure out how to show the result of issuing the permission
  // request in the UI. Currently, we assume the permission request was created
  // successfully.
}

void ManagedUserService::AddAccessRequest(const GURL& url) {
  // Normalize the URL.
  GURL normalized_url = ManagedModeURLFilter::Normalize(url);

  // Escape the URL.
  std::string output(net::EscapeQueryParamValue(normalized_url.spec(), true));

  waiting_for_permissions_ = true;
  permissions_creator_->CreatePermissionRequest(
      output,
      base::Bind(&ManagedUserService::OnPermissionRequestIssued,
                 weak_ptr_factory_.GetWeakPtr()));
}

ManagedUserService::ManualBehavior ManagedUserService::GetManualBehaviorForHost(
    const std::string& hostname) {
  const base::DictionaryValue* dict =
      profile_->GetPrefs()->GetDictionary(prefs::kSupervisedUserManualHosts);
  bool allow = false;
  if (!dict->GetBooleanWithoutPathExpansion(hostname, &allow))
    return MANUAL_NONE;

  return allow ? MANUAL_ALLOW : MANUAL_BLOCK;
}

ManagedUserService::ManualBehavior ManagedUserService::GetManualBehaviorForURL(
    const GURL& url) {
  const base::DictionaryValue* dict =
      profile_->GetPrefs()->GetDictionary(prefs::kSupervisedUserManualURLs);
  GURL normalized_url = ManagedModeURLFilter::Normalize(url);
  bool allow = false;
  if (!dict->GetBooleanWithoutPathExpansion(normalized_url.spec(), &allow))
    return MANUAL_NONE;

  return allow ? MANUAL_ALLOW : MANUAL_BLOCK;
}

void ManagedUserService::GetManualExceptionsForHost(const std::string& host,
                                                    std::vector<GURL>* urls) {
  const base::DictionaryValue* dict =
      profile_->GetPrefs()->GetDictionary(prefs::kSupervisedUserManualURLs);
  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    GURL url(it.key());
    if (url.host() == host)
      urls->push_back(url);
  }
}

void ManagedUserService::InitSync(const std::string& refresh_token) {
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  // Tell the sync service that setup is in progress so we don't start syncing
  // until we've finished configuration.
  service->SetSetupInProgress(true);

  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile_);
  token_service->UpdateCredentials(managed_users::kManagedUserPseudoEmail,
                                   refresh_token);

  // Continue in SetupSync() once the Sync backend has been initialized.
  if (service->sync_initialized()) {
    SetupSync();
  } else {
    ProfileSyncServiceFactory::GetForProfile(profile_)->AddObserver(this);
    waiting_for_sync_initialization_ = true;
  }
}

void ManagedUserService::Init() {
  DCHECK(GetSettingsService()->IsReady());

  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      prefs::kSupervisedUserId,
      base::Bind(&ManagedUserService::OnManagedUserIdChanged,
          base::Unretained(this)));

  SetActive(ProfileIsManaged());
}

void ManagedUserService::SetActive(bool active) {
  if (active_ == active)
    return;
  active_ = active;

  if (!delegate_ || !delegate_->SetActive(active_)) {
    if (active_) {
      SupervisedUserPrefMappingServiceFactory::GetForBrowserContext(profile_)
          ->Init();

      CommandLine* command_line = CommandLine::ForCurrentProcess();
      if (command_line->HasSwitch(switches::kSupervisedUserSyncToken)) {
        InitSync(
            command_line->GetSwitchValueASCII(
                switches::kSupervisedUserSyncToken));
      }

      ProfileOAuth2TokenService* token_service =
          ProfileOAuth2TokenServiceFactory::GetForProfile(profile_);
      token_service->LoadCredentials(managed_users::kManagedUserPseudoEmail);
    }
  }

  // Now activate/deactivate anything not handled by the delegate yet.

#if defined(ENABLE_THEMES)
  // Re-set the default theme to turn the SU theme on/off.
  ThemeService* theme_service = ThemeServiceFactory::GetForProfile(profile_);
  if (theme_service->UsingDefaultTheme() || theme_service->UsingSystemTheme()) {
    ThemeServiceFactory::GetForProfile(profile_)->UseDefaultTheme();
  }
#endif

  ManagedUserSettingsService* settings_service = GetSettingsService();
  settings_service->SetActive(active_);

  extensions::ExtensionSystem* extension_system =
      extensions::ExtensionSystem::Get(profile_);
  extensions::ManagementPolicy* management_policy =
      extension_system->management_policy();

  if (active_) {
    if (CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kPermissionRequestApiUrl)) {
      permissions_creator_ =
          PermissionRequestCreatorApiary::CreateWithProfile(profile_);
    } else {
      PrefService* pref_service = profile_->GetPrefs();
      permissions_creator_.reset(new PermissionRequestCreatorSync(
          settings_service,
          ManagedUserSharedSettingsServiceFactory::GetForBrowserContext(
              profile_),
          pref_service->GetString(prefs::kProfileName),
          pref_service->GetString(prefs::kSupervisedUserId)));
    }

    if (management_policy)
      management_policy->RegisterProvider(this);

    extension_registry_observer_.Add(
        extensions::ExtensionRegistry::Get(profile_));

    pref_change_registrar_.Add(
        prefs::kDefaultSupervisedUserFilteringBehavior,
        base::Bind(&ManagedUserService::OnDefaultFilteringBehaviorChanged,
            base::Unretained(this)));
    pref_change_registrar_.Add(prefs::kSupervisedUserManualHosts,
        base::Bind(&ManagedUserService::UpdateManualHosts,
                   base::Unretained(this)));
    pref_change_registrar_.Add(prefs::kSupervisedUserManualURLs,
        base::Bind(&ManagedUserService::UpdateManualURLs,
                   base::Unretained(this)));

    // Initialize the filter.
    OnDefaultFilteringBehaviorChanged();
    UpdateSiteLists();
    UpdateManualHosts();
    UpdateManualURLs();

#if !defined(OS_ANDROID)
    // TODO(bauerb): Get rid of the platform-specific #ifdef here.
    // http://crbug.com/313377
    BrowserList::AddObserver(this);
#endif
  } else {
    permissions_creator_.reset();

    if (management_policy)
      management_policy->UnregisterProvider(this);

    extension_registry_observer_.RemoveAll();

    pref_change_registrar_.Remove(
        prefs::kDefaultSupervisedUserFilteringBehavior);
    pref_change_registrar_.Remove(prefs::kSupervisedUserManualHosts);
    pref_change_registrar_.Remove(prefs::kSupervisedUserManualURLs);

    if (waiting_for_sync_initialization_) {
      ProfileSyncService* sync_service =
          ProfileSyncServiceFactory::GetForProfile(profile_);
      sync_service->RemoveObserver(this);
    }

#if !defined(OS_ANDROID)
    // TODO(bauerb): Get rid of the platform-specific #ifdef here.
    // http://crbug.com/313377
    BrowserList::RemoveObserver(this);
#endif
  }
}

void ManagedUserService::RegisterAndInitSync(
    ManagedUserRegistrationUtility* registration_utility,
    Profile* custodian_profile,
    const std::string& managed_user_id,
    const AuthErrorCallback& callback) {
  DCHECK(ProfileIsManaged());
  DCHECK(!custodian_profile->IsSupervised());

  base::string16 name = base::UTF8ToUTF16(
      profile_->GetPrefs()->GetString(prefs::kProfileName));
  int avatar_index = profile_->GetPrefs()->GetInteger(
      prefs::kProfileAvatarIndex);
  ManagedUserRegistrationInfo info(name, avatar_index);
  registration_utility->Register(
      managed_user_id,
      info,
      base::Bind(&ManagedUserService::OnManagedUserRegistered,
                 weak_ptr_factory_.GetWeakPtr(), callback, custodian_profile));

  // Fetch the custodian's profile information, to store the name.
  // TODO(pamg): If --google-profile-info (flag: switches::kGoogleProfileInfo)
  // is ever enabled, take the name from the ProfileInfoCache instead.
  CustodianProfileDownloaderService* profile_downloader_service =
      CustodianProfileDownloaderServiceFactory::GetForProfile(
          custodian_profile);
  profile_downloader_service->DownloadProfile(
      base::Bind(&ManagedUserService::OnCustodianProfileDownloaded,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ManagedUserService::OnCustodianProfileDownloaded(
    const base::string16& full_name) {
  profile_->GetPrefs()->SetString(prefs::kSupervisedUserCustodianName,
                                  base::UTF16ToUTF8(full_name));
}

void ManagedUserService::OnManagedUserRegistered(
    const AuthErrorCallback& callback,
    Profile* custodian_profile,
    const GoogleServiceAuthError& auth_error,
    const std::string& token) {
  if (auth_error.state() == GoogleServiceAuthError::NONE) {
    InitSync(token);
    SigninManagerBase* signin =
        SigninManagerFactory::GetForProfile(custodian_profile);
    profile_->GetPrefs()->SetString(prefs::kSupervisedUserCustodianEmail,
                                    signin->GetAuthenticatedUsername());

    // The managed-user profile is now ready for use.
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    ProfileInfoCache& cache = profile_manager->GetProfileInfoCache();
    size_t index = cache.GetIndexOfProfileWithPath(profile_->GetPath());
    cache.SetIsOmittedProfileAtIndex(index, false);
  } else {
    DCHECK_EQ(std::string(), token);
  }

  callback.Run(auth_error);
}

void ManagedUserService::UpdateManualHosts() {
  const base::DictionaryValue* dict =
      profile_->GetPrefs()->GetDictionary(prefs::kSupervisedUserManualHosts);
  scoped_ptr<std::map<std::string, bool> > host_map(
      new std::map<std::string, bool>());
  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    bool allow = false;
    bool result = it.value().GetAsBoolean(&allow);
    DCHECK(result);
    (*host_map)[it.key()] = allow;
  }
  url_filter_context_.SetManualHosts(host_map.Pass());
}

void ManagedUserService::UpdateManualURLs() {
  const base::DictionaryValue* dict =
      profile_->GetPrefs()->GetDictionary(prefs::kSupervisedUserManualURLs);
  scoped_ptr<std::map<GURL, bool> > url_map(new std::map<GURL, bool>());
  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    bool allow = false;
    bool result = it.value().GetAsBoolean(&allow);
    DCHECK(result);
    (*url_map)[GURL(it.key())] = allow;
  }
  url_filter_context_.SetManualURLs(url_map.Pass());
}

void ManagedUserService::OnBrowserSetLastActive(Browser* browser) {
  bool profile_became_active = profile_->IsSameProfile(browser->profile());
  if (!is_profile_active_ && profile_became_active)
    content::RecordAction(UserMetricsAction("ManagedUsers_OpenProfile"));
  else if (is_profile_active_ && !profile_became_active)
    content::RecordAction(UserMetricsAction("ManagedUsers_SwitchProfile"));

  is_profile_active_ = profile_became_active;
}
