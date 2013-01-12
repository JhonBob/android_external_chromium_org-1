// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_TABLE_TABLE_UTILS_H_
#define UI_VIEWS_CONTROLS_TABLE_TABLE_UTILS_H_

#include <vector>

#include "ui/base/models/table_model.h"
#include "ui/views/views_export.h"

namespace gfx {
class Font;
}

namespace views {

VIEWS_EXPORT extern const int kUnspecifiedColumnWidth;

// Returns the width needed to display the contents of the specified column.
// This is used internally by CalculateTableColumnSizes() and generally not
// useful by itself. |header_padding| is padding added to the header.
VIEWS_EXPORT int WidthForContent(const gfx::Font& header_font,
                                 const gfx::Font& content_font,
                                 int padding,
                                 int header_padding,
                                 const ui::TableColumn& column,
                                 ui::TableModel* model);

// Determines the width for each of the specified columns. |width| is the width
// to fit the columns into. |header_font| the font used to draw the header and
// |content_font| the header used to draw the content. |padding| is extra
// horizontal spaced added to each cell, and |header_padding| added to the
// width needed for the header.
VIEWS_EXPORT std::vector<int> CalculateTableColumnSizes(
    int width,
    int first_column_padding,
    const gfx::Font& header_font,
    const gfx::Font& content_font,
    int padding,
    int header_padding,
    const std::vector<ui::TableColumn>& columns,
    ui::TableModel* model);

// Converts a TableColumn::Alignment to the alignment for drawing the string.
int TableColumnAlignmentToCanvasAlignment(ui::TableColumn::Alignment alignment);

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_TABLE_TABLE_UTILS_H_
