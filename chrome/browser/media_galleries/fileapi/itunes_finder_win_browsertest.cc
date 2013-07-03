// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base64.h"
#include "base/base_paths_win.h"
#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_path_override.h"
#include "chrome/browser/media_galleries/fileapi/itunes_finder_win.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"

namespace itunes {

namespace {

std::string EncodePath(const base::FilePath& path) {
  std::string input(reinterpret_cast<const char*>(path.value().data()),
                                                  path.value().size()*2);
  std::string result;
  base::Base64Encode(input, &result);
  return result;
}

void TouchFile(const base::FilePath& file) {
  ASSERT_EQ(1, file_util::WriteFile(file, " ", 1));
}

class ITunesFinderWinTest : public InProcessBrowserTest {
 public:
  ITunesFinderWinTest() : test_finder_callback_called_(false) {}

  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(app_data_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(music_dir_.CreateUniqueTempDir());
    app_data_dir_override_.reset(
        new base::ScopedPathOverride(base::DIR_APP_DATA, app_data_dir()));
    music_dir_override_.reset(
        new base::ScopedPathOverride(chrome::DIR_USER_MUSIC, music_dir()));
    InProcessBrowserTest::SetUp();
  }

  const base::FilePath& app_data_dir() {
    return app_data_dir_.path();
  }

  const base::FilePath& music_dir() {
    return music_dir_.path();
  }

  void WritePrefFile(const std::string& data) {
    base::FilePath pref_dir =
        app_data_dir().AppendASCII("Apple Computer").AppendASCII("iTunes");
    ASSERT_TRUE(file_util::CreateDirectory(pref_dir));
    ASSERT_EQ(data.size(),
              file_util::WriteFile(pref_dir.AppendASCII("iTunesPrefs.xml"),
                                   data.data(), data.size()));
  }

  void TouchDefault() {
    base::FilePath default_dir = music_dir().AppendASCII("iTunes");
    ASSERT_TRUE(file_util::CreateDirectory(default_dir));
    TouchFile(default_dir.AppendASCII("iTunes Music Library.xml"));
  }

  void TestFindITunesLibrary() {
    test_finder_callback_called_ = false;
    result_.clear();
    base::RunLoop loop;
    ITunesFinder::FindITunesLibrary(
        base::Bind(&ITunesFinderWinTest::TestFinderCallback,
                   base::Unretained(this),
                   loop.QuitClosure()));
    loop.Run();
  }

  bool EmptyResult() const {
    return result_.empty();
  }

  bool test_finder_callback_called() const {
    return test_finder_callback_called_;
  }

 private:
  void TestFinderCallback(const base::Closure& quit_closure,
                          const std::string& result) {
    test_finder_callback_called_ = true;
    result_ = result;
    quit_closure.Run();
  }

  scoped_ptr<base::ScopedPathOverride> app_data_dir_override_;
  scoped_ptr<base::ScopedPathOverride> music_dir_override_;
  base::ScopedTempDir app_data_dir_;
  base::ScopedTempDir music_dir_;

  bool test_finder_callback_called_;
  std::string result_;

  DISALLOW_COPY_AND_ASSIGN(ITunesFinderWinTest);
};

IN_PROC_BROWSER_TEST_F(ITunesFinderWinTest, NotFound) {
  TestFindITunesLibrary();
  EXPECT_TRUE(test_finder_callback_called());
  EXPECT_TRUE(EmptyResult());
}

IN_PROC_BROWSER_TEST_F(ITunesFinderWinTest, DefaultLocation) {
  TouchDefault();
  TestFindITunesLibrary();
  EXPECT_TRUE(test_finder_callback_called());
  EXPECT_FALSE(EmptyResult());
}

IN_PROC_BROWSER_TEST_F(ITunesFinderWinTest, CustomLocation) {
  base::FilePath library_xml = music_dir().AppendASCII("library.xml");
  TouchFile(library_xml);
  std::string xml = base::StringPrintf(
      "<plist>"
      "  <dict>"
      "    <key>User Preferences</key>"
      "    <dict>"
      "      <key>iTunes Library XML Location:1</key>"
      "      <data>%s</data>"
      "    </dict>"
      "  </dict>"
      "</plist>", EncodePath(library_xml).c_str());
  WritePrefFile(xml);
  TestFindITunesLibrary();
  EXPECT_TRUE(test_finder_callback_called());
  EXPECT_FALSE(EmptyResult());
}

IN_PROC_BROWSER_TEST_F(ITunesFinderWinTest, BadCustomLocation) {
  // Missing file.
  base::FilePath library_xml = music_dir().AppendASCII("library.xml");
  std::string xml = base::StringPrintf(
      "<plist>"
      "  <dict>"
      "    <key>User Preferences</key>"
      "    <dict>"
      "      <key>iTunes Library XML Location:1</key>"
      "      <data>%s</data>"
      "    </dict>"
      "  </dict>"
      "</plist>", EncodePath(library_xml).c_str());
  WritePrefFile(xml);
  TestFindITunesLibrary();
  EXPECT_TRUE(test_finder_callback_called());
  EXPECT_TRUE(EmptyResult());
  TouchFile(library_xml);

  // User Preferences dictionary at the wrong level.
  xml = base::StringPrintf(
      "<plist>"
      "    <key>User Preferences</key>"
      "    <dict>"
      "      <key>iTunes Library XML Location:1</key>"
      "      <data>%s</data>"
      "    </dict>"
      "</plist>", EncodePath(library_xml).c_str());
  WritePrefFile(xml);
  TestFindITunesLibrary();
  EXPECT_TRUE(test_finder_callback_called());
  EXPECT_TRUE(EmptyResult());

  // Library location at the wrong scope.
  xml = base::StringPrintf(
      "<plist>"
      "  <dict>"
      "    <key>User Preferences</key>"
      "    <dict/>"
      "    <key>iTunes Library XML Location:1</key>"
      "    <data>%s</data>"
      "  </dict>"
      "</plist>", EncodePath(library_xml).c_str());
  WritePrefFile(xml);
  TestFindITunesLibrary();
  EXPECT_TRUE(test_finder_callback_called());
  EXPECT_TRUE(EmptyResult());
}

}  // namespace

}  // namespace itunes
