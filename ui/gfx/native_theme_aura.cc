// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/native_theme_aura.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "grit/gfx_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/size.h"
#include "ui/gfx/skbitmap_operations.h"

namespace {

const SkColor kMenuBackgroundColor = SK_ColorWHITE;

// Theme colors returned by GetSystemColor().
const SkColor kInvalidColorIdColor = SkColorSetRGB(255, 0, 128);
// Dialogs:
const SkColor kDialogBackgroundColor = SK_ColorWHITE;
// FocusableBorder:
const SkColor kFocusedBorderColor = SkColorSetRGB(0x4D, 0x90, 0xFE);
const SkColor kUnfocusedBorderColor = SkColorSetRGB(0xD9, 0xD9, 0xD9);
// TextButton:
const SkColor kTextButtonBackgroundColor = SkColorSetRGB(0xDE, 0xDE, 0xDE);
const SkColor kTextButtonEnabledColor = SkColorSetRGB(0x44, 0x44, 0x44);
const SkColor kTextButtonDisabledColor = SkColorSetRGB(0x99, 0x99, 0x99);
const SkColor kTextButtonHighlightColor = SkColorSetRGB(0, 0, 0);
const SkColor kTextButtonHoverColor = kTextButtonEnabledColor;
// MenuItem:
const SkColor kEnabledMenuItemForegroundColor = SK_ColorBLACK;
const SkColor kDisabledMenuItemForegroundColor =
    SkColorSetRGB(0x80, 0x80, 0x80);
const SkColor kFocusedMenuItemBackgroundColor = SkColorSetRGB(0xF1, 0xF1, 0xF1);
// Textfield:
const SkColor kTextfieldDefaultColor = SK_ColorBLACK;
const SkColor kTextfieldDefaultBackground = SK_ColorWHITE;
const SkColor kTextfieldSelectionBackgroundFocused =
    SkColorSetARGB(0x54, 0x4D, 0x90, 0xFE);
const SkColor kTextfieldSelectionBackgroundUnfocused = SK_ColorLTGRAY;
const SkColor kTextfieldSelectionColor =
    color_utils::AlphaBlend(SK_ColorBLACK,
        kTextfieldSelectionBackgroundFocused, 0xdd);

}  // namespace

namespace gfx {

// static
const NativeTheme* NativeTheme::instance() {
  return NativeThemeAura::instance();
}

// static
const NativeThemeAura* NativeThemeAura::instance() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeAura, s_native_theme, ());
  return &s_native_theme;
}

NativeThemeAura::NativeThemeAura() {
  // We don't draw scrollbar buttons.
  set_scrollbar_button_length(0);
}

NativeThemeAura::~NativeThemeAura() {
}

SkColor NativeThemeAura::GetSystemColor(ColorId color_id) const {
  // This implementation returns hardcoded colors.
  switch (color_id) {

    // Dialogs
    case kColorId_DialogBackground:
      return kDialogBackgroundColor;

    // FocusableBorder
    case kColorId_FocusedBorderColor:
      return kFocusedBorderColor;
    case kColorId_UnfocusedBorderColor:
      return kUnfocusedBorderColor;

    // TextButton
    case kColorId_TextButtonBackgroundColor:
      return kTextButtonBackgroundColor;
    case kColorId_TextButtonEnabledColor:
      return kTextButtonEnabledColor;
    case kColorId_TextButtonDisabledColor:
      return kTextButtonDisabledColor;
    case kColorId_TextButtonHighlightColor:
      return kTextButtonHighlightColor;
    case kColorId_TextButtonHoverColor:
      return kTextButtonHoverColor;

    // MenuItem
    case kColorId_EnabledMenuItemForegroundColor:
      return kEnabledMenuItemForegroundColor;
    case kColorId_DisabledMenuItemForegroundColor:
      return kDisabledMenuItemForegroundColor;
    case kColorId_FocusedMenuItemBackgroundColor:
      return kFocusedMenuItemBackgroundColor;

    // Textfield
    case kColorId_TextfieldDefaultColor:
      return kTextfieldDefaultColor;
    case kColorId_TextfieldDefaultBackground:
      return kTextfieldDefaultBackground;
    case kColorId_TextfieldSelectionColor:
      return kTextfieldSelectionColor;
    case kColorId_TextfieldSelectionBackgroundFocused:
      return kTextfieldSelectionBackgroundFocused;
    case kColorId_TextfieldSelectionBackgroundUnfocused:
      return kTextfieldSelectionBackgroundUnfocused;

    default:
      NOTREACHED() << "Invalid color_id: " << color_id;
      break;
  }

  return kInvalidColorIdColor;
}

