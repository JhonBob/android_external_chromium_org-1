// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_AUDIO_SCHEDULER_H_
#define REMOTING_HOST_AUDIO_SCHEDULER_H_

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

namespace protocol {
class AudioStub;
}  // namespace protocol

class AudioCapturer;
class AudioEncoder;
class AudioPacket;

// AudioScheduler is responsible for fetching audio data from the AudioCapturer
// and encoding it before passing it to the AudioStub for delivery to the
// client. Audio is captured and encoded on the audio thread and then passed to
// AudioStub on the network thread.
class AudioScheduler : public base::RefCountedThreadSafe<AudioScheduler> {
 public:
  // Audio capture and encoding tasks are dispatched via the
  // |audio_task_runner|. |audio_stub| tasks are dispatched via the
  // |network_task_runner|. The caller must ensure that the |audio_capturer| and
  // |audio_stub| exist until the scheduler is stopped using Stop() method.
  AudioScheduler(
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> network_task_runner,
      AudioCapturer* audio_capturer,
      scoped_ptr<AudioEncoder> audio_encoder,
      protocol::AudioStub* audio_stub);

  // Stop the recording session.
  void Stop(const base::Closure& done_task);

  // Called when a client disconnects.
  void OnClientDisconnected();

 private:
  friend class base::RefCountedThreadSafe<AudioScheduler>;
  virtual ~AudioScheduler();

  void NotifyAudioPacketCaptured(scoped_ptr<AudioPacket> packet);

  void DoStart();

  // Sends an audio packet to the client.
  void DoSendAudioPacket(scoped_ptr<AudioPacket> packet);

  // Signal network thread to cease activities.
  void DoStopOnNetworkThread(const base::Closure& done_task);

  // Called when an AudioPacket has been delivered to the client.
  void OnCaptureCallbackNotified();

  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  AudioCapturer* audio_capturer_;

  scoped_ptr<AudioEncoder> audio_encoder_;

  protocol::AudioStub* audio_stub_;

  bool network_stopped_;

  DISALLOW_COPY_AND_ASSIGN(AudioScheduler);
};

}  // namespace remoting

#endif  // REMOTING_HOST_AUDIO_SCHEDULER_H_
