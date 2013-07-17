// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <vector>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/common/cancelable_request.h"
#include "chrome/browser/sessions/persistent_tab_restore_service.h"
#include "chrome/browser/ui/cocoa/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/history_menu_bridge.h"
#include "chrome/common/favicon/favicon_types.h"
#include "components/sessions/serialized_navigation_entry_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

namespace {

class MockTRS : public PersistentTabRestoreService {
 public:
  MockTRS(Profile* profile) : PersistentTabRestoreService(profile, NULL) {}
  MOCK_CONST_METHOD0(entries, const TabRestoreService::Entries&());
};

class MockBridge : public HistoryMenuBridge {
 public:
  MockBridge(Profile* profile)
      : HistoryMenuBridge(profile),
        menu_([[NSMenu alloc] initWithTitle:@"History"]) {}

  virtual NSMenu* HistoryMenu() OVERRIDE {
    return menu_.get();
  }

 private:
  base::scoped_nsobject<NSMenu> menu_;
};

class HistoryMenuBridgeTest : public CocoaProfileTest {
 public:

  virtual void SetUp() {
    CocoaProfileTest::SetUp();
    profile()->CreateFaviconService();
    bridge_.reset(new MockBridge(profile()));
  }

  // We are a friend of HistoryMenuBridge (and have access to
  // protected methods), but none of the classes generated by TEST_F()
  // are. Wraps common commands.
  void ClearMenuSection(NSMenu* menu,
                        NSInteger tag) {
    bridge_->ClearMenuSection(menu, tag);
  }

  void AddItemToBridgeMenu(HistoryMenuBridge::HistoryItem* item,
                           NSMenu* menu,
                           NSInteger tag,
                           NSInteger index) {
    bridge_->AddItemToMenu(item, menu, tag, index);
  }

  NSMenuItem* AddItemToMenu(NSMenu* menu,
                            NSString* title,
                            SEL selector,
                            int tag) {
    NSMenuItem* item = [[[NSMenuItem alloc] initWithTitle:title action:NULL
                                            keyEquivalent:@""] autorelease];
    [item setTag:tag];
    if (selector) {
      [item setAction:selector];
      [item setTarget:bridge_->controller_.get()];
    }
    [menu addItem:item];
    return item;
  }

  HistoryMenuBridge::HistoryItem* CreateItem(const string16& title) {
    HistoryMenuBridge::HistoryItem* item =
        new HistoryMenuBridge::HistoryItem();
    item->title = title;
    item->url = GURL(title);
    return item;
  }

  MockTRS::Tab CreateSessionTab(const std::string& url,
                                const std::string& title) {
    MockTRS::Tab tab;
    tab.current_navigation_index = 0;
    tab.navigations.push_back(
        sessions::SerializedNavigationEntryTestHelper::CreateNavigation(
            url, title));
    return tab;
  }

  void GetFaviconForHistoryItem(HistoryMenuBridge::HistoryItem* item) {
    bridge_->GetFaviconForHistoryItem(item);
  }

  void GotFaviconData(
      HistoryMenuBridge::HistoryItem* item,
      const chrome::FaviconImageResult& image_result) {
    bridge_->GotFaviconData(item, image_result);
  }

  CancelableRequestConsumerTSimple<HistoryMenuBridge::HistoryItem*>&
      favicon_consumer() {
    return bridge_->favicon_consumer_;
  }

