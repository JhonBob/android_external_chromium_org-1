// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/pepper_flash_component_installer.h"

#include "base/base_paths.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/values.h"
#include "chrome/browser/component_updater/component_updater_service.h"
#include "chrome/common/chrome_paths.h"
#include "content/browser/browser_thread.h"

namespace {

// CRX hash. The extension id is: mimojjlkmoijpicakmndhoigimigcmbb.
const uint8 sha2_hash[] = {0xc8, 0xce, 0x99, 0xba, 0xce, 0x89, 0xf8, 0x20,
                           0xac, 0xd3, 0x7e, 0x86, 0x8c, 0x86, 0x2c, 0x11,
                           0xb9, 0x40, 0xc5, 0x55, 0xaf, 0x08, 0x63, 0x70,
                           0x54, 0xf9, 0x56, 0xd3, 0xe7, 0x88, 0xba, 0x8c};

// File name of the Pepper Flash plugin on different platforms.
const FilePath::CharType kPepperFlashPluginFileName[] =
#if defined(OS_MACOSX)
    FILE_PATH_LITERAL("PepperFlashPlayer.plugin");
#elif defined(OS_WIN)
    FILE_PATH_LITERAL("pepflashplayer.dll");
#else  // OS_LINUX, etc.
    FILE_PATH_LITERAL("libpepflashplayer.so");
#endif

// File name of the Pepper Flash component manifest on different platforms.
const char kPepperFlashManifestName[] =
#if defined(OS_MACOSX)
    "MacFlapper";
#elif defined(OS_WIN)
    "WinFlapper";
#else  // OS_LINUX, etc.
    "NixFlapper";
#endif

// The pepper flash plugins are in a directory with this name.
const FilePath::CharType kPepperFlashBaseDirectory[] =
    FILE_PATH_LITERAL("PepperFlash");

// If we don't have a flash pepper component, this is the version we claim.
const char kNullVersion[] = "0.0.0.0";

// The base directory on windows looks like:
// <profile>\AppData\Local\Google\Chrome\User Data\PepperFlash\.
FilePath GetPepperFlashBaseDirectory() {
  FilePath result;
  PathService::Get(chrome::DIR_USER_DATA, &result);
  return result.Append(kPepperFlashBaseDirectory);
}

// Pepper flash plugins have the version encoded in the path itself
// so we need to enumerate the directories to find the full path.
// On success it returns something like:
// <profile>\AppData\Local\Google\Chrome\User Data\PepperFlash\10.3.44.555\.
bool GetLatestPepperFlashDirectory(FilePath* result, Version* latest) {
  *result = GetPepperFlashBaseDirectory();
  bool found = false;
  file_util::FileEnumerator
      file_enumerator(*result, false, file_util::FileEnumerator::DIRECTORIES);
  for (FilePath path = file_enumerator.Next(); !path.value().empty();
       path = file_enumerator.Next()) {
    Version version(path.BaseName().MaybeAsASCII());
    if (!version.IsValid())
      continue;
    if (version.CompareTo(*latest) > 0) {
      *latest = version;
      *result = path;
      found = true;
    }
  }
  return found;
}

}  // namespace

class PepperFlashComponentInstaller : public ComponentInstaller {
 public:
  explicit PepperFlashComponentInstaller(const Version& version);

  virtual ~PepperFlashComponentInstaller() {}

  virtual void OnUpdateError(int error) OVERRIDE;

  virtual bool Install(base::DictionaryValue* manifest,
                       const FilePath& unpack_path) OVERRIDE;

 private:
  Version current_version_;
};

PepperFlashComponentInstaller::PepperFlashComponentInstaller(
    const Version& version) : current_version_(version) {
  DCHECK(version.IsValid());
}

void PepperFlashComponentInstaller::OnUpdateError(int error) {
  NOTREACHED() << "pepper flash update error :" << error;
}

bool PepperFlashComponentInstaller::Install(base::DictionaryValue* manifest,
                                            const FilePath& unpack_path) {
  std::string name;
  manifest->GetStringASCII("name", &name);
  if (name != kPepperFlashManifestName)
    return false;
  std::string proposed_version;
  manifest->GetStringASCII("version", &proposed_version);
  Version version(proposed_version.c_str());
  if (!version.IsValid())
    return false;
  if (current_version_.CompareTo(version) > 0)
    return false;
  if (!file_util::PathExists(unpack_path.Append(kPepperFlashPluginFileName)))
    return false;
  // Passed the basic tests. Time to install it.
  FilePath path =
      GetPepperFlashBaseDirectory().AppendASCII(version.GetString());
  if (file_util::PathExists(path))
    return false;
  if (!file_util::Move(unpack_path, path))
    return false;
  // Installation is done. Now update the path service.
  current_version_ = version;
  path = path.Append(kPepperFlashPluginFileName);
  PathService::Override(chrome::FILE_PEPPER_FLASH_PLUGIN, path);
  return true;
}

void FinishPepperFlashRegistration(ComponentUpdateService* cus,
                                   const Version& version) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CrxComponent pepflash;
  pepflash.name = "pepper_flash";
  pepflash.installer = new PepperFlashComponentInstaller(version);
  pepflash.version = version;
  pepflash.pk_hash.assign(sha2_hash, &sha2_hash[sizeof(sha2_hash)]);
  if (cus->RegisterComponent(pepflash) != ComponentUpdateService::kOk) {
    NOTREACHED() << "pepper flash component registration fail";
  }
}

void StartPepperFlashRegistration(ComponentUpdateService* cus) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  FilePath path = GetPepperFlashBaseDirectory();
  if (!file_util::PathExists(path)) {
    if (!file_util::CreateDirectory(path)) {
      NOTREACHED() << "cannot create pepper flash directory";
      return;
    }
  }

  Version version(kNullVersion);
  if (GetLatestPepperFlashDirectory(&path, &version)) {
    path = path.Append(kPepperFlashPluginFileName);
    if (file_util::PathExists(path))
      PathService::Override(chrome::FILE_PEPPER_FLASH_PLUGIN, path);
    else
      version = Version(kNullVersion);
  }

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      NewRunnableFunction(&FinishPepperFlashRegistration, cus, version));
}

void RegisterPepperFlashComponent(ComponentUpdateService* cus) {
#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
  // TODO(cpu): support Mac and Linux flash pepper.
  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
      NewRunnableFunction(&StartPepperFlashRegistration, cus));
#endif
}
