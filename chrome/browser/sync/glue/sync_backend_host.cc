// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include "chrome/browser/sync/glue/sync_backend_host.h"

#include <algorithm>
#include <map>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/location.h"
#include "base/metrics/histogram.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer.h"
#include "base/tracked_objects.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/token_service.h"
#include "chrome/browser/sync/glue/bridged_sync_notifier.h"
#include "chrome/browser/sync/glue/change_processor.h"
#include "chrome/browser/sync/glue/chrome_encryptor.h"
#include "chrome/browser/sync/glue/chrome_sync_notification_bridge.h"
#include "chrome/browser/sync/glue/sync_backend_registrar.h"
#include "chrome/browser/sync/invalidations/invalidator_storage.h"
#include "chrome/browser/sync/sync_prefs.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/content_client.h"
#include "jingle/notifier/base/notification_method.h"
#include "jingle/notifier/base/notifier_options.h"
#include "net/base/host_port_pair.h"
#include "net/url_request/url_request_context_getter.h"
#include "sync/internal_api/public/base_transaction.h"
#include "sync/internal_api/public/engine/model_safe_worker.h"
#include "sync/internal_api/public/internal_components_factory_impl.h"
#include "sync/internal_api/public/http_bridge.h"
#include "sync/internal_api/public/read_transaction.h"
#include "sync/internal_api/public/sync_manager_factory.h"
#include "sync/internal_api/public/util/experiments.h"
#include "sync/notifier/sync_notifier.h"
#include "sync/protocol/encryption.pb.h"
#include "sync/protocol/sync.pb.h"
#include "sync/util/nigori.h"

static const int kSaveChangesIntervalSeconds = 10;
static const FilePath::CharType kSyncDataFolderName[] =
    FILE_PATH_LITERAL("Sync Data");

typedef TokenService::TokenAvailableDetails TokenAvailableDetails;

typedef GoogleServiceAuthError AuthError;

