// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ATTESTATION_ATTESTATION_CA_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_ATTESTATION_ATTESTATION_CA_CLIENT_H_

#include <map>
#include <string>

#include "base/basictypes.h"

#include "chromeos/attestation/attestation_flow.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace chromeos {
namespace attestation {

// This class is a ServerProxy implementation for the Chrome OS attestation
// flow.  It sends all requests to an Attestation CA via HTTPS.
class AttestationCAClient : public ServerProxy,
                            public net::URLFetcherDelegate {
 public:
  AttestationCAClient();
  virtual ~AttestationCAClient();

  // chromeos::attestation::ServerProxy:
  virtual void SendEnrollRequest(const std::string& request,
                                 const DataCallback& on_response) OVERRIDE;
  virtual void SendCertificateRequest(const std::string& request,
                                      const DataCallback& on_response) OVERRIDE;

  // net::URLFetcherDelegate:
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;

 private:
  typedef std::map<const net::URLFetcher*, DataCallback> FetcherCallbackMap;

  // POSTs |request| data to |url| and calls |on_response| with the response
  // data when the fetch is complete.
  void FetchURL(const std::string& url,
                const std::string& request,
                const DataCallback& on_response);

  // Tracks all URL requests we have started.
  FetcherCallbackMap pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(AttestationCAClient);
};

}  // namespace attestation
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ATTESTATION_ATTESTATION_CA_CLIENT_H_
