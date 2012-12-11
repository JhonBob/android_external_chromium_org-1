// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_stream_devices_controller.h"

#include "base/values.h"
#include "chrome/browser/content_settings/content_settings_provider.h"
#include "chrome/browser/content_settings/host_content_settings_map.h"
#include "chrome/browser/extensions/api/tab_capture/tab_capture_registry.h"
#include "chrome/browser/extensions/api/tab_capture/tab_capture_registry_factory.h"
#include "chrome/browser/prefs/scoped_user_pref_update.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/content_settings.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/media_stream_request.h"

using content::BrowserThread;

namespace {

// A predicate that checks if a StreamDeviceInfo object has the same device
// name as the device name specified at construction.
class DeviceNameEquals {
 public:
  explicit DeviceNameEquals(const std::string& device_name)
      : device_name_(device_name) {
  }

  bool operator() (const content::MediaStreamDevice& device) {
    return device.name == device_name_;
  }

 private:
  std::string device_name_;
};

const char kAudioKey[] = "audio";
const char kVideoKey[] = "video";

}  // namespace

MediaStreamDevicesController::MediaStreamDevicesController(
    Profile* profile,
    const content::MediaStreamRequest* request,
    const content::MediaResponseCallback& callback)
    : has_audio_(false),
      has_video_(false),
      profile_(profile),
      request_(*request),
      callback_(callback) {
  for (content::MediaStreamDeviceMap::const_iterator it =
           request_.devices.begin();
       it != request_.devices.end(); ++it) {
    if (content::IsAudioMediaType(it->first)) {
      has_audio_ |= !it->second.empty();
    } else if (content::IsVideoMediaType(it->first)) {
      has_video_ |= !it->second.empty();
    }
    if (IsAudioDeviceBlockedByPolicy())
      has_audio_ = false;
    if (IsVideoDeviceBlockedByPolicy())
      has_video_ = false;
  }
}

MediaStreamDevicesController::~MediaStreamDevicesController() {}

// static
void MediaStreamDevicesController::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterBooleanPref(prefs::kVideoCaptureAllowed,
                             true,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterBooleanPref(prefs::kAudioCaptureAllowed,
                             true,
                             PrefService::UNSYNCABLE_PREF);
}


bool MediaStreamDevicesController::DismissInfoBarAndTakeActionOnSettings() {
  // For tab media requests, we need to make sure the request came from the
  // extension API, so we check the registry here.
  content::MediaStreamDeviceMap::const_iterator tab_video =
      request_.devices.find(content::MEDIA_TAB_VIDEO_CAPTURE);
  content::MediaStreamDeviceMap::const_iterator tab_audio =
      request_.devices.find(content::MEDIA_TAB_AUDIO_CAPTURE);
  if (tab_video != request_.devices.end() ||
      tab_audio != request_.devices.end()) {
    extensions::TabCaptureRegistry* registry =
        extensions::TabCaptureRegistryFactory::GetForProfile(profile_);

    if (!registry->VerifyRequest(request_.render_process_id,
                                 request_.render_view_id)) {
      Deny();
    } else {
      content::MediaStreamDevices devices;

      if (tab_video != request_.devices.end()) {
        devices.push_back(content::MediaStreamDevice(
            content::MEDIA_TAB_VIDEO_CAPTURE, "", ""));
      }
      if (tab_audio != request_.devices.end()) {
        devices.push_back(content::MediaStreamDevice(
            content::MEDIA_TAB_AUDIO_CAPTURE, "", ""));
      }

      callback_.Run(devices);
    }

    return true;
  }

  // Deny the request if the security origin is empty, this happens with
  // file access without |--allow-file-access-from-files| flag.
  if (request_.security_origin.is_empty()) {
    Deny();
    return true;
  }

  // Deny the request and don't show the infobar if there is no devices.
  if (!has_audio_ && !has_video_) {
    // TODO(xians): We should detect this in a early state, and post a callback
    // to tell the users that no device is available. Remove the code and add
    // a DCHECK when this is done.
    Deny();
    return true;
  }

  // Checks if any exception has been made for this request, get the "always
  // allowed" devices if they are available.
  std::string audio, video;
  GetAlwaysAllowedDevices(&audio, &video);
  if ((has_audio_ && audio.empty()) || (has_video_ && video.empty())) {
    // If there is no "always allowed" device for the origin, or the device is
    // not available in the device lists, Check the default setting to see if
    // the user has blocked the access to the media device.
    if (IsMediaDeviceBlocked() || IsRequestBlockedByDefault()) {
      Deny();
      return true;
    }

    // Show the infobar.
    return false;
  }

  // Dismiss the infobar by selecting the "always allowed" devices.
  Accept(audio, video, false);
  return true;
}

content::MediaStreamDevices
MediaStreamDevicesController::GetAudioDevices() const {
  content::MediaStreamDevices all_audio_devices;
  if (has_audio_)
    FindSubsetOfDevices(&content::IsAudioMediaType, &all_audio_devices);
  return all_audio_devices;
}

