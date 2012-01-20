// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_SHELL_DOWNLOAD_MANAGER_DELEGATE_H_
#define CONTENT_SHELL_SHELL_DOWNLOAD_MANAGER_DELEGATE_H_
#pragma once

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/download_manager_delegate.h"

struct DownloadStateInfo;

namespace content {

class DownloadManager;

class ShellDownloadManagerDelegate
    : public DownloadManagerDelegate,
      public base::RefCountedThreadSafe<ShellDownloadManagerDelegate> {
 public:
  ShellDownloadManagerDelegate();

  void SetDownloadManager(DownloadManager* manager);

  virtual void Shutdown() OVERRIDE;
  virtual bool ShouldStartDownload(int32 download_id) OVERRIDE;
  virtual void ChooseDownloadPath(WebContents* web_contents,
                                  const FilePath& suggested_path,
                                  void* data) OVERRIDE;
  virtual bool OverrideIntermediatePath(DownloadItem* item,
                                        FilePath* intermediate_path) OVERRIDE;
  virtual WebContents* GetAlternativeWebContentsToNotifyForDownload() OVERRIDE;
  virtual bool ShouldOpenFileBasedOnExtension(const FilePath& path) OVERRIDE;
  virtual bool ShouldCompleteDownload(DownloadItem* item) OVERRIDE;
  virtual bool ShouldOpenDownload(DownloadItem* item) OVERRIDE;
  virtual bool GenerateFileHash() OVERRIDE;
  virtual void OnResponseCompleted(DownloadItem* item) OVERRIDE;
  virtual void AddItemToPersistentStore(DownloadItem* item) OVERRIDE;
  virtual void UpdateItemInPersistentStore(DownloadItem* item) OVERRIDE;
  virtual void UpdatePathForItemInPersistentStore(
      DownloadItem* item,
      const FilePath& new_path) OVERRIDE;
  virtual void RemoveItemFromPersistentStore(DownloadItem* item) OVERRIDE;
  virtual void RemoveItemsFromPersistentStoreBetween(
      base::Time remove_begin,
      base::Time remove_end) OVERRIDE;
  virtual void GetSaveDir(WebContents* web_contents,
                          FilePath* website_save_dir,
                          FilePath* download_save_dir) OVERRIDE;
  virtual void ChooseSavePath(
      content::WebContents* web_contents,
      const FilePath& suggested_path,
      const FilePath::StringType& default_extension,
      bool can_save_as_complete,
      content::SaveFilePathPickedCallback callback) OVERRIDE;
  virtual void DownloadProgressUpdated() OVERRIDE;

 private:
  friend class base::RefCountedThreadSafe<ShellDownloadManagerDelegate>;

  virtual ~ShellDownloadManagerDelegate();

  void GenerateFilename(int32 download_id,
                        DownloadStateInfo state,
                        const FilePath& generated_name);
  void RestartDownload(int32 download_id,
                       DownloadStateInfo state);

  DownloadManager* download_manager_;

  DISALLOW_COPY_AND_ASSIGN(ShellDownloadManagerDelegate);
};

}  // namespace content

#endif  // CONTENT_SHELL_SHELL_DOWNLOAD_MANAGER_DELEGATE_H_