namespace browser_sync {

using content::BrowserThread;
using syncer::InternalComponentsFactory;
using syncer::InternalComponentsFactoryImpl;
using syncer::sessions::SyncSessionSnapshot;
using syncer::SyncCredentials;

// Helper macros to log with the syncer thread name; useful when there
// are multiple syncers involved.

#define SLOG(severity) LOG(severity) << name_ << ": "

#define SDVLOG(verbose_level) DVLOG(verbose_level) << name_ << ": "

class SyncBackendHost::Core
    : public base::RefCountedThreadSafe<SyncBackendHost::Core>,
      public syncer::SyncManager::Observer {
 public:
  Core(const std::string& name,
       const FilePath& sync_data_folder_path,
       const base::WeakPtr<SyncBackendHost>& backend);

  // SyncManager::Observer implementation.  The Core just acts like an air
  // traffic controller here, forwarding incoming messages to appropriate
  // landing threads.
  virtual void OnSyncCycleCompleted(
      const syncer::sessions::SyncSessionSnapshot& snapshot) OVERRIDE;
  virtual void OnInitializationComplete(
      const syncer::WeakHandle<syncer::JsBackend>& js_backend,
      bool success,
      syncer::ModelTypeSet restored_types) OVERRIDE;
  virtual void OnConnectionStatusChange(
      syncer::ConnectionStatus status) OVERRIDE;
  virtual void OnPassphraseRequired(
      syncer::PassphraseRequiredReason reason,
      const sync_pb::EncryptedData& pending_keys) OVERRIDE;
  virtual void OnPassphraseAccepted() OVERRIDE;
  virtual void OnBootstrapTokenUpdated(
      const std::string& bootstrap_token) OVERRIDE;
  virtual void OnStopSyncingPermanently() OVERRIDE;
  virtual void OnUpdatedToken(const std::string& token) OVERRIDE;
  virtual void OnEncryptedTypesChanged(
      syncer::ModelTypeSet encrypted_types,
      bool encrypt_everything) OVERRIDE;
  virtual void OnEncryptionComplete() OVERRIDE;
  virtual void OnActionableError(
      const syncer::SyncProtocolError& sync_error) OVERRIDE;

  // Note:
  //
  // The Do* methods are the various entry points from our
  // SyncBackendHost.  They are all called on the sync thread to
  // actually perform synchronous (and potentially blocking) syncapi
  // operations.
  //
  // Called to perform initialization of the syncapi on behalf of
  // SyncBackendHost::Initialize.
  void DoInitialize(const DoInitializeOptions& options);

  // Called to perform credential update on behalf of
  // SyncBackendHost::UpdateCredentials
  void DoUpdateCredentials(const syncer::SyncCredentials& credentials);

  // Called to tell the syncapi to start syncing (generally after
  // initialization and authentication).
  void DoStartSyncing(const syncer::ModelSafeRoutingInfo& routing_info);

  // Called to set the passphrase for encryption.
  void DoSetEncryptionPassphrase(const std::string& passphrase,
                                 bool is_explicit);

  // Called to decrypt the pending keys.
  void DoSetDecryptionPassphrase(const std::string& passphrase);

  // Called to turn on encryption of all sync data as well as
  // reencrypt everything.
  void DoEnableEncryptEverything();

  // Called to refresh encryption with the most recent passphrase
  // and set of encrypted types. Also adds device information to the nigori
  // node. |done_callback| is called on the sync thread.
  void DoRefreshNigori(const base::Closure& done_callback);

  // The shutdown order is a bit complicated:
  // 1) From |sync_thread_|, invoke the syncapi Shutdown call to do
  //    a final SaveChanges, and close sqlite handles.
  // 2) Then, from |frontend_loop_|, halt the sync_thread_ (which is
  //    a blocking call). This causes syncapi thread-exit handlers
  //    to run and make use of cached pointers to various components
  //    owned implicitly by us.
  // 3) Destroy this Core. That will delete syncapi components in a
  //    safe order because the thread that was using them has exited
  //    (in step 2).
  void DoStopSyncManagerForShutdown(const base::Closure& closure);
  void DoShutdown(bool stopping_sync);

  // Configuration methods that must execute on sync loop.
  void DoConfigureSyncer(
      syncer::ConfigureReason reason,
      syncer::ModelTypeSet types_to_config,
      const syncer::ModelSafeRoutingInfo routing_info,
      const base::Callback<void(syncer::ModelTypeSet)>& ready_task,
      const base::Closure& retry_callback);
  void DoFinishConfigureDataTypes(
      syncer::ModelTypeSet types_to_config,
      const base::Callback<void(syncer::ModelTypeSet)>& ready_task);
  void DoRetryConfiguration(
      const base::Closure& retry_callback);

  // Set the base request context to use when making HTTP calls.
  // This method will add a reference to the context to persist it
  // on the IO thread. Must be removed from IO thread.

  syncer::SyncManager* sync_manager() { return sync_manager_.get(); }

  // Delete the sync data folder to cleanup backend data.  Happens the first
  // time sync is enabled for a user (to prevent accidentally reusing old
  // sync databases), as well as shutdown when you're no longer syncing.
  void DeleteSyncDataFolder();

 private:
  friend class base::RefCountedThreadSafe<SyncBackendHost::Core>;
  friend class SyncBackendHostForProfileSyncTest;

  virtual ~Core();

  // Invoked when initialization of syncapi is complete and we can start
  // our timer.
  // This must be called from the thread on which SaveChanges is intended to
  // be run on; the host's |sync_thread_|.
  void StartSavingChanges();

  // Invoked periodically to tell the syncapi to persist its state
  // by writing to disk.
  // This is called from the thread we were created on (which is the
  // SyncBackendHost |sync_thread_|), using a repeating timer that is kicked
  // off as soon as the SyncManager tells us it completed
  // initialization.
  void SaveChanges();

  // Name used for debugging.
  const std::string name_;

  // Path of the folder that stores the sync data files.
  const FilePath sync_data_folder_path_;

  // Our parent SyncBackendHost.
  syncer::WeakHandle<SyncBackendHost> host_;

  // The loop where all the sync backend operations happen.
  // Non-NULL only between calls to DoInitialize() and DoShutdown().
  MessageLoop* sync_loop_;

  // Our parent's registrar (not owned).  Non-NULL only between
  // calls to DoInitialize() and DoShutdown().
  SyncBackendRegistrar* registrar_;

  // Our parent's notification bridge (not owned).  Non-NULL only
  // between calls to DoInitialize() and DoShutdown().
  ChromeSyncNotificationBridge* chrome_sync_notification_bridge_;

  // The timer used to periodically call SaveChanges.
  scoped_ptr<base::RepeatingTimer<Core> > save_changes_timer_;

  // Our encryptor, which uses Chrome's encryption functions.
  ChromeEncryptor encryptor_;

  // The top-level syncapi entry point.  Lives on the sync thread.
  scoped_ptr<syncer::SyncManager> sync_manager_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

namespace {

// Parses the given command line for notifier options.
notifier::NotifierOptions ParseNotifierOptions(
    const CommandLine& command_line,
    const scoped_refptr<net::URLRequestContextGetter>&
        request_context_getter) {
  notifier::NotifierOptions notifier_options;
  notifier_options.request_context_getter = request_context_getter;

  if (command_line.HasSwitch(switches::kSyncNotificationHostPort)) {
    notifier_options.xmpp_host_port =
        net::HostPortPair::FromString(
            command_line.GetSwitchValueASCII(
                switches::kSyncNotificationHostPort));
    DVLOG(1) << "Using " << notifier_options.xmpp_host_port.ToString()
             << " for test sync notification server.";
  }

  notifier_options.try_ssltcp_first =
      command_line.HasSwitch(switches::kSyncTrySsltcpFirstForXmpp);
  DVLOG_IF(1, notifier_options.try_ssltcp_first)
      << "Trying SSL/TCP port before XMPP port for notifications.";

  notifier_options.invalidate_xmpp_login =
      command_line.HasSwitch(switches::kSyncInvalidateXmppLogin);
  DVLOG_IF(1, notifier_options.invalidate_xmpp_login)
      << "Invalidating sync XMPP login.";

  notifier_options.allow_insecure_connection =
      command_line.HasSwitch(switches::kSyncAllowInsecureXmppConnection);
  DVLOG_IF(1, notifier_options.allow_insecure_connection)
      << "Allowing insecure XMPP connections.";

  if (command_line.HasSwitch(switches::kSyncNotificationMethod)) {
    const std::string notification_method_str(
        command_line.GetSwitchValueASCII(switches::kSyncNotificationMethod));
    notifier_options.notification_method =
        notifier::StringToNotificationMethod(notification_method_str);
  }

  return notifier_options;
}

}  // namespace

SyncBackendHost::SyncBackendHost(
    const std::string& name,
    Profile* profile,
    const base::WeakPtr<SyncPrefs>& sync_prefs,
    const base::WeakPtr<InvalidatorStorage>& invalidator_storage)
    : weak_ptr_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
      sync_thread_("Chrome_SyncThread"),
      frontend_loop_(MessageLoop::current()),
      profile_(profile),
      name_(name),
      core_(new Core(name, profile_->GetPath().Append(kSyncDataFolderName),
                     weak_ptr_factory_.GetWeakPtr())),
      initialization_state_(NOT_ATTEMPTED),
      sync_prefs_(sync_prefs),
      sync_notifier_factory_(
          ParseNotifierOptions(*CommandLine::ForCurrentProcess(),
                               profile_->GetRequestContext()),
          content::GetUserAgent(GURL()),
          invalidator_storage),
      frontend_(NULL) {
}

SyncBackendHost::SyncBackendHost(Profile* profile)
    : weak_ptr_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
      sync_thread_("Chrome_SyncThread"),
      frontend_loop_(MessageLoop::current()),
      profile_(profile),
      name_("Unknown"),
      initialization_state_(NOT_ATTEMPTED),
      sync_notifier_factory_(
          ParseNotifierOptions(*CommandLine::ForCurrentProcess(),
                               profile_->GetRequestContext()),
          content::GetUserAgent(GURL()),
          base::WeakPtr<syncer::InvalidationStateTracker>()),
      frontend_(NULL) {
}

SyncBackendHost::~SyncBackendHost() {
  DCHECK(!core_ && !frontend_) << "Must call Shutdown before destructor.";
  DCHECK(!chrome_sync_notification_bridge_.get());
  DCHECK(!registrar_.get());
}

namespace {

// Helper to construct a user agent string (ASCII) suitable for use by
// the syncapi for any HTTP communication. This string is used by the sync
// backend for classifying client types when calculating statistics.
std::string MakeUserAgentForSyncApi() {
  std::string user_agent;
  user_agent = "Chrome ";
#if defined(OS_WIN)
  user_agent += "WIN ";
#elif defined(OS_CHROMEOS)
  user_agent += "CROS ";
#elif defined(OS_LINUX)
  user_agent += "LINUX ";
#elif defined(OS_FREEBSD)
  user_agent += "FREEBSD ";
#elif defined(OS_OPENBSD)
  user_agent += "OPENBSD ";
#elif defined(OS_MACOSX)
  user_agent += "MAC ";
#endif
  chrome::VersionInfo version_info;
  if (!version_info.is_valid()) {
    DLOG(ERROR) << "Unable to create chrome::VersionInfo object";
    return user_agent;
  }

  user_agent += version_info.Version();
  user_agent += " (" + version_info.LastChange() + ")";
  if (!version_info.IsOfficialBuild())
    user_agent += "-devel";
  return user_agent;
}

scoped_ptr<syncer::HttpPostProviderFactory> MakeHttpBridgeFactory(
    const scoped_refptr<net::URLRequestContextGetter>& getter) {
  return scoped_ptr<syncer::HttpPostProviderFactory>(
      new syncer::HttpBridgeFactory(getter, MakeUserAgentForSyncApi()));
}

}  // namespace

void SyncBackendHost::Initialize(
    SyncFrontend* frontend,
    const syncer::WeakHandle<syncer::JsEventHandler>& event_handler,
    const GURL& sync_service_url,
    const SyncCredentials& credentials,
    bool delete_sync_data_folder,
    syncer::SyncManagerFactory* sync_manager_factory,
    syncer::UnrecoverableErrorHandler* unrecoverable_error_handler,
    syncer::ReportUnrecoverableErrorFunction
        report_unrecoverable_error_function) {
  if (!sync_thread_.Start())
    return;

  chrome_sync_notification_bridge_.reset(
      new ChromeSyncNotificationBridge(
          profile_, sync_thread_.message_loop_proxy()));

  frontend_ = frontend;
  DCHECK(frontend);

  registrar_.reset(new SyncBackendRegistrar(name_,
                                            profile_,
                                            sync_thread_.message_loop()));
  syncer::ModelSafeRoutingInfo routing_info;
  std::vector<syncer::ModelSafeWorker*> workers;
  registrar_->GetModelSafeRoutingInfo(&routing_info);
  registrar_->GetWorkers(&workers);

  initialization_state_ = CREATING_SYNC_MANAGER;
  InitCore(DoInitializeOptions(
      sync_thread_.message_loop(),
      registrar_.get(),
      routing_info,
      workers,
      &extensions_activity_monitor_,
      event_handler,
      sync_service_url,
      base::Bind(&MakeHttpBridgeFactory,
                 make_scoped_refptr(profile_->GetRequestContext())),
      credentials,
      chrome_sync_notification_bridge_.get(),
      &sync_notifier_factory_,
      sync_manager_factory,
      delete_sync_data_folder,
      sync_prefs_->GetEncryptionBootstrapToken(),
      new InternalComponentsFactoryImpl(),
      unrecoverable_error_handler,
      report_unrecoverable_error_function));
}

void SyncBackendHost::UpdateCredentials(const SyncCredentials& credentials) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      base::Bind(&SyncBackendHost::Core::DoUpdateCredentials, core_.get(),
                 credentials));
}

