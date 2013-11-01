// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_action.h"

#include "base/files/file_path.h"
#include "base/json/json_file_value_serializer.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/declarative_webrequest/request_stage.h"
#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_condition.h"
#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_constants.h"
#include "chrome/browser/extensions/api/web_request/web_request_api_helpers.h"
#include "chrome/browser/extensions/extension_info_map.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/request_priority.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::DictionaryValue;
using base::ListValue;
using extension_test_util::LoadManifestUnchecked;
using testing::HasSubstr;

namespace extensions {

namespace {

const char kUnknownActionType[] = "unknownType";

scoped_ptr<WebRequestActionSet> CreateSetOfActions(const char* json) {
  scoped_ptr<Value> parsed_value(base::test::ParseJson(json));
  const ListValue* parsed_list;
  CHECK(parsed_value->GetAsList(&parsed_list));

  WebRequestActionSet::AnyVector actions;
  for (ListValue::const_iterator it = parsed_list->begin();
       it != parsed_list->end();
       ++it) {
    const DictionaryValue* dict;
    CHECK((*it)->GetAsDictionary(&dict));
    actions.push_back(linked_ptr<base::Value>(dict->DeepCopy()));
  }

  std::string error;
  bool bad_message = false;

  scoped_ptr<WebRequestActionSet> action_set(
      WebRequestActionSet::Create(NULL, actions, &error, &bad_message));
  EXPECT_EQ("", error);
  EXPECT_FALSE(bad_message);
  CHECK(action_set);
  return action_set.Pass();
}

}  // namespace

namespace keys = declarative_webrequest_constants;

class WebRequestActionWithThreadsTest : public testing::Test {
 public:
  WebRequestActionWithThreadsTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

 protected:
  virtual void SetUp() OVERRIDE;

  // Creates a URL request for URL |url_string|, and applies the actions from
  // |action_set| as if they were triggered by the extension with
  // |extension_id| during |stage|.
  bool ActionWorksOnRequest(const char* url_string,
                            const std::string& extension_id,
                            const WebRequestActionSet* action_set,
                            RequestStage stage);

  // Expects a JSON description of an |action| requiring <all_urls> host
  // permission, and checks that only an extensions with full host permissions
  // can execute that action at |stage|. Also checks that the action is not
  // executable for http://clients1.google.com.
  void CheckActionNeedsAllUrls(const char* action, RequestStage stage);

  net::TestURLRequestContext context_;

  // An extension with *.com host permissions and the DWR permission.
  scoped_refptr<Extension> extension_;
  // An extension with host permissions for all URLs and the DWR permission.
  scoped_refptr<Extension> extension_all_urls_;
  scoped_refptr<ExtensionInfoMap> extension_info_map_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
};

void WebRequestActionWithThreadsTest::SetUp() {
  testing::Test::SetUp();

  std::string error;
  extension_ = LoadManifestUnchecked("permissions",
                                     "web_request_com_host_permissions.json",
                                     Manifest::INVALID_LOCATION,
                                     Extension::NO_FLAGS,
                                     "ext_id_1",
                                     &error);
  ASSERT_TRUE(extension_.get()) << error;
  extension_all_urls_ =
      LoadManifestUnchecked("permissions",
                            "web_request_all_host_permissions.json",
                            Manifest::INVALID_LOCATION,
                            Extension::NO_FLAGS,
                            "ext_id_2",
                            &error);
  ASSERT_TRUE(extension_all_urls_.get()) << error;
  extension_info_map_ = new ExtensionInfoMap;
  ASSERT_TRUE(extension_info_map_.get());
  extension_info_map_->AddExtension(
      extension_.get(), base::Time::Now(), false /*incognito_enabled*/);
  extension_info_map_->AddExtension(extension_all_urls_.get(),
                                    base::Time::Now(),
                                    false /*incognito_enabled*/);
}

bool WebRequestActionWithThreadsTest::ActionWorksOnRequest(
    const char* url_string,
    const std::string& extension_id,
    const WebRequestActionSet* action_set,
    RequestStage stage) {
  net::TestURLRequest regular_request(
      GURL(url_string), net::DEFAULT_PRIORITY, NULL, &context_);
  std::list<LinkedPtrEventResponseDelta> deltas;
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(""));
  WebRequestData request_data(&regular_request, stage, headers.get());
  std::set<std::string> ignored_tags;
  WebRequestAction::ApplyInfo apply_info = { extension_info_map_.get(),
                                             request_data,
                                             false /*crosses_incognito*/,
                                             &deltas, &ignored_tags };
  action_set->Apply(extension_id, base::Time(), &apply_info);
  return (1u == deltas.size() || 0u < ignored_tags.size());
}

