// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_CONTROLS_MENU_MENU_MODEL_ADAPTER_H_
#define VIEWS_CONTROLS_MENU_MENU_MODEL_ADAPTER_H_
#pragma once

#include <map>

#include "views/controls/menu/menu_delegate.h"

namespace ui {
class MenuModel;
}

namespace views {
class MenuItemView;

// This class wraps an instance of ui::MenuModel with the
// views::MenuDelegate interface required by views::MenuItemView.
class VIEWS_EXPORT MenuModelAdapter : public MenuDelegate {
 public:
  // The caller retains ownership of the ui::MenuModel instance and
  // must ensure it exists for the lifetime of the adapter.
  explicit MenuModelAdapter(ui::MenuModel* menu_model);
  virtual ~MenuModelAdapter();

  // Populate a MenuItemView menu with the ui::MenuModel items
  // (including submenus).
  virtual void BuildMenu(MenuItemView* menu);

  void set_triggerable_event_flags(int triggerable_event_flags) {
    triggerable_event_flags_ = triggerable_event_flags;
  }
  int triggerable_event_flags() const { return triggerable_event_flags_; }

 protected:
  // views::MenuDelegate implementation.
  virtual void ExecuteCommand(int id) OVERRIDE;
  virtual void ExecuteCommand(int id, int mouse_event_flags) OVERRIDE;
  virtual bool IsTriggerableEvent(MenuItemView* source,
                                  const MouseEvent& e) OVERRIDE;
  virtual bool GetAccelerator(int id,
                              views::Accelerator* accelerator) OVERRIDE;
  virtual std::wstring GetLabel(int id) const OVERRIDE;
  virtual const gfx::Font& GetLabelFont(int id) const OVERRIDE;
  virtual bool IsCommandEnabled(int id) const OVERRIDE;
  virtual bool IsItemChecked(int id) const OVERRIDE;
  virtual void SelectionChanged(MenuItemView* menu) OVERRIDE;
  virtual void WillShowMenu(MenuItemView* menu) OVERRIDE;
  virtual void WillHideMenu(MenuItemView* menu) OVERRIDE;

 private:
  // Implementation of BuildMenu().  index_offset is both input and output;
  // on input it contains the offset from index to command id for the model,
  // and on output it contains the offset for the next model.
  void BuildMenuImpl(MenuItemView* menu, ui::MenuModel* model);

  // Container of ui::MenuModel pointers as encountered by preorder
  // traversal.  The first element is always the top-level model
  // passed to the constructor.
  ui::MenuModel* menu_model_;

  // Mouse event flags which can trigger menu actions.
  int triggerable_event_flags_;

  // Map MenuItems to MenuModels.  Used to implement WillShowMenu().
  std::map<MenuItemView*, ui::MenuModel*> menu_map_;

  DISALLOW_COPY_AND_ASSIGN(MenuModelAdapter);
};

}  // namespace views

#endif  // VIEWS_CONTROLS_MENU_MENU_MODEL_ADAPTER_H_