void SyncBackendHost::StartSyncingWithServer() {
  SDVLOG(1) << "SyncBackendHost::StartSyncingWithServer called.";

  syncer::ModelSafeRoutingInfo routing_info;
  registrar_->GetModelSafeRoutingInfo(&routing_info);

  sync_thread_.message_loop()->PostTask(FROM_HERE,
      base::Bind(&SyncBackendHost::Core::DoStartSyncing,
                 core_.get(), routing_info));
}

void SyncBackendHost::SetEncryptionPassphrase(const std::string& passphrase,
                                              bool is_explicit) {
  if (!IsNigoriEnabled()) {
    NOTREACHED() << "SetEncryptionPassphrase must never be called when nigori"
                    " is disabled.";
    return;
  }

  // We should never be called with an empty passphrase.
  DCHECK(!passphrase.empty());

  // This should only be called by the frontend.
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);

  // SetEncryptionPassphrase should never be called if we are currently
  // encrypted with an explicit passphrase.
  DCHECK(!IsUsingExplicitPassphrase());

  // Post an encryption task on the syncer thread.
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      base::Bind(&SyncBackendHost::Core::DoSetEncryptionPassphrase, core_.get(),
                 passphrase, is_explicit));
}

bool SyncBackendHost::SetDecryptionPassphrase(const std::string& passphrase) {
  if (!IsNigoriEnabled()) {
    NOTREACHED() << "SetDecryptionPassphrase must never be called when nigori"
                    " is disabled.";
    return false;
  }

  // We should never be called with an empty passphrase.
  DCHECK(!passphrase.empty());

  // This should only be called by the frontend.
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);

  // This should only be called when we have cached pending keys.
  DCHECK(cached_pending_keys_.has_blob());

  // Check the passphrase that was provided against our local cache of the
  // cryptographer's pending keys. If this was unsuccessful, the UI layer can
  // immediately call OnPassphraseRequired without showing the user a spinner.
  if (!CheckPassphraseAgainstCachedPendingKeys(passphrase))
    return false;

  // Post a decryption task on the syncer thread.
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      base::Bind(&SyncBackendHost::Core::DoSetDecryptionPassphrase, core_.get(),
                 passphrase));

  // Since we were able to decrypt the cached pending keys with the passphrase
  // provided, we immediately alert the UI layer that the passphrase was
  // accepted. This will avoid the situation where a user enters a passphrase,
  // clicks OK, immediately reopens the advanced settings dialog, and gets an
  // unnecessary prompt for a passphrase.
  // Note: It is not guaranteed that the passphrase will be accepted by the
  // syncer thread, since we could receive a new nigori node while the task is
  // pending. This scenario is a valid race, and SetDecryptionPassphrase can
  // trigger a new OnPassphraseRequired if it needs to.
  NotifyPassphraseAccepted();
  return true;
}

