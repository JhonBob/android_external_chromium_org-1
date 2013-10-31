// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This test generate synthetic data. For audio it's a sinusoid waveform with
// frequency kSoundFrequency and different amplitudes. For video it's a pattern
// that is shifting by one pixel per frame, each pixels neighbors right and down
// is this pixels value +1, since the pixel value is 8 bit it will wrap
// frequently within the image. Visually this will create diagonally color bands
// that moves across the screen

#include <math.h>

#include <list>

#include "base/bind.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/tick_clock.h"
#include "media/cast/cast_config.h"
#include "media/cast/cast_environment.h"
#include "media/cast/cast_receiver.h"
#include "media/cast/cast_sender.h"
#include "media/cast/test/fake_task_runner.h"
#include "media/cast/test/video_utility.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace cast {

static const int64 kStartMillisecond = GG_INT64_C(1245);
static const int kAudioChannels = 2;
static const int kAudioSamplingFrequency = 48000;
static const int kSoundFrequency = 1234;  // Frequency of sinusoid wave.
static const int kVideoWidth = 1280;
static const int kVideoHeight = 720;
static const int kCommonRtpHeaderLength = 12;
static const uint8 kCastReferenceFrameIdBitReset = 0xDF;  // Mask is 0x40.

// Since the video encoded and decoded an error will be introduced; when
// comparing individual pixels the error can be quite large; we allow a PSNR of
// at least |kVideoAcceptedPSNR|.
static const double kVideoAcceptedPSNR = 38.0;

// The tests are commonly implemented with |kFrameTimerMs| RunTask function;
// a normal video is 30 fps hence the 33 ms between frames.
static const int kFrameTimerMs = 33;

// The packets pass through the pacer which can delay the beginning of the
// frame by 10 ms if there is packets belonging to the previous frame being
// retransmitted.
static const int kTimerErrorMs = 11;

// Class that sends the packet direct from sender into the receiver with the
// ability to drop packets between the two.
class LoopBackTransport : public PacketSender {
 public:
  explicit LoopBackTransport(scoped_refptr<CastEnvironment> cast_environment)
      : packet_receiver_(NULL),
        send_packets_(true),
        drop_packets_belonging_to_odd_frames_(false),
        reset_reference_frame_id_(false),
        cast_environment_(cast_environment) {
  }

  void RegisterPacketReceiver(PacketReceiver* packet_receiver) {
    DCHECK(packet_receiver);
    packet_receiver_ = packet_receiver;
  }

  virtual bool SendPacket(const Packet& packet) OVERRIDE {
    DCHECK(packet_receiver_);
    DCHECK(cast_environment_->CurrentlyOn(CastEnvironment::MAIN));
    if (!send_packets_) return false;

    uint8* packet_copy = new uint8[packet.size()];
    memcpy(packet_copy, packet.data(), packet.size());
    packet_receiver_->ReceivedPacket(packet_copy, packet.size(),
        base::Bind(PacketReceiver::DeletePacket, packet_copy));
    return true;
  }

  virtual bool SendPackets(const PacketList& packets) OVERRIDE {
    DCHECK(packet_receiver_);
    DCHECK(cast_environment_->CurrentlyOn(CastEnvironment::MAIN));
    if (!send_packets_) return false;

    for (size_t i = 0; i < packets.size(); ++i) {
      const Packet& packet = packets[i];
      if (drop_packets_belonging_to_odd_frames_) {
        uint8 frame_id = packet[13];
        if (frame_id % 2 == 1) continue;
      }
      uint8* packet_copy = new uint8[packet.size()];
      memcpy(packet_copy, packet.data(), packet.size());
      if (reset_reference_frame_id_) {
        // Reset the is_reference bit in the cast header.
        packet_copy[kCommonRtpHeaderLength] &= kCastReferenceFrameIdBitReset;
      }
      packet_receiver_->ReceivedPacket(packet_copy, packet.size(),
          base::Bind(PacketReceiver::DeletePacket, packet_copy));
    }
    return true;
  }

  void SetSendPackets(bool send_packets) {
    send_packets_ = send_packets;
  }

  void DropAllPacketsBelongingToOddFrames() {
    drop_packets_belonging_to_odd_frames_ = true;
  }

  void AlwaysResetReferenceFrameId() {
    reset_reference_frame_id_ = true;
  }

 private:
  PacketReceiver* packet_receiver_;
  bool send_packets_;
  bool drop_packets_belonging_to_odd_frames_;
  bool reset_reference_frame_id_;
  scoped_refptr<CastEnvironment> cast_environment_;
};