content::MediaStreamDevices
MediaStreamDevicesController::GetVideoDevices() const {
  content::MediaStreamDevices all_video_devices;
  if (has_video_)
    FindSubsetOfDevices(&content::IsVideoMediaType, &all_video_devices);
  return all_video_devices;
}

const std::string& MediaStreamDevicesController::GetSecurityOriginSpec() const {
  return request_.security_origin.spec();
}

bool MediaStreamDevicesController::IsSafeToAlwaysAllowAudio() const {
  return IsSafeToAlwaysAllow(
      &content::IsAudioMediaType, content::MEDIA_DEVICE_AUDIO_CAPTURE);
}

bool MediaStreamDevicesController::IsSafeToAlwaysAllowVideo() const {
  return IsSafeToAlwaysAllow(
      &content::IsVideoMediaType, content::MEDIA_DEVICE_VIDEO_CAPTURE);
}

void MediaStreamDevicesController::Accept(const std::string& audio_id,
                                          const std::string& video_id,
                                          bool always_allow) {
  content::MediaStreamDevices devices;
  std::string audio_device_name, video_device_name;

  const content::MediaStreamDevice* const audio_device =
      FindFirstDeviceWithIdInSubset(&content::IsAudioMediaType, audio_id);
  if (audio_device) {
    if (audio_device->type != content::MEDIA_DEVICE_AUDIO_CAPTURE)
      always_allow = false;  // Override for virtual audio device type.
    devices.push_back(*audio_device);
    audio_device_name = audio_device->name;
  }

  const content::MediaStreamDevice* const video_device =
      FindFirstDeviceWithIdInSubset(&content::IsVideoMediaType, video_id);
  if (video_device) {
    if (video_device->type != content::MEDIA_DEVICE_VIDEO_CAPTURE)
      always_allow = false;  // Override for virtual video device type.
    devices.push_back(*video_device);
    video_device_name = video_device->name;
  }

  DCHECK(!devices.empty());

  if (always_allow)
    SetPermission(true);

  callback_.Run(devices);
}

void MediaStreamDevicesController::Deny() {
  SetPermission(false);
  callback_.Run(content::MediaStreamDevices());
}

bool MediaStreamDevicesController::IsSafeToAlwaysAllow(
    FilterByDeviceTypeFunc is_included,
    content::MediaStreamDeviceType device_type) const {
  DCHECK(device_type == content::MEDIA_DEVICE_AUDIO_CAPTURE ||
         device_type == content::MEDIA_DEVICE_VIDEO_CAPTURE);

  if (!request_.security_origin.SchemeIsSecure())
    return false;

  // If non-physical devices are available for the choosing, then it's not safe.
  bool safe_devices_found = false;
  for (content::MediaStreamDeviceMap::const_iterator it =
           request_.devices.begin();
       it != request_.devices.end(); ++it) {
    if (it->first != device_type && is_included(it->first))
      return false;
    safe_devices_found = true;
  }

  return safe_devices_found;
}

bool MediaStreamDevicesController::ShouldAlwaysAllowOrigin() {
  return profile_->GetHostContentSettingsMap()->ShouldAllowAllContent(
      request_.security_origin, request_.security_origin,
      CONTENT_SETTINGS_TYPE_MEDIASTREAM);
}

bool MediaStreamDevicesController::IsMediaDeviceBlocked() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ContentSetting current_setting =
      profile_->GetHostContentSettingsMap()->GetDefaultContentSetting(
          CONTENT_SETTINGS_TYPE_MEDIASTREAM, NULL);
  return (current_setting == CONTENT_SETTING_BLOCK);
}

bool MediaStreamDevicesController::IsAudioDeviceBlockedByPolicy() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return (!profile_->GetPrefs()->GetBoolean(prefs::kAudioCaptureAllowed) &&
          profile_->GetPrefs()->IsManagedPreference(
              prefs::kAudioCaptureAllowed));
}

bool MediaStreamDevicesController::IsVideoDeviceBlockedByPolicy() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return (!profile_->GetPrefs()->GetBoolean(prefs::kVideoCaptureAllowed) &&
          profile_->GetPrefs()->IsManagedPreference(
              prefs::kVideoCaptureAllowed));
}

bool MediaStreamDevicesController::IsRequestBlockedByDefault() const {
  if (has_audio_) {
    if (profile_->GetHostContentSettingsMap()->GetContentSetting(
            request_.security_origin,
            request_.security_origin,
            CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
            NO_RESOURCE_IDENTIFIER) != CONTENT_SETTING_BLOCK)
      return false;
  }

  if (has_video_) {
    if (profile_->GetHostContentSettingsMap()->GetContentSetting(
            request_.security_origin,
            request_.security_origin,
            CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
            NO_RESOURCE_IDENTIFIER) != CONTENT_SETTING_BLOCK)
      return false;
  }

  return true;
}

void MediaStreamDevicesController::SetPermission(bool allowed) const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromURLNoWildcard(request_.security_origin);
  // Check the pattern is valid or not. When the request is from a file access,
  // no exception will be made.
  if (!primary_pattern.IsValid())
    return;

  ContentSetting content_setting = allowed ?
      CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  if (has_audio_) {
    profile_->GetHostContentSettingsMap()->SetContentSetting(
        primary_pattern,
        ContentSettingsPattern::Wildcard(),
        CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
        std::string(),
        content_setting);
  }
  if (has_video_) {
    profile_->GetHostContentSettingsMap()->SetContentSetting(
        primary_pattern,
        ContentSettingsPattern::Wildcard(),
        CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
        std::string(),
        content_setting);
  }
}

