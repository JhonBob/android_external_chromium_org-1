// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_TEST_TOOLS_QUIC_TEST_UTILS_H_
#define NET_TOOLS_QUIC_TEST_TOOLS_QUIC_TEST_UTILS_H_

#include <string>

#include "base/strings/string_piece.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_packet_writer.h"
#include "net/quic/quic_session.h"
#include "net/quic/quic_spdy_decompressor.h"
#include "net/spdy/spdy_framer.h"
#include "net/tools/quic/quic_server_session.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace net {

class EpollServer;
class IPEndPoint;

namespace tools {
namespace test {

// Simple random number generator used to compute random numbers suitable
// for pseudo-randomly dropping packets in tests.  It works by computing
// the sha1 hash of the current seed, and using the first 64 bits as
// the next random number, and the next seed.
class SimpleRandom {
 public:
  SimpleRandom() : seed_(0) {}

  // Returns a random number in the range [0, kuint64max].
  uint64 RandUint64();

  void set_seed(uint64 seed) { seed_ = seed; }

 private:
  uint64 seed_;
};

class MockConnection : public QuicConnection {
 public:
  // Uses a QuicConnectionHelper created with fd and eps.
  MockConnection(QuicGuid guid,
                 IPEndPoint address,
                 int fd,
                 EpollServer* eps,
                 bool is_server);
  // Uses a MockHelper.
  MockConnection(QuicGuid guid, IPEndPoint address, bool is_server);
  MockConnection(QuicGuid guid,
                 IPEndPoint address,
                 QuicConnectionHelperInterface* helper,
                 QuicPacketWriter* writer,
                 bool is_server);
  virtual ~MockConnection();

  // If the constructor that uses a MockHelper has been used then this method
  // will advance the time of the MockClock.
  void AdvanceTime(QuicTime::Delta delta);

  MOCK_METHOD3(ProcessUdpPacket, void(const IPEndPoint& self_address,
                                      const IPEndPoint& peer_address,
                                      const QuicEncryptedPacket& packet));
  MOCK_METHOD1(SendConnectionClose, void(QuicErrorCode error));
  MOCK_METHOD2(SendConnectionCloseWithDetails, void(
      QuicErrorCode error,
      const std::string& details));
  MOCK_METHOD2(SendRstStream, void(QuicStreamId id,
                                   QuicRstStreamErrorCode error));
  MOCK_METHOD3(SendGoAway, void(QuicErrorCode error,
                                QuicStreamId last_good_stream_id,
                                const std::string& reason));
  MOCK_METHOD0(OnCanWrite, bool());

  void ReallyProcessUdpPacket(const IPEndPoint& self_address,
                              const IPEndPoint& peer_address,
                              const QuicEncryptedPacket& packet) {
    return QuicConnection::ProcessUdpPacket(self_address, peer_address, packet);
  }

  virtual bool OnProtocolVersionMismatch(QuicVersion version) { return false; }

 private:
  const bool has_mock_helper_;
  scoped_ptr<QuicPacketWriter> writer_;

  DISALLOW_COPY_AND_ASSIGN(MockConnection);
};

class TestSession : public QuicSession {
 public:
  TestSession(QuicConnection* connection,
              const QuicConfig& config,
              bool is_server);
  virtual ~TestSession();

  MOCK_METHOD1(CreateIncomingReliableStream,
               ReliableQuicStream*(QuicStreamId id));
  MOCK_METHOD0(CreateOutgoingReliableStream, ReliableQuicStream*());

  void SetCryptoStream(QuicCryptoStream* stream);

  virtual QuicCryptoStream* GetCryptoStream();

 private:
  QuicCryptoStream* crypto_stream_;
  DISALLOW_COPY_AND_ASSIGN(TestSession);
};

class MockPacketWriter : public QuicPacketWriter {
 public:
  MockPacketWriter();
  virtual ~MockPacketWriter();

  MOCK_METHOD5(WritePacket,
               WriteResult(const char* buffer,
                           size_t buf_len,
                           const IPAddressNumber& self_address,
                           const IPEndPoint& peer_address,
                           QuicBlockedWriterInterface* blocked_writer));
  MOCK_CONST_METHOD0(IsWriteBlockedDataBuffered, bool());
};

class MockQuicSessionOwner : public QuicSessionOwner {
 public:
  MockQuicSessionOwner();
  ~MockQuicSessionOwner();
  MOCK_METHOD2(OnConnectionClose, void(QuicGuid guid, QuicErrorCode error));
};

class TestDecompressorVisitor : public QuicSpdyDecompressor::Visitor {
 public:
  virtual ~TestDecompressorVisitor() {}
  virtual bool OnDecompressedData(base::StringPiece data) OVERRIDE;
  virtual void OnDecompressionError() OVERRIDE;

  std::string data() { return data_; }
  bool error() { return error_; }

 private:
  std::string data_;
  bool error_;
};

class MockAckNotifierDelegate : public QuicAckNotifier::DelegateInterface {
 public:
  MockAckNotifierDelegate();
  virtual ~MockAckNotifierDelegate();

  MOCK_METHOD0(OnAckNotification, void());
};

}  // namespace test
}  // namespace tools
}  // namespace net

#endif  // NET_TOOLS_QUIC_TEST_TOOLS_QUIC_TEST_UTILS_H_
