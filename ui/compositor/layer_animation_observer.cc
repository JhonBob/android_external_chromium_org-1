// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/layer_animation_observer.h"

#include "ui/compositor/layer_animation_sequence.h"

namespace ui {

////////////////////////////////////////////////////////////////////////////////
// LayerAnimationObserver

LayerAnimationObserver::LayerAnimationObserver() {
}

LayerAnimationObserver::~LayerAnimationObserver() {
  StopObserving();
}

bool LayerAnimationObserver::RequiresNotificationWhenAnimatorDestroyed() const {
  return false;
}

void LayerAnimationObserver::OnAttachedToSequence(
    LayerAnimationSequence* sequence) {
}

void LayerAnimationObserver::OnDetachedFromSequence(
    LayerAnimationSequence* sequence) {
}

void LayerAnimationObserver::StopObserving() {
  while (!attached_sequences_.empty()) {
    LayerAnimationSequence* sequence = *attached_sequences_.begin();
    sequence->RemoveObserver(this);
  }
}

void LayerAnimationObserver::AttachedToSequence(
    LayerAnimationSequence* sequence) {
  DCHECK(attached_sequences_.find(sequence) == attached_sequences_.end());
  attached_sequences_.insert(sequence);
  OnAttachedToSequence(sequence);
}

void LayerAnimationObserver::DetachedFromSequence(
    LayerAnimationSequence* sequence, bool send_notification) {
  if (attached_sequences_.find(sequence) != attached_sequences_.end())
    attached_sequences_.erase(sequence);
  if (send_notification)
    OnDetachedFromSequence(sequence);
}

////////////////////////////////////////////////////////////////////////////////
// ImplicitAnimationObserver

ImplicitAnimationObserver::ImplicitAnimationObserver()
    : active_(false),
      destroyed_(NULL) {
}

ImplicitAnimationObserver::~ImplicitAnimationObserver() {
  if (destroyed_)
    *destroyed_ = true;
}

void ImplicitAnimationObserver::SetActive(bool active) {
  active_ = active;
  CheckCompleted();
}

void ImplicitAnimationObserver::StopObservingImplicitAnimations() {
  SetActive(false);
  StopObserving();
}

void ImplicitAnimationObserver::OnLayerAnimationEnded(
    LayerAnimationSequence* sequence) {
  bool destroyed = false;
  destroyed_ = &destroyed;
  sequence->RemoveObserver(this);
  if (destroyed)
    return;
  destroyed_ = NULL;
  DCHECK(attached_sequences().find(sequence) == attached_sequences().end());
  CheckCompleted();
}

void ImplicitAnimationObserver::OnLayerAnimationAborted(
    LayerAnimationSequence* sequence) {
  sequence->RemoveObserver(this);
  DCHECK(attached_sequences().find(sequence) == attached_sequences().end());
  CheckCompleted();
}

void ImplicitAnimationObserver::OnLayerAnimationScheduled(
    LayerAnimationSequence* sequence) {
}

void ImplicitAnimationObserver::OnAttachedToSequence(
    LayerAnimationSequence* sequence) {
}

void ImplicitAnimationObserver::OnDetachedFromSequence(
    LayerAnimationSequence* sequence) {
  DCHECK(attached_sequences().find(sequence) == attached_sequences().end());
  CheckCompleted();
}

void ImplicitAnimationObserver::CheckCompleted() {
  if (active_ && attached_sequences().empty()) {
    OnImplicitAnimationsCompleted();
    active_ = false;
  }
}

}  // namespace ui
