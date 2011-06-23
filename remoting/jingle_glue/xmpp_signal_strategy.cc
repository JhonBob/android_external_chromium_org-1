// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/jingle_glue/xmpp_signal_strategy.h"

#include "base/logging.h"
#include "jingle/notifier/base/gaia_token_pre_xmpp_auth.h"
#include "remoting/jingle_glue/jingle_thread.h"
#include "remoting/jingle_glue/xmpp_iq_request.h"
#include "remoting/jingle_glue/xmpp_socket_adapter.h"
#include "third_party/libjingle/source/talk/base/asyncsocket.h"
#include "third_party/libjingle/source/talk/p2p/client/sessionmanagertask.h"
#include "third_party/libjingle/source/talk/p2p/base/sessionmanager.h"
#include "third_party/libjingle/source/talk/xmpp/prexmppauth.h"
#include "third_party/libjingle/source/talk/xmpp/saslcookiemechanism.h"

namespace remoting {

XmppSignalStrategy::XmppSignalStrategy(JingleThread* jingle_thread,
                                       const std::string& username,
                                       const std::string& auth_token,
                                       const std::string& auth_token_service)
   : thread_(jingle_thread),
     listener_(NULL),
     username_(username),
     auth_token_(auth_token),
     auth_token_service_(auth_token_service),
     xmpp_client_(NULL),
     observer_(NULL) {
}

XmppSignalStrategy::~XmppSignalStrategy() {
  if (xmpp_client_)
    xmpp_client_->engine()->RemoveStanzaHandler(this);

  DCHECK(listener_ == NULL);
}

void XmppSignalStrategy::Init(StatusObserver* observer) {
  observer_ = observer;

  buzz::Jid login_jid(username_);

  buzz::XmppClientSettings settings;
  settings.set_user(login_jid.node());
  settings.set_host(login_jid.domain());
  settings.set_resource("chromoting");
  settings.set_use_tls(true);
  settings.set_token_service(auth_token_service_);
  settings.set_auth_cookie(auth_token_);
  settings.set_server(talk_base::SocketAddress("talk.google.com", 5222));

  buzz::AsyncSocket* socket = new XmppSocketAdapter(settings, false);

  xmpp_client_ = new buzz::XmppClient(thread_->task_pump());
  xmpp_client_->Connect(settings, "", socket, CreatePreXmppAuth(settings));
  xmpp_client_->SignalStateChange.connect(
      this, &XmppSignalStrategy::OnConnectionStateChanged);
  xmpp_client_->engine()->AddStanzaHandler(this, buzz::XmppEngine::HL_PEEK);
  xmpp_client_->Start();
}

void XmppSignalStrategy::SetListener(Listener* listener) {
  // Don't overwrite an listener without explicitly going
  // through "NULL" first.
  DCHECK(listener_ == NULL || listener == NULL);
  listener_ = listener;
}

void XmppSignalStrategy::SendStanza(buzz::XmlElement* stanza) {
  xmpp_client_->SendStanza(stanza);
}

void XmppSignalStrategy::StartSession(
    cricket::SessionManager* session_manager) {
  cricket::SessionManagerTask* receiver =
      new cricket::SessionManagerTask(xmpp_client_, session_manager);
  receiver->EnableOutgoingMessages();
  receiver->Start();
}

void XmppSignalStrategy::EndSession() {
  if (xmpp_client_) {
    xmpp_client_->engine()->RemoveStanzaHandler(this);
    xmpp_client_->Disconnect();
    // Client is deleted by TaskRunner.
    xmpp_client_ = NULL;
  }
}

IqRequest* XmppSignalStrategy::CreateIqRequest() {
  return new XmppIqRequest(thread_->message_loop(), xmpp_client_);
}

bool XmppSignalStrategy::HandleStanza(const buzz::XmlElement* stanza) {
  listener_->OnIncomingStanza(stanza);
  return false;
}

void XmppSignalStrategy::OnConnectionStateChanged(
    buzz::XmppEngine::State state) {
  switch (state) {
    case buzz::XmppEngine::STATE_START:
      observer_->OnStateChange(StatusObserver::START);
      break;
    case buzz::XmppEngine::STATE_OPENING:
      observer_->OnStateChange(StatusObserver::CONNECTING);
      break;
    case buzz::XmppEngine::STATE_OPEN:
      observer_->OnJidChange(xmpp_client_->jid().Str());
      observer_->OnStateChange(StatusObserver::CONNECTED);
      break;
    case buzz::XmppEngine::STATE_CLOSED:
      observer_->OnStateChange(StatusObserver::CLOSED);
      // Client is destroyed by the TaskRunner after the client is
      // closed. Reset the pointer so we don't try to use it later.
      xmpp_client_ = NULL;
      break;
    default:
      NOTREACHED();
      break;
  }
}

// static
buzz::PreXmppAuth* XmppSignalStrategy::CreatePreXmppAuth(
    const buzz::XmppClientSettings& settings) {
  buzz::Jid jid(settings.user(), settings.host(), buzz::STR_EMPTY);
  std::string mechanism = notifier::GaiaTokenPreXmppAuth::kDefaultAuthMechanism;
  if (settings.token_service() == "oauth2") {
    mechanism = "X-OAUTH2";
  }

  return new notifier::GaiaTokenPreXmppAuth(
      jid.Str(),
      settings.auth_cookie(),
      settings.token_service(),
      mechanism);
}

}  // namespace remoting
