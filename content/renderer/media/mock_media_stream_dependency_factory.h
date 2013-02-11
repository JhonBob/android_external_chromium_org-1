// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MOCK_MEDIA_STREAM_DEPENDENCY_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_MOCK_MEDIA_STREAM_DEPENDENCY_FACTORY_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "content/renderer/media/media_stream_dependency_factory.h"

namespace content {

class MockVideoSource : public webrtc::VideoSourceInterface {
 public:
  MockVideoSource();

  virtual void RegisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual void UnregisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual MediaSourceInterface::SourceState state() const OVERRIDE;
  virtual cricket::VideoCapturer* GetVideoCapturer() OVERRIDE;
  virtual void AddSink(cricket::VideoRenderer* output) OVERRIDE;
  virtual void RemoveSink(cricket::VideoRenderer* output) OVERRIDE;
  virtual const cricket::VideoOptions* options() const OVERRIDE;

  // Changes the state of the source to live and notifies the observer.
  void SetLive();
  // Changes the state of the source to ended and notifies the observer.
  void SetEnded();

 protected:
  virtual ~MockVideoSource();

 private:
   webrtc::ObserverInterface* observer_;
   MediaSourceInterface::SourceState state_;
};

class MockAudioSource : public webrtc::AudioSourceInterface {
 public:
  MockAudioSource(const webrtc::MediaConstraintsInterface* constraints);

  virtual void RegisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual void UnregisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual MediaSourceInterface::SourceState state() const OVERRIDE;

  // Changes the state of the source to live and notifies the observer.
  void SetLive();
  // Changes the state of the source to ended and notifies the observer.
  void SetEnded();

  const webrtc::MediaConstraintsInterface::Constraints& optional_constraints() {
    return optional_constraints_;
  }

  const webrtc::MediaConstraintsInterface::Constraints&
  mandatory_constraints() {
    return mandatory_constraints_;
  }

 protected:
  virtual ~MockAudioSource();

 private:
  webrtc::ObserverInterface* observer_;
  MediaSourceInterface::SourceState state_;
  webrtc::MediaConstraintsInterface::Constraints optional_constraints_;
  webrtc::MediaConstraintsInterface::Constraints mandatory_constraints_;
};

class MockLocalVideoTrack : public webrtc::VideoTrackInterface {
 public:
  MockLocalVideoTrack(std::string id,
                      webrtc::VideoSourceInterface* source);
  virtual void AddRenderer(webrtc::VideoRendererInterface* renderer) OVERRIDE;
  virtual void RemoveRenderer(
      webrtc::VideoRendererInterface* renderer) OVERRIDE;
  virtual cricket::VideoRenderer* FrameInput() OVERRIDE;
  virtual std::string kind() const OVERRIDE;
  virtual std::string id() const OVERRIDE;
  virtual bool enabled() const OVERRIDE;
  virtual TrackState state() const OVERRIDE;
  virtual bool set_enabled(bool enable) OVERRIDE;
  virtual bool set_state(TrackState new_state) OVERRIDE;
  virtual void RegisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual void UnregisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual webrtc::VideoSourceInterface* GetSource() const OVERRIDE;

 protected:
  virtual ~MockLocalVideoTrack();

 private:
  bool enabled_;
  std::string id_;
  scoped_refptr<webrtc::VideoSourceInterface> source_;
};

class MockLocalAudioTrack : public webrtc::AudioTrackInterface {
 public:
  explicit MockLocalAudioTrack(const std::string& id)
    : enabled_(false),
      id_(id) {
  }
  virtual std::string kind() const OVERRIDE;
  virtual std::string id() const OVERRIDE;
  virtual bool enabled() const OVERRIDE;
  virtual TrackState state() const OVERRIDE;
  virtual bool set_enabled(bool enable) OVERRIDE;
  virtual bool set_state(TrackState new_state) OVERRIDE;
  virtual void RegisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual void UnregisterObserver(webrtc::ObserverInterface* observer) OVERRIDE;
  virtual webrtc::AudioSourceInterface* GetSource() const OVERRIDE;

 protected:
  virtual ~MockLocalAudioTrack() {}

 private:
  bool enabled_;
  std::string id_;
};

// A mock factory for creating different objects for
// RTC MediaStreams and PeerConnections.
class MockMediaStreamDependencyFactory : public MediaStreamDependencyFactory {
 public:
  MockMediaStreamDependencyFactory();
  virtual ~MockMediaStreamDependencyFactory();

  virtual scoped_refptr<webrtc::PeerConnectionInterface>
      CreatePeerConnection(const webrtc::JsepInterface::IceServers& ice_servers,
                           const webrtc::MediaConstraintsInterface* constraints,
                           WebKit::WebFrame* frame,
                           webrtc::PeerConnectionObserver* observer) OVERRIDE;
  virtual scoped_refptr<webrtc::AudioSourceInterface>
      CreateLocalAudioSource(
          const webrtc::MediaConstraintsInterface* constraints) OVERRIDE;
  virtual scoped_refptr<webrtc::VideoSourceInterface>
      CreateLocalVideoSource(
          int video_session_id,
          bool is_screencast,
          const webrtc::MediaConstraintsInterface* constraints) OVERRIDE;
  virtual bool InitializeAudioSource(
      const StreamDeviceInfo& device_info) OVERRIDE;
  virtual bool CreateWebAudioSource(
      WebKit::WebMediaStreamSource* source) OVERRIDE;
  virtual scoped_refptr<webrtc::LocalMediaStreamInterface>
      CreateLocalMediaStream(const std::string& label) OVERRIDE;
  virtual scoped_refptr<webrtc::VideoTrackInterface>
      CreateLocalVideoTrack(const std::string& id,
                            webrtc::VideoSourceInterface* source) OVERRIDE;
  virtual scoped_refptr<webrtc::LocalAudioTrackInterface>
      CreateLocalAudioTrack(const std::string& id,
                            webrtc::AudioSourceInterface* source) OVERRIDE;
  virtual webrtc::SessionDescriptionInterface* CreateSessionDescription(
      const std::string& type,
      const std::string& sdp,
      webrtc::SdpParseError* error) OVERRIDE;
  virtual webrtc::IceCandidateInterface* CreateIceCandidate(
      const std::string& sdp_mid,
      int sdp_mline_index,
      const std::string& sdp) OVERRIDE;

  virtual bool EnsurePeerConnectionFactory() OVERRIDE;
  virtual bool PeerConnectionFactoryCreated() OVERRIDE;

  MockAudioSource* last_audio_source() { return last_audio_source_; }
  MockVideoSource* last_video_source() { return last_video_source_; }

 private:
  bool mock_pc_factory_created_;
  scoped_refptr <MockAudioSource> last_audio_source_;
  scoped_refptr <MockVideoSource> last_video_source_;

  DISALLOW_COPY_AND_ASSIGN(MockMediaStreamDependencyFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MOCK_MEDIA_STREAM_DEPENDENCY_FACTORY_H_
