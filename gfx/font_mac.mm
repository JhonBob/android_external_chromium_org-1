// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gfx/font.h"

#include <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "base/scoped_nsobject.h"
#include "base/sys_string_conversions.h"

namespace gfx {

// static
Font Font::CreateFont(const std::wstring& font_name, int font_size) {
  return Font(font_name, font_size, NORMAL);
}

Font::Font(const std::wstring& font_name, int font_size, int style)
    : font_name_(font_name),
      font_size_(font_size),
      style_(style) {
  calculateMetrics();
}

Font::Font()
    : font_size_([NSFont systemFontSize]),
      style_(NORMAL) {
  NSFont* system_font = [NSFont systemFontOfSize:font_size_];
  font_name_ = base::SysNSStringToWide([system_font fontName]);
  calculateMetrics();
}

void Font::calculateMetrics() {
  NSFont* font = nativeFont();
  scoped_nsobject<NSLayoutManager> layout_manager(
      [[NSLayoutManager alloc] init]);
  height_ = [layout_manager defaultLineHeightForFont:font];
  ascent_ = [font ascender];
  avg_width_ = [font boundingRectForGlyph:[font glyphWithName:@"x"]].size.width;
}

Font Font::DeriveFont(int size_delta, int style) const {
  return Font(font_name_, font_size_ + size_delta, style);
}

int Font::height() const {
  return height_;
}

int Font::baseline() const {
  return ascent_;
}

int Font::ave_char_width() const {
  return avg_width_;
}

int Font::GetStringWidth(const std::wstring& text) const {
  int width = 0, height = 0;
  Canvas::SizeStringInt(text, *this, &width, &height, gfx::Canvas::NO_ELLIPSIS);
  return width;
}

int Font::GetExpectedTextWidth(int length) const {
  return length * avg_width_;
}

int Font::style() const {
  return style_;
}

const std::wstring& Font::FontName() const {
  return font_name_;
}

int Font::FontSize() {
  return font_size_;
}

NativeFont Font::nativeFont() const {
  // TODO(pinkerton): apply |style_| to font. http://crbug.com/34667
  // We could cache this, but then we'd have to conditionally change the
  // dtor just for MacOS. Not sure if we want to/need to do that.
  return [NSFont fontWithName:base::SysWideToNSString(font_name_)
                         size:font_size_];
}

}  // namespace gfx
