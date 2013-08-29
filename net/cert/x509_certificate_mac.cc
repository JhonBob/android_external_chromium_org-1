// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/x509_certificate.h"

#include <CommonCrypto/CommonDigest.h>
#include <CoreServices/CoreServices.h>
#include <Security/Security.h>

#include <cert.h>

#include <vector>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/memory/singleton.h"
#include "base/pickle.h"
#include "base/sha1.h"
#include "base/strings/string_piece.h"
#include "base/strings/sys_string_conversions.h"
#include "base/synchronization/lock.h"
#include "crypto/cssm_init.h"
#include "crypto/mac_security_services_lock.h"
#include "crypto/nss_util.h"
#include "net/cert/x509_util_mac.h"

using base::ScopedCFTypeRef;
using base::Time;

namespace net {

namespace {

void GetCertDistinguishedName(
    const x509_util::CSSMCachedCertificate& cached_cert,
    const CSSM_OID* oid,
    CertPrincipal* result) {
  x509_util::CSSMFieldValue distinguished_name;
  OSStatus status = cached_cert.GetField(oid, &distinguished_name);
  if (status || !distinguished_name.field())
    return;
  result->ParseDistinguishedName(distinguished_name.field()->Data,
                                 distinguished_name.field()->Length);
}

bool IsCertIssuerInEncodedList(X509Certificate::OSCertHandle cert_handle,
                               const std::vector<std::string>& issuers) {
  x509_util::CSSMCachedCertificate cached_cert;
  if (cached_cert.Init(cert_handle) != CSSM_OK)
    return false;

  x509_util::CSSMFieldValue distinguished_name;
  OSStatus status = cached_cert.GetField(&CSSMOID_X509V1IssuerNameStd,
                                         &distinguished_name);
  if (status || !distinguished_name.field())
    return false;

  base::StringPiece name_piece(
      reinterpret_cast<const char*>(distinguished_name.field()->Data),
      static_cast<size_t>(distinguished_name.field()->Length));

  for (std::vector<std::string>::const_iterator it = issuers.begin();
       it != issuers.end(); ++it) {
    base::StringPiece issuer_piece(*it);
    if (name_piece == issuer_piece)
      return true;
  }

  return false;
}

void GetCertDateForOID(const x509_util::CSSMCachedCertificate& cached_cert,
                       const CSSM_OID* oid,
                       Time* result) {
  *result = Time::Time();

  x509_util::CSSMFieldValue field;
  OSStatus status = cached_cert.GetField(oid, &field);
  if (status)
    return;

  const CSSM_X509_TIME* x509_time = field.GetAs<CSSM_X509_TIME>();
  if (x509_time->timeType != BER_TAG_UTC_TIME &&
      x509_time->timeType != BER_TAG_GENERALIZED_TIME) {
    LOG(ERROR) << "Unsupported date/time format "
               << x509_time->timeType;
    return;
  }

  base::StringPiece time_string(
      reinterpret_cast<const char*>(x509_time->time.Data),
      x509_time->time.Length);
  CertDateFormat format = x509_time->timeType == BER_TAG_UTC_TIME ?
      CERT_DATE_FORMAT_UTC_TIME : CERT_DATE_FORMAT_GENERALIZED_TIME;
  if (!ParseCertificateDate(time_string, format, result))
    LOG(ERROR) << "Invalid certificate date/time " << time_string;
}

std::string GetCertSerialNumber(
    const x509_util::CSSMCachedCertificate& cached_cert) {
  x509_util::CSSMFieldValue serial_number;
  OSStatus status = cached_cert.GetField(&CSSMOID_X509V1SerialNumber,
                                         &serial_number);
  if (status || !serial_number.field())
    return std::string();

  return std::string(
      reinterpret_cast<const char*>(serial_number.field()->Data),
      serial_number.field()->Length);
}

// Returns true if |purpose| is listed as allowed in |usage|. This
// function also considers the "Any" purpose. If the attribute is
// present and empty, we return false.
bool ExtendedKeyUsageAllows(const CE_ExtendedKeyUsage* usage,
                            const CSSM_OID* purpose) {
  for (unsigned p = 0; p < usage->numPurposes; ++p) {
    if (CSSMOIDEqual(&usage->purposes[p], purpose))
      return true;
    if (CSSMOIDEqual(&usage->purposes[p], &CSSMOID_ExtendedKeyUsageAny))
      return true;
  }
  return false;
}

// Test that a given |cert_handle| is actually a valid X.509 certificate, and
// return true if it is.
//
// On OS X, SecCertificateCreateFromData() does not return any errors if
// called with invalid data, as long as data is present. The actual decoding
// of the certificate does not happen until an API that requires a CSSM
// handle is called. While SecCertificateGetCLHandle is the most likely
// candidate, as it performs the parsing, it does not check whether the
// parsing was actually successful. Instead, SecCertificateGetSubject is
// used (supported since 10.3), as a means to check that the certificate
// parsed as a valid X.509 certificate.
bool IsValidOSCertHandle(SecCertificateRef cert_handle) {
  const CSSM_X509_NAME* sanity_check = NULL;
  OSStatus status = SecCertificateGetSubject(cert_handle, &sanity_check);
  return status == noErr && sanity_check;
}

// Parses |data| of length |length|, attempting to decode it as the specified
// |format|. If |data| is in the specified format, any certificates contained
// within are stored into |output|.
void AddCertificatesFromBytes(const char* data, size_t length,
                              SecExternalFormat format,
                              X509Certificate::OSCertHandles* output) {
  SecExternalFormat input_format = format;
  ScopedCFTypeRef<CFDataRef> local_data(CFDataCreateWithBytesNoCopy(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(data), length,
      kCFAllocatorNull));

  CFArrayRef items = NULL;
  OSStatus status;
  {
    base::AutoLock lock(crypto::GetMacSecurityServicesLock());
    status = SecKeychainItemImport(local_data, NULL, &input_format,
                                   NULL, 0, NULL, NULL, &items);
  }

  if (status) {
    OSSTATUS_DLOG(WARNING, status)
        << "Unable to import items from data of length " << length;
    return;
  }

  ScopedCFTypeRef<CFArrayRef> scoped_items(items);
  CFTypeID cert_type_id = SecCertificateGetTypeID();

  for (CFIndex i = 0; i < CFArrayGetCount(items); ++i) {
    SecKeychainItemRef item = reinterpret_cast<SecKeychainItemRef>(
        const_cast<void*>(CFArrayGetValueAtIndex(items, i)));

    // While inputFormat implies only certificates will be imported, if/when
    // other formats (eg: PKCS#12) are supported, this may also include
    // private keys or other items types, so filter appropriately.
    if (CFGetTypeID(item) == cert_type_id) {
      SecCertificateRef cert = reinterpret_cast<SecCertificateRef>(item);
      // OS X ignores |input_format| if it detects that |local_data| is PEM
      // encoded, attempting to decode data based on internal rules for PEM
      // block headers. If a PKCS#7 blob is encoded with a PEM block of
      // CERTIFICATE, OS X 10.5 will return a single, invalid certificate
      // based on the decoded data. If this happens, the certificate should
      // not be included in |output|. Because |output| is empty,
      // CreateCertificateListfromBytes will use PEMTokenizer to decode the
      // data. When called again with the decoded data, OS X will honor
      // |input_format|, causing decode to succeed. On OS X 10.6, the data
      // is properly decoded as a PKCS#7, whether PEM or not, which avoids
      // the need to fallback to internal decoding.
      if (IsValidOSCertHandle(cert)) {
        CFRetain(cert);
        output->push_back(cert);
      }
    }
  }
}

struct CSSMOIDString {
  const CSSM_OID* oid_;
  std::string string_;
};

typedef std::vector<CSSMOIDString> CSSMOIDStringVector;

bool CERTNameToCSSMOIDVector(CERTName* name, CSSMOIDStringVector* out_values) {
  struct OIDCSSMMap {
    SECOidTag sec_OID_;
    const CSSM_OID* cssm_OID_;
  };

  const OIDCSSMMap kOIDs[] = {
      { SEC_OID_AVA_COMMON_NAME, &CSSMOID_CommonName },
      { SEC_OID_AVA_COUNTRY_NAME, &CSSMOID_CountryName },
      { SEC_OID_AVA_LOCALITY, &CSSMOID_LocalityName },
      { SEC_OID_AVA_STATE_OR_PROVINCE, &CSSMOID_StateProvinceName },
      { SEC_OID_AVA_STREET_ADDRESS, &CSSMOID_StreetAddress },
      { SEC_OID_AVA_ORGANIZATION_NAME, &CSSMOID_OrganizationName },
      { SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME, &CSSMOID_OrganizationalUnitName },
      { SEC_OID_AVA_DN_QUALIFIER, &CSSMOID_DNQualifier },
      { SEC_OID_RFC1274_UID, &CSSMOID_UniqueIdentifier },
      { SEC_OID_PKCS9_EMAIL_ADDRESS, &CSSMOID_EmailAddress },
  };

  CERTRDN** rdns = name->rdns;
  for (size_t rdn = 0; rdns[rdn]; ++rdn) {
    CERTAVA** avas = rdns[rdn]->avas;
    for (size_t pair = 0; avas[pair] != 0; ++pair) {
      SECOidTag tag = CERT_GetAVATag(avas[pair]);
      if (tag == SEC_OID_UNKNOWN) {
        return false;
      }
      CSSMOIDString oidString;
      bool found_oid = false;
      for (size_t oid = 0; oid < ARRAYSIZE_UNSAFE(kOIDs); ++oid) {
        if (kOIDs[oid].sec_OID_ == tag) {
          SECItem* decode_item = CERT_DecodeAVAValue(&avas[pair]->value);
          if (!decode_item)
            return false;

          // TODO(wtc): Pass decode_item to CERT_RFC1485_EscapeAndQuote.
          std::string value(reinterpret_cast<char*>(decode_item->data),
                            decode_item->len);
          oidString.oid_ = kOIDs[oid].cssm_OID_;
          oidString.string_ = value;
          out_values->push_back(oidString);
          SECITEM_FreeItem(decode_item, PR_TRUE);
          found_oid = true;
          break;
        }
      }
      if (!found_oid) {
        DLOG(ERROR) << "Unrecognized OID: " << tag;
      }
    }
  }
  return true;
}

class ScopedCertName {
 public:
  explicit ScopedCertName(CERTName* name) : name_(name) { }
  ~ScopedCertName() {
    if (name_) CERT_DestroyName(name_);
  }
  operator CERTName*() { return name_; }