void WebRequestActionWithThreadsTest::CheckActionNeedsAllUrls(
    const char* action,
    RequestStage stage) {
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(action));

  // Although |extension_| has matching *.com host permission, |action|
  // is intentionally forbidden -- in Declarative WR, host permission
  // for less than all URLs are ignored (except in SendMessageToExtension).
  EXPECT_FALSE(ActionWorksOnRequest(
      "http://test.com", extension_->id(), action_set.get(), stage));
  // With the "<all_urls>" host permission they are allowed.
  EXPECT_TRUE(ActionWorksOnRequest(
      "http://test.com", extension_all_urls_->id(), action_set.get(), stage));

  // The protected URLs should not be touched at all.
  EXPECT_FALSE(ActionWorksOnRequest(
      "http://clients1.google.com", extension_->id(), action_set.get(), stage));
  EXPECT_FALSE(ActionWorksOnRequest("http://clients1.google.com",
                                    extension_all_urls_->id(),
                                    action_set.get(),
                                    stage));
}

TEST(WebRequestActionTest, CreateAction) {
  std::string error;
  bool bad_message = false;
  scoped_refptr<const WebRequestAction> result;

  // Test wrong data type passed.
  error.clear();
  base::ListValue empty_list;
  result = WebRequestAction::Create(NULL, empty_list, &error, &bad_message);
  EXPECT_TRUE(bad_message);
  EXPECT_FALSE(result.get());

  // Test missing instanceType element.
  base::DictionaryValue input;
  error.clear();
  result = WebRequestAction::Create(NULL, input, &error, &bad_message);
  EXPECT_TRUE(bad_message);
  EXPECT_FALSE(result.get());

  // Test wrong instanceType element.
  input.SetString(keys::kInstanceTypeKey, kUnknownActionType);
  error.clear();
  result = WebRequestAction::Create(NULL, input, &error, &bad_message);
  EXPECT_NE("", error);
  EXPECT_FALSE(result.get());

  // Test success
  input.SetString(keys::kInstanceTypeKey, keys::kCancelRequestType);
  error.clear();
  result = WebRequestAction::Create(NULL, input, &error, &bad_message);
  EXPECT_EQ("", error);
  EXPECT_FALSE(bad_message);
  ASSERT_TRUE(result.get());
  EXPECT_EQ(WebRequestAction::ACTION_CANCEL_REQUEST, result->type());
}

TEST(WebRequestActionTest, CreateActionSet) {
  std::string error;
  bool bad_message = false;
  scoped_ptr<WebRequestActionSet> result;

  WebRequestActionSet::AnyVector input;

  // Test empty input.
  error.clear();
  result = WebRequestActionSet::Create(NULL, input, &error, &bad_message);
  EXPECT_TRUE(error.empty()) << error;
  EXPECT_FALSE(bad_message);
  ASSERT_TRUE(result.get());
  EXPECT_TRUE(result->actions().empty());
  EXPECT_EQ(std::numeric_limits<int>::min(), result->GetMinimumPriority());

  DictionaryValue correct_action;
  correct_action.SetString(keys::kInstanceTypeKey, keys::kIgnoreRulesType);
  correct_action.SetInteger(keys::kLowerPriorityThanKey, 10);
  DictionaryValue incorrect_action;
  incorrect_action.SetString(keys::kInstanceTypeKey, kUnknownActionType);

  // Test success.
  input.push_back(linked_ptr<base::Value>(correct_action.DeepCopy()));
  error.clear();
  result = WebRequestActionSet::Create(NULL, input, &error, &bad_message);
  EXPECT_TRUE(error.empty()) << error;
  EXPECT_FALSE(bad_message);
  ASSERT_TRUE(result.get());
  ASSERT_EQ(1u, result->actions().size());
  EXPECT_EQ(WebRequestAction::ACTION_IGNORE_RULES,
            result->actions()[0]->type());
  EXPECT_EQ(10, result->GetMinimumPriority());

  // Test failure.
  input.push_back(linked_ptr<base::Value>(incorrect_action.DeepCopy()));
  error.clear();
  result = WebRequestActionSet::Create(NULL, input, &error, &bad_message);
  EXPECT_NE("", error);
  EXPECT_FALSE(result.get());
}

