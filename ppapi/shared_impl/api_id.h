// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_API_ID_H_
#define PPAPI_SHARED_IMPL_API_ID_H_

namespace ppapi {

// These numbers must be all small integers. They are used in a lookup table
// to route messages to the appropriate message handler.
enum ApiID {
  // Zero is reserved for control messages.
  API_ID_NONE = 0,
  API_ID_PPB_AUDIO = 1,
  API_ID_PPB_AUDIO_CONFIG,
  API_ID_PPB_BROKER,
  API_ID_PPB_BUFFER,
  API_ID_PPB_CONTEXT_3D,
  API_ID_PPB_CORE,
  API_ID_PPB_CURSORCONTROL,
  API_ID_PPB_FILE_CHOOSER,
  API_ID_PPB_FILE_REF,
  API_ID_PPB_FILE_SYSTEM,
  API_ID_PPB_FLASH,
  API_ID_PPB_FLASH_CLIPBOARD,
  API_ID_PPB_FLASH_FILE_FILEREF,
  API_ID_PPB_FLASH_FILE_MODULELOCAL,
  API_ID_PPB_FLASH_MENU,
  API_ID_PPB_FLASH_NETCONNECTOR,
  API_ID_PPB_FLASH_TCPSOCKET,
  API_ID_PPB_FLASH_UDPSOCKET,
  API_ID_PPB_FONT,
  API_ID_PPB_GRAPHICS_2D,
  API_ID_PPB_GRAPHICS_3D,
  API_ID_PPB_INSTANCE,
  API_ID_PPB_INSTANCE_PRIVATE,
  API_ID_PPB_OPENGLES2,
  API_ID_PPB_PDF,
  API_ID_PPB_SURFACE_3D,
  API_ID_PPB_TESTING,
  API_ID_PPB_TEXT_INPUT,
  API_ID_PPB_URL_LOADER,
  API_ID_PPB_URL_RESPONSE_INFO,
  API_ID_PPB_VAR_DEPRECATED,
  API_ID_PPB_VIDEO_CAPTURE_DEV,
  API_ID_PPB_VIDEO_DECODER_DEV,

  API_ID_PPP_CLASS,
  API_ID_PPP_GRAPHICS_3D,
  API_ID_PPP_INPUT_EVENT,
  API_ID_PPP_INSTANCE,
  API_ID_PPP_INSTANCE_PRIVATE,
  API_ID_PPP_MESSAGING,
  API_ID_PPP_MOUSE_LOCK,
  API_ID_PPP_VIDEO_CAPTURE_DEV,
  API_ID_PPP_VIDEO_DECODER_DEV,

  API_ID_RESOURCE_CREATION,

  // Must be last to indicate the number of interface IDs.
  API_ID_COUNT
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_API_ID_H_
