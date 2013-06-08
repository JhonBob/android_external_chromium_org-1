// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_section_container.h"

#include "base/mac/foundation_util.h"
#include "base/memory/scoped_nsobject.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/autofill/autofill_dialog_models.h"
#include "chrome/browser/ui/autofill/mock_autofill_dialog_controller.h"
#import "chrome/browser/ui/cocoa/autofill/layout_view.h"
#import "chrome/browser/ui/cocoa/menu_button.h"
#include "grit/generated_resources.h"
#include "testing/gtest_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/simple_menu_model.h"
#import "ui/base/test/ui_cocoa_test_helper.h"

namespace {

class AutofillSectionContainerTest : public ui::CocoaTest {
 public:
  virtual void SetUp() {
    CocoaTest::SetUp();
    container_.reset(
        [[AutofillSectionContainer alloc]
            initWithController:&controller_
                    forSection:autofill::SECTION_CC]);
    [[test_window() contentView] addSubview:[container_ view]];
  }

  void ResetContainer() {
    container_.reset(
        [[AutofillSectionContainer alloc]
            initWithController:&controller_
                    forSection:autofill::SECTION_CC]);
    [[test_window() contentView] setSubviews:@[ [container_ view] ]];
  }

 protected:
  scoped_nsobject<AutofillSectionContainer> container_;
  testing::NiceMock<autofill::MockAutofillDialogController> controller_;
};

}  // namespace

TEST_VIEW(AutofillSectionContainerTest, [container_ view])

TEST_F(AutofillSectionContainerTest, HasSubviews) {
  bool hasLayoutView = false;
  bool hasTextField = false;
  bool hasSuggestButton = false;
  bool hasSuggestionView = false;

  ASSERT_EQ(4U, [[[container_ view] subviews] count]);
  for (NSView* view in [[container_ view] subviews]) {
    if ([view isKindOfClass:[NSTextField class]]) {
      hasTextField = true;
    } else if ([view isKindOfClass:[LayoutView class]]) {
      hasLayoutView = true;
    } else if ([view isKindOfClass:[MenuButton class]]) {
      hasSuggestButton = true;
    } else if ([view isKindOfClass:[NSView class]]) {
      hasSuggestionView = true;
    }
  }

  EXPECT_TRUE(hasSuggestButton);
  EXPECT_TRUE(hasLayoutView);
  EXPECT_TRUE(hasTextField);
  EXPECT_TRUE(hasSuggestionView);
}

TEST_F(AutofillSectionContainerTest, ModelsPopulateComboboxes) {
  using namespace autofill;
  using namespace testing;

  const DetailInput kTestInputs[] = {
    { 2, CREDIT_CARD_EXP_MONTH }
  };

  DetailInputs inputs;
  inputs.push_back(kTestInputs[0]);

  autofill::MonthComboboxModel comboModel;
  EXPECT_CALL(controller_, RequestedFieldsForSection(autofill::SECTION_CC))
      .WillOnce(ReturnRef(inputs));
  EXPECT_CALL(controller_, ComboboxModelForAutofillType(CREDIT_CARD_EXP_MONTH))
      .WillRepeatedly(Return(&comboModel));
  ResetContainer();

  NSPopUpButton* popup = base::mac::ObjCCastStrict<NSPopUpButton>(
      [container_ getField:CREDIT_CARD_EXP_MONTH]);
  EXPECT_EQ(13, [popup numberOfItems]);
  EXPECT_NSEQ(@"", [popup itemTitleAtIndex:0]);
  EXPECT_NSEQ(@"01", [popup itemTitleAtIndex:1]);
  EXPECT_NSEQ(@"02", [popup itemTitleAtIndex:2]);
  EXPECT_NSEQ(@"03", [popup itemTitleAtIndex:3]);
  EXPECT_NSEQ(@"12", [popup itemTitleAtIndex:12]);
};

TEST_F(AutofillSectionContainerTest, OutputMatchesDefinition) {
  using namespace autofill;
  using namespace testing;

  const DetailInput kTestInputs[] = {
    { 1, EMAIL_ADDRESS, IDS_AUTOFILL_DIALOG_PLACEHOLDER_EMAIL },
    { 2, CREDIT_CARD_EXP_MONTH }
  };
  autofill::MonthComboboxModel comboModel;
  DetailInputs inputs;
  inputs.push_back(kTestInputs[0]);
  inputs.push_back(kTestInputs[1]);

  EXPECT_CALL(controller_, RequestedFieldsForSection(autofill::SECTION_CC))
      .WillOnce(ReturnRef(inputs));
  EXPECT_CALL(controller_, ComboboxModelForAutofillType(EMAIL_ADDRESS))
      .WillRepeatedly(ReturnNull());
  EXPECT_CALL(controller_, ComboboxModelForAutofillType(CREDIT_CARD_EXP_MONTH))
      .WillRepeatedly(Return(&comboModel));

  ResetContainer();

  NSPopUpButton* popup = base::mac::ObjCCastStrict<NSPopUpButton>(
      [container_ getField:CREDIT_CARD_EXP_MONTH]);
  [popup selectItemWithTitle:@"02"];
  [[container_ getField:EMAIL_ADDRESS] setStringValue:@"magic@example.org"];

  autofill::DetailOutputMap output;
  [container_ getInputs:&output];

  ASSERT_EQ(inputs.size(), output.size());
  EXPECT_EQ(ASCIIToUTF16("magic@example.org"), output[&inputs[0]]);
  EXPECT_EQ(ASCIIToUTF16("02"), output[&inputs[1]]);
}

TEST_F(AutofillSectionContainerTest, SuggestionsPopulatedByController) {
  ui::SimpleMenuModel model(NULL);
  model.AddItem(10, ASCIIToUTF16("a"));
  model.AddItem(11, ASCIIToUTF16("b"));

  using namespace autofill;
  using namespace testing;

  EXPECT_CALL(controller_, MenuModelForSection(autofill::SECTION_CC))
      .WillOnce(Return(&model));

  ResetContainer();
  MenuButton* button = nil;
  for (id item in [[container_ view] subviews]) {
    if ([item isKindOfClass:[MenuButton class]]) {
      button = item;
      break;
    }
  }

  NSMenu* menu = [button attachedMenu];
  // Expect _three_ items - popup menus need an empty first item.
  ASSERT_EQ(3, [menu numberOfItems]);

  EXPECT_NSEQ(@"a", [[menu itemAtIndex:1] title]);
  EXPECT_NSEQ(@"b", [[menu itemAtIndex:2] title]);
}