// Test capture group syntax conversions of WebRequestRedirectByRegExAction
TEST(WebRequestActionTest, PerlToRe2Style) {
#define CallPerlToRe2Style WebRequestRedirectByRegExAction::PerlToRe2Style
  // foo$1bar -> foo\1bar
  EXPECT_EQ("foo\\1bar", CallPerlToRe2Style("foo$1bar"));
  // foo\$1bar -> foo$1bar
  EXPECT_EQ("foo$1bar", CallPerlToRe2Style("foo\\$1bar"));
  // foo\\$1bar -> foo\\\1bar
  EXPECT_EQ("foo\\\\\\1bar", CallPerlToRe2Style("foo\\\\$1bar"));
  // foo\bar -> foobar
  EXPECT_EQ("foobar", CallPerlToRe2Style("foo\\bar"));
  // foo$bar -> foo$bar
  EXPECT_EQ("foo$bar", CallPerlToRe2Style("foo$bar"));
#undef CallPerlToRe2Style
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRedirect) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RedirectRequest\","
      " \"redirectUrl\": \"http://www.foobar.com\""
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_REQUEST);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRedirectByRegEx) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RedirectByRegEx\","
      " \"from\": \".*\","
      " \"to\": \"http://www.foobar.com\""
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_REQUEST);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToSetRequestHeader) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.SetRequestHeader\","
      " \"name\": \"testname\","
      " \"value\": \"testvalue\""
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_SEND_HEADERS);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRemoveRequestHeader) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RemoveRequestHeader\","
      " \"name\": \"testname\""
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_SEND_HEADERS);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToAddResponseHeader) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.AddResponseHeader\","
      " \"name\": \"testname\","
      " \"value\": \"testvalue\""
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_HEADERS_RECEIVED);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRemoveResponseHeader) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RemoveResponseHeader\","
      " \"name\": \"testname\""
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_HEADERS_RECEIVED);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToSendMessageToExtension) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.SendMessageToExtension\","
      " \"message\": \"testtext\""
      "}]";
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(kAction));

  // For sending messages, specific host permissions actually matter.
  EXPECT_TRUE(ActionWorksOnRequest("http://test.com",
                                   extension_->id(),
                                   action_set.get(),
                                   ON_BEFORE_REQUEST));
  // With the "<all_urls>" host permission they are allowed.
  EXPECT_TRUE(ActionWorksOnRequest("http://test.com",
                                   extension_all_urls_->id(),
                                   action_set.get(),
                                   ON_BEFORE_REQUEST));

  // The protected URLs should not be touched at all.
  EXPECT_FALSE(ActionWorksOnRequest("http://clients1.google.com",
                                    extension_->id(),
                                    action_set.get(),
                                    ON_BEFORE_REQUEST));
  EXPECT_FALSE(ActionWorksOnRequest("http://clients1.google.com",
                                    extension_all_urls_->id(),
                                    action_set.get(),
                                    ON_BEFORE_REQUEST));
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToAddRequestCookie) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.AddRequestCookie\","
      " \"cookie\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_SEND_HEADERS);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToAddResponseCookie) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.AddResponseCookie\","
      " \"cookie\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_HEADERS_RECEIVED);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToEditRequestCookie) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.EditRequestCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" },"
      " \"modification\": { \"name\": \"name2\", \"value\": \"value2\" }"
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_SEND_HEADERS);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToEditResponseCookie) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.EditResponseCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" },"
      " \"modification\": { \"name\": \"name2\", \"value\": \"value2\" }"
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_HEADERS_RECEIVED);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRemoveRequestCookie) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RemoveRequestCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_BEFORE_SEND_HEADERS);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRemoveResponseCookie) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RemoveResponseCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "}]";
  CheckActionNeedsAllUrls(kAction, ON_HEADERS_RECEIVED);
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToCancel) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.CancelRequest\""
      "}]";
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(kAction));

  // Cancelling requests works without full host permissions.
  EXPECT_TRUE(ActionWorksOnRequest("http://test.org",
                                   extension_->id(),
                                   action_set.get(),
                                   ON_BEFORE_REQUEST));
}