// Class that verifies the audio frames coming out of the receiver.
class TestReceiverAudioCallback :
     public base::RefCountedThreadSafe<TestReceiverAudioCallback>  {
 public:
  struct ExpectedAudioFrame {
    PcmAudioFrame audio_frame;
    int num_10ms_blocks;
    base::TimeTicks record_time;
  };

  TestReceiverAudioCallback()
      : num_called_(0),
       avg_snr_(0) {}

  void SetExpectedResult(int expected_sampling_frequency,
                         int expected_min_snr,
                         int expected_avg_snr) {
    expected_sampling_frequency_ = expected_sampling_frequency;
    expected_min_snr_ = expected_min_snr;
    expected_avg_snr_ = expected_avg_snr;
  }

  void AddExpectedResult(PcmAudioFrame* audio_frame,
                         int expected_num_10ms_blocks,
                         const base::TimeTicks& record_time) {
    ExpectedAudioFrame expected_audio_frame;
    expected_audio_frame.audio_frame = *audio_frame;
    expected_audio_frame.num_10ms_blocks = expected_num_10ms_blocks;
    expected_audio_frame.record_time = record_time;
    expected_frame_.push_back(expected_audio_frame);
  }

  void IgnoreAudioFrame(scoped_ptr<PcmAudioFrame> audio_frame,
                        const base::TimeTicks& playout_time) {}

  // Check the audio frame parameters but not the audio samples.
  void CheckBasicAudioFrame(const scoped_ptr<PcmAudioFrame>& audio_frame,
                            const base::TimeTicks& playout_time) {
    EXPECT_FALSE(expected_frame_.empty());  // Test for bug in test code.
    ExpectedAudioFrame expected_audio_frame = expected_frame_.front();
    EXPECT_EQ(audio_frame->channels, kAudioChannels);
    EXPECT_EQ(audio_frame->frequency, expected_sampling_frequency_);
    EXPECT_EQ(static_cast<int>(audio_frame->samples.size()),
        expected_audio_frame.num_10ms_blocks * kAudioChannels *
            expected_sampling_frequency_ / 100);

    EXPECT_GE(expected_audio_frame.record_time +
        base::TimeDelta::FromMilliseconds(kDefaultRtpMaxDelayMs +
            kTimerErrorMs), playout_time);
    EXPECT_LT(expected_audio_frame.record_time, playout_time);

    EXPECT_EQ(audio_frame->samples.size(),
              expected_audio_frame.audio_frame.samples.size());
  }

  size_t CalculateMaxResamplingDelay(size_t src_sample_rate_hz,
                                     size_t dst_sample_rate_hz,
                                     size_t number_of_channels) {
    // The sinc resampler has a known delay, which we compute here. Multiplying
    // by two gives us a crude maximum for any resampling, as the old resampler
    // typically (but not always) has lower delay. Since we sample up and down
    // we need to double our delay.
    static const size_t kInputKernelDelaySamples = 16;
    if (src_sample_rate_hz == dst_sample_rate_hz) return 0;

    return (dst_sample_rate_hz * kInputKernelDelaySamples *
        number_of_channels * 4) / src_sample_rate_hz;
  }

  // Computes the SNR based on the error between |reference_audio_frame| and
  // |output_audio_frame| given a sample offset of |delay|.
  double ComputeSNR(const PcmAudioFrame& reference_audio_frame,
                    const std::vector<int16>& output_audio_samples,
                    size_t delay) {
    // Check all out allowed delays.
    double square_error = 0;
    double variance = 0;
    for (size_t i = 0; i < reference_audio_frame.samples.size() - delay; ++i) {
      size_t error = reference_audio_frame.samples[i] -
          output_audio_samples[i + delay];

      square_error += error * error;
      variance += reference_audio_frame.samples[i] *
         reference_audio_frame.samples[i];
    }
    // 16-bit audio has a dynamic range of 96 dB.
    double snr = 96.0;  // Assigning 96 dB to the zero-error case.
    if (square_error > 0) {
      snr = 10 * log10(variance / square_error);
    }
    return snr;
  }

  // Computes the best SNR based on the error between |ref_frame| and
  // |test_frame|. It allows for up to a |max_delay| in samples between the
  // signals to compensate for the re-sampling delay.
  double ComputeBestSNR(const PcmAudioFrame& reference_audio_frame,
                        const std::vector<int16>& output_audio_samples,
                        size_t max_delay) {
    double best_snr = 0;

    // Check all out allowed delays.
    for (size_t delay = 0; delay <= max_delay;
        delay += reference_audio_frame.channels) {
      double snr = ComputeSNR(reference_audio_frame, output_audio_samples,
                              delay);
      if (snr > best_snr) {
        best_snr = snr;
      }
    }
    if (avg_snr_ == 0) {
      avg_snr_ = best_snr;
    } else {
      avg_snr_ = (avg_snr_ * 7  + best_snr) / 8;
    }
    return best_snr;
  }

  void CheckPcmAudioFrame(scoped_ptr<PcmAudioFrame> audio_frame,
                          const base::TimeTicks& playout_time) {
    ++num_called_;

    CheckBasicAudioFrame(audio_frame, playout_time);
    ExpectedAudioFrame expected_audio_frame = expected_frame_.front();
    expected_frame_.pop_front();
    if (audio_frame->samples.size() == 0) return;  // No more checks needed.

    size_t max_delay = CalculateMaxResamplingDelay(48000, 32000,
        expected_audio_frame.audio_frame.channels);
    EXPECT_GE(ComputeBestSNR(expected_audio_frame.audio_frame,
        audio_frame->samples, max_delay),
            expected_min_snr_);
  }

  void CheckCodedPcmAudioFrame(scoped_ptr<EncodedAudioFrame> audio_frame,
                               const base::TimeTicks& playout_time) {
    ++num_called_;

    EXPECT_FALSE(expected_frame_.empty());  // Test for bug in test code.
    ExpectedAudioFrame expected_audio_frame = expected_frame_.front();
    expected_frame_.pop_front();

    EXPECT_EQ(static_cast<int>(audio_frame->data.size()),
        2 * kAudioChannels * expected_sampling_frequency_ / 100);

    base::TimeDelta time_since_recording =
       playout_time - expected_audio_frame.record_time;

    EXPECT_LE(time_since_recording, base::TimeDelta::FromMilliseconds(
        kDefaultRtpMaxDelayMs + kTimerErrorMs));

    EXPECT_LT(expected_audio_frame.record_time, playout_time);
    if (audio_frame->data.size() == 0) return;  // No more checks needed.

    size_t max_delay = CalculateMaxResamplingDelay(48000, 32000,
        expected_audio_frame.audio_frame.channels);

    // We need to convert our "coded" audio frame to our raw format.
    std::vector<int16> output_audio_samples;
    size_t number_of_samples = audio_frame->data.size() / 2;

    for (size_t i = 0; i < number_of_samples; ++i) {
      uint16 sample = (audio_frame->data[1 + i * sizeof(uint16)]) +
            (static_cast<uint16>(audio_frame->data[i * sizeof(uint16)]) << 8);
      output_audio_samples.push_back(static_cast<int16>(sample));
    }
    EXPECT_GE(ComputeBestSNR(expected_audio_frame.audio_frame,
        output_audio_samples, max_delay),
            expected_min_snr_);
  }

  int number_times_called() {
    EXPECT_GE(avg_snr_, expected_avg_snr_);
    return num_called_;
  }

 protected:
  virtual ~TestReceiverAudioCallback() {}

 private:
  friend class base::RefCountedThreadSafe<TestReceiverAudioCallback>;

  int num_called_;
  int expected_sampling_frequency_;
  int expected_min_snr_;
  int expected_avg_snr_;
  double avg_snr_;
  std::list<ExpectedAudioFrame> expected_frame_;
};

