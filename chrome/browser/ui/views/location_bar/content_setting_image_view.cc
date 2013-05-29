// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/content_setting_image_view.h"

#include "base/utf_string_conversions.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/browser/ui/content_settings/content_setting_image_model.h"
#include "chrome/browser/ui/views/content_setting_bubble_contents.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"


namespace {
const int kStayOpenTimeMS = 3200;  // Time spent with animation fully open.

// Margins for animated box (pixels).
const int kHorizMargin = 4;
const int kIconLabelSpacing = 4;
}


// static
const int ContentSettingImageView::kOpenTimeMS = 150;
const int ContentSettingImageView::kAnimationDurationMS =
    (kOpenTimeMS * 2) + kStayOpenTimeMS;

ContentSettingImageView::ContentSettingImageView(
    ContentSettingsType content_type,
    const int background_images[],
    LocationBarView* parent,
    const gfx::Font& font,
    int font_y_offset,
    SkColor font_color)
    : parent_(parent),
      content_setting_image_model_(
          ContentSettingImageModel::CreateContentSettingImageModel(
              content_type)),
      background_painter_(new views::HorizontalPainter(background_images)),
      icon_(new views::ImageView),
      text_label_(NULL),
      slide_animator_(this),
      pause_animation_(false),
      pause_animation_state_(0.0),
      bubble_widget_(NULL),
      font_(font),
      text_size_(0) {
  SetLayoutManager(new views::BoxLayout(
      views::BoxLayout::kHorizontal, 0, 0, 0));
  icon_->SetHorizontalAlignment(views::ImageView::LEADING);
  AddChildView(icon_);
  TouchableLocationBarView::Init(this);

  slide_animator_.SetSlideDuration(kAnimationDurationMS);
  slide_animator_.SetTweenType(ui::Tween::LINEAR);
}

ContentSettingImageView::~ContentSettingImageView() {
  if (bubble_widget_)
    bubble_widget_->RemoveObserver(this);
}

int ContentSettingImageView::GetBuiltInHorizontalPadding() const {
  return GetBuiltInHorizontalPaddingImpl();
}

void ContentSettingImageView::Update(content::WebContents* web_contents) {
  if (web_contents)
    content_setting_image_model_->UpdateFromWebContents(web_contents);

  if (!content_setting_image_model_->is_visible()) {
    SetVisible(false);
    return;
  }

  icon_->SetImage(ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      content_setting_image_model_->get_icon()));
  icon_->SetTooltipText(
      UTF8ToUTF16(content_setting_image_model_->get_tooltip()));
  SetVisible(true);

  // If the content blockage should be indicated to the user, start the
  // animation and record that we indicated the blockage.
  TabSpecificContentSettings* content_settings = web_contents ?
      TabSpecificContentSettings::FromWebContents(web_contents) : NULL;
  if (!content_settings || content_settings->IsBlockageIndicated(
      content_setting_image_model_->get_content_settings_type()))
    return;

  // We just ignore this blockage if we're already showing some other string to
  // the user.  If this becomes a problem, we could design some sort of queueing
  // mechanism to show one after the other, but it doesn't seem important now.
  int string_id = content_setting_image_model_->explanatory_string_id();
  if (string_id && !background_showing()) {
    // Initialize animated string. It will be cleared when animation is
    // completed.
    if (!text_label_) {
      text_label_ = new views::Label;
      text_label_->SetElideBehavior(views::Label::NO_ELIDE);
      text_label_->SetFont(font_);
      SetLayoutManager(new views::BoxLayout(
          views::BoxLayout::kHorizontal, kHorizMargin, 0, kIconLabelSpacing));
      AddChildView(text_label_);
    }
    text_label_->SetText(l10n_util::GetStringUTF16(string_id));
    text_size_ = font_.GetStringWidth(text_label_->text());
    text_size_ += kHorizMargin;
    slide_animator_.Show();
  }

  content_settings->SetBlockageHasBeenIndicated(
      content_setting_image_model_->get_content_settings_type());
}

