// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/scoped_ptr.h"
#include "media/base/data_buffer.h"
#include "media/base/mock_ffmpeg.h"
#include "media/base/mock_task.h"
#include "media/filters/ffmpeg_video_decode_engine.h"
#include "media/filters/ffmpeg_video_decoder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::SetArgumentPointee;
using ::testing::StrictMock;

namespace media {

static const int kWidth = 320;
static const int kHeight = 240;

class FFmpegVideoDecodeEngineTest : public testing::Test {
 protected:
  FFmpegVideoDecodeEngineTest() {
    // Setup FFmpeg structures.
    frame_buffer_.reset(new uint8[kWidth * kHeight]);
    memset(&yuv_frame_, 0, sizeof(yuv_frame_));
    // DecodeFrame will check these pointers as non-NULL value.
    yuv_frame_.data[0] = yuv_frame_.data[1] = yuv_frame_.data[2]
        = frame_buffer_.get();
    yuv_frame_.linesize[0] = kWidth;
    yuv_frame_.linesize[1] = yuv_frame_.linesize[2] = kWidth >> 1;
    memset(&codec_context_, 0, sizeof(codec_context_));
    codec_context_.width = kWidth;
    codec_context_.height = kHeight;
    memset(&codec_, 0, sizeof(codec_));
    memset(&stream_, 0, sizeof(stream_));
    stream_.codec = &codec_context_;

    buffer_ = new DataBuffer(1);

    // Initialize MockFFmpeg.
    MockFFmpeg::set(&mock_ffmpeg_);

    test_engine_.reset(new FFmpegVideoDecodeEngine());
    test_engine_->SetCodecContextForTest(&codec_context_);
  }

  ~FFmpegVideoDecodeEngineTest() {
    test_engine_.reset(NULL);
    MockFFmpeg::set(NULL);
  }

  void Initialize() {
    EXPECT_CALL(*MockFFmpeg::get(), AVCodecFindDecoder(CODEC_ID_NONE))
        .WillOnce(Return(&codec_));
    EXPECT_CALL(*MockFFmpeg::get(), AVCodecAllocFrame())
        .WillOnce(Return(&yuv_frame_));
    EXPECT_CALL(*MockFFmpeg::get(), AVCodecThreadInit(&codec_context_, 2))
        .WillOnce(Return(0));
    EXPECT_CALL(*MockFFmpeg::get(), AVCodecOpen(&codec_context_, &codec_))
        .WillOnce(Return(0));
    EXPECT_CALL(*MockFFmpeg::get(), AVFree(&yuv_frame_))
        .Times(1);

    TaskMocker done_cb;
    EXPECT_CALL(done_cb, Run());
    test_engine_->Initialize(&stream_, done_cb.CreateTask());
    EXPECT_EQ(VideoDecodeEngine::kNormal, test_engine_->state());
  }

  scoped_ptr<FFmpegVideoDecodeEngine> test_engine_;
  scoped_array<uint8_t> frame_buffer_;
  StrictMock<MockFFmpeg> mock_ffmpeg_;

  AVFrame yuv_frame_;
  AVCodecContext codec_context_;
  AVStream stream_;
  AVCodec codec_;
  scoped_refptr<DataBuffer> buffer_;
};

TEST_F(FFmpegVideoDecodeEngineTest, Construction) {
  FFmpegVideoDecodeEngine engine;
  EXPECT_FALSE(engine.codec_context());
  EXPECT_EQ(FFmpegVideoDecodeEngine::kCreated, engine.state());
}

TEST_F(FFmpegVideoDecodeEngineTest, Initialize_Normal) {
  Initialize();
}

TEST_F(FFmpegVideoDecodeEngineTest, Initialize_FindDecoderFails) {
  // Test avcodec_find_decoder() returning NULL.
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecFindDecoder(CODEC_ID_NONE))
      .WillOnce(ReturnNull());
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecAllocFrame())
      .WillOnce(Return(&yuv_frame_));
  EXPECT_CALL(*MockFFmpeg::get(), AVFree(&yuv_frame_))
      .Times(1);

  TaskMocker done_cb;
  EXPECT_CALL(done_cb, Run());

  test_engine_->Initialize(&stream_, done_cb.CreateTask());
  EXPECT_EQ(VideoDecodeEngine::kError, test_engine_->state());
}