// Class that verifies the video frames coming out of the receiver.
class TestReceiverVideoCallback :
     public base::RefCountedThreadSafe<TestReceiverVideoCallback>  {
 public:
  struct ExpectedVideoFrame {
    int start_value;
    int width;
    int height;
    base::TimeTicks capture_time;
  };

  TestReceiverVideoCallback()
      : num_called_(0) {}

  void AddExpectedResult(int start_value,
                         int width,
                         int height,
                         const base::TimeTicks& capture_time) {
    ExpectedVideoFrame expected_video_frame;
    expected_video_frame.start_value = start_value;
    expected_video_frame.capture_time = capture_time;
    expected_video_frame.width = width;
    expected_video_frame.height = height;
    expected_frame_.push_back(expected_video_frame);
  }

  void CheckVideoFrame(scoped_ptr<I420VideoFrame> video_frame,
                       const base::TimeTicks& render_time) {
    ++num_called_;

    EXPECT_FALSE(expected_frame_.empty());  // Test for bug in test code.
    ExpectedVideoFrame expected_video_frame = expected_frame_.front();
    expected_frame_.pop_front();

    base::TimeDelta time_since_capture =
       render_time - expected_video_frame.capture_time;

    EXPECT_LE(time_since_capture, base::TimeDelta::FromMilliseconds(
        kDefaultRtpMaxDelayMs + kTimerErrorMs));
    EXPECT_LE(expected_video_frame.capture_time, render_time);
    EXPECT_EQ(expected_video_frame.width, video_frame->width);
    EXPECT_EQ(expected_video_frame.height, video_frame->height);

    I420VideoFrame* expected_I420_frame = new I420VideoFrame();
    expected_I420_frame->width = expected_video_frame.width;
    expected_I420_frame->height = expected_video_frame.height;
    PopulateVideoFrame(expected_I420_frame, expected_video_frame.start_value);

    double psnr = I420PSNR(*expected_I420_frame, *(video_frame.get()));
    EXPECT_GE(psnr, kVideoAcceptedPSNR);

    FrameInput::DeleteVideoFrame(expected_I420_frame);
  }

  int number_times_called() { return num_called_;}

 protected:
  virtual ~TestReceiverVideoCallback() {}

 private:
  friend class base::RefCountedThreadSafe<TestReceiverVideoCallback>;

  int num_called_;
  std::list<ExpectedVideoFrame> expected_frame_;
};

