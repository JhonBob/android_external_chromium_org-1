// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_ANIMATION_UTILS_H
#define CHROME_BROWSER_UI_COCOA_ANIMATION_UTILS_H
#pragma once

#import <Cocoa/Cocoa.h>

// This class is a stack-based helper useful for unit testing of Cocoa UI,
// and any other situation where you want to temporarily turn off Cocoa
// animation for the life of a function call or other limited scope.
// Just declare one of these, and all animations will complete instantly until
// this goes out of scope and pops our state off the Core Animation stack.
//
// Example:
//  MyUnitTest() {
//    WithNoAnimation at_all; // Turn off Cocoa auto animation in this scope.


class WithNoAnimation {
 public:
  WithNoAnimation() {
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.0];
  }

  ~WithNoAnimation() {
   [NSAnimationContext endGrouping];
  }
};


#endif // CHROME_BROWSER_UI_COCOA_ANIMATION_UTILS_H
