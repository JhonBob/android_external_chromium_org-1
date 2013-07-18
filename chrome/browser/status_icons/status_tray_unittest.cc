// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/compiler_specific.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/status_icons/status_icon.h"
#include "chrome/browser/status_icons/status_tray.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Return;

class MockStatusIcon : public StatusIcon {
  virtual void SetImage(const gfx::ImageSkia& image) OVERRIDE {}
  virtual void SetPressedImage(const gfx::ImageSkia& image) OVERRIDE {}
  virtual void SetToolTip(const string16& tool_tip) OVERRIDE {}
  virtual void DisplayBalloon(const gfx::ImageSkia& icon,
                              const string16& title,
                              const string16& contents) OVERRIDE {}
  virtual void UpdatePlatformContextMenu(ui::MenuModel* menu) OVERRIDE {}
};

class TestStatusTray : public StatusTray {
 public:
  MOCK_METHOD1(CreatePlatformStatusIcon,
               StatusIcon*(StatusTray::StatusIconType type));
  MOCK_METHOD1(UpdatePlatformContextMenu, void(ui::MenuModel*));

  const StatusIcons& GetStatusIconsForTest() const { return status_icons(); }
};

TEST(StatusTrayTest, Create) {
  // Check for creation and leaks.
  TestStatusTray tray;
  EXPECT_CALL(tray, CreatePlatformStatusIcon(StatusTray::OTHER_ICON)).WillOnce(
      Return(new MockStatusIcon()));
  tray.CreateStatusIcon(StatusTray::OTHER_ICON);
}

// Make sure that removing an icon removes it from the list.
TEST(StatusTrayTest, CreateRemove) {
  TestStatusTray tray;
  EXPECT_CALL(tray, CreatePlatformStatusIcon(StatusTray::OTHER_ICON)).WillOnce(
      Return(new MockStatusIcon()));
  StatusIcon* icon = tray.CreateStatusIcon(StatusTray::OTHER_ICON);
  EXPECT_EQ(1U, tray.GetStatusIconsForTest().size());
  tray.RemoveStatusIcon(icon);
  EXPECT_EQ(0U, tray.GetStatusIconsForTest().size());
}
