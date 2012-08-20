// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_INPUT_FILTER_H_
#define REMOTING_PROTOCOL_INPUT_FILTER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "remoting/protocol/input_stub.h"

namespace remoting {
namespace protocol {

// Forwards input events to |input_stub|, if configured.  Input forwarding may
// also be disabled independently of the |input_stub| being set.  InputFilters
// initially have input forwarding enabled.
class InputFilter : public InputStub {
 public:
  InputFilter();
  explicit InputFilter(InputStub* input_stub);
  virtual ~InputFilter();

  // Set the InputStub that events will be forwarded to.
  void set_input_stub(InputStub* input_stub) {
    input_stub_ = input_stub;
  }

  // Enable/disable routing of events to the InputStub.
  void set_enabled(bool enabled) {
    enabled_ = enabled;
  }
  bool enabled() const {
    return enabled_;
  }

  // InputStub interface.
  virtual void InjectKeyEvent(const KeyEvent& event) OVERRIDE;
  virtual void InjectMouseEvent(const MouseEvent& event) OVERRIDE;

 private:
  InputStub* input_stub_;
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(InputFilter);
};

} // namespace protocol
} // namespace remoting

#endif // REMOTING_PROTOCOL_INPUT_FILTER_H_