void MediaStreamDevicesController::GetAlwaysAllowedDevices(
    std::string* audio_id, std::string* video_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(audio_id->empty());
  DCHECK(video_id->empty());
  // If the request is from internal objects like chrome://URLs, use the first
  // devices on the lists.
  if (ShouldAlwaysAllowOrigin()) {
    if (has_audio_)
      *audio_id = GetFirstDeviceId(content::MEDIA_DEVICE_AUDIO_CAPTURE);
    if (has_video_)
      *video_id = GetFirstDeviceId(content::MEDIA_DEVICE_VIDEO_CAPTURE);
    return;
  }

  // "Always allowed" option is only available for secure connection.
  if (!request_.security_origin.SchemeIsSecure())
    return;

  // Checks the media exceptions to get the "always allowed" devices.
  scoped_ptr<Value> value(
      profile_->GetHostContentSettingsMap()->GetWebsiteSetting(
          request_.security_origin,
          request_.security_origin,
          CONTENT_SETTINGS_TYPE_MEDIASTREAM,
          NO_RESOURCE_IDENTIFIER,
          NULL));
  if (!value.get()) {
    NOTREACHED();
    return;
  }

  const DictionaryValue* value_dict = NULL;
  std::string audio_name, video_name;
  if (value->GetAsDictionary(&value_dict) && !value_dict->empty()) {
    value_dict->GetString(kAudioKey, &audio_name);
    value_dict->GetString(kVideoKey, &video_name);
  }

  if (has_audio_) {
    if (!audio_name.empty()) {
      *audio_id = GetDeviceIdByName(content::MEDIA_DEVICE_AUDIO_CAPTURE,
                                    audio_name);
    } else if (profile_->GetHostContentSettingsMap()->GetContentSetting(
                   request_.security_origin,
                   request_.security_origin,
                   CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
                   NO_RESOURCE_IDENTIFIER) == CONTENT_SETTING_ALLOW) {
      PrefService* prefs = profile_->GetPrefs();
      *audio_id = prefs->GetString(prefs::kDefaultAudioCaptureDevice);
    }
  }

  if (has_video_) {
    if (!video_name.empty()) {
      *video_id = GetDeviceIdByName(content::MEDIA_DEVICE_VIDEO_CAPTURE,
                                    video_name);
    } else if (profile_->GetHostContentSettingsMap()->GetContentSetting(
                   request_.security_origin,
                   request_.security_origin,
                   CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
                   NO_RESOURCE_IDENTIFIER) == CONTENT_SETTING_ALLOW) {
      PrefService* prefs = profile_->GetPrefs();
      *video_id = prefs->GetString(prefs::kDefaultVideoCaptureDevice);
    }
  }
}

std::string MediaStreamDevicesController::GetDeviceIdByName(
    content::MediaStreamDeviceType type,
    const std::string& name) {
  content::MediaStreamDeviceMap::const_iterator device_it =
      request_.devices.find(type);
  if (device_it != request_.devices.end()) {
    content::MediaStreamDevices::const_iterator it = std::find_if(
        device_it->second.begin(), device_it->second.end(),
        DeviceNameEquals(name));
    if (it != device_it->second.end())
      return it->device_id;
  }

  // Device is not available, return an empty string.
  return std::string();
}

std::string MediaStreamDevicesController::GetFirstDeviceId(
    content::MediaStreamDeviceType type) {
  content::MediaStreamDeviceMap::const_iterator device_it =
      request_.devices.find(type);
  if (device_it != request_.devices.end())
    return device_it->second.begin()->device_id;

  return std::string();
}

void MediaStreamDevicesController::FindSubsetOfDevices(
    FilterByDeviceTypeFunc is_included,
    content::MediaStreamDevices* out) const {
  for (content::MediaStreamDeviceMap::const_iterator it =
           request_.devices.begin();
       it != request_.devices.end(); ++it) {
    if (is_included(it->first))
      out->insert(out->end(), it->second.begin(), it->second.end());
  }
}

const content::MediaStreamDevice*
MediaStreamDevicesController::FindFirstDeviceWithIdInSubset(
    FilterByDeviceTypeFunc is_included,
    const std::string& device_id) const {
  for (content::MediaStreamDeviceMap::const_iterator it =
           request_.devices.begin();
       it != request_.devices.end(); ++it) {
    if (!is_included(it->first)) continue;
    content::MediaStreamDevices::const_iterator device_it = it->second.begin();
    for (; device_it != it->second.end(); ++device_it) {
      const content::MediaStreamDevice& candidate = *device_it;
      if (candidate.device_id == device_id)
        return &candidate;
    }
    // Return the first device if the preferred device is not available.
    if (device_it == it->second.end())
      return &(*it->second.begin());
  }
  return NULL;
}