 private:
  CERTName* name_;
};

class ScopedEncodedCertResults {
 public:
  explicit ScopedEncodedCertResults(CSSM_TP_RESULT_SET* results)
      : results_(results) { }
  ~ScopedEncodedCertResults() {
    if (results_) {
      CSSM_ENCODED_CERT* encCert =
          reinterpret_cast<CSSM_ENCODED_CERT*>(results_->Results);
      for (uint32 i = 0; i < results_->NumberOfResults; i++) {
        crypto::CSSMFree(encCert[i].CertBlob.Data);
      }
      crypto::CSSMFree(results_->Results);
      crypto::CSSMFree(results_);
    }
  }

 private:
  CSSM_TP_RESULT_SET* results_;
};

}  // namespace

void X509Certificate::Initialize() {
  x509_util::CSSMCachedCertificate cached_cert;
  if (cached_cert.Init(cert_handle_) == CSSM_OK) {
    GetCertDistinguishedName(cached_cert, &CSSMOID_X509V1SubjectNameStd,
                             &subject_);
    GetCertDistinguishedName(cached_cert, &CSSMOID_X509V1IssuerNameStd,
                             &issuer_);
    GetCertDateForOID(cached_cert, &CSSMOID_X509V1ValidityNotBefore,
                      &valid_start_);
    GetCertDateForOID(cached_cert, &CSSMOID_X509V1ValidityNotAfter,
                      &valid_expiry_);
    serial_number_ = GetCertSerialNumber(cached_cert);
  }

  fingerprint_ = CalculateFingerprint(cert_handle_);
  ca_fingerprint_ = CalculateCAFingerprint(intermediate_ca_certs_);
}

bool X509Certificate::IsIssuedByEncoded(
    const std::vector<std::string>& valid_issuers) {
  if (IsCertIssuerInEncodedList(cert_handle_, valid_issuers))
    return true;

  for (OSCertHandles::iterator it = intermediate_ca_certs_.begin();
       it != intermediate_ca_certs_.end(); ++it) {
    if (IsCertIssuerInEncodedList(*it, valid_issuers))
      return true;
  }
  return false;
}

void X509Certificate::GetSubjectAltName(
    std::vector<std::string>* dns_names,
    std::vector<std::string>* ip_addrs) const {
  if (dns_names)
    dns_names->clear();
  if (ip_addrs)
    ip_addrs->clear();

  x509_util::CSSMCachedCertificate cached_cert;
  OSStatus status = cached_cert.Init(cert_handle_);
  if (status)
    return;
  x509_util::CSSMFieldValue subject_alt_name;
  status = cached_cert.GetField(&CSSMOID_SubjectAltName, &subject_alt_name);
  if (status || !subject_alt_name.field())
    return;
  const CSSM_X509_EXTENSION* cssm_ext =
      subject_alt_name.GetAs<CSSM_X509_EXTENSION>();
  if (!cssm_ext || !cssm_ext->value.parsedValue)
    return;
  const CE_GeneralNames* alt_name =
      reinterpret_cast<const CE_GeneralNames*>(cssm_ext->value.parsedValue);

  for (size_t name = 0; name < alt_name->numNames; ++name) {
    const CE_GeneralName& name_struct = alt_name->generalName[name];
    const CSSM_DATA& name_data = name_struct.name;
    // DNSName and IPAddress are encoded as IA5String and OCTET STRINGs
    // respectively, both of which can be byte copied from
    // CSSM_DATA::data into the appropriate output vector.
    if (dns_names && name_struct.nameType == GNT_DNSName) {
      dns_names->push_back(std::string(
          reinterpret_cast<const char*>(name_data.Data),
          name_data.Length));
    } else if (ip_addrs && name_struct.nameType == GNT_IPAddress) {
      ip_addrs->push_back(std::string(
          reinterpret_cast<const char*>(name_data.Data),
          name_data.Length));
    }
  }
}

// static
bool X509Certificate::GetDEREncoded(X509Certificate::OSCertHandle cert_handle,
                                    std::string* encoded) {
  CSSM_DATA der_data;
  if (SecCertificateGetData(cert_handle, &der_data) != noErr)
    return false;
  encoded->assign(reinterpret_cast<char*>(der_data.Data),
                  der_data.Length);
  return true;
}

// static
bool X509Certificate::IsSameOSCert(X509Certificate::OSCertHandle a,
                                   X509Certificate::OSCertHandle b) {
  DCHECK(a && b);
  if (a == b)
    return true;
  if (CFEqual(a, b))
    return true;
  CSSM_DATA a_data, b_data;
  return SecCertificateGetData(a, &a_data) == noErr &&
      SecCertificateGetData(b, &b_data) == noErr &&
      a_data.Length == b_data.Length &&
      memcmp(a_data.Data, b_data.Data, a_data.Length) == 0;
}

// static
X509Certificate::OSCertHandle X509Certificate::CreateOSCertHandleFromBytes(
    const char* data, int length) {
  CSSM_DATA cert_data;
  cert_data.Data = const_cast<uint8*>(reinterpret_cast<const uint8*>(data));
  cert_data.Length = length;

  OSCertHandle cert_handle = NULL;
  OSStatus status = SecCertificateCreateFromData(&cert_data,
                                                 CSSM_CERT_X_509v3,
                                                 CSSM_CERT_ENCODING_DER,
                                                 &cert_handle);
  if (status != noErr)
    return NULL;
  if (!IsValidOSCertHandle(cert_handle)) {
    CFRelease(cert_handle);
    return NULL;
  }
  return cert_handle;
}

// static
X509Certificate::OSCertHandles X509Certificate::CreateOSCertHandlesFromBytes(
    const char* data, int length, Format format) {
  OSCertHandles results;

  switch (format) {
    case FORMAT_SINGLE_CERTIFICATE: {
      OSCertHandle handle = CreateOSCertHandleFromBytes(data, length);
      if (handle)
        results.push_back(handle);
      break;
    }
    case FORMAT_PKCS7:
      AddCertificatesFromBytes(data, length, kSecFormatPKCS7, &results);
      break;
    default:
      NOTREACHED() << "Certificate format " << format << " unimplemented";
      break;
  }

  return results;
}

// static
X509Certificate::OSCertHandle X509Certificate::DupOSCertHandle(
    OSCertHandle handle) {
  if (!handle)
    return NULL;
  return reinterpret_cast<OSCertHandle>(const_cast<void*>(CFRetain(handle)));
}

// static
void X509Certificate::FreeOSCertHandle(OSCertHandle cert_handle) {
  CFRelease(cert_handle);
}

// static
SHA1HashValue X509Certificate::CalculateFingerprint(
    OSCertHandle cert) {
  SHA1HashValue sha1;
  memset(sha1.data, 0, sizeof(sha1.data));

  CSSM_DATA cert_data;
  OSStatus status = SecCertificateGetData(cert, &cert_data);
  if (status)
    return sha1;

  DCHECK(cert_data.Data);
  DCHECK_NE(cert_data.Length, 0U);

  CC_SHA1(cert_data.Data, cert_data.Length, sha1.data);

  return sha1;
}

// static
SHA1HashValue X509Certificate::CalculateCAFingerprint(
    const OSCertHandles& intermediates) {
  SHA1HashValue sha1;
  memset(sha1.data, 0, sizeof(sha1.data));

  // The CC_SHA(3cc) man page says all CC_SHA1_xxx routines return 1, so
  // we don't check their return values.
  CC_SHA1_CTX sha1_ctx;
  CC_SHA1_Init(&sha1_ctx);
  CSSM_DATA cert_data;
  for (size_t i = 0; i < intermediates.size(); ++i) {
    OSStatus status = SecCertificateGetData(intermediates[i], &cert_data);
    if (status)
      return sha1;
    CC_SHA1_Update(&sha1_ctx, cert_data.Data, cert_data.Length);
  }
  CC_SHA1_Final(sha1.data, &sha1_ctx);

  return sha1;
}

bool X509Certificate::SupportsSSLClientAuth() const {
  x509_util::CSSMCachedCertificate cached_cert;
  OSStatus status = cached_cert.Init(cert_handle_);
  if (status)
    return false;

  // RFC5280 says to take the intersection of the two extensions.
  //
  // Our underlying crypto libraries don't expose
  // ClientCertificateType, so for now we will not support fixed
  // Diffie-Hellman mechanisms. For rsa_sign, we need the
  // digitalSignature bit.
  //
  // In particular, if a key has the nonRepudiation bit and not the
  // digitalSignature one, we will not offer it to the user.
  x509_util::CSSMFieldValue key_usage;
  status = cached_cert.GetField(&CSSMOID_KeyUsage, &key_usage);
  if (status == CSSM_OK && key_usage.field()) {
    const CSSM_X509_EXTENSION* ext = key_usage.GetAs<CSSM_X509_EXTENSION>();
    const CE_KeyUsage* key_usage_value =
        reinterpret_cast<const CE_KeyUsage*>(ext->value.parsedValue);
    if (!((*key_usage_value) & CE_KU_DigitalSignature))
      return false;
  }

  status = cached_cert.GetField(&CSSMOID_ExtendedKeyUsage, &key_usage);
  if (status == CSSM_OK && key_usage.field()) {
    const CSSM_X509_EXTENSION* ext = key_usage.GetAs<CSSM_X509_EXTENSION>();
    const CE_ExtendedKeyUsage* ext_key_usage =
        reinterpret_cast<const CE_ExtendedKeyUsage*>(ext->value.parsedValue);
    if (!ExtendedKeyUsageAllows(ext_key_usage, &CSSMOID_ClientAuth))
      return false;
  }
  return true;
}

CFArrayRef X509Certificate::CreateOSCertChainForCert() const {
  CFMutableArrayRef cert_list =
      CFArrayCreateMutable(kCFAllocatorDefault, 0,
                           &kCFTypeArrayCallBacks);
  if (!cert_list)
    return NULL;

  CFArrayAppendValue(cert_list, os_cert_handle());
  for (size_t i = 0; i < intermediate_ca_certs_.size(); ++i)
    CFArrayAppendValue(cert_list, intermediate_ca_certs_[i]);

  return cert_list;
}

// static
X509Certificate::OSCertHandle
X509Certificate::ReadOSCertHandleFromPickle(PickleIterator* pickle_iter) {
  const char* data;
  int length;
  if (!pickle_iter->ReadData(&data, &length))
    return NULL;

  return CreateOSCertHandleFromBytes(data, length);
}

// static
bool X509Certificate::WriteOSCertHandleToPickle(OSCertHandle cert_handle,
                                                Pickle* pickle) {
  CSSM_DATA cert_data;
  OSStatus status = SecCertificateGetData(cert_handle, &cert_data);
  if (status)
    return false;

  return pickle->WriteData(reinterpret_cast<char*>(cert_data.Data),
                           cert_data.Length);
}

// static
void X509Certificate::GetPublicKeyInfo(OSCertHandle cert_handle,
                                       size_t* size_bits,
                                       PublicKeyType* type) {
  // Since we might fail, set the output parameters to default values first.
  *type = kPublicKeyTypeUnknown;
  *size_bits = 0;

  SecKeyRef key;
  OSStatus status = SecCertificateCopyPublicKey(cert_handle, &key);
  if (status) {
    NOTREACHED() << "SecCertificateCopyPublicKey failed: " << status;
    return;
  }
  ScopedCFTypeRef<SecKeyRef> scoped_key(key);

  const CSSM_KEY* cssm_key;
  status = SecKeyGetCSSMKey(key, &cssm_key);
  if (status) {
    NOTREACHED() << "SecKeyGetCSSMKey failed: " << status;
    return;
  }

  *size_bits = cssm_key->KeyHeader.LogicalKeySizeInBits;

  switch (cssm_key->KeyHeader.AlgorithmId) {
    case CSSM_ALGID_RSA:
      *type = kPublicKeyTypeRSA;
      break;
    case CSSM_ALGID_DSA:
      *type = kPublicKeyTypeDSA;
      break;
    case CSSM_ALGID_ECDSA:
      *type = kPublicKeyTypeECDSA;
      break;
    case CSSM_ALGID_DH:
      *type = kPublicKeyTypeDH;
      break;
    default:
      *type = kPublicKeyTypeUnknown;
      *size_bits = 0;
      break;
  }
}

}  // namespace net
