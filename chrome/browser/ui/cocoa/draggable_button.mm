// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/draggable_button.h"

#include "base/logging.h"

@implementation DraggableButton

- (id)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    draggableButtonImpl_.reset(
        [[DraggableButtonImpl alloc] initWithButton:self]);
  }
  return self;
}

- (id)initWithCoder:(NSCoder*)coder {
  if ((self = [super initWithCoder:coder])) {
    draggableButtonImpl_.reset(
        [[DraggableButtonImpl alloc] initWithButton:self]);
  }
  return self;
}

- (DraggableButtonImpl*)draggableButton {
  return draggableButtonImpl_.get();
}

- (void)mouseUp:(NSEvent*)theEvent {
  if ([draggableButtonImpl_ mouseUp:theEvent] == kDraggableButtonMixinCallSuper)
    [super mouseUp:theEvent];
}

- (void)mouseDown:(NSEvent*)theEvent {
  if ([draggableButtonImpl_ mouseDown:theEvent] ==
          kDraggableButtonMixinCallSuper) {
    [super mouseDown:theEvent];
  }
}

- (void)beginDrag:(NSEvent*)dragEvent {
  // Must be overridden by subclasses.
  NOTREACHED();
}

@end  // @interface DraggableButton