void SyncBackendHost::StopSyncManagerForShutdown(
    const base::Closure& closure) {
  DCHECK_GT(initialization_state_, NOT_ATTEMPTED);
  if (initialization_state_ == CREATING_SYNC_MANAGER) {
    // We post here to implicitly wait for the SyncManager to be created,
    // if needed.  We have to wait, since we need to shutdown immediately,
    // and we need to tell the SyncManager so it can abort any activity
    // (net I/O, data application).
    DCHECK(sync_thread_.IsRunning());
    sync_thread_.message_loop()->PostTask(FROM_HERE,
        base::Bind(
            &SyncBackendHost::Core::DoStopSyncManagerForShutdown,
            core_.get(),
            closure));
  } else {
    core_->DoStopSyncManagerForShutdown(closure);
  }
}

void SyncBackendHost::StopSyncingForShutdown() {
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  // Thread shutdown should occur in the following order:
  // - Sync Thread
  // - UI Thread (stops some time after we return from this call).
  //
  // In order to acheive this, we first shutdown components from the UI thread
  // and send signals to abort components that may be busy on the sync thread.
  // The callback (OnSyncerShutdownComplete) will happen on the sync thread,
  // after which we'll shutdown components on the sync thread, and then be
  // able to stop the sync loop.
  if (sync_thread_.IsRunning()) {
    StopSyncManagerForShutdown(
        base::Bind(&SyncBackendRegistrar::OnSyncerShutdownComplete,
                   base::Unretained(registrar_.get())));

    // Before joining the sync_thread_, we wait for the UIModelWorker to
    // give us the green light that it is not depending on the frontend_loop_
    // to process any more tasks. Stop() blocks until this termination
    // condition is true.
    base::Time stop_registrar_start_time = base::Time::Now();
    if (registrar_.get())
      registrar_->StopOnUIThread();
    base::TimeDelta stop_registrar_time = base::Time::Now() -
        stop_registrar_start_time;
    UMA_HISTOGRAM_TIMES("Sync.Shutdown.StopRegistrarTime",
                        stop_registrar_time);
  } else {
    // If the sync thread isn't running, then the syncer is effectively
    // stopped.  Moreover, it implies that we never attempted initialization,
    // so the registrar won't need stopping either.
    DCHECK_EQ(NOT_ATTEMPTED, initialization_state_);
    DCHECK(!registrar_.get());
  }
}

void SyncBackendHost::Shutdown(bool sync_disabled) {
  // TODO(tim): DCHECK(registrar_->StoppedOnUIThread()) would be nice.
  if (sync_thread_.IsRunning()) {
    sync_thread_.message_loop()->PostTask(FROM_HERE,
        base::Bind(&SyncBackendHost::Core::DoShutdown, core_.get(),
                   sync_disabled));
  }

  // Stop will return once the thread exits, which will be after DoShutdown
  // runs. DoShutdown needs to run from sync_thread_ because the sync backend
  // requires any thread that opened sqlite handles to relinquish them
  // personally. We need to join threads, because otherwise the main Chrome
  // thread (ui loop) can exit before DoShutdown finishes, at which point
  // virtually anything the sync backend does (or the post-back to
  // frontend_loop_ by our Core) will epically fail because the CRT won't be
  // initialized.
  // Since we are blocking the UI thread here, we need to turn ourselves in
  // with the ThreadRestriction police.  For sentencing and how we plan to fix
  // this, see bug 19757.
  base::Time stop_thread_start_time = base::Time::Now();
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    sync_thread_.Stop();
  }
  base::TimeDelta stop_sync_thread_time = base::Time::Now() -
      stop_thread_start_time;
  UMA_HISTOGRAM_TIMES("Sync.Shutdown.StopSyncThreadTime",
                      stop_sync_thread_time);

  registrar_.reset();
  frontend_ = NULL;
  chrome_sync_notification_bridge_.reset();
  core_ = NULL;  // Releases reference to core_.
}

void SyncBackendHost::ConfigureDataTypes(
    syncer::ConfigureReason reason,
    syncer::ModelTypeSet types_to_add,
    syncer::ModelTypeSet types_to_remove,
    NigoriState nigori_state,
    const base::Callback<void(syncer::ModelTypeSet)>& ready_task,
    const base::Callback<void()>& retry_callback) {
  // Only one configure is allowed at a time.  This is guaranteed by our
  // callers.  The SyncBackendHost requests one configure as the backend is
  // initializing and waits for it to complete.  After initialization, all
  // configurations will pass through the DataTypeManager, which is careful to
  // never send a new configure request until the current request succeeds.

  DCHECK_GT(initialization_state_, NOT_INITIALIZED);

  syncer::ModelTypeSet types_to_add_with_nigori = types_to_add;
  syncer::ModelTypeSet types_to_remove_with_nigori = types_to_remove;
  if (nigori_state == WITH_NIGORI) {
    types_to_add_with_nigori.Put(syncer::NIGORI);
    types_to_remove_with_nigori.Remove(syncer::NIGORI);
  } else {
    types_to_add_with_nigori.Remove(syncer::NIGORI);
    types_to_remove_with_nigori.Put(syncer::NIGORI);
  }

  // The SyncBackendRegistrar's routing info will be updated by adding the
  // types_to_add_with_nigori to the list then removing
  // types_to_remove_with_nigori.  Any types which are not in either of those
  // sets will remain untouched.
  //
  // Types which were not in the list previously are not fully downloaded, so we
  // must ask the syncer to download them.  Any newly supported datatypes will
  // not have been in that routing info list, so they will be among the types
  // downloaded if they are enabled.
  //
  // The SyncBackendRegistrar's state was initially derived from the types
  // marked initial_sync_ended when the sync database was loaded.  Afterwards it
  // is modified only by this function.  We expect it to remain in sync with the
  // backend because configuration requests are never aborted; they are retried
  // until they succeed or the browser is closed.

  syncer::ModelTypeSet types_to_download = registrar_->ConfigureDataTypes(
      types_to_add_with_nigori, types_to_remove_with_nigori);
  if (!types_to_download.Empty())
    types_to_download.Put(syncer::NIGORI);

  // TODO(sync): crbug.com/137550.
  // It's dangerous to configure types that have progress markers.  Types with
  // progress markers can trigger a MIGRATION_DONE response.  We are not
  // prepared to handle a migration during a configure, so we must ensure that
  // all our types_to_download actually contain no data before we sync them.
  //
  // The most common way to end up in this situation used to be types which had
  // !initial_sync_ended, but did have some progress markers.  We avoid problems
  // with those types by purging the data of any such partially synced types
  // soon after we load the directory.
  //
  // Another possible scenario is that we have newly supported or newly enabled
  // data types being downloaded here but the nigori type, which is always
  // included in any GetUpdates request, requires migration.  The server has
  // code to detect this scenario based on the configure reason, the fact that
  // the nigori type is the only requested type which requires migration, and
  // that the requested types list includes at least one non-nigori type.  It
  // will not send a MIGRATION_DONE response in that case.  We still need to be
  // careful to not send progress markers for non-nigori types, though.  If a
  // non-nigori type in the request requires migration, a MIGRATION_DONE
  // response will be sent.

  syncer::ModelSafeRoutingInfo routing_info;
  registrar_->GetModelSafeRoutingInfo(&routing_info);

  SDVLOG(1) << "Types "
            << syncer::ModelTypeSetToString(types_to_download)
            << " added; calling DoConfigureSyncer";
  // TODO(zea): figure out how to bypass this call if no types are being
  // configured and GetKey is not needed. For now we rely on determining the
  // need for GetKey as part of the SyncManager::ConfigureSyncer logic.
  RequestConfigureSyncer(reason,
                         types_to_download,
                         routing_info,
                         ready_task,
                         retry_callback);
}

