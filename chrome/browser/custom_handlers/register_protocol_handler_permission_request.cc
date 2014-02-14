// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/custom_handlers/register_protocol_handler_permission_request.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "content/public/browser/user_metrics.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

base::string16 GetProtocolName(
    const ProtocolHandler& handler) {
  if (handler.protocol() == "mailto")
    return l10n_util::GetStringUTF16(IDS_REGISTER_PROTOCOL_HANDLER_MAILTO_NAME);
  if (handler.protocol() == "webcal")
    return l10n_util::GetStringUTF16(IDS_REGISTER_PROTOCOL_HANDLER_WEBCAL_NAME);
  return base::UTF8ToUTF16(handler.protocol());
}

}  // namespace

RegisterProtocolHandlerPermissionRequest
::RegisterProtocolHandlerPermissionRequest(
      ProtocolHandlerRegistry* registry,
      const ProtocolHandler& handler)
    : registry_(registry),
      handler_(handler) {}

RegisterProtocolHandlerPermissionRequest::
~RegisterProtocolHandlerPermissionRequest() {}

base::string16
RegisterProtocolHandlerPermissionRequest::GetMessageText() const {
  ProtocolHandler old_handler = registry_->GetHandlerFor(handler_.protocol());
  return old_handler.IsEmpty() ?
      l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_CONFIRM,
          handler_.title(), base::UTF8ToUTF16(handler_.url().host()),
          GetProtocolName(handler_)) :
      l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_CONFIRM_REPLACE,
          handler_.title(), base::UTF8ToUTF16(handler_.url().host()),
          GetProtocolName(handler_), old_handler.title());
}

base::string16
RegisterProtocolHandlerPermissionRequest::GetMessageTextFragment() const {
  ProtocolHandler old_handler = registry_->GetHandlerFor(handler_.protocol());
  return old_handler.IsEmpty() ?
      l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_CONFIRM_FRAGMENT,
          GetProtocolName(handler_)) :
      l10n_util::GetStringFUTF16(
          IDS_REGISTER_PROTOCOL_HANDLER_CONFIRM_REPLACE_FRAGMENT,
          GetProtocolName(handler_), old_handler.title());
}

base::string16 RegisterProtocolHandlerPermissionRequest::
GetAlternateAcceptButtonText() const {
  return l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_ACCEPT,
                                    handler_.title());
}

base::string16 RegisterProtocolHandlerPermissionRequest::
GetAlternateDenyButtonText() const {
  return l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_DENY,
                                    handler_.title());
}

void RegisterProtocolHandlerPermissionRequest::PermissionGranted() {
  content::RecordAction(
      base::UserMetricsAction("RegisterProtocolHandler.Infobar_Accept"));
  registry_->OnAcceptRegisterProtocolHandler(handler_);
}

void RegisterProtocolHandlerPermissionRequest::PermissionDenied() {
  content::RecordAction(
      base::UserMetricsAction("RegisterProtocolHandler.InfoBar_Deny"));
  registry_->OnIgnoreRegisterProtocolHandler(handler_);
}

void RegisterProtocolHandlerPermissionRequest::Cancelled() {
  content::RecordAction(
      base::UserMetricsAction("RegisterProtocolHandler.InfoBar_Deny"));
  registry_->OnIgnoreRegisterProtocolHandler(handler_);
}

void RegisterProtocolHandlerPermissionRequest::RequestFinished() {
  delete this;
}
