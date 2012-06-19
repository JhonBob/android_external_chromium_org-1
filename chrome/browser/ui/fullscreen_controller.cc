// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/fullscreen_controller.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "chrome/browser/content_settings/host_content_settings_map.h"
#include "chrome/browser/download/download_shelf.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tab_contents/tab_contents.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"

using content::RenderViewHost;
using content::UserMetricsAction;
using content::WebContents;

FullscreenController::FullscreenController(BrowserWindow* window,
                                           Profile* profile,
                                           Browser* browser)
  : window_(window),
    profile_(profile),
    browser_(browser),
    fullscreened_tab_(NULL),
    tab_caused_fullscreen_(false),
    tab_fullscreen_accepted_(false),
    toggled_into_fullscreen_(false),
    mouse_lock_tab_(NULL),
    mouse_lock_state_(MOUSELOCK_NOT_REQUESTED),
    cancel_fullscreen_on_navigate_mode_(false) {
}

void FullscreenController::Observe(int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_NAV_ENTRY_COMMITTED:
      if (content::Details<content::LoadCommittedDetails>(details)->
          is_navigation_to_different_page()) {
        ExitTabFullscreenOrMouseLockIfNecessary();
      }
      break;

    default:
      NOTREACHED() << "Got a notification we didn't register for.";
  }
}

bool FullscreenController::IsFullscreenForBrowser() const {
  return window_->IsFullscreen() && !tab_caused_fullscreen_;
}

bool FullscreenController::IsFullscreenForTabOrPending() const {
  return fullscreened_tab_ != NULL;
}

bool FullscreenController::IsFullscreenForTabOrPending(
    const WebContents* web_contents) const {
  const TabContents* tab_contents =
      TabContents::FromWebContents(web_contents);
  if (!tab_contents || (tab_contents != fullscreened_tab_))
    return false;
  DCHECK(web_contents == browser_->GetActiveWebContents());
  return true;
}

#if defined(OS_WIN)
bool FullscreenController::IsInMetroSnapMode() {
  return window_->IsInMetroSnapMode();
}
#endif

bool FullscreenController::IsMouseLockRequested() const {
  return mouse_lock_state_ == MOUSELOCK_REQUESTED;
}

bool FullscreenController::IsMouseLocked() const {
  return mouse_lock_state_ == MOUSELOCK_ACCEPTED ||
         mouse_lock_state_ == MOUSELOCK_ACCEPTED_SILENTLY;
}

void FullscreenController::RequestToLockMouse(WebContents* web_contents,
                                              bool user_gesture,
                                              bool last_unlocked_by_target) {
  DCHECK(!IsMouseLocked());

  // Check for command line switch disabling mouse lock when not tab fullscreen.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableNonFullscreenMouseLock) &&
      !IsFullscreenForTabOrPending(web_contents)) {
    web_contents->GotResponseToLockMouseRequest(false);
    return;
  }

  NotifyMouseLockChange();

  // Must have a user gesture to prevent misbehaving sites from constantly
  // re-locking the mouse. Exceptions are when the page has unlocked
  // (i.e. not the user), or if we're in tab fullscreen (user gesture required
  // for that)
  if (!last_unlocked_by_target && !user_gesture &&
      !IsFullscreenForTabOrPending(web_contents)) {
    web_contents->GotResponseToLockMouseRequest(false);
    return;
  }
  mouse_lock_tab_ = TabContents::FromWebContents(web_contents);
  FullscreenExitBubbleType bubble_type = GetFullscreenExitBubbleType();

  switch (GetMouseLockSetting(web_contents->GetURL())) {
    case CONTENT_SETTING_ALLOW:
      // If bubble already displaying buttons we must not lock the mouse yet,
      // or it would prevent pressing those buttons. Instead, merge the request.
      if (fullscreen_bubble::ShowButtonsForType(bubble_type)) {
        mouse_lock_state_ = MOUSELOCK_REQUESTED;
      } else {
        // Lock mouse.
        if (web_contents->GotResponseToLockMouseRequest(true)) {
          if (last_unlocked_by_target) {
            mouse_lock_state_ = MOUSELOCK_ACCEPTED_SILENTLY;
          } else {
            mouse_lock_state_ = MOUSELOCK_ACCEPTED;
          }
        } else {
          mouse_lock_tab_ = NULL;
          mouse_lock_state_ = MOUSELOCK_NOT_REQUESTED;
        }
      }
      break;
    case CONTENT_SETTING_BLOCK:
      web_contents->GotResponseToLockMouseRequest(false);
      mouse_lock_tab_ = NULL;
      mouse_lock_state_ = MOUSELOCK_NOT_REQUESTED;
      break;
    case CONTENT_SETTING_ASK:
      mouse_lock_state_ = MOUSELOCK_REQUESTED;
      break;
    default:
      NOTREACHED();
  }
  UpdateFullscreenExitBubbleContent();
}

