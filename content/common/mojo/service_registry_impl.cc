// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/mojo/service_registry_impl.h"

#include "mojo/common/common_type_converters.h"

namespace content {

ServiceRegistryImpl::ServiceRegistryImpl() : bound_(false) {
}

ServiceRegistryImpl::ServiceRegistryImpl(mojo::ScopedMessagePipeHandle handle)
    : bound_(false) {
  BindRemoteServiceProvider(handle.Pass());
}

ServiceRegistryImpl::~ServiceRegistryImpl() {
  while (!pending_connects_.empty()) {
    mojo::CloseRaw(pending_connects_.front().second);
    pending_connects_.pop();
  }
}

void ServiceRegistryImpl::BindRemoteServiceProvider(
    mojo::ScopedMessagePipeHandle handle) {
  if (bound_)
    return;

  mojo::BindToPipe(this, handle.Pass());
  bound_ = true;
  while (!pending_connects_.empty()) {
    client()->GetInterface(
        mojo::String::From(pending_connects_.front().first),
        mojo::ScopedMessagePipeHandle(pending_connects_.front().second));
    pending_connects_.pop();
  }
}

void ServiceRegistryImpl::OnConnectionError() {
  // TODO(sammc): Support reporting this to our owner.
  bound_ = false;
}

void ServiceRegistryImpl::AddService(
    const std::string& service_name,
    const base::Callback<void(mojo::ScopedMessagePipeHandle)> service_factory) {
  service_factories_[service_name] = service_factory;
}

void ServiceRegistryImpl::RemoveService(const std::string& service_name) {
  service_factories_.erase(service_name);
}

void ServiceRegistryImpl::GetRemoteInterface(
    const base::StringPiece& service_name,
    mojo::ScopedMessagePipeHandle handle) {
  if (!bound_) {
    pending_connects_.push(
        std::make_pair(service_name.as_string(), handle.release()));
    return;
  }
  client()->GetInterface(mojo::String::From(service_name), handle.Pass());
}

void ServiceRegistryImpl::GetInterface(
    const mojo::String& name,
    mojo::ScopedMessagePipeHandle client_handle) {
  std::map<std::string,
           base::Callback<void(mojo::ScopedMessagePipeHandle)> >::iterator it =
      service_factories_.find(name);
  if (it == service_factories_.end())
    return;

  it->second.Run(client_handle.Pass());
}

}  // namespace content
