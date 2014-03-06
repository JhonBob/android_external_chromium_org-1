// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "content/public/test/async_file_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/browser/fileapi/file_system_backend.h"
#include "webkit/browser/fileapi/file_system_context.h"
#include "webkit/browser/fileapi/file_system_operation_runner.h"
#include "webkit/browser/fileapi/file_system_url.h"
#include "webkit/browser/quota/quota_manager.h"
#include "webkit/common/fileapi/file_system_util.h"

namespace content {

typedef fileapi::FileSystemOperation::FileEntryList FileEntryList;

void AssignAndQuit(base::RunLoop* run_loop,
                   base::File::Error* result_out,
                   base::File::Error result) {
  *result_out = result;
  run_loop->Quit();
}

base::Callback<void(base::File::Error)>
AssignAndQuitCallback(base::RunLoop* run_loop,
                      base::File::Error* result) {
  return base::Bind(&AssignAndQuit, run_loop, base::Unretained(result));
}

void GetMetadataCallback(base::RunLoop* run_loop,
                         base::File::Error* result_out,
                         base::File::Info* file_info_out,
                         base::File::Error result,
                         const base::File::Info& file_info) {
  *result_out = result;
  if (file_info_out)
    *file_info_out = file_info;
  run_loop->Quit();
}

void CreateSnapshotFileCallback(
    base::RunLoop* run_loop,
    base::File::Error* result_out,
    base::FilePath* platform_path_out,
    base::File::Error result,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    const scoped_refptr<webkit_blob::ShareableFileReference>& file_ref) {
  DCHECK(!file_ref.get());
  *result_out = result;
  if (platform_path_out)
    *platform_path_out = platform_path;
  run_loop->Quit();
}

void ReadDirectoryCallback(base::RunLoop* run_loop,
                           base::File::Error* result_out,
                           FileEntryList* entries_out,
                           base::File::Error result,
                           const FileEntryList& entries,
                           bool has_more) {
  *result_out = result;
  *entries_out = entries;
  if (result != base::File::FILE_OK || !has_more)
    run_loop->Quit();
}

void DidGetUsageAndQuota(quota::QuotaStatusCode* status_out,
                         int64* usage_out,
                         int64* quota_out,
                         quota::QuotaStatusCode status,
                         int64 usage,
                         int64 quota) {
  if (status_out)
    *status_out = status;
  if (usage_out)
    *usage_out = usage;
  if (quota_out)
    *quota_out = quota;
}

const int64 AsyncFileTestHelper::kDontCheckSize = -1;

base::File::Error AsyncFileTestHelper::Copy(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& src,
    const fileapi::FileSystemURL& dest) {
  return CopyWithProgress(context, src, dest, CopyProgressCallback());
}

base::File::Error AsyncFileTestHelper::CopyWithProgress(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& src,
    const fileapi::FileSystemURL& dest,
    const CopyProgressCallback& progress_callback) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->Copy(
      src, dest, fileapi::FileSystemOperation::OPTION_NONE, progress_callback,
      AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::Move(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& src,
    const fileapi::FileSystemURL& dest) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->Move(
      src, dest, fileapi::FileSystemOperation::OPTION_NONE,
      AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::Remove(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    bool recursive) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->Remove(
      url, recursive, AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::ReadDirectory(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    FileEntryList* entries) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  DCHECK(entries);
  entries->clear();
  base::RunLoop run_loop;
  context->operation_runner()->ReadDirectory(
      url, base::Bind(&ReadDirectoryCallback, &run_loop, &result, entries));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::CreateDirectory(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->CreateDirectory(
      url,
      false /* exclusive */,
      false /* recursive */,
      AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::CreateFile(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->CreateFile(
      url, false /* exclusive */,
      AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::CreateFileWithData(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    const char* buf,
    int buf_size) {
  base::ScopedTempDir dir;
  if (!dir.CreateUniqueTempDir())
    return base::File::FILE_ERROR_FAILED;
  base::FilePath local_path = dir.path().AppendASCII("tmp");
  if (buf_size != base::WriteFile(local_path, buf, buf_size))
    return base::File::FILE_ERROR_FAILED;
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->CopyInForeignFile(
      local_path, url, AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::TruncateFile(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    size_t size) {
  base::RunLoop run_loop;
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  context->operation_runner()->Truncate(
      url, size, AssignAndQuitCallback(&run_loop, &result));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::GetMetadata(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    base::File::Info* file_info) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->GetMetadata(
      url, base::Bind(&GetMetadataCallback, &run_loop, &result,
                      file_info));
  run_loop.Run();
  return result;
}

base::File::Error AsyncFileTestHelper::GetPlatformPath(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    base::FilePath* platform_path) {
  base::File::Error result = base::File::FILE_ERROR_FAILED;
  base::RunLoop run_loop;
  context->operation_runner()->CreateSnapshotFile(
      url, base::Bind(&CreateSnapshotFileCallback, &run_loop, &result,
                      platform_path));
  run_loop.Run();
  return result;
}

bool AsyncFileTestHelper::FileExists(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url,
    int64 expected_size) {
  base::File::Info file_info;
  base::File::Error result = GetMetadata(context, url, &file_info);
  if (result != base::File::FILE_OK || file_info.is_directory)
    return false;
  return expected_size == kDontCheckSize || file_info.size == expected_size;
}

bool AsyncFileTestHelper::DirectoryExists(
    fileapi::FileSystemContext* context,
    const fileapi::FileSystemURL& url) {
  base::File::Info file_info;
  base::File::Error result = GetMetadata(context, url, &file_info);
  return (result == base::File::FILE_OK) && file_info.is_directory;
}

quota::QuotaStatusCode AsyncFileTestHelper::GetUsageAndQuota(
    quota::QuotaManager* quota_manager,
    const GURL& origin,
    fileapi::FileSystemType type,
    int64* usage,
    int64* quota) {
  quota::QuotaStatusCode status = quota::kQuotaStatusUnknown;
  quota_manager->GetUsageAndQuota(
      origin,
      FileSystemTypeToQuotaStorageType(type),
      base::Bind(&DidGetUsageAndQuota, &status, usage, quota));
  base::RunLoop().RunUntilIdle();
  return status;
}

}  // namespace fileapi
