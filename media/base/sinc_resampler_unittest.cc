// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// MSVC++ requires this to be set before any other includes to get M_PI.
#define _USE_MATH_DEFINES

#include <cmath>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "base/strings/stringize_macros.h"
#include "base/time.h"
#include "build/build_config.h"
#include "media/base/sinc_resampler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace media {

static const double kSampleRateRatio = 192000.0 / 44100.0;
static const double kKernelInterpolationFactor = 0.5;

// Command line switch for runtime adjustment of ConvolveBenchmark iterations.
static const char kConvolveIterations[] = "convolve-iterations";

// Helper class to ensure ChunkedResample() functions properly.
class MockSource {
 public:
  MOCK_METHOD2(ProvideInput, void(float* destination, int frames));
};

ACTION(ClearBuffer) {
  memset(arg0, 0, arg1 * sizeof(float));
}

ACTION(FillBuffer) {
  // Value chosen arbitrarily such that SincResampler resamples it to something
  // easily representable on all platforms; e.g., using kSampleRateRatio this
  // becomes 1.81219.
  memset(arg0, 64, arg1 * sizeof(float));
}

// Test requesting multiples of ChunkSize() frames results in the proper number
// of callbacks.
TEST(SincResamplerTest, ChunkedResample) {
  MockSource mock_source;

  // Choose a high ratio of input to output samples which will result in quick
  // exhaustion of SincResampler's internal buffers.
  SincResampler resampler(
      kSampleRateRatio,
      base::Bind(&MockSource::ProvideInput, base::Unretained(&mock_source)));

  static const int kChunks = 2;
  int max_chunk_size = resampler.ChunkSize() * kChunks;
  scoped_array<float> resampled_destination(new float[max_chunk_size]);

  // Verify requesting ChunkSize() frames causes a single callback.
  EXPECT_CALL(mock_source, ProvideInput(_, _))
      .Times(1).WillOnce(ClearBuffer());
  resampler.Resample(resampled_destination.get(), resampler.ChunkSize());

  // Verify requesting kChunks * ChunkSize() frames causes kChunks callbacks.
  testing::Mock::VerifyAndClear(&mock_source);
  EXPECT_CALL(mock_source, ProvideInput(_, _))
      .Times(kChunks).WillRepeatedly(ClearBuffer());
  resampler.Resample(resampled_destination.get(), max_chunk_size);
}

// Test flush resets the internal state properly.
TEST(SincResamplerTest, Flush) {
  MockSource mock_source;
  SincResampler resampler(
      kSampleRateRatio,
      base::Bind(&MockSource::ProvideInput, base::Unretained(&mock_source)));
  scoped_array<float> resampled_destination(new float[resampler.ChunkSize()]);

  // Fill the resampler with junk data.
  EXPECT_CALL(mock_source, ProvideInput(_, _))
      .Times(1).WillOnce(FillBuffer());
  resampler.Resample(resampled_destination.get(), resampler.ChunkSize() / 2);
  ASSERT_NE(resampled_destination[0], 0);

  // Flush and request more data, which should all be zeros now.
  resampler.Flush();
  testing::Mock::VerifyAndClear(&mock_source);
  EXPECT_CALL(mock_source, ProvideInput(_, _))
      .Times(1).WillOnce(ClearBuffer());
  resampler.Resample(resampled_destination.get(), resampler.ChunkSize() / 2);
  for (int i = 0; i < resampler.ChunkSize() / 2; ++i)
    ASSERT_FLOAT_EQ(resampled_destination[i], 0);
}

// Define platform independent function name for Convolve* tests.
#if defined(ARCH_CPU_X86_FAMILY)
#define CONVOLVE_FUNC Convolve_SSE
#elif defined(ARCH_CPU_ARM_FAMILY) && defined(USE_NEON)
#define CONVOLVE_FUNC Convolve_NEON
#endif