void FullscreenController::ToggleFullscreenModeForTab(WebContents* web_contents,
                                                      bool enter_fullscreen) {
  if (web_contents != browser_->GetActiveWebContents())
    return;

  bool in_browser_or_tab_fullscreen_mode;
#if defined(OS_MACOSX)
  in_browser_or_tab_fullscreen_mode = window_->InPresentationMode();
#else
  in_browser_or_tab_fullscreen_mode = window_->IsFullscreen();
#endif

  if (enter_fullscreen) {
    fullscreened_tab_ = TabContents::FromWebContents(web_contents);
    EnterCancelFullscreenOnNavigateMode();
    if (!in_browser_or_tab_fullscreen_mode) {
      tab_caused_fullscreen_ = true;
#if defined(OS_MACOSX)
      TogglePresentationModeInternal(true);
#else
      ToggleFullscreenModeInternal(true);
#endif
    } else {
      // We need to update the fullscreen exit bubble, e.g., going from browser
      // fullscreen to tab fullscreen will need to show different content.
      const GURL& url = web_contents->GetURL();
      if (!tab_fullscreen_accepted_) {
        tab_fullscreen_accepted_ =
            GetFullscreenSetting(url) == CONTENT_SETTING_ALLOW;
      }
      UpdateFullscreenExitBubbleContent();
    }
  } else {
    ExitCancelFullscreenOnNavigateMode();
    if (in_browser_or_tab_fullscreen_mode) {
      if (tab_caused_fullscreen_) {
#if defined(OS_MACOSX)
        TogglePresentationModeInternal(true);
#else
        ToggleFullscreenModeInternal(true);
#endif
      } else {
        // If currently there is a tab in "tab fullscreen" mode and fullscreen
        // was not caused by it (i.e., previously it was in "browser fullscreen"
        // mode), we need to switch back to "browser fullscreen" mode. In this
        // case, all we have to do is notifying the tab that it has exited "tab
        // fullscreen" mode.
        NotifyTabOfExitIfNecessary();
      }
    }
  }
}

#if defined(OS_WIN)
void FullscreenController::SetMetroSnapMode(bool enable) {
  window_->SetMetroSnapMode(enable);
}
#endif

#if defined(OS_MACOSX)
void FullscreenController::TogglePresentationMode() {
  TogglePresentationModeInternal(false);
}
#endif

void FullscreenController::ToggleFullscreenMode() {
  extension_caused_fullscreen_ = GURL();
  ToggleFullscreenModeInternal(false);
}

void FullscreenController::ToggleFullscreenModeWithExtension(
    const GURL& extension_url) {
  // |extension_caused_fullscreen_| will be reset if this causes fullscreen to
  // exit.
  extension_caused_fullscreen_ = extension_url;
  ToggleFullscreenModeInternal(false);
}

void FullscreenController::LostMouseLock() {
  mouse_lock_state_ = MOUSELOCK_NOT_REQUESTED;
  mouse_lock_tab_ = NULL;
  NotifyMouseLockChange();
  UpdateFullscreenExitBubbleContent();
}

void FullscreenController::OnTabClosing(WebContents* web_contents) {
  if (IsFullscreenForTabOrPending(web_contents)) {
    ExitTabFullscreenOrMouseLockIfNecessary();
    // The call to exit fullscreen may result in asynchronous notification of
    // fullscreen state change (e.g., on Linux). We don't want to rely on it
    // to call NotifyTabOfExitIfNecessary(), because at that point
    // |fullscreened_tab_| may not be valid. Instead, we call it here to clean
    // up tab fullscreen related state.
    NotifyTabOfExitIfNecessary();
  }
}

