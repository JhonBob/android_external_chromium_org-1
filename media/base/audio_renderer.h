// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_AUDIO_RENDERER_H_
#define MEDIA_BASE_AUDIO_RENDERER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/time.h"
#include "media/base/media_export.h"
#include "media/base/pipeline_status.h"

namespace media {

class DemuxerStream;

class MEDIA_EXPORT AudioRenderer {
 public:
  // First parameter is the current time that has been rendered.
  // Second parameter is the maximum time value that the clock cannot exceed.
  typedef base::Callback<void(base::TimeDelta, base::TimeDelta)> TimeCB;

  AudioRenderer();
  virtual ~AudioRenderer();

  // Initialize an AudioRenderer with |stream|, executing |init_cb| upon
  // completion.
  //
  // |statistics_cb| is executed periodically with audio rendering stats.
  //
  // |underflow_cb| is executed when the renderer runs out of data to pass to
  // the audio card during playback. ResumeAfterUnderflow() must be called
  // to resume playback. Pause(), Preroll(), or Stop() cancels the underflow
  // condition.
  //
  // |time_cb| is executed whenever time has advanced by way of audio rendering.
  //
  // |ended_cb| is executed when audio rendering has reached the end of stream.
  //
  // |disabled_cb| is executed when audio rendering has been disabled due to
  // external factors (i.e., device was removed). |time_cb| will no longer be
  // executed. TODO(scherkus): this might not be needed http://crbug.com/234708
  //
  // |error_cb| is executed if an error was encountered.
  virtual void Initialize(DemuxerStream* stream,
                          const PipelineStatusCB& init_cb,
                          const StatisticsCB& statistics_cb,
                          const base::Closure& underflow_cb,
                          const TimeCB& time_cb,
                          const base::Closure& ended_cb,
                          const base::Closure& disabled_cb,
                          const PipelineStatusCB& error_cb) = 0;

  // Start audio decoding and rendering at the current playback rate, executing
  // |callback| when playback is underway.
  virtual void Play(const base::Closure& callback) = 0;

  // Temporarily suspend decoding and rendering audio, executing |callback| when
  // playback has been suspended.
  virtual void Pause(const base::Closure& callback) = 0;

  // Discard any audio data, executing |callback| when completed.
  virtual void Flush(const base::Closure& callback) = 0;

  // Start prerolling audio data for samples starting at |time|, executing
  // |callback| when completed.
  //
  // Only valid to call after a successful Initialize() or Flush().
  virtual void Preroll(base::TimeDelta time,
                       const PipelineStatusCB& callback) = 0;

  // Stop all operations in preparation for being deleted, executing |callback|
  // when complete.
  virtual void Stop(const base::Closure& callback) = 0;

  // Updates the current playback rate.
  virtual void SetPlaybackRate(float playback_rate) = 0;

  // Sets the output volume.
  virtual void SetVolume(float volume) = 0;

  // Resumes playback after underflow occurs.
  virtual void ResumeAfterUnderflow() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioRenderer);
};

}  // namespace media

#endif  // MEDIA_BASE_AUDIO_RENDERER_H_
