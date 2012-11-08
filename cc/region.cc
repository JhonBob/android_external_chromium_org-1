// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "cc/region.h"

namespace cc {

// TODO(danakj) Use method from ui/gfx/skia_utils.h when it exists.
static inline SkIRect ToSkIRect(gfx::Rect rect)
{
  return SkIRect::MakeXYWH(rect.x(), rect.y(), rect.width(), rect.height());
}

Region::Region() {
}

Region::Region(const Region& region)
    : skregion_(region.skregion_) {
}

Region::Region(gfx::Rect rect)
    : skregion_(ToSkIRect(rect)) {
}

Region::~Region() {
}

const Region& Region::operator=(gfx::Rect rect) {
  skregion_ = SkRegion(ToSkIRect(rect));
  return *this;
}

const Region& Region::operator=(const Region& region) {
  skregion_ = region.skregion_;
  return *this;
}

bool Region::IsEmpty() const {
  return skregion_.isEmpty();
}

bool Region::Contains(gfx::Point point) const {
  return skregion_.contains(point.x(), point.y());
}

bool Region::Contains(gfx::Rect rect) const {
  return skregion_.contains(ToSkIRect(rect));
}

bool Region::Contains(const Region& region) const {
  return skregion_.contains(region.skregion_);
}

bool Region::Intersects(gfx::Rect rect) const {
  return skregion_.intersects(ToSkIRect(rect));
}

bool Region::Intersects(const Region& region) const {
  return skregion_.intersects(region.skregion_);
}

void Region::Subtract(gfx::Rect rect) {
  skregion_.op(ToSkIRect(rect), SkRegion::kDifference_Op);
}

void Region::Subtract(const Region& region) {
  skregion_.op(region.skregion_, SkRegion::kDifference_Op);
}

void Region::Union(gfx::Rect rect) {
  skregion_.op(ToSkIRect(rect), SkRegion::kUnion_Op);
}

void Region::Union(const Region& region) {
  skregion_.op(region.skregion_, SkRegion::kUnion_Op);
}

void Region::Intersect(gfx::Rect rect) {
  skregion_.op(ToSkIRect(rect), SkRegion::kIntersect_Op);
}

void Region::Intersect(const Region& region) {
  skregion_.op(region.skregion_, SkRegion::kIntersect_Op);
}

std::string Region::ToString() const {
  if (IsEmpty())
    return gfx::Rect().ToString();

  std::string result;
  for (Iterator it(*this); it.has_rect(); it.next()) {
    if (!result.empty())
      result += " | ";
    result += it.rect().ToString();
  }
  return result;
}

Region::Iterator::Iterator(const Region& region)
    : it_(region.skregion_) {
}

Region::Iterator::~Iterator() {
}

}  // namespace cc