void FullscreenController::OnTabDeactivated(TabContents* contents) {
  if (contents == fullscreened_tab_)
    ExitTabFullscreenOrMouseLockIfNecessary();
}

void FullscreenController::OnAcceptFullscreenPermission(
    const GURL& url,
    FullscreenExitBubbleType bubble_type) {
  bool mouse_lock = false;
  bool fullscreen = false;
  fullscreen_bubble::PermissionRequestedByType(bubble_type, &fullscreen,
                                               &mouse_lock);
  DCHECK(!(fullscreen && tab_fullscreen_accepted_));
  DCHECK(!(mouse_lock && IsMouseLocked()));

  HostContentSettingsMap* settings_map = profile_->GetHostContentSettingsMap();
  ContentSettingsPattern pattern = ContentSettingsPattern::FromURL(url);

  if (mouse_lock && !IsMouseLocked()) {
    DCHECK(IsMouseLockRequested());
    // TODO(markusheintz): We should allow patterns for all possible URLs here.
    if (pattern.IsValid()) {
      settings_map->SetContentSetting(
          pattern, ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_MOUSELOCK, std::string(),
          CONTENT_SETTING_ALLOW);
    }

    if (mouse_lock_tab_ &&
        mouse_lock_tab_->web_contents() &&
        mouse_lock_tab_->web_contents()->GotResponseToLockMouseRequest(true)) {
      mouse_lock_state_ = MOUSELOCK_ACCEPTED;
    } else {
      mouse_lock_state_ = MOUSELOCK_NOT_REQUESTED;
      mouse_lock_tab_ = NULL;
    }
    NotifyMouseLockChange();
  }

  if (fullscreen && !tab_fullscreen_accepted_) {
    DCHECK(fullscreened_tab_);
    if (pattern.IsValid()) {
      settings_map->SetContentSetting(
          pattern, ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_FULLSCREEN, std::string(),
          CONTENT_SETTING_ALLOW);
    }
    tab_fullscreen_accepted_ = true;
  }
  UpdateFullscreenExitBubbleContent();
}

void FullscreenController::OnDenyFullscreenPermission(
    FullscreenExitBubbleType bubble_type) {
  bool mouse_lock = false;
  bool fullscreen = false;
  fullscreen_bubble::PermissionRequestedByType(bubble_type, &fullscreen,
                                               &mouse_lock);
  DCHECK(fullscreened_tab_ || mouse_lock_tab_);
  DCHECK(!(fullscreen && tab_fullscreen_accepted_));
  DCHECK(!(mouse_lock && IsMouseLocked()));

  if (mouse_lock) {
    DCHECK(IsMouseLockRequested());
    mouse_lock_state_ = MOUSELOCK_NOT_REQUESTED;
    if (mouse_lock_tab_ && mouse_lock_tab_->web_contents())
      mouse_lock_tab_->web_contents()->GotResponseToLockMouseRequest(false);
    mouse_lock_tab_ = NULL;
    NotifyMouseLockChange();

    // UpdateFullscreenExitBubbleContent() must be called, but to avoid
    // duplicate calls we do so only if not adjusting the fullscreen state
    // below, which also calls UpdateFullscreenExitBubbleContent().
    if (!fullscreen)
      UpdateFullscreenExitBubbleContent();
  }

  if (fullscreen)
    ExitTabFullscreenOrMouseLockIfNecessary();
}

void FullscreenController::WindowFullscreenStateChanged() {
  bool exiting_fullscreen;
#if defined(OS_MACOSX)
  exiting_fullscreen = !window_->InPresentationMode();
#else
  exiting_fullscreen = !window_->IsFullscreen();
#endif
  MessageLoop::current()->PostTask(FROM_HERE,
      base::Bind(&FullscreenController::NotifyFullscreenChange,
          this, !exiting_fullscreen));
  if (exiting_fullscreen)
    NotifyTabOfExitIfNecessary();
  if (exiting_fullscreen)
    window_->GetDownloadShelf()->Unhide();
  else
    window_->GetDownloadShelf()->Hide();
}

bool FullscreenController::HandleUserPressedEscape() {
  if (IsFullscreenForTabOrPending() ||
      IsMouseLocked() || IsMouseLockRequested()) {
    ExitTabFullscreenOrMouseLockIfNecessary();
    return true;
  }

  return false;
}

