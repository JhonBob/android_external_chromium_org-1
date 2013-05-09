// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CRYPTO_QUIC_ENCRYPTER_H_
#define NET_QUIC_CRYPTO_QUIC_ENCRYPTER_H_

#include "net/base/net_export.h"
#include "net/quic/crypto/crypto_protocol.h"
#include "net/quic/quic_protocol.h"

namespace net {

class NET_EXPORT_PRIVATE QuicEncrypter {
 public:
  virtual ~QuicEncrypter() {}

  static QuicEncrypter* Create(QuicTag algorithm);

  // Sets the encryption key. Returns true on success, false on failure.
  //
  // NOTE: The key is the client_write_key or server_write_key derived from
  // the master secret.
  virtual bool SetKey(base::StringPiece key) = 0;

  // Sets the fixed initial bytes of the nonce. Returns true on success,
  // false on failure.
  //
  // NOTE: The nonce prefix is the client_write_iv or server_write_iv
  // derived from the master secret. A 64-bit packet sequence number will
  // be appended to form the nonce.
  //
  //                          <------------ 64 bits ----------->
  //   +---------------------+----------------------------------+
  //   |    Fixed prefix     |      Packet sequence number      |
  //   +---------------------+----------------------------------+
  //                          Nonce format
  //
  // The security of the nonce format requires that QUIC never reuse a
  // packet sequence number, even when retransmitting a lost packet.
  virtual bool SetNoncePrefix(base::StringPiece nonce_prefix) = 0;

  // Encrypt encrypts |plaintext| and writes the ciphertext, plus a MAC over
  // both |associated_data| and |plaintext| to |output|, using |nonce| as the
  // nonce. |nonce| must be |8+GetNoncePrefixSize()| bytes long and |output|
  // must point to a buffer that is at least
  // |GetCiphertextSize(plaintext.size()| bytes long.
  virtual bool Encrypt(base::StringPiece nonce,
                       base::StringPiece associated_data,
                       base::StringPiece plaintext,
                       unsigned char* output) = 0;

  // Returns a newly created QuicData object containing the encrypted
  // |plaintext| as well as a MAC over both |plaintext| and |associated_data|,
  // or NULL if there is an error. |sequence_number| is appended to the
  // |nonce_prefix| value provided in SetNoncePrefix() to form the nonce.
  virtual QuicData* EncryptPacket(QuicPacketSequenceNumber sequence_number,
                                  base::StringPiece associated_data,
                                  base::StringPiece plaintext) = 0;

  // GetKeySize() and GetNoncePrefixSize() tell the HKDF class how many bytes
  // of key material needs to be derived from the master secret.
  // NOTE: the sizes returned by GetKeySize() and GetNoncePrefixSize() are
  // also correct for the QuicDecrypter of the same algorithm. So only
  // QuicEncrypter has these two methods.

  // Returns the size in bytes of a key for the algorithm.
  virtual size_t GetKeySize() const = 0;
  // Returns the size in bytes of the fixed initial part of the nonce.
  virtual size_t GetNoncePrefixSize() const = 0;

  // Returns the maximum length of plaintext that can be encrypted
  // to ciphertext no larger than |ciphertext_size|.
  virtual size_t GetMaxPlaintextSize(size_t ciphertext_size) const = 0;

  // Returns the length of the ciphertext that would be generated by encrypting
  // to plaintext of size |plaintext_size|.
  virtual size_t GetCiphertextSize(size_t plaintext_size) const = 0;

  // For use by unit tests only.
  virtual base::StringPiece GetKey() const = 0;
  virtual base::StringPiece GetNoncePrefix() const = 0;
};

}  // namespace net

#endif  // NET_QUIC_CRYPTO_QUIC_ENCRYPTER_H_