void SyncBackendHost::EnableEncryptEverything() {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
     base::Bind(&SyncBackendHost::Core::DoEnableEncryptEverything,
                core_.get()));
}

void SyncBackendHost::ActivateDataType(
    syncer::ModelType type, syncer::ModelSafeGroup group,
    ChangeProcessor* change_processor) {
  registrar_->ActivateDataType(type, group, change_processor, GetUserShare());
}

void SyncBackendHost::DeactivateDataType(syncer::ModelType type) {
  registrar_->DeactivateDataType(type);
}

syncer::UserShare* SyncBackendHost::GetUserShare() const {
  return core_->sync_manager()->GetUserShare();
}

SyncBackendHost::Status SyncBackendHost::GetDetailedStatus() {
  DCHECK(initialized());
  return core_->sync_manager()->GetDetailedStatus();
}

SyncSessionSnapshot SyncBackendHost::GetLastSessionSnapshot() const {
  return last_snapshot_;
}

bool SyncBackendHost::HasUnsyncedItems() const {
  DCHECK(initialized());
  return core_->sync_manager()->HasUnsyncedItems();
}

bool SyncBackendHost::IsNigoriEnabled() const {
  return registrar_.get() && registrar_->IsNigoriEnabled();
}

bool SyncBackendHost::IsUsingExplicitPassphrase() {
  // This should only be called once the nigori node has been downloaded, as
  // otherwise we have no idea what kind of passphrase we are using. This will
  // NOTREACH in sync_manager and return false if we fail to load the nigori
  // node.
  return IsNigoriEnabled() &&
      core_->sync_manager()->IsUsingExplicitPassphrase();
}

bool SyncBackendHost::IsCryptographerReady(
    const syncer::BaseTransaction* trans) const {
  return initialized() && trans->GetCryptographer()->is_ready();
}

void SyncBackendHost::GetModelSafeRoutingInfo(
    syncer::ModelSafeRoutingInfo* out) const {
  if (initialized()) {
    CHECK(registrar_.get());
    registrar_->GetModelSafeRoutingInfo(out);
  } else {
    NOTREACHED();
  }
}

void SyncBackendHost::InitCore(const DoInitializeOptions& options) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      base::Bind(&SyncBackendHost::Core::DoInitialize, core_.get(), options));
}

void SyncBackendHost::RequestConfigureSyncer(
    syncer::ConfigureReason reason,
    syncer::ModelTypeSet types_to_config,
    const syncer::ModelSafeRoutingInfo& routing_info,
    const base::Callback<void(syncer::ModelTypeSet)>& ready_task,
    const base::Closure& retry_callback) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
       base::Bind(&SyncBackendHost::Core::DoConfigureSyncer,
                  core_.get(),
                  reason,
                  types_to_config,
                  routing_info,
                  ready_task,
                  retry_callback));
}

void SyncBackendHost::FinishConfigureDataTypesOnFrontendLoop(
    syncer::ModelTypeSet failed_configuration_types,
    const base::Callback<void(syncer::ModelTypeSet)>& ready_task) {
  if (!frontend_)
    return;
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  if (!ready_task.is_null())
    ready_task.Run(failed_configuration_types);
}

void SyncBackendHost::HandleSyncManagerInitializationOnFrontendLoop(
    const syncer::WeakHandle<syncer::JsBackend>& js_backend, bool success,
    syncer::ModelTypeSet restored_types) {
  registrar_->SetInitialTypes(restored_types);
  HandleInitializationCompletedOnFrontendLoop(js_backend, success);
}

