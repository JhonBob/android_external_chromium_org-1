// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_http_stream.h"

#include "crypto/ec_private_key.h"
#include "crypto/ec_signature_creator.h"
#include "crypto/signature_creator.h"
#include "net/base/asn1_util.h"
#include "net/base/default_origin_bound_cert_store.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/spdy/spdy_session.h"
#include "net/spdy/spdy_test_util_spdy2.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace net::test_spdy2;

namespace net {

class SpdyHttpStreamSpdy2Test : public testing::Test {
 public:
  OrderedSocketData* data() { return data_.get(); }
 protected:
  SpdyHttpStreamSpdy2Test() {}

  void EnableCompression(bool enabled) {
    spdy::SpdyFramer::set_enable_compression_default(enabled);
  }

  virtual void SetUp() {
    SpdySession::set_default_protocol(SSLClientSocket::kProtoSPDY2);
  }

  virtual void TearDown() {
    crypto::ECSignatureCreator::SetFactoryForTesting(NULL);
    MessageLoop::current()->RunAllPending();
  }

  int InitSession(MockRead* reads, size_t reads_count,
                  MockWrite* writes, size_t writes_count,
                  HostPortPair& host_port_pair) {
    HostPortProxyPair pair(host_port_pair, ProxyServer::Direct());
    data_.reset(new OrderedSocketData(reads, reads_count,
                                      writes, writes_count));
    session_deps_.socket_factory->AddSocketDataProvider(data_.get());
    http_session_ = SpdySessionDependencies::SpdyCreateSession(&session_deps_);
    session_ = http_session_->spdy_session_pool()->Get(pair, BoundNetLog());
    transport_params_ = new TransportSocketParams(host_port_pair,
                                      MEDIUM, false, false);
    TestCompletionCallback callback;
    scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
    EXPECT_EQ(ERR_IO_PENDING,
              connection->Init(host_port_pair.ToString(),
                               transport_params_,
                               MEDIUM,
                               callback.callback(),
                               http_session_->GetTransportSocketPool(
                                   HttpNetworkSession::NORMAL_SOCKET_POOL),
                               BoundNetLog()));
    EXPECT_EQ(OK, callback.WaitForResult());
    return session_->InitializeWithSocket(connection.release(), false, OK);
  }

  void TestSendCredentials(
    OriginBoundCertService* obc_service,
    const std::string& cert,
    const std::string& proof,
    SSLClientCertType type);

