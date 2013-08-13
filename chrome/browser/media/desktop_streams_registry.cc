// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/desktop_streams_registry.h"

#include "base/base64.h"
#include "base/location.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/random.h"

namespace {

const int kStreamIdLengthBytes = 16;

const int kApprovedStreamTimeToLiveSeconds = 10;

std::string GenerateRandomStreamId() {
  char buffer[kStreamIdLengthBytes];
  crypto::RandBytes(buffer, arraysize(buffer));
  std::string result;
  if (!base::Base64Encode(base::StringPiece(buffer, arraysize(buffer)),
                          &result)) {
    LOG(FATAL) << "Base64Encode failed.";
  }
  return result;
}

}  // namespace

DesktopStreamsRegistry::DesktopStreamsRegistry() {}
DesktopStreamsRegistry::~DesktopStreamsRegistry() {}

std::string DesktopStreamsRegistry::RegisterStream(
    const GURL& origin,
    const content::DesktopMediaID& source) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  std::string id = GenerateRandomStreamId();
  ApprovedDesktopMediaStream& stream = approved_streams_[id];
  stream.origin = origin;
  stream.source = source;

  content::BrowserThread::PostDelayedTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&DesktopStreamsRegistry::CleanupStream,
                 base::Unretained(this), id),
      base::TimeDelta::FromSeconds(kApprovedStreamTimeToLiveSeconds));

  return id;
}

content::DesktopMediaID DesktopStreamsRegistry::RequestMediaForStreamId(
    const std::string& id,
    const GURL& origin) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  StreamsMap::iterator it = approved_streams_.find(id);
  if (it == approved_streams_.end() || origin != it->second.origin)
    return content::DesktopMediaID();
  content::DesktopMediaID result = it->second.source;
  approved_streams_.erase(it);
  return result;
}

void DesktopStreamsRegistry::CleanupStream(const std::string& id) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  approved_streams_.erase(id);
}