SyncBackendHost::DoInitializeOptions::DoInitializeOptions(
    MessageLoop* sync_loop,
    SyncBackendRegistrar* registrar,
    const syncer::ModelSafeRoutingInfo& routing_info,
    const std::vector<syncer::ModelSafeWorker*>& workers,
    syncer::ExtensionsActivityMonitor* extensions_activity_monitor,
    const syncer::WeakHandle<syncer::JsEventHandler>& event_handler,
    const GURL& service_url,
    MakeHttpBridgeFactoryFn make_http_bridge_factory_fn,
    const syncer::SyncCredentials& credentials,
    ChromeSyncNotificationBridge* chrome_sync_notification_bridge,
    syncer::SyncNotifierFactory* sync_notifier_factory,
    syncer::SyncManagerFactory* sync_manager_factory,
    bool delete_sync_data_folder,
    const std::string& restored_key_for_bootstrapping,
    InternalComponentsFactory* internal_components_factory,
    syncer::UnrecoverableErrorHandler* unrecoverable_error_handler,
    syncer::ReportUnrecoverableErrorFunction
        report_unrecoverable_error_function)
    : sync_loop(sync_loop),
      registrar(registrar),
      routing_info(routing_info),
      workers(workers),
      extensions_activity_monitor(extensions_activity_monitor),
      event_handler(event_handler),
      service_url(service_url),
      make_http_bridge_factory_fn(make_http_bridge_factory_fn),
      credentials(credentials),
      chrome_sync_notification_bridge(chrome_sync_notification_bridge),
      sync_notifier_factory(sync_notifier_factory),
      sync_manager_factory(sync_manager_factory),
      delete_sync_data_folder(delete_sync_data_folder),
      restored_key_for_bootstrapping(restored_key_for_bootstrapping),
      internal_components_factory(internal_components_factory),
      unrecoverable_error_handler(unrecoverable_error_handler),
      report_unrecoverable_error_function(
          report_unrecoverable_error_function) {
}

SyncBackendHost::DoInitializeOptions::~DoInitializeOptions() {}

SyncBackendHost::Core::Core(const std::string& name,
                            const FilePath& sync_data_folder_path,
                            const base::WeakPtr<SyncBackendHost>& backend)
    : name_(name),
      sync_data_folder_path_(sync_data_folder_path),
      host_(backend),
      sync_loop_(NULL),
      registrar_(NULL),
      chrome_sync_notification_bridge_(NULL) {
  DCHECK(backend.get());
}

SyncBackendHost::Core::~Core() {
  DCHECK(!sync_manager_.get());
  DCHECK(!sync_loop_);
}

void SyncBackendHost::Core::OnSyncCycleCompleted(
    const SyncSessionSnapshot& snapshot) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::HandleSyncCycleCompletedOnFrontendLoop,
      snapshot);
}


void SyncBackendHost::Core::OnInitializationComplete(
    const syncer::WeakHandle<syncer::JsBackend>& js_backend,
    bool success,
    const syncer::ModelTypeSet restored_types) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::HandleSyncManagerInitializationOnFrontendLoop,
      js_backend, success, restored_types);

  if (success) {
    // Initialization is complete, so we can schedule recurring SaveChanges.
    sync_loop_->PostTask(FROM_HERE,
                         base::Bind(&Core::StartSavingChanges, this));
  }
}

void SyncBackendHost::Core::OnConnectionStatusChange(
    syncer::ConnectionStatus status) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::HandleConnectionStatusChangeOnFrontendLoop, status);
}

void SyncBackendHost::Core::OnPassphraseRequired(
    syncer::PassphraseRequiredReason reason,
    const sync_pb::EncryptedData& pending_keys) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::NotifyPassphraseRequired, reason, pending_keys);
}

void SyncBackendHost::Core::OnPassphraseAccepted() {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::NotifyPassphraseAccepted);
}

void SyncBackendHost::Core::OnBootstrapTokenUpdated(
    const std::string& bootstrap_token) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::PersistEncryptionBootstrapToken, bootstrap_token);
}

void SyncBackendHost::Core::OnStopSyncingPermanently() {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::HandleStopSyncingPermanentlyOnFrontendLoop);
}

void SyncBackendHost::Core::OnUpdatedToken(const std::string& token) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::NotifyUpdatedToken, token);
}

void SyncBackendHost::Core::OnEncryptedTypesChanged(
    syncer::ModelTypeSet encrypted_types,
    bool encrypt_everything) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  // NOTE: We're in a transaction.
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::NotifyEncryptedTypesChanged,
      encrypted_types, encrypt_everything);
}

void SyncBackendHost::Core::OnEncryptionComplete() {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  // NOTE: We're in a transaction.
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::NotifyEncryptionComplete);
}

void SyncBackendHost::Core::OnActionableError(
    const syncer::SyncProtocolError& sync_error) {
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(
      FROM_HERE,
      &SyncBackendHost::HandleActionableErrorEventOnFrontendLoop,
      sync_error);
}

void SyncBackendHost::Core::DoInitialize(const DoInitializeOptions& options) {
  DCHECK(!sync_loop_);
  sync_loop_ = options.sync_loop;
  DCHECK(sync_loop_);

  // Blow away the partial or corrupt sync data folder before doing any more
  // initialization, if necessary.
  if (options.delete_sync_data_folder) {
    DeleteSyncDataFolder();
  }

  // Make sure that the directory exists before initializing the backend.
  // If it already exists, this will do no harm.
  bool success = file_util::CreateDirectory(sync_data_folder_path_);
  DCHECK(success);

  DCHECK(!registrar_);
  registrar_ = options.registrar;
  DCHECK(registrar_);

  DCHECK(!chrome_sync_notification_bridge_);
  chrome_sync_notification_bridge_ = options.chrome_sync_notification_bridge;
  DCHECK(chrome_sync_notification_bridge_);

  sync_manager_ = options.sync_manager_factory->CreateSyncManager(name_);
  sync_manager_->AddObserver(this);
  success = sync_manager_->Init(
      sync_data_folder_path_,
      options.event_handler,
      options.service_url.host() + options.service_url.path(),
      options.service_url.EffectiveIntPort(),
      options.service_url.SchemeIsSecure(),
      BrowserThread::GetBlockingPool(),
      options.make_http_bridge_factory_fn.Run().Pass(),
      options.workers,
      options.extensions_activity_monitor,
      options.registrar /* as SyncManager::ChangeDelegate */,
      options.credentials,
      scoped_ptr<syncer::SyncNotifier>(new BridgedSyncNotifier(
          options.chrome_sync_notification_bridge,
          options.sync_notifier_factory->CreateSyncNotifier())),
      options.restored_key_for_bootstrapping,
      scoped_ptr<InternalComponentsFactory>(
          options.internal_components_factory),
      &encryptor_,
      options.unrecoverable_error_handler,
      options.report_unrecoverable_error_function);
  LOG_IF(ERROR, !success) << "Syncapi initialization failed!";

  // Now check the command line to see if we need to simulate an
  // unrecoverable error for testing purpose. Note the error is thrown
  // only if the initialization succeeded. Also it makes sense to use this
  // flag only when restarting the browser with an account already setup. If
  // you use this before setting up the setup would not succeed as an error
  // would be encountered.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSyncThrowUnrecoverableError)) {
    sync_manager_->ThrowUnrecoverableError();
  }
}