  scoped_ptr<MockBridge> bridge_;
};

// Edge case test for clearing until the end of a menu.
TEST_F(HistoryMenuBridgeTest, ClearHistoryMenuUntilEnd) {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"history foo"] autorelease];
  AddItemToMenu(menu, @"HEADER", NULL, HistoryMenuBridge::kVisitedTitle);

  NSInteger tag = HistoryMenuBridge::kVisited;
  AddItemToMenu(menu, @"alpha", @selector(openHistoryMenuItem:), tag);
  AddItemToMenu(menu, @"bravo", @selector(openHistoryMenuItem:), tag);
  AddItemToMenu(menu, @"charlie", @selector(openHistoryMenuItem:), tag);
  AddItemToMenu(menu, @"delta", @selector(openHistoryMenuItem:), tag);

  ClearMenuSection(menu, HistoryMenuBridge::kVisited);

  EXPECT_EQ(1, [menu numberOfItems]);
  EXPECT_NSEQ(@"HEADER",
      [[menu itemWithTag:HistoryMenuBridge::kVisitedTitle] title]);
}

// Skip menu items that are not hooked up to |-openHistoryMenuItem:|.
TEST_F(HistoryMenuBridgeTest, ClearHistoryMenuSkipping) {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"history foo"] autorelease];
  AddItemToMenu(menu, @"HEADER", NULL, HistoryMenuBridge::kVisitedTitle);

  NSInteger tag = HistoryMenuBridge::kVisited;
  AddItemToMenu(menu, @"alpha", @selector(openHistoryMenuItem:), tag);
  AddItemToMenu(menu, @"bravo", @selector(openHistoryMenuItem:), tag);
  AddItemToMenu(menu, @"TITLE", NULL, HistoryMenuBridge::kRecentlyClosedTitle);
  AddItemToMenu(menu, @"charlie", @selector(openHistoryMenuItem:), tag);

  ClearMenuSection(menu, tag);

  EXPECT_EQ(2, [menu numberOfItems]);
  EXPECT_NSEQ(@"HEADER",
      [[menu itemWithTag:HistoryMenuBridge::kVisitedTitle] title]);
  EXPECT_NSEQ(@"TITLE",
      [[menu itemAtIndex:1] title]);
}

// Edge case test for clearing an empty menu.
TEST_F(HistoryMenuBridgeTest, ClearHistoryMenuEmpty) {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"history foo"] autorelease];
  AddItemToMenu(menu, @"HEADER", NULL, HistoryMenuBridge::kVisited);

  ClearMenuSection(menu, HistoryMenuBridge::kVisited);

  EXPECT_EQ(1, [menu numberOfItems]);
  EXPECT_NSEQ(@"HEADER",
      [[menu itemWithTag:HistoryMenuBridge::kVisited] title]);
}

// Test that AddItemToMenu() properly adds HistoryItem objects as menus.
TEST_F(HistoryMenuBridgeTest, AddItemToMenu) {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"history foo"] autorelease];

  const string16 short_url = ASCIIToUTF16("http://foo/");
  const string16 long_url = ASCIIToUTF16("http://super-duper-long-url--."
      "that.cannot.possibly.fit.even-in-80-columns"
      "or.be.reasonably-displayed-in-a-menu"
      "without.looking-ridiculous.com/"); // 140 chars total

  // HistoryItems are owned by the HistoryMenuBridge when AddItemToBridgeMenu()
  // is called, which places them into the |menu_item_map_|, which owns them.
  HistoryMenuBridge::HistoryItem* item1 = CreateItem(short_url);
  AddItemToBridgeMenu(item1, menu, 100, 0);

  HistoryMenuBridge::HistoryItem* item2 = CreateItem(long_url);
  AddItemToBridgeMenu(item2, menu, 101, 1);

  EXPECT_EQ(2, [menu numberOfItems]);

  EXPECT_EQ(@selector(openHistoryMenuItem:), [[menu itemAtIndex:0] action]);
  EXPECT_EQ(@selector(openHistoryMenuItem:), [[menu itemAtIndex:1] action]);

  EXPECT_EQ(100, [[menu itemAtIndex:0] tag]);
  EXPECT_EQ(101, [[menu itemAtIndex:1] tag]);

  // Make sure a short title looks fine
  NSString* s = [[menu itemAtIndex:0] title];
  EXPECT_EQ(base::SysNSStringToUTF16(s), short_url);

  // Make sure a super-long title gets trimmed
  s = [[menu itemAtIndex:0] title];
  EXPECT_TRUE([s length] < long_url.length());

  // Confirm tooltips and confirm they are not trimmed (like the item
  // name might be).  Add tolerance for URL fixer-upping;
  // e.g. http://foo becomes http://foo/)
  EXPECT_GE([[[menu itemAtIndex:0] toolTip] length], (2*short_url.length()-5));
  EXPECT_GE([[[menu itemAtIndex:1] toolTip] length], (2*long_url.length()-5));
}