// Ensure various optimized Convolve() methods return the same value.  Only run
// this test if other optimized methods exist, otherwise the default Convolve()
// will be tested by the parameterized SincResampler tests below.
#if defined(CONVOLVE_FUNC)
TEST(SincResamplerTest, Convolve) {
#if defined(ARCH_CPU_X86_FAMILY)
  ASSERT_TRUE(base::CPU().has_sse());
#endif

  // Initialize a dummy resampler.
  MockSource mock_source;
  SincResampler resampler(
      kSampleRateRatio,
      base::Bind(&MockSource::ProvideInput, base::Unretained(&mock_source)));

  // The optimized Convolve methods are slightly more precise than Convolve_C(),
  // so comparison must be done using an epsilon.
  static const double kEpsilon = 0.00000005;

  // Use a kernel from SincResampler as input and kernel data, this has the
  // benefit of already being properly sized and aligned for Convolve_SSE().
  double result = resampler.Convolve_C(
      resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  double result2 = resampler.CONVOLVE_FUNC(
      resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  EXPECT_NEAR(result2, result, kEpsilon);

  // Test Convolve() w/ unaligned input pointer.
  result = resampler.Convolve_C(
      resampler.kernel_storage_.get() + 1, resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  result2 = resampler.CONVOLVE_FUNC(
      resampler.kernel_storage_.get() + 1, resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  EXPECT_NEAR(result2, result, kEpsilon);
}
#endif

// Benchmark for the various Convolve() methods.  Make sure to build with
// branding=Chrome so that DCHECKs are compiled out when benchmarking.  Original
// benchmarks were run with --convolve-iterations=50000000.
TEST(SincResamplerTest, ConvolveBenchmark) {
  // Initialize a dummy resampler.
  MockSource mock_source;
  SincResampler resampler(
      kSampleRateRatio,
      base::Bind(&MockSource::ProvideInput, base::Unretained(&mock_source)));

  // Retrieve benchmark iterations from command line.
  int convolve_iterations = 10;
  std::string iterations(CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      kConvolveIterations));
  if (!iterations.empty())
    base::StringToInt(iterations, &convolve_iterations);

  printf("Benchmarking %d iterations:\n", convolve_iterations);

  // Benchmark Convolve_C().
  base::TimeTicks start = base::TimeTicks::HighResNow();
  for (int i = 0; i < convolve_iterations; ++i) {
    resampler.Convolve_C(
        resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
        resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  }
  double total_time_c_ms =
      (base::TimeTicks::HighResNow() - start).InMillisecondsF();
  printf("Convolve_C took %.2fms.\n", total_time_c_ms);

#if defined(CONVOLVE_FUNC)
#if defined(ARCH_CPU_X86_FAMILY)
  ASSERT_TRUE(base::CPU().has_sse());
#endif

  // Benchmark with unaligned input pointer.
  start = base::TimeTicks::HighResNow();
  for (int j = 0; j < convolve_iterations; ++j) {
    resampler.CONVOLVE_FUNC(
        resampler.kernel_storage_.get() + 1, resampler.kernel_storage_.get(),
        resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  }
  double total_time_optimized_unaligned_ms =
      (base::TimeTicks::HighResNow() - start).InMillisecondsF();
  printf(STRINGIZE(CONVOLVE_FUNC) "(unaligned) took %.2fms; which is %.2fx "
         "faster than Convolve_C.\n", total_time_optimized_unaligned_ms,
         total_time_c_ms / total_time_optimized_unaligned_ms);

  // Benchmark with aligned input pointer.
  start = base::TimeTicks::HighResNow();
  for (int j = 0; j < convolve_iterations; ++j) {
    resampler.CONVOLVE_FUNC(
        resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
        resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  }
  double total_time_optimized_aligned_ms =
      (base::TimeTicks::HighResNow() - start).InMillisecondsF();
  printf(STRINGIZE(CONVOLVE_FUNC) " (aligned) took %.2fms; which is %.2fx "
         "faster than Convolve_C and %.2fx faster than "
         STRINGIZE(CONVOLVE_FUNC) " (unaligned).\n",
         total_time_optimized_aligned_ms,
         total_time_c_ms / total_time_optimized_aligned_ms,
         total_time_optimized_unaligned_ms / total_time_optimized_aligned_ms);
#endif
}

#undef CONVOLVE_FUNC

// Fake audio source for testing the resampler.  Generates a sinusoidal linear
// chirp (http://en.wikipedia.org/wiki/Chirp) which can be tuned to stress the
// resampler for the specific sample rate conversion being used.
class SinusoidalLinearChirpSource {
 public:
  SinusoidalLinearChirpSource(int sample_rate, int samples,
                              double max_frequency)
      : sample_rate_(sample_rate),
        total_samples_(samples),
        max_frequency_(max_frequency),
        current_index_(0) {
    // Chirp rate.
    double duration = static_cast<double>(total_samples_) / sample_rate_;
    k_ = (max_frequency_ - kMinFrequency) / duration;
  }

  virtual ~SinusoidalLinearChirpSource() {}

  void ProvideInput(float* destination, int frames) {
    for (int i = 0; i < frames; ++i, ++current_index_) {
      // Filter out frequencies higher than Nyquist.
      if (Frequency(current_index_) > 0.5 * sample_rate_) {
        destination[i] = 0;
      } else {
        // Calculate time in seconds.
        double t = static_cast<double>(current_index_) / sample_rate_;

        // Sinusoidal linear chirp.
        destination[i] = sin(2 * M_PI * (kMinFrequency * t + (k_ / 2) * t * t));
      }
    }
  }

  double Frequency(int position) {
    return kMinFrequency + position * (max_frequency_ - kMinFrequency)
        / total_samples_;
  }

 private:
  enum {
    kMinFrequency = 5
  };

  double sample_rate_;
  int total_samples_;
  double max_frequency_;
  double k_;
  int current_index_;

  DISALLOW_COPY_AND_ASSIGN(SinusoidalLinearChirpSource);
};

typedef std::tr1::tuple<int, int, double, double> SincResamplerTestData;
class SincResamplerTest
    : public testing::TestWithParam<SincResamplerTestData> {
 public:
  SincResamplerTest()
      : input_rate_(std::tr1::get<0>(GetParam())),
        output_rate_(std::tr1::get<1>(GetParam())),
        rms_error_(std::tr1::get<2>(GetParam())),
        low_freq_error_(std::tr1::get<3>(GetParam())) {
  }

  virtual ~SincResamplerTest() {}

 protected:
  int input_rate_;
  int output_rate_;
  double rms_error_;
  double low_freq_error_;
};

// Tests resampling using a given input and output sample rate.
TEST_P(SincResamplerTest, Resample) {
  // Make comparisons using one second of data.
  static const double kTestDurationSecs = 1;
  int input_samples = kTestDurationSecs * input_rate_;
  int output_samples = kTestDurationSecs * output_rate_;

  // Nyquist frequency for the input sampling rate.
  double input_nyquist_freq = 0.5 * input_rate_;

  // Source for data to be resampled.
  SinusoidalLinearChirpSource resampler_source(
      input_rate_, input_samples, input_nyquist_freq);

  SincResampler resampler(
      input_rate_ / static_cast<double>(output_rate_),
      base::Bind(&SinusoidalLinearChirpSource::ProvideInput,
                 base::Unretained(&resampler_source)));

  // TODO(dalecurtis): If we switch to AVX/SSE optimization, we'll need to
  // allocate these on 32-byte boundaries and ensure they're sized % 32 bytes.
  scoped_array<float> resampled_destination(new float[output_samples]);
  scoped_array<float> pure_destination(new float[output_samples]);

  // Generate resampled signal.
  resampler.Resample(resampled_destination.get(), output_samples);

  // Generate pure signal.
  SinusoidalLinearChirpSource pure_source(
      output_rate_, output_samples, input_nyquist_freq);
  pure_source.ProvideInput(pure_destination.get(), output_samples);

  // Range of the Nyquist frequency (0.5 * min(input rate, output_rate)) which
  // we refer to as low and high.
  static const double kLowFrequencyNyquistRange = 0.7;
  static const double kHighFrequencyNyquistRange = 0.9;

  // Calculate Root-Mean-Square-Error and maximum error for the resampling.
  double sum_of_squares = 0;
  double low_freq_max_error = 0;
  double high_freq_max_error = 0;
  int minimum_rate = std::min(input_rate_, output_rate_);
  double low_frequency_range = kLowFrequencyNyquistRange * 0.5 * minimum_rate;
  double high_frequency_range = kHighFrequencyNyquistRange * 0.5 * minimum_rate;
  for (int i = 0; i < output_samples; ++i) {
    double error = fabs(resampled_destination[i] - pure_destination[i]);

    if (pure_source.Frequency(i) < low_frequency_range) {
      if (error > low_freq_max_error)
        low_freq_max_error = error;
    } else if (pure_source.Frequency(i) < high_frequency_range) {
      if (error > high_freq_max_error)
        high_freq_max_error = error;
    }
    // TODO(dalecurtis): Sanity check frequencies > kHighFrequencyNyquistRange.

    sum_of_squares += error * error;
  }

  double rms_error = sqrt(sum_of_squares / output_samples);

  // Convert each error to dbFS.
  #define DBFS(x) 20 * log10(x)
  rms_error = DBFS(rms_error);
  low_freq_max_error = DBFS(low_freq_max_error);
  high_freq_max_error = DBFS(high_freq_max_error);

  EXPECT_LE(rms_error, rms_error_);
  EXPECT_LE(low_freq_max_error, low_freq_error_);

  // All conversions currently have a high frequency error around -6 dbFS.
  static const double kHighFrequencyMaxError = -6.02;
  EXPECT_LE(high_freq_max_error, kHighFrequencyMaxError);
}

// Almost all conversions have an RMS error of around -14 dbFS.
static const double kResamplingRMSError = -14.58;

// Thresholds chosen arbitrarily based on what each resampling reported during
// testing.  All thresholds are in dbFS, http://en.wikipedia.org/wiki/DBFS.
INSTANTIATE_TEST_CASE_P(
    SincResamplerTest, SincResamplerTest, testing::Values(
        // To 44.1kHz
        std::tr1::make_tuple(8000, 44100, kResamplingRMSError, -62.73),
        std::tr1::make_tuple(11025, 44100, kResamplingRMSError, -72.19),
        std::tr1::make_tuple(16000, 44100, kResamplingRMSError, -62.54),
        std::tr1::make_tuple(22050, 44100, kResamplingRMSError, -73.53),
        std::tr1::make_tuple(32000, 44100, kResamplingRMSError, -63.32),
        std::tr1::make_tuple(44100, 44100, kResamplingRMSError, -73.53),
        std::tr1::make_tuple(48000, 44100, -15.01, -64.04),
        std::tr1::make_tuple(96000, 44100, -18.49, -25.51),
        std::tr1::make_tuple(192000, 44100, -20.50, -13.31),

        // To 48kHz
        std::tr1::make_tuple(8000, 48000, kResamplingRMSError, -63.43),
        std::tr1::make_tuple(11025, 48000, kResamplingRMSError, -62.61),
        std::tr1::make_tuple(16000, 48000, kResamplingRMSError, -63.96),
        std::tr1::make_tuple(22050, 48000, kResamplingRMSError, -62.42),
        std::tr1::make_tuple(32000, 48000, kResamplingRMSError, -64.04),
        std::tr1::make_tuple(44100, 48000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 48000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(96000, 48000, -18.40, -28.44),
        std::tr1::make_tuple(192000, 48000, -20.43, -14.11),

        // To 96kHz
        std::tr1::make_tuple(8000, 96000, kResamplingRMSError, -63.19),
        std::tr1::make_tuple(11025, 96000, kResamplingRMSError, -62.61),
        std::tr1::make_tuple(16000, 96000, kResamplingRMSError, -63.39),
        std::tr1::make_tuple(22050, 96000, kResamplingRMSError, -62.42),
        std::tr1::make_tuple(32000, 96000, kResamplingRMSError, -63.95),
        std::tr1::make_tuple(44100, 96000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 96000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(96000, 96000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(192000, 96000, kResamplingRMSError, -28.41),

        // To 192kHz
        std::tr1::make_tuple(8000, 192000, kResamplingRMSError, -63.10),
        std::tr1::make_tuple(11025, 192000, kResamplingRMSError, -62.61),
        std::tr1::make_tuple(16000, 192000, kResamplingRMSError, -63.14),
        std::tr1::make_tuple(22050, 192000, kResamplingRMSError, -62.42),
        std::tr1::make_tuple(32000, 192000, kResamplingRMSError, -63.38),
        std::tr1::make_tuple(44100, 192000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 192000, kResamplingRMSError, -73.44),
        std::tr1::make_tuple(96000, 192000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(192000, 192000, kResamplingRMSError, -73.52)));

}  // namespace media