// The actual test class, generate synthetic data for both audio and video and
// send those through the sender and receiver and analyzes the result.
class End2EndTest : public ::testing::Test {
 protected:
  End2EndTest()
      : task_runner_(new test::FakeTaskRunner(&testing_clock_)),
        cast_environment_(new CastEnvironment(&testing_clock_, task_runner_,
            task_runner_, task_runner_, task_runner_, task_runner_)),
        sender_to_receiver_(cast_environment_),
        receiver_to_sender_(cast_environment_),
        test_receiver_audio_callback_(new TestReceiverAudioCallback()),
        test_receiver_video_callback_(new TestReceiverVideoCallback()),
        audio_angle_(0) {
    testing_clock_.Advance(
        base::TimeDelta::FromMilliseconds(kStartMillisecond));
  }

  void SetupConfig(AudioCodec audio_codec,
                   int audio_sampling_frequency,
                   bool external_audio_decoder,
                   int max_number_of_video_buffers_used) {
    audio_sender_config_.sender_ssrc = 1;
    audio_sender_config_.incoming_feedback_ssrc = 2;
    audio_sender_config_.rtp_payload_type = 96;
    audio_sender_config_.use_external_encoder = false;
    audio_sender_config_.frequency = audio_sampling_frequency;
    audio_sender_config_.channels = kAudioChannels;
    audio_sender_config_.bitrate = 64000;
    audio_sender_config_.codec = audio_codec;

    audio_receiver_config_.feedback_ssrc =
        audio_sender_config_.incoming_feedback_ssrc;
    audio_receiver_config_.incoming_ssrc =
        audio_sender_config_.sender_ssrc;
    audio_receiver_config_.rtp_payload_type =
        audio_sender_config_.rtp_payload_type;
    audio_receiver_config_.use_external_decoder = external_audio_decoder;
    audio_receiver_config_.frequency = audio_sender_config_.frequency;
    audio_receiver_config_.channels = kAudioChannels;
    audio_receiver_config_.codec = audio_sender_config_.codec;

    video_sender_config_.sender_ssrc = 3;
    video_sender_config_.incoming_feedback_ssrc = 4;
    video_sender_config_.rtp_payload_type = 97;
    video_sender_config_.use_external_encoder = false;
    video_sender_config_.width = kVideoWidth;
    video_sender_config_.height = kVideoHeight;
    video_sender_config_.max_bitrate = 5000000;
    video_sender_config_.min_bitrate = 1000000;
    video_sender_config_.start_bitrate = 5000000;
    video_sender_config_.max_qp = 30;
    video_sender_config_.min_qp = 4;
    video_sender_config_.max_frame_rate = 30;
    video_sender_config_.max_number_of_video_buffers_used =
        max_number_of_video_buffers_used;
    video_sender_config_.codec = kVp8;
    video_sender_config_.number_of_cores = 1;

    video_receiver_config_.feedback_ssrc =
        video_sender_config_.incoming_feedback_ssrc;
    video_receiver_config_.incoming_ssrc =
        video_sender_config_.sender_ssrc;
    video_receiver_config_.rtp_payload_type =
        video_sender_config_.rtp_payload_type;
    video_receiver_config_.use_external_decoder = false;
    video_receiver_config_.codec = video_sender_config_.codec;
  }