void NativeThemeAura::PaintMenuPopupBackground(SkCanvas* canvas,
                                               const gfx::Size& size) const {
  canvas->drawColor(kMenuBackgroundColor, SkXfermode::kSrc_Mode);
}

void NativeThemeAura::PaintScrollbarTrack(
    SkCanvas* canvas,
    Part part,
    State state,
    const ScrollbarTrackExtraParams& extra_params,
    const gfx::Rect& rect) const {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  if (part == kScrollbarVerticalTrack) {
    int center_offset = 0;
    int center_height = rect.height();

    if (rect.y() == extra_params.track_y) {
      // TODO(derat): Honor |state| instead of only using the highlighted images
      // after updating WebKit so we can draw the entire track in one go instead
      // of as two separate pieces: otherwise, only the portion of the scrollbar
      // that the mouse is over gets the highlighted state.
      SkBitmap* top = rb.GetBitmapNamed(IDR_SCROLL_BASE_VERTICAL_TOP_H);
      DrawTiledImage(canvas, *top,
                     0, 0, 1.0, 1.0,
                     rect.x(), rect.y(), top->width(), top->height());
      center_offset += top->height();
      center_height -= top->height();
    }

    if (rect.y() + rect.height() ==
        extra_params.track_y + extra_params.track_height) {
      SkBitmap* bottom = rb.GetBitmapNamed(IDR_SCROLL_BASE_VERTICAL_BOTTOM_H);
      DrawTiledImage(canvas, *bottom,
                     0, 0, 1.0, 1.0,
                     rect.x(), rect.y() + rect.height() - bottom->height(),
                     bottom->width(), bottom->height());
      center_height -= bottom->height();
    }

    if (center_height > 0) {
      SkBitmap* center = rb.GetBitmapNamed(IDR_SCROLL_BASE_VERTICAL_CENTER_H);
      DrawTiledImage(canvas, *center,
                     0, 0, 1.0, 1.0,
                     rect.x(), rect.y() + center_offset,
                     center->width(), center_height);
    }
  } else {
    int center_offset = 0;
    int center_width = rect.width();

    if (rect.x() == extra_params.track_x) {
      SkBitmap* left = rb.GetBitmapNamed(IDR_SCROLL_BASE_HORIZONTAL_LEFT_H);
      DrawTiledImage(canvas, *left,
                     0, 0, 1.0, 1.0,
                     rect.x(), rect.y(), left->width(), left->height());
      center_offset += left->width();
      center_width -= left->width();
    }

    if (rect.x() + rect.width() ==
        extra_params.track_x + extra_params.track_width) {
      SkBitmap* right = rb.GetBitmapNamed(IDR_SCROLL_BASE_HORIZONTAL_RIGHT_H);
      DrawTiledImage(canvas, *right,
                     0, 0, 1.0, 1.0,
                     rect.x() + rect.width() - right->width(), rect.y(),
                     right->width(), right->height());
      center_width -= right->width();
    }

    if (center_width > 0) {
      SkBitmap* center = rb.GetBitmapNamed(IDR_SCROLL_BASE_HORIZONTAL_CENTER_H);
      DrawTiledImage(canvas, *center,
                     0, 0, 1.0, 1.0,
                     rect.x() + center_offset, rect.y(),
                     center_width, center->height());
    }
  }
}