  SpdySessionDependencies session_deps_;
  scoped_ptr<OrderedSocketData> data_;
  scoped_refptr<HttpNetworkSession> http_session_;
  scoped_refptr<SpdySession> session_;
  scoped_refptr<TransportSocketParams> transport_params_;
};

TEST_F(SpdyHttpStreamSpdy2Test, SendRequest) {
  EnableCompression(false);

  scoped_ptr<spdy::SpdyFrame> req(ConstructSpdyGet(NULL, 0, false, 1, LOWEST));
  MockWrite writes[] = {
    CreateMockWrite(*req.get(), 1),
  };
  scoped_ptr<spdy::SpdyFrame> resp(ConstructSpdyGetSynReply(NULL, 0, 1));
  MockRead reads[] = {
    CreateMockRead(*resp, 2),
    MockRead(SYNCHRONOUS, 0, 3)  // EOF
  };

  HostPortPair host_port_pair("www.google.com", 80);
  HostPortProxyPair pair(host_port_pair, ProxyServer::Direct());
  EXPECT_EQ(OK, InitSession(reads, arraysize(reads), writes, arraysize(writes),
      host_port_pair));

  HttpRequestInfo request;
  request.method = "GET";
  request.url = GURL("http://www.google.com/");
  TestCompletionCallback callback;
  HttpResponseInfo response;
  HttpRequestHeaders headers;
  BoundNetLog net_log;
  scoped_ptr<SpdyHttpStream> http_stream(
      new SpdyHttpStream(session_.get(), true));
  ASSERT_EQ(
      OK,
      http_stream->InitializeStream(&request, net_log, CompletionCallback()));

  EXPECT_EQ(ERR_IO_PENDING, http_stream->SendRequest(headers, NULL, &response,
                                                     callback.callback()));
  EXPECT_TRUE(http_session_->spdy_session_pool()->HasSession(pair));

  // This triggers the MockWrite and read 2
  callback.WaitForResult();

  // This triggers read 3. The empty read causes the session to shut down.
  data()->CompleteRead();

  // Because we abandoned the stream, we don't expect to find a session in the
  // pool anymore.
  EXPECT_FALSE(http_session_->spdy_session_pool()->HasSession(pair));
  EXPECT_TRUE(data()->at_read_eof());
  EXPECT_TRUE(data()->at_write_eof());
}

TEST_F(SpdyHttpStreamSpdy2Test, SendChunkedPost) {
  EnableCompression(false);
  UploadDataStream::set_merge_chunks(false);

  scoped_ptr<spdy::SpdyFrame> req(ConstructChunkedSpdyPost(NULL, 0));
  scoped_ptr<spdy::SpdyFrame> chunk1(ConstructSpdyBodyFrame(1, false));
  scoped_ptr<spdy::SpdyFrame> chunk2(ConstructSpdyBodyFrame(1, true));
  MockWrite writes[] = {
    CreateMockWrite(*req.get(), 1),
    CreateMockWrite(*chunk1, 2),  // POST upload frames
    CreateMockWrite(*chunk2, 3),
  };
  scoped_ptr<spdy::SpdyFrame> resp(ConstructSpdyPostSynReply(NULL, 0));
  MockRead reads[] = {
    CreateMockRead(*resp, 4),
    CreateMockRead(*chunk1, 5),
    CreateMockRead(*chunk2, 5),
    MockRead(SYNCHRONOUS, 0, 6)  // EOF
  };

  HostPortPair host_port_pair("www.google.com", 80);
  HostPortProxyPair pair(host_port_pair, ProxyServer::Direct());
  EXPECT_EQ(OK, InitSession(reads, arraysize(reads), writes, arraysize(writes),
                            host_port_pair));

  HttpRequestInfo request;
  request.method = "POST";
  request.url = GURL("http://www.google.com/");
  request.upload_data = new UploadData();
  request.upload_data->set_is_chunked(true);
  request.upload_data->AppendChunk(kUploadData, kUploadDataSize, false);
  request.upload_data->AppendChunk(kUploadData, kUploadDataSize, true);
  TestCompletionCallback callback;
  HttpResponseInfo response;
  HttpRequestHeaders headers;
  BoundNetLog net_log;
  SpdyHttpStream http_stream(session_.get(), true);
  ASSERT_EQ(
      OK,
      http_stream.InitializeStream(&request, net_log, CompletionCallback()));

  // http_stream.SendRequest() will take ownership of upload_stream.
  UploadDataStream* upload_stream = new UploadDataStream(request.upload_data);
  ASSERT_EQ(OK, upload_stream->Init());
  EXPECT_EQ(ERR_IO_PENDING, http_stream.SendRequest(
      headers, upload_stream, &response, callback.callback()));
  EXPECT_TRUE(http_session_->spdy_session_pool()->HasSession(pair));

  // This triggers the MockWrite and read 2
  callback.WaitForResult();

  // This triggers read 3. The empty read causes the session to shut down.
  data()->CompleteRead();
  MessageLoop::current()->RunAllPending();

  // Because we abandoned the stream, we don't expect to find a session in the
  // pool anymore.
  EXPECT_FALSE(http_session_->spdy_session_pool()->HasSession(pair));
  EXPECT_TRUE(data()->at_read_eof());
  EXPECT_TRUE(data()->at_write_eof());
}

// Test case for bug: http://code.google.com/p/chromium/issues/detail?id=50058
TEST_F(SpdyHttpStreamSpdy2Test, SpdyURLTest) {
  EnableCompression(false);

  const char * const full_url = "http://www.google.com/foo?query=what#anchor";
  const char * const base_url = "http://www.google.com/foo?query=what";
  scoped_ptr<spdy::SpdyFrame> req(ConstructSpdyGet(base_url, false, 1, LOWEST));
  MockWrite writes[] = {
    CreateMockWrite(*req.get(), 1),
  };
  scoped_ptr<spdy::SpdyFrame> resp(ConstructSpdyGetSynReply(NULL, 0, 1));
  MockRead reads[] = {
    CreateMockRead(*resp, 2),
    MockRead(SYNCHRONOUS, 0, 3)  // EOF
  };

  HostPortPair host_port_pair("www.google.com", 80);
  HostPortProxyPair pair(host_port_pair, ProxyServer::Direct());
  EXPECT_EQ(OK, InitSession(reads, arraysize(reads), writes, arraysize(writes),
      host_port_pair));

  HttpRequestInfo request;
  request.method = "GET";
  request.url = GURL(full_url);
  TestCompletionCallback callback;
  HttpResponseInfo response;
  HttpRequestHeaders headers;
  BoundNetLog net_log;
  scoped_ptr<SpdyHttpStream> http_stream(new SpdyHttpStream(session_, true));
  ASSERT_EQ(
      OK,
      http_stream->InitializeStream(&request, net_log, CompletionCallback()));

  EXPECT_EQ(ERR_IO_PENDING, http_stream->SendRequest(headers, NULL, &response,
                                                     callback.callback()));

  spdy::SpdyHeaderBlock* spdy_header =
    http_stream->stream()->spdy_headers().get();
  EXPECT_TRUE(spdy_header != NULL);
  if (spdy_header->find("url") != spdy_header->end())
    EXPECT_EQ("/foo?query=what", spdy_header->find("url")->second);
  else
    FAIL() << "No url is set in spdy_header!";

  // This triggers the MockWrite and read 2
  callback.WaitForResult();

  // This triggers read 3. The empty read causes the session to shut down.
  data()->CompleteRead();

  // Because we abandoned the stream, we don't expect to find a session in the
  // pool anymore.
  EXPECT_FALSE(http_session_->spdy_session_pool()->HasSession(pair));
  EXPECT_TRUE(data()->at_read_eof());
  EXPECT_TRUE(data()->at_write_eof());
}

namespace {

void GetECOriginBoundCertAndProof(const std::string& origin,
                                OriginBoundCertService* obc_service,
                                std::string* cert,
                                std::string* proof) {
  TestCompletionCallback callback;
  std::vector<uint8> requested_cert_types;
  requested_cert_types.push_back(CLIENT_CERT_ECDSA_SIGN);
  SSLClientCertType cert_type;
  std::string key;
  OriginBoundCertService::RequestHandle request_handle;
  int rv = obc_service->GetOriginBoundCert(origin, requested_cert_types,
                                           &cert_type, &key, cert,
                                           callback.callback(),
                                           &request_handle);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(OK, callback.WaitForResult());
  EXPECT_EQ(CLIENT_CERT_ECDSA_SIGN, cert_type);

  unsigned char secret[32];
  memset(secret, 'A', arraysize(secret));

  // Convert the key string into a vector<unit8>
  std::vector<uint8> key_data(key.begin(), key.end());

  base::StringPiece spki_piece;
  ASSERT_TRUE(asn1::ExtractSPKIFromDERCert(*cert, &spki_piece));
  std::vector<uint8> spki(spki_piece.data(),
                          spki_piece.data() + spki_piece.size());

  std::vector<uint8> proof_data;
  scoped_ptr<crypto::ECPrivateKey> private_key(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          OriginBoundCertService::kEPKIPassword, key_data, spki));
  scoped_ptr<crypto::ECSignatureCreator> creator(
      crypto::ECSignatureCreator::Create(private_key.get()));
  creator->Sign(secret, arraysize(secret), &proof_data);
  proof->assign(proof_data.begin(), proof_data.end());
}

}  // namespace

