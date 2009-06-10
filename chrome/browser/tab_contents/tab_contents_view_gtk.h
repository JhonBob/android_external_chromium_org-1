// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_GTK_H_
#define CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_GTK_H_

#include <gtk/gtk.h>

#include "base/scoped_ptr.h"
#include "chrome/browser/gtk/focus_store_gtk.h"
#include "chrome/browser/tab_contents/tab_contents_view.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/owned_widget_gtk.h"

class BlockedPopupContainerViewGtk;
class RenderViewContextMenuGtk;
class SadTabGtk;
typedef struct _GtkFloatingContainer GtkFloatingContainer;

class TabContentsViewGtk : public TabContentsView,
                           public NotificationObserver {
 public:
  // The corresponding TabContents is passed in the constructor, and manages our
  // lifetime. This doesn't need to be the case, but is this way currently
  // because that's what was easiest when they were split.
  explicit TabContentsViewGtk(TabContents* tab_contents);
  virtual ~TabContentsViewGtk();

  // Unlike Windows, the BlockedPopupContainerView needs to collaborate with
  // the TabContentsViewGtk to position the notification.
  void AttachBlockedPopupView(BlockedPopupContainerViewGtk* popup_view);
  void RemoveBlockedPopupView(BlockedPopupContainerViewGtk* popup_view);

  // TabContentsView implementation --------------------------------------------

  virtual void CreateView();
  virtual RenderWidgetHostView* CreateViewForWidget(
      RenderWidgetHost* render_widget_host);

  virtual gfx::NativeView GetNativeView() const;
  virtual gfx::NativeView GetContentNativeView() const;
  virtual gfx::NativeWindow GetTopLevelNativeWindow() const;
  virtual void GetContainerBounds(gfx::Rect* out) const;
  virtual void OnContentsDestroy();
  virtual void SetPageTitle(const std::wstring& title);
  virtual void OnTabCrashed();
  virtual void SizeContents(const gfx::Size& size);
  virtual void Focus();
  virtual void SetInitialFocus();
  virtual void StoreFocus();
  virtual void RestoreFocus();

  // Backend implementation of RenderViewHostDelegate::View.
  virtual void ShowContextMenu(const ContextMenuParams& params);
  virtual void StartDragging(const WebDropData& drop_data);
  virtual void UpdateDragCursor(bool is_drop_target);
  virtual void TakeFocus(bool reverse);
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event);

  // NotificationObserver implementation ---------------------------------------

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  // Insert the given widget into the content area. Should only be used for
  // web pages and the like (including interstitials and sad tab). Note that
  // this will be perfectly happy to insert overlapping render views, so care
  // should be taken that the correct one is hidden/shown.
  void InsertIntoContentArea(GtkWidget* widget);

  // We keep track of the timestamp of the latest mousedown event.
  static gboolean OnMouseDown(GtkWidget* widget,
                              GdkEventButton* event, TabContentsViewGtk* view);

  // Used to propagate size changes on |fixed_| to its children.
  static gboolean OnSizeAllocate(GtkWidget* widget,
                                 GtkAllocation* config,
                                 TabContentsViewGtk* view);

  static void OnSetFloatingPosition(
      GtkFloatingContainer* floating_container, GtkAllocation* allocation,
      TabContentsViewGtk* tab_contents_view);

  // Contains |fixed_| as its GtkBin member and a possible floating widget from
  // |popup_view_|.
  OwnedWidgetGtk floating_;

  // This container holds the tab's web page views. It is a GtkFixed so that we
  // can control the size of the web pages.
  GtkWidget* fixed_;

  // The context menu is reset every time we show it, but we keep a pointer to
  // between uses so that it won't go out of scope before we're done with it.
  scoped_ptr<RenderViewContextMenuGtk> context_menu_;

  // The event time for the last mouse down we handled. We need this to properly
  // show context menus.
  guint32 last_mouse_down_time_;

  // Used to get notifications about renderers coming and going.
  NotificationRegistrar registrar_;

  scoped_ptr<SadTabGtk> sad_tab_;

  FocusStoreGtk focus_store_;

  BlockedPopupContainerViewGtk* popup_view_;

  DISALLOW_COPY_AND_ASSIGN(TabContentsViewGtk);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_GTK_H_