  void Create() {
    cast_receiver_.reset(CastReceiver::CreateCastReceiver(cast_environment_,
        audio_receiver_config_, video_receiver_config_, &receiver_to_sender_));

    cast_sender_.reset(CastSender::CreateCastSender(cast_environment_,
                                                    audio_sender_config_,
                                                    video_sender_config_,
                                                    NULL,
                                                    &sender_to_receiver_));

    receiver_to_sender_.RegisterPacketReceiver(cast_sender_->packet_receiver());
    sender_to_receiver_.RegisterPacketReceiver(
        cast_receiver_->packet_receiver());

    frame_input_ = cast_sender_->frame_input();
    frame_receiver_ = cast_receiver_->frame_receiver();
  }

  virtual ~End2EndTest() {}

  void SendVideoFrame(int start_value, const base::TimeTicks& capture_time) {
    I420VideoFrame* video_frame = new I420VideoFrame();
    video_frame->width = video_sender_config_.width;
    video_frame->height = video_sender_config_.height;
    PopulateVideoFrame(video_frame, start_value);
    frame_input_->InsertRawVideoFrame(video_frame, capture_time,
        base::Bind(FrameInput::DeleteVideoFrame, video_frame));
  }

  PcmAudioFrame* CreateAudioFrame(int num_10ms_blocks, int sound_frequency,
                                  int sampling_frequency) {
    int number_of_samples = kAudioChannels * num_10ms_blocks *
        sampling_frequency / 100;
    int amplitude = 1000;

    PcmAudioFrame* audio_frame = new PcmAudioFrame();
    audio_frame->channels = kAudioChannels;
    audio_frame->frequency = sampling_frequency;
    audio_frame->samples.reserve(number_of_samples);

    // Create the sinusoid.
    double increment = (2 * 3.1415926535897932384626433) /
        (static_cast<double>(sampling_frequency) / sound_frequency);
    int sample = 0;
    while (sample < number_of_samples) {
      int16 value = static_cast<int16>(amplitude * sin(audio_angle_));

      for (int i = 0; i < kAudioChannels; ++i) {
        audio_frame->samples.insert(audio_frame->samples.end(), value);
        ++sample;
      }
      audio_angle_ += increment;
    }
    return audio_frame;
  }

  void RunTasks(int during_ms) {
    for (int i = 0; i < during_ms; ++i) {
      // Call process the timers every 1 ms.
      testing_clock_.Advance(base::TimeDelta::FromMilliseconds(1));
      task_runner_->RunTasks();
    }
  }

  AudioReceiverConfig audio_receiver_config_;
  VideoReceiverConfig video_receiver_config_;
  AudioSenderConfig audio_sender_config_;
  VideoSenderConfig video_sender_config_;

  base::SimpleTestTickClock testing_clock_;
  scoped_refptr<test::FakeTaskRunner> task_runner_;
  scoped_refptr<CastEnvironment> cast_environment_;

  LoopBackTransport sender_to_receiver_;
  LoopBackTransport receiver_to_sender_;

  scoped_ptr<CastReceiver> cast_receiver_;
  scoped_ptr<CastSender> cast_sender_;
  scoped_refptr<FrameInput> frame_input_;
  scoped_refptr<FrameReceiver> frame_receiver_;

  scoped_refptr<TestReceiverAudioCallback> test_receiver_audio_callback_;
  scoped_refptr<TestReceiverVideoCallback> test_receiver_video_callback_;

  double audio_angle_;
};

