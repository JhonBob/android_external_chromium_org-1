// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBCONTENTDECRYPTIONMODULESESSION_IMPL_H_
#define CONTENT_RENDERER_MEDIA_WEBCONTENTDECRYPTIONMODULESESSION_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/base/media_keys.h"
#include "third_party/WebKit/public/platform/WebContentDecryptionModuleSession.h"
#include "third_party/WebKit/public/platform/WebString.h"

namespace media {
class MediaKeys;
}

namespace content {
class CdmSessionAdapter;

class WebContentDecryptionModuleSessionImpl
    : public blink::WebContentDecryptionModuleSession {
 public:
  WebContentDecryptionModuleSessionImpl(
      Client* client,
      const scoped_refptr<CdmSessionAdapter>& adapter);
  virtual ~WebContentDecryptionModuleSessionImpl();

  // blink::WebContentDecryptionModuleSession implementation.
  virtual blink::WebString sessionId() const;
  // TODO(jrummell): Remove the next 3 methods once blink updated.
  virtual void initializeNewSession(const blink::WebString& mime_type,
                                    const uint8* init_data,
                                    size_t init_data_length);
  virtual void update(const uint8* response, size_t response_length);
  virtual void release();
  virtual void initializeNewSession(
      const blink::WebString& init_data_type,
      const uint8* init_data,
      size_t init_data_length,
      const blink::WebString& session_type,
      blink::WebContentDecryptionModuleResult result);
  virtual void update(const uint8* response,
                      size_t response_length,
                      blink::WebContentDecryptionModuleResult result);
  virtual void release(blink::WebContentDecryptionModuleResult result);

  // Callbacks.
  void OnSessionMessage(const std::vector<uint8>& message,
                        const GURL& destination_url);
  void OnSessionReady();
  void OnSessionClosed();
  void OnSessionError(media::MediaKeys::Exception exception_code,
                      uint32 system_code,
                      const std::string& error_message);

 private:
  typedef std::map<uint32, blink::WebContentDecryptionModuleResult> ResultMap;

  // These function are used as callbacks when CdmPromise resolves/rejects.
  // |result_index| = kReservedIndex means that there is no
  // WebContentDecryptionModuleResult.
  void SessionCreated(uint32 result_index, const std::string& web_session_id);
  void SessionUpdatedOrReleased(uint32 result_index);
  void SessionError(uint32 result_index,
                    media::MediaKeys::Exception exception_code,
                    uint32 system_code,
                    const std::string& error_message);

  // As initializeNewSession(), update(), and release() get passed a
  // WebContentDecryptionModuleResult, keep track of them since this class owns
  // it and needs to keep them around until completed. Returns the index used
  // to locate the WebContentDecryptionModuleResult when the operation is
  // complete.
  uint32 AddResult(blink::WebContentDecryptionModuleResult result);

  scoped_refptr<CdmSessionAdapter> adapter_;

  // Non-owned pointer.
  Client* client_;

  // Web session ID is the app visible ID for this session generated by the CDM.
  // This value is not set until the CDM resolves the initializeNewSession()
  // promise.
  std::string web_session_id_;

  // Don't pass more than 1 close() event to blink::
  // TODO(jrummell): Remove this once blink tests handle close() promise and
  // closed() event.
  bool is_closed_;

  // Keep track of all the outstanding WebContentDecryptionModuleResult objects.
  uint32 next_available_result_index_;
  ResultMap outstanding_results_;

  // Since promises will live until they are fired, use a weak reference when
  // creating a promise in case this class disappears before the promise
  // actually fires.
  base::WeakPtrFactory<WebContentDecryptionModuleSessionImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebContentDecryptionModuleSessionImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBCONTENTDECRYPTIONMODULESESSION_IMPL_H_
