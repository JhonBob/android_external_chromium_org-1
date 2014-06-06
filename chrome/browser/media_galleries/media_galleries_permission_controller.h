// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_GALLERIES_MEDIA_GALLERIES_PERMISSION_CONTROLLER_H_
#define CHROME_BROWSER_MEDIA_GALLERIES_MEDIA_GALLERIES_PERMISSION_CONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/media_galleries/media_galleries_dialog_controller.h"
#include "chrome/browser/media_galleries/media_galleries_preferences.h"
#include "components/storage_monitor/removable_storage_observer.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class WebContents;
}

namespace extensions {
class Extension;
}

namespace ui {
class MenuModel;
}

class MediaGalleriesDialogController;
class MediaGalleryContextMenu;
class Profile;

// Newly added galleries are not added to preferences until the dialog commits,
// so they do not have a pref id while the dialog is open; leading to
// complicated code in the dialogs. To solve this complication, the controller
// maps pref ids into a new space where it can also assign ids to new galleries.
// The new number space is only valid for the lifetime of the controller. To
// make it more clear where real pref ids are used and where the fake ids are
// used, the GalleryDialogId type is used where fake ids are needed.
typedef MediaGalleryPrefId GalleryDialogId;

class MediaGalleriesPermissionController
    : public MediaGalleriesDialogController,
      public ui::SelectFileDialog::Listener,
      public storage_monitor::RemovableStorageObserver,
      public MediaGalleriesPreferences::GalleryChangeObserver {
 public:
  // The constructor creates a dialog controller which owns itself.
  MediaGalleriesPermissionController(content::WebContents* web_contents,
                                     const extensions::Extension& extension,
                                     const base::Closure& on_finish);

  // MediaGalleriesDialogController implementation.
  virtual base::string16 GetHeader() const OVERRIDE;
  virtual base::string16 GetSubtext() const OVERRIDE;
  virtual bool IsAcceptAllowed() const OVERRIDE;
  virtual bool ShouldShowFolderViewer(const Entry& entry) const OVERRIDE;
  virtual std::vector<base::string16> GetSectionHeaders() const OVERRIDE;
  virtual Entries GetSectionEntries(size_t index) const OVERRIDE;
  // Auxiliary button for this dialog is the 'Add Folder' button.
  virtual base::string16 GetAuxiliaryButtonText() const OVERRIDE;
  virtual void DidClickAuxiliaryButton() OVERRIDE;
  virtual void DidToggleEntry(GalleryDialogId gallery_id,
                              bool selected) OVERRIDE;
  virtual void DidClickOpenFolderViewer(GalleryDialogId gallery_id) OVERRIDE;
  virtual void DidForgetEntry(GalleryDialogId gallery_id) OVERRIDE;
  virtual base::string16 GetAcceptButtonText() const OVERRIDE;
  virtual void DialogFinished(bool accepted) OVERRIDE;
  virtual ui::MenuModel* GetContextMenu(GalleryDialogId gallery_id) OVERRIDE;
  virtual content::WebContents* WebContents() OVERRIDE;

 protected:
  friend class MediaGalleriesPermissionControllerTest;

  typedef base::Callback<MediaGalleriesDialog* (
      MediaGalleriesDialogController*)> CreateDialogCallback;

  // For use with tests.
  MediaGalleriesPermissionController(
      const extensions::Extension& extension,
      MediaGalleriesPreferences* preferences,
      const CreateDialogCallback& create_dialog_callback,
      const base::Closure& on_finish);

  virtual ~MediaGalleriesPermissionController();

 private:
  // This type keeps track of media galleries already known to the prefs system.
  typedef std::map<GalleryDialogId, Entry> GalleryPermissionsMap;

  class DialogIdMap {
   public:
    DialogIdMap();
    ~DialogIdMap();
    GalleryDialogId GetDialogId(MediaGalleryPrefId pref_id);
    MediaGalleryPrefId GetPrefId(GalleryDialogId id);

   private:
    GalleryDialogId next_dialog_id_;
    std::map<MediaGalleryPrefId, GalleryDialogId> back_map_;
    std::vector<MediaGalleryPrefId> forward_mapping_;
    DISALLOW_COPY_AND_ASSIGN(DialogIdMap);
  };


  // Bottom half of constructor -- called when |preferences_| is initialized.
  void OnPreferencesInitialized();

  // SelectFileDialog::Listener implementation:
  virtual void FileSelected(const base::FilePath& path,
                            int index,
                            void* params) OVERRIDE;

  // RemovableStorageObserver implementation.
  // Used to keep dialog in sync with removable device status.
  virtual void OnRemovableStorageAttached(
      const storage_monitor::StorageInfo& info) OVERRIDE;
  virtual void OnRemovableStorageDetached(
      const storage_monitor::StorageInfo& info) OVERRIDE;

  // MediaGalleriesPreferences::GalleryChangeObserver implementations.
  // Used to keep the dialog in sync when the preferences change.
  virtual void OnPermissionAdded(MediaGalleriesPreferences* pref,
                                 const std::string& extension_id,
                                 MediaGalleryPrefId pref_id) OVERRIDE;
  virtual void OnPermissionRemoved(MediaGalleriesPreferences* pref,
                                   const std::string& extension_id,
                                   MediaGalleryPrefId pref_id) OVERRIDE;
  virtual void OnGalleryAdded(MediaGalleriesPreferences* pref,
                              MediaGalleryPrefId pref_id) OVERRIDE;
  virtual void OnGalleryRemoved(MediaGalleriesPreferences* pref,
                                MediaGalleryPrefId pref_id) OVERRIDE;
  virtual void OnGalleryInfoUpdated(MediaGalleriesPreferences* pref,
                                    MediaGalleryPrefId pref_id) OVERRIDE;

  // Populates |known_galleries_| from |preferences_|. Subsequent calls merge
  // into |known_galleries_| and do not change permissions for user toggled
  // galleries.
  void InitializePermissions();

  // Saves state of |known_galleries_|, |new_galleries_| and
  // |forgotten_galleries_| to model.
  //
  // NOTE: possible states for a gallery:
  //   K   N   F   (K = Known, N = New, F = Forgotten)
  // +---+---+---+
  // | Y | N | N |
  // +---+---+---+
  // | N | Y | N |
  // +---+---+---+
  // | Y | N | Y |
  // +---+---+---+
  void SavePermissions();

  // Updates the model and view when |preferences_| changes. Some of the
  // possible changes includes a gallery getting blacklisted, or a new
  // auto detected gallery becoming available.
  void UpdateGalleriesOnPreferencesEvent();

  // Updates the model and view when a device is attached or detached.
  void UpdateGalleriesOnDeviceEvent(const std::string& device_id);

  GalleryDialogId GetDialogId(MediaGalleryPrefId pref_id);
  MediaGalleryPrefId GetPrefId(GalleryDialogId id);

  Profile* GetProfile();

  // The web contents from which the request originated.
  content::WebContents* web_contents_;

  // This is just a reference, but it's assumed that it won't become invalid
  // while the dialog is showing.
  const extensions::Extension* extension_;

  // Mapping between pref ids and dialog ids.
  DialogIdMap id_map_;

  // This map excludes those galleries which have been blacklisted; it only
  // counts active known galleries.
  GalleryPermissionsMap known_galleries_;

  // Galleries in |known_galleries_| that the user have toggled.
  std::set<GalleryDialogId> toggled_galleries_;

  // Map of new galleries the user added, but have not saved. This list should
  // never overlap with |known_galleries_|.
  GalleryPermissionsMap new_galleries_;

  // Galleries in |known_galleries_| that the user has forgotten.
  std::set<GalleryDialogId> forgotten_galleries_;

  // Callback to run when the dialog closes.
  base::Closure on_finish_;

  // The model that tracks galleries and extensions' permissions.
  // This is the authoritative source for gallery information.
  MediaGalleriesPreferences* preferences_;

  // The view that's showing.
  scoped_ptr<MediaGalleriesDialog> dialog_;

  scoped_refptr<ui::SelectFileDialog> select_folder_dialog_;

  scoped_ptr<MediaGalleryContextMenu> context_menu_;

  // Creates the dialog. Only changed for unit tests.
  CreateDialogCallback create_dialog_callback_;

  DISALLOW_COPY_AND_ASSIGN(MediaGalleriesPermissionController);
};

#endif  // CHROME_BROWSER_MEDIA_GALLERIES_MEDIA_GALLERIES_PERMISSION_CONTROLLER_H_
