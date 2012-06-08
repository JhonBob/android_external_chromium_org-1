// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/blob/shareable_file_reference.h"

#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace webkit_blob {

TEST(ShareableFileReferenceTest, TestReferences) {
  MessageLoop message_loop;
  scoped_refptr<base::MessageLoopProxy> loop_proxy =
      base::MessageLoopProxy::current();
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Create a file.
  FilePath file;
  file_util::CreateTemporaryFileInDir(temp_dir.path(), &file);
  EXPECT_TRUE(file_util::PathExists(file));

  // Create a first reference to that file.
  scoped_refptr<ShareableFileReference> reference1;
  reference1 = ShareableFileReference::Get(file);
  EXPECT_FALSE(reference1.get());
  reference1 = ShareableFileReference::GetOrCreate(
      file, ShareableFileReference::DELETE_ON_FINAL_RELEASE, loop_proxy);
  EXPECT_TRUE(reference1.get());
  EXPECT_TRUE(file == reference1->path());

  // Get a second reference to that file.
  scoped_refptr<ShareableFileReference> reference2;
  reference2 = ShareableFileReference::Get(file);
  EXPECT_EQ(reference1.get(), reference2.get());
  reference2 = ShareableFileReference::GetOrCreate(
      file, ShareableFileReference::DELETE_ON_FINAL_RELEASE, loop_proxy);
  EXPECT_EQ(reference1.get(), reference2.get());

  // Drop the first reference, the file and reference should still be there.
  reference1 = NULL;
  EXPECT_TRUE(ShareableFileReference::Get(file).get());
  MessageLoop::current()->RunAllPending();
  EXPECT_TRUE(file_util::PathExists(file));

  // Drop the second reference, the file and reference should get deleted.
  reference2 = NULL;
  EXPECT_FALSE(ShareableFileReference::Get(file).get());
  MessageLoop::current()->RunAllPending();
  EXPECT_FALSE(file_util::PathExists(file));

  // TODO(michaeln): add a test for files that aren't deletable behavior.
}

}  // namespace webkit_blob
