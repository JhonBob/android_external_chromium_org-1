// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/textfield_with_margin.h"

namespace {

// Holds ratio of the margin to the preferred text height.
const double kTextMarginRate = 0.33;

// Size of each vertical margin (top, bottom).
const int kVerticalMargin = 3;

}  // namespace

namespace chromeos {

TextfieldWithMargin::TextfieldWithMargin() {
}

void TextfieldWithMargin::Layout() {
  int margin = GetPreferredSize().height() * kTextMarginRate;
  SetHorizontalMargins(margin, margin);
  SetVerticalMargins(kVerticalMargin, kVerticalMargin);
  views::Textfield::Layout();
}

}  // namespace chromeos