TEST_F(WebRequestActionWithThreadsTest,
       PermissionsToRedirectToTransparentImage) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RedirectToTransparentImage\""
      "}]";
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(kAction));

  // Redirecting to transparent images works without full host permissions.
  EXPECT_TRUE(ActionWorksOnRequest("http://test.org",
                                   extension_->id(),
                                   action_set.get(),
                                   ON_BEFORE_REQUEST));
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToRedirectToEmptyDocument) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RedirectToEmptyDocument\""
      "}]";
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(kAction));

  // Redirecting to the empty document works without full host permissions.
  EXPECT_TRUE(ActionWorksOnRequest("http://test.org",
                                   extension_->id(),
                                   action_set.get(),
                                   ON_BEFORE_REQUEST));
}

TEST_F(WebRequestActionWithThreadsTest, PermissionsToIgnore) {
  const char kAction[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.IgnoreRules\","
      " \"lowerPriorityThan\": 123,"
      " \"hasTag\": \"some_tag\""
      "}]";
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(kAction));

  // Ignoring rules works without full host permissions.
  EXPECT_TRUE(ActionWorksOnRequest("http://test.org",
                                   extension_->id(),
                                   action_set.get(),
                                   ON_BEFORE_REQUEST));
}

TEST(WebRequestActionTest, GetName) {
  const char kActions[] =
      "[{"
      " \"instanceType\": \"declarativeWebRequest.RedirectRequest\","
      " \"redirectUrl\": \"http://www.foobar.com\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RedirectByRegEx\","
      " \"from\": \".*\","
      " \"to\": \"http://www.foobar.com\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.SetRequestHeader\","
      " \"name\": \"testname\","
      " \"value\": \"testvalue\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RemoveRequestHeader\","
      " \"name\": \"testname\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.AddResponseHeader\","
      " \"name\": \"testname\","
      " \"value\": \"testvalue\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RemoveResponseHeader\","
      " \"name\": \"testname\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.SendMessageToExtension\","
      " \"message\": \"testtext\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.AddRequestCookie\","
      " \"cookie\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.AddResponseCookie\","
      " \"cookie\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.EditRequestCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" },"
      " \"modification\": { \"name\": \"name2\", \"value\": \"value2\" }"
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.EditResponseCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" },"
      " \"modification\": { \"name\": \"name2\", \"value\": \"value2\" }"
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RemoveRequestCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RemoveResponseCookie\","
      " \"filter\": { \"name\": \"cookiename\", \"value\": \"cookievalue\" }"
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.CancelRequest\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RedirectToTransparentImage\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.RedirectToEmptyDocument\""
      "},"
      "{"
      " \"instanceType\": \"declarativeWebRequest.IgnoreRules\","
      " \"lowerPriorityThan\": 123,"
      " \"hasTag\": \"some_tag\""
      "}]";
  const char* kExpectedNames[] = {
    "declarativeWebRequest.RedirectRequest",
    "declarativeWebRequest.RedirectByRegEx",
    "declarativeWebRequest.SetRequestHeader",
    "declarativeWebRequest.RemoveRequestHeader",
    "declarativeWebRequest.AddResponseHeader",
    "declarativeWebRequest.RemoveResponseHeader",
    "declarativeWebRequest.SendMessageToExtension",
    "declarativeWebRequest.AddRequestCookie",
    "declarativeWebRequest.AddResponseCookie",
    "declarativeWebRequest.EditRequestCookie",
    "declarativeWebRequest.EditResponseCookie",
    "declarativeWebRequest.RemoveRequestCookie",
    "declarativeWebRequest.RemoveResponseCookie",
    "declarativeWebRequest.CancelRequest",
    "declarativeWebRequest.RedirectToTransparentImage",
    "declarativeWebRequest.RedirectToEmptyDocument",
    "declarativeWebRequest.IgnoreRules",
  };
  scoped_ptr<WebRequestActionSet> action_set(CreateSetOfActions(kActions));
  ASSERT_EQ(arraysize(kExpectedNames), action_set->actions().size());
  size_t index = 0;
  for (WebRequestActionSet::Actions::const_iterator it =
           action_set->actions().begin();
       it != action_set->actions().end();
       ++it) {
    EXPECT_EQ(kExpectedNames[index], (*it)->GetName());
    ++index;
  }
}

}  // namespace extensions
