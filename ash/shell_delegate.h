// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_DELEGATE_H_
#define ASH_SHELL_DELEGATE_H_

#include <string>

#include "ash/ash_export.h"
#include "ash/shell.h"
#include "base/callback.h"
#include "base/strings/string16.h"

namespace app_list {
class AppListViewDelegate;
}

namespace aura {
class RootWindow;
class Window;
}

namespace content {
class BrowserContext;
}

namespace ui {
class MenuModel;
}

namespace views {
class Widget;
}

namespace keyboard {
class KeyboardControllerProxy;
}

namespace ash {

class AccessibilityDelegate;
class MediaDelegate;
class NewWindowDelegate;
class WindowTreeHostFactory;
class SessionStateDelegate;
class ShelfDelegate;
class ShelfItemDelegate;
class ShelfModel;
class SystemTrayDelegate;
class UserWallpaperDelegate;
struct ShelfItem;

// Delegate of the Shell.
class ASH_EXPORT ShellDelegate {
 public:
  // The Shell owns the delegate.
  virtual ~ShellDelegate() {}

  // Returns true if this is the first time that the shell has been run after
  // the system has booted.  false is returned after the shell has been
  // restarted, typically due to logging in as a guest or logging out.
  virtual bool IsFirstRunAfterBoot() const = 0;

  // Returns true if multi-profiles feature is enabled.
  virtual bool IsMultiProfilesEnabled() const = 0;

  // Returns true if incognito mode is allowed for the user.
  // Incognito windows are restricted for supervised users.
  virtual bool IsIncognitoAllowed() const = 0;

  // Returns true if we're running in forced app mode.
  virtual bool IsRunningInForcedAppMode() const = 0;

  // Called before processing |Shell::Init()| so that the delegate
  // can perform tasks necessary before the shell is initialized.
  virtual void PreInit() = 0;

  // Shuts down the environment.
  virtual void Shutdown() = 0;

  // Invoked when the user uses Ctrl-Shift-Q to close chrome.
  virtual void Exit() = 0;

  // Create a shell-specific keyboard::KeyboardControllerProxy
  virtual keyboard::KeyboardControllerProxy*
      CreateKeyboardControllerProxy() = 0;

  // Get the active browser context. This will get us the active profile
  // in chrome.
  virtual content::BrowserContext* GetActiveBrowserContext() = 0;

  // Invoked to create an AppListViewDelegate. Shell takes the ownership of
  // the created delegate.
  virtual app_list::AppListViewDelegate* CreateAppListViewDelegate() = 0;

  // Creates a new ShelfDelegate. Shell takes ownership of the returned
  // value.
  virtual ShelfDelegate* CreateShelfDelegate(ShelfModel* model) = 0;

  // Creates a system-tray delegate. Shell takes ownership of the delegate.
  virtual SystemTrayDelegate* CreateSystemTrayDelegate() = 0;

  // Creates a user wallpaper delegate. Shell takes ownership of the delegate.
  virtual UserWallpaperDelegate* CreateUserWallpaperDelegate() = 0;

  // Creates a session state delegate. Shell takes ownership of the delegate.
  virtual SessionStateDelegate* CreateSessionStateDelegate() = 0;

  // Creates a accessibility delegate. Shell takes ownership of the delegate.
  virtual AccessibilityDelegate* CreateAccessibilityDelegate() = 0;

  // Creates an application delegate. Shell takes ownership of the delegate.
  virtual NewWindowDelegate* CreateNewWindowDelegate() = 0;

  // Creates a media delegate. Shell takes ownership of the delegate.
  virtual MediaDelegate* CreateMediaDelegate() = 0;

  // Creates a menu model of the context for the |root_window|.
  // When a ContextMenu is used for an item created by ShelfWindowWatcher,
  // passes its ShelfItemDelegate and ShelfItem.
  virtual ui::MenuModel* CreateContextMenu(
      aura::Window* root_window,
      ash::ShelfItemDelegate* item_delegate,
      ash::ShelfItem* item) = 0;

  // Creates a root window host factory. Shell takes ownership of the returned
  // value.
  virtual WindowTreeHostFactory* CreateWindowTreeHostFactory() = 0;

  // Creates a GPU support object. Shell takes ownership of the object.
  virtual GPUSupport* CreateGPUSupport() = 0;

  // Get the product name.
  virtual base::string16 GetProductName() const = 0;
};

}  // namespace ash

#endif  // ASH_SHELL_DELEGATE_H_