FullscreenController::~FullscreenController() {}

void FullscreenController::NotifyTabOfExitIfNecessary() {
  if (fullscreened_tab_) {
    ExitCancelFullscreenOnNavigateMode();
    RenderViewHost* rvh =
        fullscreened_tab_->web_contents()->GetRenderViewHost();
    fullscreened_tab_ = NULL;
    tab_caused_fullscreen_ = false;
    tab_fullscreen_accepted_ = false;
    if (rvh)
      rvh->ExitFullscreen();
  }

  if (mouse_lock_tab_) {
    WebContents* web_contents = mouse_lock_tab_->web_contents();
    if (IsMouseLockRequested()) {
      web_contents->GotResponseToLockMouseRequest(false);
    } else if (web_contents->GetRenderViewHost() &&
               web_contents->GetRenderViewHost()->GetView()) {
      web_contents->GetRenderViewHost()->GetView()->UnlockMouse();
    }
    mouse_lock_tab_ = NULL;
    mouse_lock_state_ = MOUSELOCK_NOT_REQUESTED;
  }

  UpdateFullscreenExitBubbleContent();
}

void FullscreenController::EnterCancelFullscreenOnNavigateMode() {
  if (cancel_fullscreen_on_navigate_mode_)
    return;
  cancel_fullscreen_on_navigate_mode_ = true;
  registrar_.Add(this, content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::Source<content::NavigationController>(
          &fullscreened_tab_->web_contents()->GetController()));
}

void FullscreenController::ExitCancelFullscreenOnNavigateMode() {
  if (!cancel_fullscreen_on_navigate_mode_)
    return;
  cancel_fullscreen_on_navigate_mode_ = false;
  registrar_.RemoveAll();
}


void FullscreenController::ExitTabFullscreenOrMouseLockIfNecessary() {
  if (tab_caused_fullscreen_)
    ToggleFullscreenMode();
  else
    NotifyTabOfExitIfNecessary();
}

void FullscreenController::UpdateFullscreenExitBubbleContent() {
  GURL url;
  if (fullscreened_tab_)
    url = fullscreened_tab_->web_contents()->GetURL();
  else if (mouse_lock_tab_)
    url = mouse_lock_tab_->web_contents()->GetURL();
  else if (!extension_caused_fullscreen_.is_empty())
    url = extension_caused_fullscreen_;

  FullscreenExitBubbleType bubble_type = GetFullscreenExitBubbleType();

  // If bubble displays buttons, unlock mouse to allow pressing them.
  if (fullscreen_bubble::ShowButtonsForType(bubble_type) &&
      IsMouseLocked() &&
      mouse_lock_tab_->web_contents()) {
    WebContents* web_contents = mouse_lock_tab_->web_contents();
    if (web_contents && web_contents->GetRenderViewHost() &&
        web_contents->GetRenderViewHost()->GetView())
      web_contents->GetRenderViewHost()->GetView()->UnlockMouse();
  }

  window_->UpdateFullscreenExitBubbleContent(url, bubble_type);
}

void FullscreenController::NotifyFullscreenChange(bool is_fullscreen) {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::Source<FullscreenController>(this),
      content::Details<bool>(&is_fullscreen));
}

void FullscreenController::NotifyMouseLockChange() {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_MOUSE_LOCK_CHANGED,
      content::Source<FullscreenController>(this),
      content::NotificationService::NoDetails());
}

FullscreenExitBubbleType FullscreenController::GetFullscreenExitBubbleType()
    const {
  // In kiosk mode we always want to be fullscreen and do not want to show
  // exit instructions for browser mode fullscreen.
  bool kiosk = false;
#if !defined(OS_MACOSX)  // Kiosk mode not available on Mac.
  kiosk = CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode);