// TODO(rch): When openssl supports origin bound certifictes, this
// guard can be removed
#if !defined(USE_OPENSSL)
// Test that if we request a resource for a new origin on a session that
// used origin bound certificates, that we send a CREDENTIAL frame for
// the new origin before we send the new request.
void SpdyHttpStreamSpdy2Test::TestSendCredentials(
    OriginBoundCertService* obc_service,
    const std::string& cert,
    const std::string& proof,
    SSLClientCertType type) {
  EnableCompression(false);

  spdy::SpdyCredential cred;
  cred.slot = 1;
  cred.proof = proof;
  cred.certs.push_back(cert);

  scoped_ptr<spdy::SpdyFrame> req(ConstructSpdyGet(NULL, 0, false, 1, LOWEST));
  scoped_ptr<spdy::SpdyFrame> credential(ConstructSpdyCredential(cred));
  scoped_ptr<spdy::SpdyFrame> req2(ConstructSpdyGet("http://www.gmail.com",
                                                    false, 3, LOWEST));
  MockWrite writes[] = {
    CreateMockWrite(*req.get(), 0),
    CreateMockWrite(*credential.get(), 2),
    CreateMockWrite(*req2.get(), 3),
  };

  scoped_ptr<spdy::SpdyFrame> resp(ConstructSpdyGetSynReply(NULL, 0, 1));
  scoped_ptr<spdy::SpdyFrame> resp2(ConstructSpdyGetSynReply(NULL, 0, 3));
  MockRead reads[] = {
    CreateMockRead(*resp, 1),
    CreateMockRead(*resp2, 4),
    MockRead(SYNCHRONOUS, 0, 5)  // EOF
  };

  HostPortPair host_port_pair("www.google.com", 80);
  HostPortProxyPair pair(host_port_pair, ProxyServer::Direct());

  DeterministicMockClientSocketFactory* socket_factory =
      session_deps_.deterministic_socket_factory.get();
  scoped_refptr<DeterministicSocketData> data(
      new DeterministicSocketData(reads, arraysize(reads),
                                  writes, arraysize(writes)));
  socket_factory->AddSocketDataProvider(data.get());
  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  ssl.origin_bound_cert_type = type;
  ssl.origin_bound_cert_service = obc_service;
  ssl.protocol_negotiated = SSLClientSocket::kProtoSPDY3;
  socket_factory->AddSSLSocketDataProvider(&ssl);
  http_session_ = SpdySessionDependencies::SpdyCreateSessionDeterministic(
      &session_deps_);
  session_ = http_session_->spdy_session_pool()->Get(pair, BoundNetLog());
  transport_params_ = new TransportSocketParams(host_port_pair,
                                                MEDIUM, false, false);
  TestCompletionCallback callback;
  scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
  SSLConfig ssl_config;
  scoped_refptr<SOCKSSocketParams> socks_params;
  scoped_refptr<HttpProxySocketParams> http_proxy_params;
  scoped_refptr<SSLSocketParams> ssl_params(
      new SSLSocketParams(transport_params_,
                          socks_params,
                          http_proxy_params,
                          ProxyServer::SCHEME_DIRECT,
                          host_port_pair,
                          ssl_config,
                          0,
                          false,
                          false));
  EXPECT_EQ(ERR_IO_PENDING,
            connection->Init(host_port_pair.ToString(),
                             ssl_params,
                             MEDIUM,
                             callback.callback(),
                             http_session_->GetSSLSocketPool(
                                 HttpNetworkSession::NORMAL_SOCKET_POOL),
                             BoundNetLog()));
  callback.WaitForResult();
  EXPECT_EQ(OK,
            session_->InitializeWithSocket(connection.release(), true, OK));

  HttpRequestInfo request;
  request.method = "GET";
  request.url = GURL("http://www.google.com/");
  HttpResponseInfo response;
  HttpRequestHeaders headers;
  BoundNetLog net_log;
  scoped_ptr<SpdyHttpStream> http_stream(
      new SpdyHttpStream(session_.get(), true));
  ASSERT_EQ(
      OK,
      http_stream->InitializeStream(&request, net_log, CompletionCallback()));

  EXPECT_FALSE(session_->NeedsCredentials(host_port_pair));
  HostPortPair new_host_port_pair("www.gmail.com", 80);
  EXPECT_TRUE(session_->NeedsCredentials(new_host_port_pair));

  EXPECT_EQ(ERR_IO_PENDING, http_stream->SendRequest(headers, NULL, &response,
                                                     callback.callback()));
  EXPECT_TRUE(http_session_->spdy_session_pool()->HasSession(pair));

  data->RunFor(2);
  callback.WaitForResult();

  // Start up second request for resource on a new origin.
  scoped_ptr<SpdyHttpStream> http_stream2(
      new SpdyHttpStream(session_.get(), true));
  request.url = GURL("http://www.gmail.com/");
  ASSERT_EQ(
      OK,
      http_stream2->InitializeStream(&request, net_log, CompletionCallback()));
  EXPECT_EQ(ERR_IO_PENDING, http_stream2->SendRequest(headers, NULL, &response,
                                                     callback.callback()));
  data->RunFor(2);
  callback.WaitForResult();

  EXPECT_EQ(ERR_IO_PENDING, http_stream2->ReadResponseHeaders(
      callback.callback()));
  data->RunFor(1);
  EXPECT_EQ(OK, callback.WaitForResult());
  ASSERT_TRUE(response.headers.get() != NULL);
  ASSERT_EQ(200, response.headers->response_code());
}