void SyncBackendHost::Core::DoUpdateCredentials(
    const SyncCredentials& credentials) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->UpdateCredentials(credentials);
}

void SyncBackendHost::Core::DoStartSyncing(
    const syncer::ModelSafeRoutingInfo& routing_info) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->StartSyncingNormally(routing_info);
}

void SyncBackendHost::Core::DoSetEncryptionPassphrase(
    const std::string& passphrase,
    bool is_explicit) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->SetEncryptionPassphrase(passphrase, is_explicit);
}

void SyncBackendHost::Core::DoSetDecryptionPassphrase(
    const std::string& passphrase) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->SetDecryptionPassphrase(passphrase);
}

void SyncBackendHost::Core::DoEnableEncryptEverything() {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->EnableEncryptEverything();
}

void SyncBackendHost::Core::DoRefreshNigori(
    const base::Closure& done_callback) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  chrome::VersionInfo version_info;
  sync_manager_->RefreshNigori(version_info.CreateVersionString(),
                               done_callback);
}

void SyncBackendHost::Core::DoStopSyncManagerForShutdown(
    const base::Closure& closure) {
  DCHECK(sync_manager_.get());
  sync_manager_->StopSyncingForShutdown(closure);
}

void SyncBackendHost::Core::DoShutdown(bool sync_disabled) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  if (!sync_manager_.get())
    return;

  save_changes_timer_.reset();
  sync_manager_->ShutdownOnSyncThread();
  sync_manager_->RemoveObserver(this);
  sync_manager_.reset();
  chrome_sync_notification_bridge_ = NULL;
  registrar_ = NULL;

  if (sync_disabled)
    DeleteSyncDataFolder();

  sync_loop_ = NULL;

  host_.Reset();
}

void SyncBackendHost::Core::DoConfigureSyncer(
    syncer::ConfigureReason reason,
    syncer::ModelTypeSet types_to_config,
    const syncer::ModelSafeRoutingInfo routing_info,
    const base::Callback<void(syncer::ModelTypeSet)>& ready_task,
    const base::Closure& retry_callback) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->ConfigureSyncer(
      reason,
      types_to_config,
      routing_info,
      base::Bind(&SyncBackendHost::Core::DoFinishConfigureDataTypes,
                 this,
                 types_to_config,
                 ready_task),
      base::Bind(&SyncBackendHost::Core::DoRetryConfiguration,
                 this,
                 retry_callback));
}

void SyncBackendHost::Core::DoFinishConfigureDataTypes(
    syncer::ModelTypeSet types_to_config,
    const base::Callback<void(syncer::ModelTypeSet)>& ready_task) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);

  // Update the enabled types for the bridge and sync manager.
  syncer::ModelSafeRoutingInfo routing_info;
  registrar_->GetModelSafeRoutingInfo(&routing_info);
  const syncer::ModelTypeSet enabled_types = GetRoutingInfoTypes(routing_info);
  chrome_sync_notification_bridge_->UpdateEnabledTypes(enabled_types);
  sync_manager_->UpdateEnabledTypes(enabled_types);

  const syncer::ModelTypeSet failed_configuration_types =
      Difference(types_to_config, sync_manager_->InitialSyncEndedTypes());
  host_.Call(FROM_HERE,
             &SyncBackendHost::FinishConfigureDataTypesOnFrontendLoop,
             failed_configuration_types,
             ready_task);
}

void SyncBackendHost::Core::DoRetryConfiguration(
    const base::Closure& retry_callback) {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  host_.Call(FROM_HERE,
             &SyncBackendHost::RetryConfigurationOnFrontendLoop,
             retry_callback);
}

void SyncBackendHost::Core::DeleteSyncDataFolder() {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  if (file_util::DirectoryExists(sync_data_folder_path_)) {
    if (!file_util::Delete(sync_data_folder_path_, true))
      SLOG(DFATAL) << "Could not delete the Sync Data folder.";
  }
}

void SyncBackendHost::Core::StartSavingChanges() {
  // We may already be shut down.
  if (!sync_loop_)
    return;
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  DCHECK(!save_changes_timer_.get());
  save_changes_timer_.reset(new base::RepeatingTimer<Core>());
  save_changes_timer_->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(kSaveChangesIntervalSeconds),
      this, &Core::SaveChanges);
}

void SyncBackendHost::Core::SaveChanges() {
  DCHECK_EQ(MessageLoop::current(), sync_loop_);
  sync_manager_->SaveChanges();
}

void SyncBackendHost::AddExperimentalTypes() {
  CHECK(initialized());
  syncer::Experiments experiments;
  if (core_->sync_manager()->ReceivedExperiment(&experiments))
    frontend_->OnExperimentsChanged(experiments);
}

void SyncBackendHost::OnNigoriDownloadRetry() {
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  if (!frontend_)
    return;

  frontend_->OnSyncConfigureRetry();
}

