// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/host_window_proxy.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "remoting/host/client_session_control.h"

namespace remoting {

// Runs an instance of |HostWindow| on the |ui_task_runner_| thread.
class HostWindowProxy::Core
    : public base::RefCountedThreadSafe<Core>,
      public ClientSessionControl {
 public:
  Core(scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
       scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
       scoped_ptr<HostWindow> host_window);

  // Starts |host_window_| on the |ui_task_runner_| thread.
  void Start(const base::WeakPtr<ClientSessionControl>& client_session_control);

  // Destroys |host_window_| on the |ui_task_runner_| thread.
  void Stop();

 private:
  friend class base::RefCountedThreadSafe<Core>;
  virtual ~Core();

  // Start() and Stop() equivalents called on the |ui_task_runner_| thread.
  void StartOnUiThread(const std::string& client_jid);
  void StopOnUiThread();

  // ClientSessionControl interface.
  virtual const std::string& client_jid() const OVERRIDE;
  virtual void DisconnectSession() OVERRIDE;
  virtual void OnLocalMouseMoved(const SkIPoint& position) OVERRIDE;
  virtual void SetDisableInputs(bool disable_inputs) OVERRIDE;

  // Task runner on which public methods of this class must be called.
  scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

  // Task runner on which |host_window_| is running.
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  // Stores the client's JID so it can be read on the |ui_task_runner_| thread.
  std::string client_jid_;

  // Used to notify the caller about the local user's actions on
  // the |caller_task_runner| thread.
  base::WeakPtr<ClientSessionControl> client_session_control_;

  // The wrapped |HostWindow| instance running on the |ui_task_runner_| thread.
  scoped_ptr<HostWindow> host_window_;

  // Used to create the control pointer passed to |host_window_|.
  base::WeakPtrFactory<ClientSessionControl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

HostWindowProxy::HostWindowProxy(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_ptr<HostWindow> host_window) {
  DCHECK(caller_task_runner->BelongsToCurrentThread());

  // Detach |host_window| from the calling thread so that |Core| could run it on
  // the |ui_task_runner_| thread.
  host_window->DetachFromThread();
  core_ = new Core(caller_task_runner, ui_task_runner, host_window.Pass());
}

HostWindowProxy::~HostWindowProxy() {
  DCHECK(CalledOnValidThread());

  core_->Stop();
}

void HostWindowProxy::Start(
    const base::WeakPtr<ClientSessionControl>& client_session_control) {
  DCHECK(CalledOnValidThread());

  core_->Start(client_session_control);
}

HostWindowProxy::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_ptr<HostWindow> host_window)
    : caller_task_runner_(caller_task_runner),
      ui_task_runner_(ui_task_runner),
      host_window_(host_window.Pass()),
      weak_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
  DCHECK(caller_task_runner->BelongsToCurrentThread());
}

void HostWindowProxy::Core::Start(
    const base::WeakPtr<ClientSessionControl>& client_session_control) {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
  DCHECK(!client_session_control_);
  DCHECK(client_session_control);

  client_session_control_ = client_session_control;
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Core::StartOnUiThread, this,
                            client_session_control->client_jid()));
}

void HostWindowProxy::Core::Stop() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  ui_task_runner_->PostTask(FROM_HERE, base::Bind(&Core::StopOnUiThread, this));
}

HostWindowProxy::Core::~Core() {
  DCHECK(!host_window_);
}

void HostWindowProxy::Core::StartOnUiThread(const std::string& client_jid) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  DCHECK(client_jid_.empty());

  client_jid_ = client_jid;
  host_window_->Start(weak_factory_.GetWeakPtr());
}

void HostWindowProxy::Core::StopOnUiThread() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  host_window_.reset();
}

const std::string& HostWindowProxy::Core::client_jid() const {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  return client_jid_;
}

void HostWindowProxy::Core::DisconnectSession() {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(FROM_HERE,
                                  base::Bind(&Core::DisconnectSession, this));
    return;
  }

  if (client_session_control_)
    client_session_control_->DisconnectSession();
}

void HostWindowProxy::Core::OnLocalMouseMoved(const SkIPoint& position) {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::OnLocalMouseMoved, this, position));
    return;
  }

  if (client_session_control_)
    client_session_control_->OnLocalMouseMoved(position);
}

void HostWindowProxy::Core::SetDisableInputs(bool disable_inputs) {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::SetDisableInputs, this, disable_inputs));
    return;
  }

  if (client_session_control_)
    client_session_control_->SetDisableInputs(disable_inputs);
}

}  // namespace remoting