#endif

  if (mouse_lock_state_ == MOUSELOCK_ACCEPTED_SILENTLY) {
    return FEB_TYPE_NONE;
  }

  if (fullscreened_tab_) {
    if (tab_fullscreen_accepted_) {
      if (IsMouseLocked()) {
        return FEB_TYPE_FULLSCREEN_MOUSELOCK_EXIT_INSTRUCTION;
      } else if (IsMouseLockRequested()) {
        return FEB_TYPE_MOUSELOCK_BUTTONS;
      } else {
        return FEB_TYPE_FULLSCREEN_EXIT_INSTRUCTION;
      }
    } else {  // Full screen not yet accepted.
      if (IsMouseLockRequested()) {
        return FEB_TYPE_FULLSCREEN_MOUSELOCK_BUTTONS;
      } else {
        return FEB_TYPE_FULLSCREEN_BUTTONS;
      }
    }
  } else {  // Not tab full screen.
    if (IsMouseLocked()) {
      return FEB_TYPE_MOUSELOCK_EXIT_INSTRUCTION;
    } else if (IsMouseLockRequested()) {
      return FEB_TYPE_MOUSELOCK_BUTTONS;
    } else {
      if (!extension_caused_fullscreen_.is_empty()) {
        return FEB_TYPE_BROWSER_EXTENSION_FULLSCREEN_EXIT_INSTRUCTION;
      } else if (toggled_into_fullscreen_ && !kiosk) {
        return FEB_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION;
      } else {
        return FEB_TYPE_NONE;
      }
    }
  }
  NOTREACHED();
  return FEB_TYPE_NONE;
}

ContentSetting
    FullscreenController::GetFullscreenSetting(const GURL& url) const {
  if (url.SchemeIsFile())
    return CONTENT_SETTING_ALLOW;

  return profile_->GetHostContentSettingsMap()->GetContentSetting(url, url,
      CONTENT_SETTINGS_TYPE_FULLSCREEN, std::string());
}

ContentSetting
    FullscreenController::GetMouseLockSetting(const GURL& url) const {
  if (url.SchemeIsFile())
    return CONTENT_SETTING_ALLOW;

  HostContentSettingsMap* settings_map = profile_->GetHostContentSettingsMap();
  return settings_map->GetContentSetting(url, url,
      CONTENT_SETTINGS_TYPE_MOUSELOCK, std::string());
}

#if defined(OS_MACOSX)
void FullscreenController::TogglePresentationModeInternal(bool for_tab) {
  toggled_into_fullscreen_ = !window_->InPresentationMode();
  GURL url;
  if (for_tab) {
    url = browser_->GetActiveWebContents()->GetURL();
    tab_fullscreen_accepted_ = toggled_into_fullscreen_ &&
        GetFullscreenSetting(url) == CONTENT_SETTING_ALLOW;
  }
  if (toggled_into_fullscreen_)
    window_->EnterPresentationMode(url, GetFullscreenExitBubbleType());
  else
    window_->ExitPresentationMode();
  UpdateFullscreenExitBubbleContent();

  // WindowFullscreenStateChanged will be called by BrowserWindowController
  // when the transition completes.
}
#endif

// TODO(koz): Change |for_tab| to an enum.
void FullscreenController::ToggleFullscreenModeInternal(bool for_tab) {
  toggled_into_fullscreen_ = !window_->IsFullscreen();

 // In kiosk mode, we always want to be fullscreen. When the browser first
 // starts we're not yet fullscreen, so let the initial toggle go through.
#if !defined(OS_MACOSX)  // Kiosk mode not available on Mac.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode) &&
      window_->IsFullscreen())
    return;
#endif

  GURL url;
  if (for_tab) {
    url = browser_->GetActiveWebContents()->GetURL();
    tab_fullscreen_accepted_ = toggled_into_fullscreen_ &&
        GetFullscreenSetting(url) == CONTENT_SETTING_ALLOW;
  } else {
    if (!extension_caused_fullscreen_.is_empty())
      url = extension_caused_fullscreen_;
    content::RecordAction(UserMetricsAction("ToggleFullscreen"));
  }
  if (toggled_into_fullscreen_) {
    window_->EnterFullscreen(url, GetFullscreenExitBubbleType());
  } else {
    window_->ExitFullscreen();
    extension_caused_fullscreen_ = GURL();
  }
  UpdateFullscreenExitBubbleContent();

  // Once the window has become fullscreen it'll call back to
  // WindowFullscreenStateChanged(). We don't do this immediately as
  // BrowserWindow::EnterFullscreen() asks for bookmark_bar_state_, so we let
  // the BrowserWindow invoke WindowFullscreenStateChanged when appropriate.
}
