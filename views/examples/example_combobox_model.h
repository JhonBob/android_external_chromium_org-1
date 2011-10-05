// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_EXAMPLES_EXAMPLE_COMBOBOX_MODEL_H_
#define VIEWS_EXAMPLES_EXAMPLE_COMBOBOX_MODEL_H_
#pragma once

#include "base/compiler_specific.h"
#include "ui/base/models/combobox_model.h"

namespace examples {

class ExampleComboboxModel : public ui::ComboboxModel {
 public:
  ExampleComboboxModel(const char** strings, int count);
  virtual ~ExampleComboboxModel();

  // Overridden from ui::ComboboxModel:
  virtual int GetItemCount() OVERRIDE;
  virtual string16 GetItemAt(int index) OVERRIDE;

 private:
  const char** strings_;
  int count_;

  DISALLOW_COPY_AND_ASSIGN(ExampleComboboxModel);
};

}  // namespace examples

#endif  // VIEWS_EXAMPLES_EXAMPLE_COMBOBOX_MODEL_H_