// Audio and video test without packet loss using raw PCM 16 audio "codec";
// note: even though the audio is not coded it is still re-sampled between
// 48 and 32 KHz.
TEST_F(End2EndTest, LoopNoLossPcm16) {
  // Note running codec in different sampling frequency.
  SetupConfig(kPcm16, 32000, false, 1);
  Create();
  test_receiver_audio_callback_->SetExpectedResult(kAudioSamplingFrequency, 20,
      25);

  int video_start = 1;
  int audio_diff = kFrameTimerMs;
  int i = 0;

  std::cout << "Progress ";
  for (; i < 100; ++i) {
    int num_10ms_blocks = audio_diff / 10;
    audio_diff -= num_10ms_blocks * 10;
    base::TimeTicks send_time = testing_clock_.NowTicks();

    test_receiver_video_callback_->AddExpectedResult(video_start,
        video_sender_config_.width, video_sender_config_.height, send_time);

    PcmAudioFrame* audio_frame = CreateAudioFrame(num_10ms_blocks,
        kSoundFrequency, kAudioSamplingFrequency);

    if (i != 0) {
      // Due to the re-sampler and NetEq in the webrtc AudioCodingModule the
      // first samples will be 0 and then slowly ramp up to its real amplitude;
      // ignore the first frame.
      test_receiver_audio_callback_->AddExpectedResult(audio_frame,
          num_10ms_blocks, send_time);
    }

    frame_input_->InsertRawAudioFrame(audio_frame, send_time,
        base::Bind(FrameInput::DeleteAudioFrame, audio_frame));

    SendVideoFrame(video_start, send_time);

    RunTasks(kFrameTimerMs);
    audio_diff += kFrameTimerMs;

    if (i == 0) {
      frame_receiver_->GetRawAudioFrame(num_10ms_blocks,
          kAudioSamplingFrequency,
          base::Bind(&TestReceiverAudioCallback::IgnoreAudioFrame,
              test_receiver_audio_callback_));
    } else {
      frame_receiver_->GetRawAudioFrame(num_10ms_blocks,
          kAudioSamplingFrequency,
          base::Bind(&TestReceiverAudioCallback::CheckPcmAudioFrame,
              test_receiver_audio_callback_));
    }

    frame_receiver_->GetRawVideoFrame(
        base::Bind(&TestReceiverVideoCallback::CheckVideoFrame,
            test_receiver_video_callback_));

    std::cout << " " << i << std::flush;
    video_start++;
  }
  std::cout << std::endl;

  RunTasks(2 * kFrameTimerMs + 1);  // Empty the receiver pipeline.
  EXPECT_EQ(i - 1, test_receiver_audio_callback_->number_times_called());
  EXPECT_EQ(i, test_receiver_video_callback_->number_times_called());
}

// This tests our external decoder interface for Audio.
// Audio test without packet loss using raw PCM 16 audio "codec";
TEST_F(End2EndTest, LoopNoLossPcm16ExternalDecoder) {
  // Note: Create an input in the same sampling frequency as the codec to avoid
  // re-sampling.
  const int audio_sampling_frequency = 32000;
  SetupConfig(kPcm16, audio_sampling_frequency, true, 1);
  Create();
  test_receiver_audio_callback_->SetExpectedResult(audio_sampling_frequency, 96,
      96);

  int i = 0;
  for (; i < 100; ++i) {
    base::TimeTicks send_time = testing_clock_.NowTicks();
    PcmAudioFrame* audio_frame = CreateAudioFrame(1, kSoundFrequency,
        audio_sampling_frequency);
    test_receiver_audio_callback_->AddExpectedResult(audio_frame, 1,
        send_time);

    frame_input_->InsertRawAudioFrame(audio_frame, send_time,
        base::Bind(FrameInput::DeleteAudioFrame, audio_frame));

    RunTasks(10);
    frame_receiver_->GetCodedAudioFrame(
        base::Bind(&TestReceiverAudioCallback::CheckCodedPcmAudioFrame,
            test_receiver_audio_callback_));
  }
  RunTasks(2 * kFrameTimerMs + 1);  // Empty the receiver pipeline.
  EXPECT_EQ(100, test_receiver_audio_callback_->number_times_called());
}

// This tests our Opus audio codec without video.
TEST_F(End2EndTest, LoopNoLossOpus) {
  SetupConfig(kOpus, kAudioSamplingFrequency, false, 1);
  Create();
  test_receiver_audio_callback_->SetExpectedResult(
      kAudioSamplingFrequency, 18, 20);

  int i = 0;
  for (; i < 100; ++i) {
    int num_10ms_blocks = 3;
    base::TimeTicks send_time = testing_clock_.NowTicks();

    PcmAudioFrame* audio_frame = CreateAudioFrame(num_10ms_blocks,
        kSoundFrequency, kAudioSamplingFrequency);

    if (i != 0) {
      test_receiver_audio_callback_->AddExpectedResult(audio_frame,
          num_10ms_blocks, send_time);
    }

    frame_input_->InsertRawAudioFrame(audio_frame, send_time,
        base::Bind(FrameInput::DeleteAudioFrame, audio_frame));

    RunTasks(30);

    if (i == 0) {
      frame_receiver_->GetRawAudioFrame(num_10ms_blocks,
          kAudioSamplingFrequency,
          base::Bind(&TestReceiverAudioCallback::IgnoreAudioFrame,
              test_receiver_audio_callback_));
    } else {
      frame_receiver_->GetRawAudioFrame(num_10ms_blocks,
          kAudioSamplingFrequency,
          base::Bind(&TestReceiverAudioCallback::CheckPcmAudioFrame,
              test_receiver_audio_callback_));
    }
  }
  RunTasks(2 * kFrameTimerMs + 1);  // Empty the receiver pipeline.
  EXPECT_EQ(i - 1, test_receiver_audio_callback_->number_times_called());
}