// Test that the menu is created for a set of simple tabs.
TEST_F(HistoryMenuBridgeTest, RecentlyClosedTabs) {
  scoped_ptr<MockTRS> trs(new MockTRS(profile()));
  MockTRS::Entries entries;

  MockTRS::Tab tab1 = CreateSessionTab("http://google.com", "Google");
  tab1.id = 24;
  entries.push_back(&tab1);

  MockTRS::Tab tab2 = CreateSessionTab("http://apple.com", "Apple");
  tab2.id = 42;
  entries.push_back(&tab2);

  using ::testing::ReturnRef;
  EXPECT_CALL(*trs.get(), entries()).WillOnce(ReturnRef(entries));

  bridge_->TabRestoreServiceChanged(trs.get());

  NSMenu* menu = bridge_->HistoryMenu();
  ASSERT_EQ(2U, [[menu itemArray] count]);

  NSMenuItem* item1 = [menu itemAtIndex:0];
  MockBridge::HistoryItem* hist1 = bridge_->HistoryItemForMenuItem(item1);
  EXPECT_TRUE(hist1);
  EXPECT_EQ(24, hist1->session_id);
  EXPECT_NSEQ(@"Google", [item1 title]);

  NSMenuItem* item2 = [menu itemAtIndex:1];
  MockBridge::HistoryItem* hist2 = bridge_->HistoryItemForMenuItem(item2);
  EXPECT_TRUE(hist2);
  EXPECT_EQ(42, hist2->session_id);
  EXPECT_NSEQ(@"Apple", [item2 title]);
}