void NativeThemeAura::PaintArrowButton(SkCanvas* canvas,
    const gfx::Rect& rect, Part part, State state) const {
  DCHECK(rect.IsEmpty());
}

void NativeThemeAura::PaintScrollbarThumb(SkCanvas* canvas,
    Part part, State state, const gfx::Rect& rect) const {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  if (part == kScrollbarVerticalThumb) {
    int top_resource_id =
        state == kHovered ? IDR_SCROLL_THUMB_VERTICAL_TOP_H :
        state == kPressed ? IDR_SCROLL_THUMB_VERTICAL_TOP_P :
        IDR_SCROLL_THUMB_VERTICAL_TOP;
    SkBitmap* top = rb.GetBitmapNamed(top_resource_id);
    DrawTiledImage(canvas, *top,
                   0, 0, 1.0, 1.0,
                   rect.x(), rect.y(), top->width(), top->height());

    int bottom_resource_id =
        state == kHovered ? IDR_SCROLL_THUMB_VERTICAL_BOTTOM_H :
        state == kPressed ? IDR_SCROLL_THUMB_VERTICAL_BOTTOM_P :
        IDR_SCROLL_THUMB_VERTICAL_BOTTOM;
    SkBitmap* bottom = rb.GetBitmapNamed(bottom_resource_id);
    DrawTiledImage(canvas, *bottom,
                   0, 0, 1.0, 1.0,
                   rect.x(), rect.y() + rect.height() - bottom->height(),
                   bottom->width(), bottom->height());

    if (rect.height() > top->height() + bottom->height()) {
      int center_resource_id =
          state == kHovered ? IDR_SCROLL_THUMB_VERTICAL_CENTER_H :
          state == kPressed ? IDR_SCROLL_THUMB_VERTICAL_CENTER_P :
          IDR_SCROLL_THUMB_VERTICAL_CENTER;
      SkBitmap* center = rb.GetBitmapNamed(center_resource_id);
      DrawTiledImage(canvas, *center,
                     0, 0, 1.0, 1.0,
                     rect.x(), rect.y() + top->height(),
                     center->width(),
                     rect.height() - top->height() - bottom->height());
    }
  } else {
    int left_resource_id =
        state == kHovered ? IDR_SCROLL_THUMB_HORIZONTAL_LEFT_H :
        state == kPressed ? IDR_SCROLL_THUMB_HORIZONTAL_LEFT_P :
        IDR_SCROLL_THUMB_HORIZONTAL_LEFT;
    SkBitmap* left = rb.GetBitmapNamed(left_resource_id);
    DrawTiledImage(canvas, *left,
                   0, 0, 1.0, 1.0,
                   rect.x(), rect.y(), left->width(), left->height());

    int right_resource_id =
        state == kHovered ? IDR_SCROLL_THUMB_HORIZONTAL_RIGHT_H :
        state == kPressed ? IDR_SCROLL_THUMB_HORIZONTAL_RIGHT_P :
        IDR_SCROLL_THUMB_HORIZONTAL_RIGHT;
    SkBitmap* right = rb.GetBitmapNamed(right_resource_id);
    DrawTiledImage(canvas, *right,
                   0, 0, 1.0, 1.0,
                   rect.x() + rect.width() - right->width(), rect.y(),
                   right->width(), right->height());

    if (rect.width() > left->width() + right->width()) {
      int center_resource_id =
          state == kHovered ? IDR_SCROLL_THUMB_HORIZONTAL_CENTER_H :
          state == kPressed ? IDR_SCROLL_THUMB_HORIZONTAL_CENTER_P :
          IDR_SCROLL_THUMB_HORIZONTAL_CENTER;
      SkBitmap* center = rb.GetBitmapNamed(center_resource_id);
      DrawTiledImage(canvas, *center,
                     0, 0, 1.0, 1.0,
                     rect.x() + left->width(), rect.y(),
                     rect.width() - left->width() - right->width(),
                     center->height());
    }
  }
}

}  // namespace gfx