// This tests start sending audio and video before the receiver is ready.
TEST_F(End2EndTest, StartSenderBeforeReceiver) {
  SetupConfig(kOpus, kAudioSamplingFrequency, false, 1);
  Create();
  test_receiver_audio_callback_->SetExpectedResult(
      kAudioSamplingFrequency, 18, 20);

  int video_start = 1;
  int audio_diff = kFrameTimerMs;

  sender_to_receiver_.SetSendPackets(false);

  for (int i = 0; i < 3; ++i) {
    int num_10ms_blocks = audio_diff / 10;
    audio_diff -= num_10ms_blocks * 10;

    base::TimeTicks send_time = testing_clock_.NowTicks();
    PcmAudioFrame* audio_frame = CreateAudioFrame(num_10ms_blocks,
        kSoundFrequency, kAudioSamplingFrequency);

    frame_input_->InsertRawAudioFrame(audio_frame, send_time,
       base::Bind(FrameInput::DeleteAudioFrame, audio_frame));

    SendVideoFrame(video_start, send_time);
    RunTasks(kFrameTimerMs);
    audio_diff += kFrameTimerMs;
    video_start++;
  }
  RunTasks(100);
  sender_to_receiver_.SetSendPackets(true);

  int j = 0;
  const int number_of_audio_frames_to_ignore = 3;
  for (; j < 10; ++j) {
    int num_10ms_blocks = audio_diff / 10;
    audio_diff -= num_10ms_blocks * 10;
    base::TimeTicks send_time = testing_clock_.NowTicks();

    PcmAudioFrame* audio_frame = CreateAudioFrame(num_10ms_blocks,
        kSoundFrequency, kAudioSamplingFrequency);

    frame_input_->InsertRawAudioFrame(audio_frame, send_time,
       base::Bind(FrameInput::DeleteAudioFrame, audio_frame));

    if (j >= number_of_audio_frames_to_ignore) {
      test_receiver_audio_callback_->AddExpectedResult(audio_frame,
          num_10ms_blocks, send_time);
    }
    test_receiver_video_callback_->AddExpectedResult(video_start,
        video_sender_config_.width, video_sender_config_.height, send_time);

    SendVideoFrame(video_start, send_time);
    RunTasks(kFrameTimerMs);
    audio_diff += kFrameTimerMs;

    if (j < number_of_audio_frames_to_ignore) {
      frame_receiver_->GetRawAudioFrame(num_10ms_blocks,
         kAudioSamplingFrequency,
         base::Bind(&TestReceiverAudioCallback::IgnoreAudioFrame,
              test_receiver_audio_callback_));
     } else {
      frame_receiver_->GetRawAudioFrame(num_10ms_blocks,
          kAudioSamplingFrequency,
          base::Bind(&TestReceiverAudioCallback::CheckPcmAudioFrame,
              test_receiver_audio_callback_));
    }
    frame_receiver_->GetRawVideoFrame(
        base::Bind(&TestReceiverVideoCallback::CheckVideoFrame,
            test_receiver_video_callback_));
    video_start++;
  }
  RunTasks(2 * kFrameTimerMs + 1);  // Empty the receiver pipeline.
  EXPECT_EQ(j - number_of_audio_frames_to_ignore,
            test_receiver_audio_callback_->number_times_called());
  EXPECT_EQ(j, test_receiver_video_callback_->number_times_called());
}

