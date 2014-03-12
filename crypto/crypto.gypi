# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    # Put all transitive dependencies for Windows HMAC here.
    # This is required so that we can build them for nacl win64.
    'variables': {
      'hmac_win64_related_sources': [
        'hmac.cc',
        'hmac.h',
        'hmac_win.cc',
        'secure_util.cc',
        'secure_util.h',
        'symmetric_key.h',
        'symmetric_key_win.cc',
        'third_party/nss/chromium-sha256.h',
        'third_party/nss/sha512.cc',
      ],
    },
    'hmac_win64_related_sources': [ '<@(hmac_win64_related_sources)' ],
    'crypto_sources': [
      # NOTE: all transitive dependencies of HMAC on windows need
      #     to be placed in the source list above.
      '<@(hmac_win64_related_sources)',
      'apple_keychain.h',
      'apple_keychain_ios.mm',
      'apple_keychain_mac.mm',
      'capi_util.cc',
      'capi_util.h',
      'crypto_export.h',
      'cssm_init.cc',
      'cssm_init.h',
      'curve25519.cc',
      'curve25519.h',
      'curve25519-donna.c',
      'ghash.cc',
      'ghash.h',
      'ec_private_key.h',
      'ec_private_key_nss.cc',
      'ec_private_key_openssl.cc',
      'ec_signature_creator.cc',
      'ec_signature_creator.h',
      'ec_signature_creator_impl.h',
      'ec_signature_creator_nss.cc',
      'ec_signature_creator_openssl.cc',
      'encryptor.cc',
      'encryptor.h',
      'encryptor_nss.cc',
      'encryptor_openssl.cc',
      'hkdf.cc',
      'hkdf.h',
      'hmac_nss.cc',
      'hmac_openssl.cc',
      'mac_security_services_lock.cc',
      'mac_security_services_lock.h',
      'mock_apple_keychain.cc',
      'mock_apple_keychain.h',
      'mock_apple_keychain_ios.cc',
      'mock_apple_keychain_mac.cc',
      'p224_spake.cc',
      'p224_spake.h',
      'nss_crypto_module_delegate.h',
      'nss_util.cc',
      'nss_util.h',
      'nss_util_internal.h',
      'openpgp_symmetric_encryption.cc',
      'openpgp_symmetric_encryption.h',
      'openssl_util.cc',
      'openssl_util.h',
      'p224.cc',
      'p224.h',
      'random.h',
      'random.cc',
      'rsa_private_key.cc',
      'rsa_private_key.h',
      'rsa_private_key_nss.cc',
      'rsa_private_key_openssl.cc',
      'scoped_capi_types.h',
      'scoped_nss_types.h',
      'secure_hash.h',
      'secure_hash_default.cc',
      'secure_hash_openssl.cc',
      'sha2.cc',
      'sha2.h',
      'signature_creator.h',
      'signature_creator_nss.cc',
      'signature_creator_openssl.cc',
      'signature_verifier.h',
      'signature_verifier_nss.cc',
      'signature_verifier_openssl.cc',
      'symmetric_key_nss.cc',
      'symmetric_key_openssl.cc',
      'third_party/nss/chromium-blapi.h',
      'third_party/nss/chromium-blapit.h',
      'third_party/nss/chromium-nss.h',
      'third_party/nss/pk11akey.cc',
      'third_party/nss/rsawrapr.c',
      'third_party/nss/secsign.cc',
    ]
  }
}