class MockECSignatureCreator : public crypto::ECSignatureCreator {
  public:
   explicit MockECSignatureCreator(crypto::ECPrivateKey* key) : key_(key) {}

   virtual bool Sign(const uint8* data,
                     int data_len,
                     std::vector<uint8>* signature) OVERRIDE {
     std::vector<uint8> private_key_value;
     key_->ExportValue(&private_key_value);
     std::string head = "fakesignature";
     std::string tail = "/fakesignature";

     signature->clear();
     signature->insert(signature->end(), head.begin(), head.end());
     signature->insert(signature->end(), private_key_value.begin(),
                       private_key_value.end());
     signature->insert(signature->end(), '-');
     signature->insert(signature->end(), data, data + data_len);
     signature->insert(signature->end(), tail.begin(), tail.end());
     return true;
   }

 private:
   crypto::ECPrivateKey* key_;
   DISALLOW_COPY_AND_ASSIGN(MockECSignatureCreator);
};

class MockECSignatureCreatorFactory : public crypto::ECSignatureCreatorFactory {
 public:
  MockECSignatureCreatorFactory() {}
  virtual ~MockECSignatureCreatorFactory() {}

  virtual crypto::ECSignatureCreator* Create(
      crypto::ECPrivateKey* key) OVERRIDE {
    return new MockECSignatureCreator(key);
  }
 private:
   DISALLOW_COPY_AND_ASSIGN(MockECSignatureCreatorFactory);
};

#endif  // !defined(USE_OPENSSL)

// TODO(willchan): Write a longer test for SpdyStream that exercises all
// methods.

}  // namespace net
