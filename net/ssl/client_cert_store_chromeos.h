// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SSL_CLIENT_CERT_STORE_CHROMEOS_H_
#define NET_SSL_CLIENT_CERT_STORE_CHROMEOS_H_

#include <string>

#include "crypto/scoped_nss_types.h"
#include "net/cert/nss_profile_filter_chromeos.h"
#include "net/ssl/client_cert_store_nss.h"

namespace net {

class NET_EXPORT ClientCertStoreChromeOS : public ClientCertStoreNSS {
 public:
  ClientCertStoreChromeOS(
      const std::string& username_hash,
      const PasswordDelegateFactory& password_delegate_factory);
  virtual ~ClientCertStoreChromeOS();

  // ClientCertStoreNSS:
  virtual void GetClientCerts(const SSLCertRequestInfo& cert_request_info,
                              CertificateList* selected_certs,
                              const base::Closure& callback) OVERRIDE;

 protected:
  // ClientCertStoreNSS:
  virtual void GetClientCertsImpl(CERTCertList* cert_list,
                                  const SSLCertRequestInfo& request,
                                  bool query_nssdb,
                                  CertificateList* selected_certs) OVERRIDE;

 private:
  friend class ClientCertStoreChromeOSTestDelegate;

  void DidGetPrivateSlot(const SSLCertRequestInfo* request,
                         CertificateList* selected_certs,
                         const base::Closure& callback,
                         crypto::ScopedPK11Slot private_slot);

  std::string username_hash_;
  NSSProfileFilterChromeOS profile_filter_;

  DISALLOW_COPY_AND_ASSIGN(ClientCertStoreChromeOS);
};

}  // namespace net

#endif  // NET_SSL_CLIENT_CERT_STORE_CHROMEOS_H_
