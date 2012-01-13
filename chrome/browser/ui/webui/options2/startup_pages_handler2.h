// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OPTIONS2_STARTUP_PAGES_HANDLER2_H_
#define CHROME_BROWSER_UI_WEBUI_OPTIONS2_STARTUP_PAGES_HANDLER2_H_
#pragma once

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/autocomplete/autocomplete_controller_delegate.h"
#include "chrome/browser/prefs/pref_change_registrar.h"
#include "chrome/browser/prefs/pref_member.h"
#include "chrome/browser/ui/webui/options2/options_ui2.h"
#include "ui/base/models/table_model_observer.h"

class AutocompleteController;
class CustomHomePagesTableModel;
class TemplateURLService;

namespace options2 {

// Chrome browser options page UI handler.
class StartupPagesHandler : public OptionsPageUIHandler,
                            public AutocompleteControllerDelegate,
                            public ui::TableModelObserver {
 public:
  StartupPagesHandler();
  virtual ~StartupPagesHandler();

  virtual void Initialize() OVERRIDE;

  // OptionsPageUIHandler implementation.
  virtual void GetLocalizedValues(DictionaryValue* localized_strings) OVERRIDE;
  virtual void RegisterMessages() OVERRIDE;

  // AutocompleteControllerDelegate implementation.
  virtual void OnResultChanged(bool default_match_changed) OVERRIDE;

  // ui::TableModelObserver implementation.
  virtual void OnModelChanged() OVERRIDE;
  virtual void OnItemsChanged(int start, int length) OVERRIDE;
  virtual void OnItemsAdded(int start, int length) OVERRIDE;
  virtual void OnItemsRemoved(int start, int length) OVERRIDE;

 private:
  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Saves the changes that have been made. Called from WebUI.
  void CommitChanges(const ListValue* args);

  // Cancels the changes that have been made. Called from WebUI.
  void CancelChanges(const ListValue* args);

  // Removes the startup page at the given indexes. Called from WebUI.
  void RemoveStartupPages(const ListValue* args);

  // Adds a startup page with the given URL after the given index.
  // Called from WebUI.
  void AddStartupPage(const ListValue* args);

  // Changes the startup page at the given index to the given URL.
  // Called from WebUI.
  void EditStartupPage(const ListValue* args);

  // Sets the startup page set to the current pages. Called from WebUI.
  void SetStartupPagesToCurrentPages(const ListValue* args);

  // Writes the current set of startup pages to prefs. Called from WebUI.
  void DragDropStartupPage(const ListValue* args);

  // Gets autocomplete suggestions asychronously for the given string.
  // Called from WebUI.
  void RequestAutocompleteSuggestions(const ListValue* args);

  // Loads the current set of custom startup pages and reports it to the WebUI.
  void UpdateStartupPages();

  // Writes the current set of startup pages to prefs.
  void SaveStartupPagesPref();

  scoped_ptr<AutocompleteController> autocomplete_controller_;

  // Used to observe updates to the preference of the list of URLs to load
  // on startup, which can be updated via sync.
  PrefChangeRegistrar pref_change_registrar_;

  // TODO(stuartmorgan): Once there are no other clients of
  // CustomHomePagesTableModel, consider changing it to something more like
  // TemplateURLService.
  scoped_ptr<CustomHomePagesTableModel> startup_custom_pages_table_model_;

  DISALLOW_COPY_AND_ASSIGN(StartupPagesHandler);
};

}  // namespace options2

#endif  // CHROME_BROWSER_UI_WEBUI_OPTIONS2_STARTUP_PAGES_HANDLER2_H_