// This tests a network glitch lasting for 10 video frames.
TEST_F(End2EndTest, GlitchWith3Buffers) {
  SetupConfig(kOpus, kAudioSamplingFrequency, false, 3);
  video_sender_config_.rtp_max_delay_ms = 67;
  video_receiver_config_.rtp_max_delay_ms = 67;
  Create();

  int video_start = 50;
  base::TimeTicks send_time = testing_clock_.NowTicks();
  SendVideoFrame(video_start, send_time);
  RunTasks(kFrameTimerMs);

  test_receiver_video_callback_->AddExpectedResult(video_start,
      video_sender_config_.width, video_sender_config_.height, send_time);
  frame_receiver_->GetRawVideoFrame(
      base::Bind(&TestReceiverVideoCallback::CheckVideoFrame,
          test_receiver_video_callback_));

  RunTasks(750);  // Make sure that we send a RTCP packet.

  video_start++;

  // Introduce a glitch lasting for 10 frames.
  sender_to_receiver_.SetSendPackets(false);
  for (int i = 0; i < 10; ++i) {
    send_time = testing_clock_.NowTicks();
    // First 3 will be sent and lost.
    SendVideoFrame(video_start, send_time);
    RunTasks(kFrameTimerMs);
    video_start++;
  }
  sender_to_receiver_.SetSendPackets(true);
  RunTasks(100);
  send_time = testing_clock_.NowTicks();

  // Frame 1 should be acked by now and we should have an opening to send 4.
  SendVideoFrame(video_start, send_time);
  RunTasks(kFrameTimerMs);

  test_receiver_video_callback_->AddExpectedResult(video_start,
      video_sender_config_.width, video_sender_config_.height, send_time);

  frame_receiver_->GetRawVideoFrame(
      base::Bind(&TestReceiverVideoCallback::CheckVideoFrame,
          test_receiver_video_callback_));

  RunTasks(2 * kFrameTimerMs + 1);  // Empty the receiver pipeline.
  EXPECT_EQ(2, test_receiver_video_callback_->number_times_called());
}

TEST_F(End2EndTest, DropEveryOtherFrame3Buffers) {
  SetupConfig(kOpus, kAudioSamplingFrequency, false, 3);
  video_sender_config_.rtp_max_delay_ms = 67;
  video_receiver_config_.rtp_max_delay_ms = 67;
  Create();
  sender_to_receiver_.DropAllPacketsBelongingToOddFrames();

  int video_start = 50;
  base::TimeTicks send_time;

  std::cout << "Progress ";
  int i = 0;
  for (; i < 20; ++i) {
    send_time = testing_clock_.NowTicks();
    SendVideoFrame(video_start, send_time);

    if (i % 2 == 0) {
      test_receiver_video_callback_->AddExpectedResult(video_start,
          video_sender_config_.width, video_sender_config_.height, send_time);

      // GetRawVideoFrame will not return the frame until we are close in
      // time before we should render the frame.
      frame_receiver_->GetRawVideoFrame(
          base::Bind(&TestReceiverVideoCallback::CheckVideoFrame,
              test_receiver_video_callback_));
    }
    RunTasks(kFrameTimerMs);
    std::cout << " " << i << std::flush;
    video_start++;
  }
  std::cout << std::endl;
  RunTasks(2 * kFrameTimerMs + 1);  // Empty the pipeline.
  EXPECT_EQ(i / 2, test_receiver_video_callback_->number_times_called());
}

TEST_F(End2EndTest, ResetReferenceFrameId) {
  SetupConfig(kOpus, kAudioSamplingFrequency, false, 3);
  video_sender_config_.rtp_max_delay_ms = 67;
  video_receiver_config_.rtp_max_delay_ms = 67;
  Create();
  sender_to_receiver_.AlwaysResetReferenceFrameId();

  int frames_counter = 0;
  for (; frames_counter < 20; ++frames_counter) {
    const base::TimeTicks send_time = testing_clock_.NowTicks();
    SendVideoFrame(frames_counter, send_time);

    test_receiver_video_callback_->AddExpectedResult(frames_counter,
        video_sender_config_.width, video_sender_config_.height, send_time);

    // GetRawVideoFrame will not return the frame until we are close to the
    // time in which we should render the frame.
    frame_receiver_->GetRawVideoFrame(
        base::Bind(&TestReceiverVideoCallback::CheckVideoFrame,
                   test_receiver_video_callback_));
    RunTasks(kFrameTimerMs);
  }
  RunTasks(2 * kFrameTimerMs + 1);  // Empty the pipeline.
  EXPECT_EQ(frames_counter,
            test_receiver_video_callback_->number_times_called());
}

// TODO(pwestin): Add repeatable packet loss test.
// TODO(pwestin): Add test for misaligned send get calls.
// TODO(pwestin): Add more tests that does not resample.
// TODO(pwestin): Add test when we have starvation for our RunTask.

}  // namespace cast
}  // namespace media
