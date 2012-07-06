// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/media_stream_dispatcher_host.h"

#include "content/browser/browser_main_loop.h"
#include "content/common/media/media_stream_messages.h"
#include "content/common/media/media_stream_options.h"
#include "googleurl/src/gurl.h"

using content::BrowserMainLoop;
using content::BrowserMessageFilter;
using content::BrowserThread;

namespace media_stream {

struct MediaStreamDispatcherHost::StreamRequest {
  StreamRequest() : render_view_id(0), page_request_id(0) {}
  StreamRequest(int render_view_id, int page_request_id)
      : render_view_id(render_view_id),
        page_request_id(page_request_id ) {
  }
  int render_view_id;
  // Id of the request generated by MediaStreamDispatcher.
  int page_request_id;
};

MediaStreamDispatcherHost::MediaStreamDispatcherHost(int render_process_id)
    : render_process_id_(render_process_id) {
}

void MediaStreamDispatcherHost::StreamGenerated(
    const std::string& label,
    const StreamDeviceInfoArray& audio_devices,
    const StreamDeviceInfoArray& video_devices) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::StreamGenerated("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;

  Send(new MediaStreamMsg_StreamGenerated(
      request.render_view_id, request.page_request_id, label, audio_devices,
      video_devices));
}

void MediaStreamDispatcherHost::StreamGenerationFailed(
    const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::StreamGenerationFailed("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;
  streams_.erase(it);

  Send(new MediaStreamMsg_StreamGenerationFailed(request.render_view_id,
                                                 request.page_request_id));
}

void MediaStreamDispatcherHost::AudioDeviceFailed(const std::string& label,
                                                  int index) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::AudioDeviceFailed("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;
  Send(new MediaStreamHostMsg_AudioDeviceFailed(request.render_view_id,
                                                label,
                                                index));
}

void MediaStreamDispatcherHost::VideoDeviceFailed(const std::string& label,
                                                  int index) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::VideoDeviceFailed("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;
  Send(new MediaStreamHostMsg_VideoDeviceFailed(request.render_view_id,
                                                label,
                                                index));
}

void MediaStreamDispatcherHost::DevicesEnumerated(
    const std::string& label,
    const StreamDeviceInfoArray& devices) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::DevicesEnumerated("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;
  streams_.erase(it);

  Send(new MediaStreamMsg_DevicesEnumerated(
      request.render_view_id, request.page_request_id, devices));
}

void MediaStreamDispatcherHost::DevicesEnumerationFailed(
    const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::DevicesEnumerationFailed("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;
  streams_.erase(it);

  Send(new MediaStreamMsg_DevicesEnumerationFailed(
      request.render_view_id, request.page_request_id));
}

void MediaStreamDispatcherHost::DeviceOpened(
    const std::string& label,
    const StreamDeviceInfo& video_device) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::DeviceOpened("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;

  Send(new MediaStreamMsg_DeviceOpened(
      request.render_view_id, request.page_request_id, label, video_device));
}

void MediaStreamDispatcherHost::DeviceOpenFailed(
    const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "MediaStreamDispatcherHost::DeviceOpenFailed("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  StreamRequest request = it->second;
  streams_.erase(it);

  Send(new MediaStreamMsg_DeviceOpenFailed(request.render_view_id,
                                           request.page_request_id));
}

bool MediaStreamDispatcherHost::OnMessageReceived(
    const IPC::Message& message, bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(MediaStreamDispatcherHost, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(MediaStreamHostMsg_GenerateStream, OnGenerateStream)
    IPC_MESSAGE_HANDLER(MediaStreamHostMsg_CancelGenerateStream,
                        OnCancelGenerateStream)
    IPC_MESSAGE_HANDLER(MediaStreamHostMsg_StopGeneratedStream,
                        OnStopGeneratedStream)
    IPC_MESSAGE_HANDLER(MediaStreamHostMsg_EnumerateDevices,
                        OnEnumerateDevices)
    IPC_MESSAGE_HANDLER(MediaStreamHostMsg_OpenDevice,
                        OnOpenDevice)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()
  return handled;
}

void MediaStreamDispatcherHost::OnChannelClosing() {
  BrowserMessageFilter::OnChannelClosing();
  DVLOG(1) << "MediaStreamDispatcherHost::OnChannelClosing";

  // Since the IPC channel is gone, cancel pending requests and close all
  // requested VideoCaptureDevices.
  GetManager()->CancelRequests(this);
  for (StreamMap::iterator it = streams_.begin();
       it != streams_.end();
       it++) {
    std::string label = it->first;
    GetManager()->StopGeneratedStream(label);
  }
}

MediaStreamDispatcherHost::~MediaStreamDispatcherHost() {}

void MediaStreamDispatcherHost::OnGenerateStream(
    int render_view_id,
    int page_request_id,
    const media_stream::StreamOptions& components,
    const GURL& security_origin) {
  DVLOG(1) << "MediaStreamDispatcherHost::OnGenerateStream("
           << render_view_id << ", "
           << page_request_id << ", [ "
           << (components.audio ? "audio " : "")
           << (components.video ? "video " : "")
           << "], "
           << security_origin.spec() << ")";

  std::string label;
  GetManager()->GenerateStream(this, render_process_id_, render_view_id,
                            components, security_origin, &label);
  DCHECK(!label.empty());
  streams_[label] = StreamRequest(render_view_id, page_request_id);
}

void MediaStreamDispatcherHost::OnCancelGenerateStream(int render_view_id,
                                                       int page_request_id) {
  DVLOG(1) << "MediaStreamDispatcherHost::OnCancelGenerateStream("
           << render_view_id << ", "
           << page_request_id << ")";

  for (StreamMap::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    if (it->second.render_view_id == render_view_id &&
        it->second.page_request_id == page_request_id) {
      GetManager()->CancelGenerateStream(it->first);
    }
  }
}

void MediaStreamDispatcherHost::OnStopGeneratedStream(
    int render_view_id, const std::string& label) {
  DVLOG(1) << "MediaStreamDispatcherHost::OnStopGeneratedStream("
           << ", {label = " << label <<  "})";

  StreamMap::iterator it = streams_.find(label);
  DCHECK(it != streams_.end());
  GetManager()->StopGeneratedStream(label);
  streams_.erase(it);
}

void MediaStreamDispatcherHost::OnEnumerateDevices(
    int render_view_id,
    int page_request_id,
    media_stream::MediaStreamType type,
    const GURL& security_origin) {
  DVLOG(1) << "MediaStreamDispatcherHost::OnEnumerateDevices("
           << render_view_id << ", "
           << page_request_id << ", "
           << type << ", "
           << security_origin.spec() << ")";

  std::string label;
  GetManager()->EnumerateDevices(this, render_process_id_, render_view_id,
                              type, security_origin, &label);
  DCHECK(!label.empty());
  streams_[label] = StreamRequest(render_view_id, page_request_id);
}

void MediaStreamDispatcherHost::OnOpenDevice(
    int render_view_id,
    int page_request_id,
    const std::string& device_id,
    media_stream::MediaStreamType type,
    const GURL& security_origin) {
  DVLOG(1) << "MediaStreamDispatcherHost::OnOpenDevice("
           << render_view_id << ", "
           << page_request_id << ", device_id: "
           << device_id.c_str() << ", type: "
           << type << ", "
           << security_origin.spec() << ")";

  std::string label;
  GetManager()->OpenDevice(this, render_process_id_, render_view_id,
                        device_id, type, security_origin, &label);
  DCHECK(!label.empty());
  streams_[label] = StreamRequest(render_view_id, page_request_id);
}

MediaStreamManager* MediaStreamDispatcherHost::GetManager() {
  return BrowserMainLoop::GetMediaStreamManager();
}

}  // namespace media_stream
