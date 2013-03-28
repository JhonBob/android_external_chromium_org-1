// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/ppb_video_decoder_shared.h"

#include "base/logging.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/ppb_graphics_3d_shared.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/thunk/enter.h"

namespace ppapi {

PPB_VideoDecoder_Shared::PPB_VideoDecoder_Shared(PP_Instance instance)
    : Resource(OBJECT_IS_IMPL, instance),
      graphics_context_(0),
      gles2_impl_(NULL) {
}

PPB_VideoDecoder_Shared::PPB_VideoDecoder_Shared(
    const HostResource& host_resource)
    : Resource(OBJECT_IS_PROXY, host_resource),
      graphics_context_(0),
      gles2_impl_(NULL) {
}

PPB_VideoDecoder_Shared::~PPB_VideoDecoder_Shared() {
  // Destroy() must be called before the object is destroyed.
  DCHECK(graphics_context_ == 0);
}

thunk::PPB_VideoDecoder_API* PPB_VideoDecoder_Shared::AsPPB_VideoDecoder_API() {
  return this;
}

void PPB_VideoDecoder_Shared::InitCommon(
    PP_Resource graphics_context,
    gpu::gles2::GLES2Implementation* gles2_impl) {
  DCHECK(graphics_context);
  DCHECK(!gles2_impl_ && !graphics_context_);
  gles2_impl_ = gles2_impl;
  PpapiGlobals::Get()->GetResourceTracker()->AddRefResource(graphics_context);
  graphics_context_ = graphics_context;
}

void PPB_VideoDecoder_Shared::Destroy() {
  if (graphics_context_) {
    PpapiGlobals::Get()->GetResourceTracker()->ReleaseResource(
        graphics_context_);
    graphics_context_ = 0;
  }
  gles2_impl_ = NULL;
}

bool PPB_VideoDecoder_Shared::SetFlushCallback(
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(flush_callback_))
    return false;
  flush_callback_ = callback;
  return true;
}

bool PPB_VideoDecoder_Shared::SetResetCallback(
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(reset_callback_))
    return false;
  reset_callback_ = callback;
  return true;
}

bool PPB_VideoDecoder_Shared::SetBitstreamBufferCallback(
    int32 bitstream_buffer_id,
    scoped_refptr<TrackedCallback> callback) {
  return bitstream_buffer_callbacks_.insert(
      std::make_pair(bitstream_buffer_id, callback)).second;
}

void PPB_VideoDecoder_Shared::RunFlushCallback(int32 result) {
  flush_callback_->Run(result);
}

void PPB_VideoDecoder_Shared::RunResetCallback(int32 result) {
  reset_callback_->Run(result);
}

void PPB_VideoDecoder_Shared::RunBitstreamBufferCallback(
    int32 bitstream_buffer_id, int32 result) {
  CallbackById::iterator it =
      bitstream_buffer_callbacks_.find(bitstream_buffer_id);
  DCHECK(it != bitstream_buffer_callbacks_.end());
  scoped_refptr<TrackedCallback> cc = it->second;
  bitstream_buffer_callbacks_.erase(it);
  cc->Run(PP_OK);
}

void PPB_VideoDecoder_Shared::FlushCommandBuffer() {
  if (gles2_impl_) {
    // To call Flush() we have to tell Graphics3D that we hold the proxy lock.
    thunk::EnterResource<thunk::PPB_Graphics3D_API, false> enter_g3d(
        graphics_context_, false);
    DCHECK(enter_g3d.succeeded());
    PPB_Graphics3D_Shared* graphics3d =
        static_cast<PPB_Graphics3D_Shared*>(enter_g3d.object());
    PPB_Graphics3D_Shared::ScopedNoLocking dont_lock(graphics3d);
    gles2_impl_->Flush();
  }
}

}  // namespace ppapi