void SyncBackendHost::HandleInitializationCompletedOnFrontendLoop(
    const syncer::WeakHandle<syncer::JsBackend>& js_backend, bool success) {
  DCHECK_NE(NOT_ATTEMPTED, initialization_state_);
  if (!frontend_)
    return;

  // We've at least created the sync manager at this point, but if that is all
  // we've done we're just beginning the initialization process.
  if (initialization_state_ == CREATING_SYNC_MANAGER)
    initialization_state_ = NOT_INITIALIZED;

  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  if (!success) {
    initialization_state_ = NOT_INITIALIZED;
    frontend_->OnBackendInitialized(
        syncer::WeakHandle<syncer::JsBackend>(), false);
    return;
  }

  // Run initialization state machine.
  switch (initialization_state_) {
    case NOT_INITIALIZED:
      // This configuration should result in a download request if the nigori
      // type's initial_sync_ended bit is unset.  If the download request
      // contains progress markers, there is a risk that the server will try to
      // trigger migration.  That would be disastrous, so we must rely on the
      // sync manager to ensure that this type never has both progress markers
      // and !initial_sync_ended.
      initialization_state_ = DOWNLOADING_NIGORI;
      ConfigureDataTypes(
          syncer::CONFIGURE_REASON_NEW_CLIENT,
          syncer::ModelTypeSet(),
          syncer::ModelTypeSet(),
          WITH_NIGORI,
          // Calls back into this function.
          base::Bind(
              &SyncBackendHost::
                  HandleNigoriConfigurationCompletedOnFrontendLoop,
              weak_ptr_factory_.GetWeakPtr(), js_backend),
          base::Bind(&SyncBackendHost::OnNigoriDownloadRetry,
                     weak_ptr_factory_.GetWeakPtr()));
      break;
    case DOWNLOADING_NIGORI:
      initialization_state_ = REFRESHING_NIGORI;
      // Triggers OnEncryptedTypesChanged() and OnEncryptionComplete()
      // if necessary.
      RefreshNigori(
          base::Bind(
              &SyncBackendHost::
                  HandleInitializationCompletedOnFrontendLoop,
              weak_ptr_factory_.GetWeakPtr(), js_backend, true));
      break;
    case REFRESHING_NIGORI:
      initialization_state_ = INITIALIZED;
      // Now that we've downloaded the nigori node, we can see if there are any
      // experimental types to enable. This should be done before we inform
      // the frontend to ensure they're visible in the customize screen.
      AddExperimentalTypes();
      frontend_->OnBackendInitialized(js_backend, true);
      break;
    default:
      NOTREACHED();
  }
}

void SyncBackendHost::HandleSyncCycleCompletedOnFrontendLoop(
    const SyncSessionSnapshot& snapshot) {
  if (!frontend_)
    return;
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);

  last_snapshot_ = snapshot;

  SDVLOG(1) << "Got snapshot " << snapshot.ToString();

  const syncer::ModelTypeSet to_migrate =
      snapshot.model_neutral_state().types_needing_local_migration;
  if (!to_migrate.Empty())
    frontend_->OnMigrationNeededForTypes(to_migrate);

  // Process any changes to the datatypes we're syncing.
  // TODO(sync): add support for removing types.
  if (initialized())
    AddExperimentalTypes();

  if (initialized())
    frontend_->OnSyncCycleCompleted();
}

void SyncBackendHost::RetryConfigurationOnFrontendLoop(
    const base::Closure& retry_callback) {
  SDVLOG(1) << "Failed to complete configuration, informing of retry.";
  retry_callback.Run();
}

void SyncBackendHost::PersistEncryptionBootstrapToken(
    const std::string& token) {
  CHECK(sync_prefs_.get());
  sync_prefs_->SetEncryptionBootstrapToken(token);
}

void SyncBackendHost::HandleActionableErrorEventOnFrontendLoop(
    const syncer::SyncProtocolError& sync_error) {
  if (!frontend_)
    return;
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  frontend_->OnActionableError(sync_error);
}

bool SyncBackendHost::CheckPassphraseAgainstCachedPendingKeys(
    const std::string& passphrase) const {
  DCHECK(cached_pending_keys_.has_blob());
  DCHECK(!passphrase.empty());
  syncer::Nigori nigori;
  nigori.InitByDerivation("localhost", "dummy", passphrase);
  std::string plaintext;
  bool result = nigori.Decrypt(cached_pending_keys_.blob(), &plaintext);
  return result;
}

void SyncBackendHost::NotifyPassphraseRequired(
    syncer::PassphraseRequiredReason reason,
    sync_pb::EncryptedData pending_keys) {
  if (!frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), frontend_loop_);

  // Update our cache of the cryptographer's pending keys.
  cached_pending_keys_ = pending_keys;

  frontend_->OnPassphraseRequired(reason, pending_keys);
}

void SyncBackendHost::NotifyPassphraseAccepted() {
  if (!frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), frontend_loop_);

  // Clear our cache of the cryptographer's pending keys.
  cached_pending_keys_.clear_blob();
  frontend_->OnPassphraseAccepted();
}

void SyncBackendHost::NotifyUpdatedToken(const std::string& token) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TokenAvailableDetails details(GaiaConstants::kSyncService, token);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_TOKEN_UPDATED,
      content::Source<Profile>(profile_),
      content::Details<const TokenAvailableDetails>(&details));
}

void SyncBackendHost::NotifyEncryptedTypesChanged(
    syncer::ModelTypeSet encrypted_types,
    bool encrypt_everything) {
  if (!frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  frontend_->OnEncryptedTypesChanged(
      encrypted_types, encrypt_everything);
}

void SyncBackendHost::NotifyEncryptionComplete() {
  if (!frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  frontend_->OnEncryptionComplete();
}

void SyncBackendHost::HandleStopSyncingPermanentlyOnFrontendLoop() {
  if (!frontend_)
    return;
  frontend_->OnStopSyncingPermanently();
}

void SyncBackendHost::HandleConnectionStatusChangeOnFrontendLoop(
    syncer::ConnectionStatus status) {
  if (!frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), frontend_loop_);

  frontend_->OnConnectionStatusChange(status);
}

void SyncBackendHost::HandleNigoriConfigurationCompletedOnFrontendLoop(
    const syncer::WeakHandle<syncer::JsBackend>& js_backend,
    const syncer::ModelTypeSet failed_configuration_types) {
  HandleInitializationCompletedOnFrontendLoop(
      js_backend, failed_configuration_types.Empty());
}

namespace {

// Needed because MessageLoop::PostTask is overloaded.
void PostClosure(MessageLoop* message_loop,
                 const tracked_objects::Location& from_here,
                 const base::Closure& callback) {
  message_loop->PostTask(from_here, callback);
}

}  // namespace

void SyncBackendHost::RefreshNigori(const base::Closure& done_callback) {
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  base::Closure sync_thread_done_callback =
      base::Bind(&PostClosure,
                 MessageLoop::current(), FROM_HERE, done_callback);
  sync_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&SyncBackendHost::Core::DoRefreshNigori,
                 core_.get(), sync_thread_done_callback));
}

#undef SDVLOG

#undef SLOG

}  // namespace browser_sync
