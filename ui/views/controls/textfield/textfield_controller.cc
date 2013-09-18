// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/textfield/textfield_controller.h"

#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/events/event.h"

namespace views {

bool TextfieldController::HandleKeyEvent(Textfield* sender,
                                         const ui::KeyEvent& key_event) {
  return false;
}

bool TextfieldController::HandleMouseEvent(Textfield* sender,
                                           const ui::MouseEvent& mouse_event) {
  return false;
}

int TextfieldController::OnDrop(const ui::OSExchangeData& data) {
  return ui::DragDropTypes::DRAG_NONE;
}

bool TextfieldController::IsCommandIdEnabled(int command_id) const {
  return false;
}

bool TextfieldController::IsItemForCommandIdDynamic(int command_id) const {
  return false;
}

string16 TextfieldController::GetLabelForCommandId(int command_id) const {
  return string16();
}

bool TextfieldController::HandlesCommand(int command_id) const {
  return false;
}

}  // namespace views