void ContentSettingImageView::AnimationEnded(const ui::Animation* animation) {
  slide_animator_.Reset();
  if (!pause_animation_) {
    SetLayoutManager(new views::BoxLayout(
        views::BoxLayout::kHorizontal, 0, 0, 0));
    RemoveChildView(text_label_);  // will also delete the view.
    text_label_ = NULL;
    parent_->Layout();
    parent_->SchedulePaint();
  }
}

void ContentSettingImageView::AnimationProgressed(
    const ui::Animation* animation) {
  if (!pause_animation_) {
    parent_->Layout();
    parent_->SchedulePaint();
  }
}

void ContentSettingImageView::AnimationCanceled(
    const ui::Animation* animation) {
  AnimationEnded(animation);
}

gfx::Size ContentSettingImageView::GetPreferredSize() {
  // Height will be ignored by the LocationBarView.
  gfx::Size preferred_size(views::View::GetPreferredSize());
  int non_label_width = preferred_size.width() -
      (text_label_ ? text_label_->GetPreferredSize().width() : 0);

  int visible_text_size = 0;
  if (background_showing()) {
    double state = slide_animator_.GetCurrentValue();
    // The fraction of the animation we'll spend animating the string into view,
    // which is also the fraction we'll spend animating it closed; total
    // animation (slide out, show, then slide in) is 1.0.
    const double kOpenFraction =
        static_cast<double>(kOpenTimeMS) / kAnimationDurationMS;
    double size_fraction = 1.0;
    if (state < kOpenFraction)
      size_fraction = state / kOpenFraction;
    if (state > (1.0 - kOpenFraction))
      size_fraction = (1.0 - state) / kOpenFraction;
    visible_text_size = size_fraction * text_size_;
  }

  preferred_size.set_width(non_label_width + visible_text_size);
  return preferred_size;
}

bool ContentSettingImageView::OnMousePressed(const ui::MouseEvent& event) {
  // We want to show the bubble on mouse release; that is the standard behavior
  // for buttons.
  return true;
}

void ContentSettingImageView::OnMouseReleased(const ui::MouseEvent& event) {
  if (HitTestPoint(event.location()))
    OnClick();
}

void ContentSettingImageView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP)
    OnClick();
  if ((event->type() == ui::ET_GESTURE_TAP) ||
      (event->type() == ui::ET_GESTURE_TAP_DOWN))
    event->SetHandled();
}

void ContentSettingImageView::OnPaintBackground(gfx::Canvas* canvas) {
  if (background_showing())
    background_painter_->Paint(canvas, size());
}

void ContentSettingImageView::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(bubble_widget_, widget);
  bubble_widget_->RemoveObserver(this);
  bubble_widget_ = NULL;

  if (pause_animation_) {
    slide_animator_.Reset(pause_animation_state_);
    pause_animation_ = false;
    slide_animator_.Show();
  }
}

void ContentSettingImageView::OnClick() {
  if (slide_animator_.is_animating()) {
    if (!pause_animation_) {
      pause_animation_ = true;
      pause_animation_state_ = slide_animator_.GetCurrentValue();
    }
    slide_animator_.Reset();
  }

  content::WebContents* web_contents = parent_->GetWebContents();
  if (web_contents && !bubble_widget_) {
    bubble_widget_ =
        parent_->delegate()->CreateViewsBubble(new ContentSettingBubbleContents(
            ContentSettingBubbleModel::CreateContentSettingBubbleModel(
                parent_->delegate()->GetContentSettingBubbleModelDelegate(),
                web_contents, parent_->profile(),
                content_setting_image_model_->get_content_settings_type()),
            web_contents, this, views::BubbleBorder::TOP_RIGHT));
    bubble_widget_->AddObserver(this);
    bubble_widget_->Show();
  }
}
