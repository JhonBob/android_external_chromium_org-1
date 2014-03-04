// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_LOGGING_LOG_DESERIALIZER_H_
#define MEDIA_CAST_LOGGING_LOG_DESERIALIZER_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "media/cast/logging/encoding_event_subscriber.h"

namespace media {
namespace cast {

// This function takes the output of LogSerializer and deserializes it into
// its original format. Returns true if deserialization is successful. All
// output arguments are valid if this function returns true.
// |data|: Serialized event logs.
// |is_audio|: This will be set to true or false depending on whether the data
// is for an audio or video stream.
// |frame_events|: This will be populated with deserialized frame events.
// |packet_events|: This will be populated with deserialized packet events.
// |first_rtp_timestamp|: This will be populated with the first RTP timestamp
// of the events.
bool DeserializeEvents(const std::string& data,
                       bool* is_audio,
                       FrameEventMap* frame_events,
                       PacketEventMap* packet_events,
                       RtpTimestamp* first_rtp_timestamp);

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_LOGGING_LOG_DESERIALIZER_H_
