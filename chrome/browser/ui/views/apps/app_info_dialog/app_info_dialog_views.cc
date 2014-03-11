// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_dialog_views.h"

#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_manage_tab.h"
#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_permissions_tab.h"
#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_summary_tab.h"
#include "chrome/browser/ui/views/constrained_window_views.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

void ShowAppInfoDialog(gfx::NativeWindow parent_window,
                       Profile* profile,
                       const extensions::Extension* app,
                       const base::Closure& close_callback) {
  CreateBrowserModalDialogViews(
      new AppInfoDialog(parent_window, profile, app, close_callback),
      parent_window)->Show();
}

AppInfoDialog::AppInfoDialog(gfx::NativeWindow parent_window,
                             Profile* profile,
                             const extensions::Extension* app,
                             const base::Closure& close_callback)
    : parent_window_(parent_window),
      profile_(profile),
      app_(app),
      close_callback_(close_callback) {
  SetLayoutManager(new views::FillLayout());

  views::TabbedPane* tabbed_pane = new views::TabbedPane();
  AddChildView(tabbed_pane);

  tabbed_pane->AddTab(
      l10n_util::GetStringUTF16(IDS_APPLICATION_INFO_SUMMARY_TAB_TITLE),
      new AppInfoSummaryTab(parent_window_, profile_, app_, close_callback_));
  tabbed_pane->AddTab(
      l10n_util::GetStringUTF16(IDS_APPLICATION_INFO_PERMISSIONS_TAB_TITLE),
      new AppInfoPermissionsTab(
          parent_window_, profile_, app_, close_callback_));
  tabbed_pane->AddTab(
      l10n_util::GetStringUTF16(IDS_APPLICATION_INFO_MANAGE_TAB_TITLE),
      new AppInfoManageTab(parent_window_, profile_, app_, close_callback_));
}

AppInfoDialog::~AppInfoDialog() {}

bool AppInfoDialog::Cancel() {
  if (!close_callback_.is_null())
    close_callback_.Run();
  return true;
}

gfx::Size AppInfoDialog::GetPreferredSize() {
  static const int kDialogWidth = 360;
  int height =
      GetLayoutManager()->GetPreferredHeightForWidth(this, kDialogWidth);
  return gfx::Size(kDialogWidth, height);
}

base::string16 AppInfoDialog::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_CANCEL) {
    return l10n_util::GetStringUTF16(IDS_APPLICATION_INFO_CLOSE_BUTTON);
  }
  return views::DialogDelegateView::GetDialogButtonLabel(button);
}

int AppInfoDialog::GetDialogButtons() const { return ui::DIALOG_BUTTON_CANCEL; }

bool AppInfoDialog::IsDialogButtonEnabled(ui::DialogButton button) const {
  return true;
}

ui::ModalType AppInfoDialog::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}