// Test that the menu is created for a mix of windows and tabs.
TEST_F(HistoryMenuBridgeTest, RecentlyClosedTabsAndWindows) {
  scoped_ptr<MockTRS> trs(new MockTRS(profile()));
  MockTRS::Entries entries;

  MockTRS::Tab tab1 = CreateSessionTab("http://google.com", "Google");
  tab1.id = 24;
  entries.push_back(&tab1);

  MockTRS::Window win1;
  win1.id = 30;
  win1.tabs.push_back(CreateSessionTab("http://foo.com", "foo"));
  win1.tabs[0].id = 31;
  win1.tabs.push_back(CreateSessionTab("http://bar.com", "bar"));
  win1.tabs[1].id = 32;
  entries.push_back(&win1);

  MockTRS::Tab tab2 = CreateSessionTab("http://apple.com", "Apple");
  tab2.id = 42;
  entries.push_back(&tab2);

  MockTRS::Window win2;
  win2.id = 50;
  win2.tabs.push_back(CreateSessionTab("http://magic.com", "magic"));
  win2.tabs[0].id = 51;
  win2.tabs.push_back(CreateSessionTab("http://goats.com", "goats"));
  win2.tabs[1].id = 52;
  win2.tabs.push_back(CreateSessionTab("http://teleporter.com", "teleporter"));
  win2.tabs[1].id = 53;
  entries.push_back(&win2);

  using ::testing::ReturnRef;
  EXPECT_CALL(*trs.get(), entries()).WillOnce(ReturnRef(entries));

  bridge_->TabRestoreServiceChanged(trs.get());

  NSMenu* menu = bridge_->HistoryMenu();
  ASSERT_EQ(4U, [[menu itemArray] count]);

  NSMenuItem* item1 = [menu itemAtIndex:0];
  MockBridge::HistoryItem* hist1 = bridge_->HistoryItemForMenuItem(item1);
  EXPECT_TRUE(hist1);
  EXPECT_EQ(24, hist1->session_id);
  EXPECT_NSEQ(@"Google", [item1 title]);

  NSMenuItem* item2 = [menu itemAtIndex:1];
  MockBridge::HistoryItem* hist2 = bridge_->HistoryItemForMenuItem(item2);
  EXPECT_TRUE(hist2);
  EXPECT_EQ(30, hist2->session_id);
  EXPECT_EQ(2U, hist2->tabs.size());
  // Do not test menu item title because it is localized.
  NSMenu* submenu1 = [item2 submenu];
  EXPECT_EQ(4U, [[submenu1 itemArray] count]);
  // Do not test Restore All Tabs because it is localiced.
  EXPECT_TRUE([[submenu1 itemAtIndex:1] isSeparatorItem]);
  EXPECT_NSEQ(@"foo", [[submenu1 itemAtIndex:2] title]);
  EXPECT_NSEQ(@"bar", [[submenu1 itemAtIndex:3] title]);

  NSMenuItem* item3 = [menu itemAtIndex:2];
  MockBridge::HistoryItem* hist3 = bridge_->HistoryItemForMenuItem(item3);
  EXPECT_TRUE(hist3);
  EXPECT_EQ(42, hist3->session_id);
  EXPECT_NSEQ(@"Apple", [item3 title]);

  NSMenuItem* item4 = [menu itemAtIndex:3];
  MockBridge::HistoryItem* hist4 = bridge_->HistoryItemForMenuItem(item4);
  EXPECT_TRUE(hist4);
  EXPECT_EQ(50, hist4->session_id);
  EXPECT_EQ(3U, hist4->tabs.size());
  // Do not test menu item title because it is localized.
  NSMenu* submenu2 = [item4 submenu];
  EXPECT_EQ(5U, [[submenu2 itemArray] count]);
  // Do not test Restore All Tabs because it is localiced.
  EXPECT_TRUE([[submenu2 itemAtIndex:1] isSeparatorItem]);
  EXPECT_NSEQ(@"magic", [[submenu2 itemAtIndex:2] title]);
  EXPECT_NSEQ(@"goats", [[submenu2 itemAtIndex:3] title]);
  EXPECT_NSEQ(@"teleporter", [[submenu2 itemAtIndex:4] title]);
}

// Tests that we properly request an icon from the FaviconService.
TEST_F(HistoryMenuBridgeTest, GetFaviconForHistoryItem) {
  // Create a fake item.
  HistoryMenuBridge::HistoryItem item;
  item.title = ASCIIToUTF16("Title");
  item.url = GURL("http://google.com");

  // Request the icon.
  GetFaviconForHistoryItem(&item);

  // Make sure the item was modified properly.
  EXPECT_TRUE(item.icon_requested);
  EXPECT_NE(CancelableTaskTracker::kBadTaskId, item.icon_task_id);
}

TEST_F(HistoryMenuBridgeTest, GotFaviconData) {
  // Create a dummy bitmap.
  SkBitmap bitmap;
  bitmap.setConfig(SkBitmap::kARGB_8888_Config, 25, 25);
  bitmap.allocPixels();
  bitmap.eraseRGB(255, 0, 0);

  // Set up the HistoryItem.
  HistoryMenuBridge::HistoryItem item;
  item.menu_item.reset([[NSMenuItem alloc] init]);
  GetFaviconForHistoryItem(&item);

  // Pretend to be called back.
  chrome::FaviconImageResult image_result;
  image_result.image = gfx::Image::CreateFrom1xBitmap(bitmap);
  GotFaviconData(&item, image_result);

  // Make sure the callback works.
  EXPECT_FALSE(item.icon_requested);
  EXPECT_TRUE(item.icon.get());
  EXPECT_TRUE([item.menu_item image]);
}

}  // namespace
