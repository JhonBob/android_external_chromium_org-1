// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_PICKER_MODEL_H_
#define CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_PICKER_MODEL_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "content/public/common/desktop_media_id.h"
#include "ui/gfx/image/image_skia.h"

namespace webrtc {
class ScreenCapturer;
class WindowCapturer;
}

// DesktopMediaPickerModel provides the list of desktop media source (screens,
// windows, tabs), and their thumbnails, to the desktop media picker dialog. It
// transparently updates the list in the background, and notifies the desktop
// media picker when something changes.
class DesktopMediaPickerModel {
 public:
  // Interface implemented by the picker dialog to receive notifications when
  // the model's contents change.
  class Observer {
   public:
    virtual ~Observer() {}

    virtual void OnSourceAdded(int index) = 0;
    virtual void OnSourceRemoved(int index) = 0;
    virtual void OnSourceNameChanged(int index) = 0;
    virtual void OnSourceThumbnailChanged(int index) = 0;
  };

  // Struct used to represent each entry in the model.
  struct Source {
    Source(content::DesktopMediaID id, const string16& name);

    // Id of the source.
    content::DesktopMediaID id;

    // Name of the source that should be shown to the user.
    string16 name;

    // The thumbnail for the source.
    gfx::ImageSkia thumbnail;
  };

  DesktopMediaPickerModel();
  virtual ~DesktopMediaPickerModel();

  // Sets screen/window capturer implementations to use (e.g. for tests). Caller
  // may pass NULL for either of the arguments in case when only some types of
  // sources the model should be populated with (e.g. it will only contain
  // windows, if |screen_capturer| is NULL). Must be called before
  // StartUpdating().
  void SetCapturers(scoped_ptr<webrtc::ScreenCapturer> screen_capturer,
                    scoped_ptr<webrtc::WindowCapturer> window_capturer);

  // Sets time interval between updates. By default list of sources and their
  // thumbnail are updated once per second. If called after StartUpdating() then
  // it will take effect only after the next update.
  void SetUpdatePeriod(base::TimeDelta period);

  // Sets size to which the thumbnails should be scaled. If called after
  // StartUpdating() then some thumbnails may be still scaled to the old size
  // until they are updated.
  void SetThumbnailSize(const gfx::Size& thumbnail_size);

  // Starts updating the model. The model is initially empty, so OnSourceAdded()
  // notifications will be generated for each existing source as it is
  // enumerated. After the initial enumeration the model will be refreshed based
  // on the update period, and notifications generated only for changes in the
  // model.
  void StartUpdating(Observer* observer);

  int source_count() const { return sources_.size(); }
  const Source& source(int index) const { return sources_.at(index); }

 private:
  class Worker;
  friend class Worker;

  // Struct used to represent sources list the model gets from the Worker.
  struct SourceDescription {
    SourceDescription(content::DesktopMediaID id, const string16& name);

    content::DesktopMediaID id;
    string16 name;
  };

  // Order comparator for sources. Used to sort list of sources.
  static bool CompareSources(const SourceDescription& a,
                             const SourceDescription& b);

  // Post a task for the |worker_| to update list of windows and get thumbnails.
  void Refresh();

  // Called by |worker_| to refresh the model. First it posts tasks for
  // OnSourcesList() with the fresh list of sources, then follows with
  // OnSourceThumbnail() for each changed thumbnail and then calls
  // OnRefreshFinished() at the end.
  void OnSourcesList(const std::vector<SourceDescription>& sources);
  void OnSourceThumbnail(int index, const gfx::ImageSkia& thumbnail);
  void OnRefreshFinished();

  // Flags passed to the constructor.
  int flags_;

  // Capturers specified in SetCapturers() and passed to the |worker_| later.
  scoped_ptr<webrtc::ScreenCapturer> screen_capturer_;
  scoped_ptr<webrtc::WindowCapturer> window_capturer_;

  // Time interval between mode updates.
  base::TimeDelta update_period_;

  // Size of thumbnails generated by the model.
  gfx::Size thumbnail_size_;

  // The observer passed to StartUpdating().
  Observer* observer_;

  // Task runner used for the |worker_|.
  scoped_refptr<base::SequencedTaskRunner> capture_task_runner_;

  // An object that does all the work of getting list of sources on a background
  // thread (see |capture_task_runner_|). Destroyed on |capture_task_runner_|
  // after the model is destroyed.
  scoped_ptr<Worker> worker_;

  // Current list of sources.
  std::vector<Source> sources_;

  base::WeakPtrFactory<DesktopMediaPickerModel> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerModel);
};

#endif  // CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_PICKER_MODEL_H_