TEST_F(FFmpegVideoDecodeEngineTest, Initialize_InitThreadFails) {
  // Test avcodec_thread_init() failing.
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecFindDecoder(CODEC_ID_NONE))
      .WillOnce(Return(&codec_));
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecAllocFrame())
      .WillOnce(Return(&yuv_frame_));
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecThreadInit(&codec_context_, 2))
      .WillOnce(Return(-1));
  EXPECT_CALL(*MockFFmpeg::get(), AVFree(&yuv_frame_))
      .Times(1);

  TaskMocker done_cb;
  EXPECT_CALL(done_cb, Run());

  test_engine_->Initialize(&stream_, done_cb.CreateTask());
  EXPECT_EQ(VideoDecodeEngine::kError, test_engine_->state());
}

TEST_F(FFmpegVideoDecodeEngineTest, Initialize_OpenDecoderFails) {
  // Test avcodec_open() failing.
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecFindDecoder(CODEC_ID_NONE))
      .WillOnce(Return(&codec_));
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecAllocFrame())
      .WillOnce(Return(&yuv_frame_));
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecThreadInit(&codec_context_, 2))
      .WillOnce(Return(0));
  EXPECT_CALL(*MockFFmpeg::get(), AVCodecOpen(&codec_context_, &codec_))
      .WillOnce(Return(-1));
  EXPECT_CALL(*MockFFmpeg::get(), AVFree(&yuv_frame_))
      .Times(1);

  TaskMocker done_cb;
  EXPECT_CALL(done_cb, Run());

  test_engine_->Initialize(&stream_, done_cb.CreateTask());
  EXPECT_EQ(VideoDecodeEngine::kError, test_engine_->state());
}

TEST_F(FFmpegVideoDecodeEngineTest, DecodeFrame_Normal) {
  Initialize();

  // Expect a bunch of avcodec calls.
  EXPECT_CALL(mock_ffmpeg_, AVInitPacket(_));
  EXPECT_CALL(mock_ffmpeg_,
              AVCodecDecodeVideo2(&codec_context_, &yuv_frame_, _, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(1),  // Simulate 1 byte frame.
                      Return(0)));

  TaskMocker done_cb;
  EXPECT_CALL(done_cb, Run());

  bool got_result;
  scoped_refptr<VideoFrame> video_frame;
  test_engine_->DecodeFrame(buffer_, &video_frame, &got_result,
                            done_cb.CreateTask());
  EXPECT_TRUE(got_result);
}

TEST_F(FFmpegVideoDecodeEngineTest, DecodeFrame_0ByteFrame) {
  Initialize();

  // Expect a bunch of avcodec calls.
  EXPECT_CALL(mock_ffmpeg_, AVInitPacket(_));
  EXPECT_CALL(mock_ffmpeg_,
              AVCodecDecodeVideo2(&codec_context_, &yuv_frame_, _, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(0),  // Simulate 0 byte frame.
                      Return(0)));

  TaskMocker done_cb;
  EXPECT_CALL(done_cb, Run());

  bool got_result;
  scoped_refptr<VideoFrame> video_frame;
  test_engine_->DecodeFrame(buffer_, &video_frame, &got_result,
                            done_cb.CreateTask());
  EXPECT_FALSE(got_result);
}

TEST_F(FFmpegVideoDecodeEngineTest, DecodeFrame_DecodeError) {
  Initialize();

  // Expect a bunch of avcodec calls.
  EXPECT_CALL(mock_ffmpeg_, AVInitPacket(_));
  EXPECT_CALL(mock_ffmpeg_,
              AVCodecDecodeVideo2(&codec_context_, &yuv_frame_, _, _))
      .WillOnce(Return(-1));

  TaskMocker done_cb;
  EXPECT_CALL(done_cb, Run());

  bool got_result;
  scoped_refptr<VideoFrame> video_frame;
  test_engine_->DecodeFrame(buffer_, &video_frame, &got_result,
                            done_cb.CreateTask());
  EXPECT_FALSE(got_result);
}

TEST_F(FFmpegVideoDecodeEngineTest, GetSurfaceFormat) {
  // YV12 formats.
  codec_context_.pix_fmt = PIX_FMT_YUV420P;
  EXPECT_EQ(VideoFrame::YV12, test_engine_->GetSurfaceFormat());
  codec_context_.pix_fmt = PIX_FMT_YUVJ420P;
  EXPECT_EQ(VideoFrame::YV12, test_engine_->GetSurfaceFormat());

  // YV16 formats.
  codec_context_.pix_fmt = PIX_FMT_YUV422P;
  EXPECT_EQ(VideoFrame::YV16, test_engine_->GetSurfaceFormat());
  codec_context_.pix_fmt = PIX_FMT_YUVJ422P;
  EXPECT_EQ(VideoFrame::YV16, test_engine_->GetSurfaceFormat());

  // Invalid value.
  codec_context_.pix_fmt = PIX_FMT_NONE;
  EXPECT_EQ(VideoFrame::INVALID, test_engine_->GetSurfaceFormat());
}

}  // namespace media
