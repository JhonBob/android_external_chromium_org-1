// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/window_commands.h"

#include <list>

#include "base/callback.h"
#include "base/string_number_conversions.h"
#include "base/stringprintf.h"
#include "base/values.h"
#include "chrome/test/chromedriver/basic_types.h"
#include "chrome/test/chromedriver/chrome.h"
#include "chrome/test/chromedriver/element_util.h"
#include "chrome/test/chromedriver/js.h"
#include "chrome/test/chromedriver/session.h"
#include "chrome/test/chromedriver/status.h"
#include "chrome/test/chromedriver/ui_events.h"
#include "chrome/test/chromedriver/web_view.h"

namespace {

Status GetMouseButton(const base::DictionaryValue& params,
                      MouseButton* button) {
  int button_num;
  if (!params.GetInteger("button", &button_num)) {
    button_num = 0;  // Default to left mouse button.
  } else if (button_num < 0 || button_num > 2) {
    return Status(kUnknownError,
                  base::StringPrintf("invalid button: %d", button_num));
  }
  *button = static_cast<MouseButton>(button_num);
  return Status(kOk);
}

Status GetUrl(WebView* web_view, const std::string& frame, std::string* url) {
  scoped_ptr<base::Value> value;
  base::ListValue args;
  Status status = web_view->CallFunction(
      frame, "function() { return document.URL; }", args, &value);
  if (status.IsError())
    return status;
  if (!value->GetAsString(url))
    return Status(kUnknownError, "javascript failed to return the url");
  return Status(kOk);
}

struct Cookie {
  Cookie(const std::string& name,
         const std::string& value,
         const std::string& domain,
         const std::string& path,
         int expiry,
         bool secure,
         bool session)
      : name(name), value(value), domain(domain), path(path), expiry(expiry),
        secure(secure), session(session) {}

  std::string name;
  std::string value;
  std::string domain;
  std::string path;
  int expiry;
  bool secure;
  bool session;
};

base::DictionaryValue* CreateDictionaryFrom(const Cookie& cookie) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetString("name", cookie.name);
  dict->SetString("value", cookie.value);
  if (!cookie.domain.empty())
    dict->SetString("domain", cookie.domain);
  if (!cookie.path.empty())
    dict->SetString("path", cookie.path);
  if (!cookie.session)
    dict->SetInteger("expiry", cookie.expiry);
  dict->SetBoolean("secure", cookie.secure);
  return dict;
}

Status GetVisibleCookies(WebView* web_view,
                         std::list<Cookie>* cookies) {
  scoped_ptr<base::ListValue> internal_cookies;
  Status status = web_view->GetCookies(&internal_cookies);
  if (status.IsError())
    return status;
  std::list<Cookie> cookies_tmp;
  for (size_t i = 0; i < internal_cookies->GetSize(); ++i) {
    base::DictionaryValue* cookie_dict;
    if (!internal_cookies->GetDictionary(i, &cookie_dict))
      return Status(kUnknownError, "DevTools returns a non-dictionary cookie");

    std::string name;
    cookie_dict->GetString("name", &name);
    std::string value;
    cookie_dict->GetString("value", &value);
    std::string domain;
    cookie_dict->GetString("domain", &domain);
    std::string path;
    cookie_dict->GetString("path", &path);
    double expiry_tmp = 0;
    cookie_dict->GetDouble("expires", &expiry_tmp);
    int expiry = static_cast<int>(expiry_tmp/1000);
    bool session = false;
    cookie_dict->GetBoolean("session", &session);
    bool secure = false;
    cookie_dict->GetBoolean("secure", &secure);

    cookies_tmp.push_back(
        Cookie(name, value, domain, path, expiry, secure, session));
  }
  cookies->swap(cookies_tmp);
  return Status(kOk);
}

}  // namespace

Status ExecuteWindowCommand(
    WindowCommand command,
    Session* session,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  bool is_dialog_open;
  Status status = session->chrome->IsJavaScriptDialogOpen(&is_dialog_open);
  if (status.IsError())
    return status;
  if (is_dialog_open)
    return Status(kUnexpectedAlertOpen);

  WebView* web_view = NULL;
  status = session->GetTargetWindow(&web_view);
  if (status.IsError())
    return status;

  status = web_view->ConnectIfNecessary();
  if (status.IsError())
    return status;

  Status nav_status = web_view->WaitForPendingNavigations(session->frame);
  if (nav_status.IsError())
    return nav_status;
  status = command.Run(session, web_view, params, value);
  // Switch to main frame and retry command if subframe no longer exists.
  if (status.code() == kNoSuchFrame) {
    session->frame = "";
    nav_status = web_view->WaitForPendingNavigations(session->frame);
    if (nav_status.IsError())
      return nav_status;
    status = command.Run(session, web_view, params, value);
  }
  nav_status = web_view->WaitForPendingNavigations(session->frame);
  if (status.IsOk() && nav_status.IsError() &&
      nav_status.code() != kDisconnected)
    return nav_status;
  return status;
}

Status ExecuteGet(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string url;
  if (!params.GetString("url", &url))
    return Status(kUnknownError, "'url' must be a string");
  return web_view->Load(url);
}

Status ExecuteExecuteScript(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string script;
  if (!params.GetString("script", &script))
    return Status(kUnknownError, "'script' must be a string");
  const base::ListValue* args;
  if (!params.GetList("args", &args))
    return Status(kUnknownError, "'args' must be a list");

  return web_view->CallFunction(
      session->frame, "function(){" + script + "}", *args, value);
}

Status ExecuteSwitchToFrame(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  const base::Value* id;
  if (!params.Get("id", &id))
    return Status(kUnknownError, "missing 'id'");

  if (id->IsType(base::Value::TYPE_NULL)) {
    session->frame = "";
    return Status(kOk);
  }

  std::string script;
  base::ListValue args;
  const base::DictionaryValue* id_dict;
  if (id->GetAsDictionary(&id_dict)) {
    script = "function(elem) { return elem; }";
    args.Append(id_dict->DeepCopy());
  } else {
    script =
        "function(xpath) {"
        "  return document.evaluate(xpath, document, null, "
        "      XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
        "}";
    std::string xpath = "(/html/body//iframe|/html/frameset/frame)";
    std::string id_string;
    int id_int;
    if (id->GetAsString(&id_string)) {
      xpath += base::StringPrintf(
          "[@name=\"%s\" or @id=\"%s\"]", id_string.c_str(), id_string.c_str());
    } else if (id->GetAsInteger(&id_int)) {
      xpath += base::StringPrintf("[%d]", id_int + 1);
    } else {
      return Status(kUnknownError, "invalid 'id'");
    }
    args.Append(new base::StringValue(xpath));
  }
  std::string frame;
  Status status = web_view->GetFrameByFunction(
      session->frame, script, args, &frame);
  if (status.IsError())
    return status;
  session->frame = frame;
  return Status(kOk);
}

Status ExecuteGetTitle(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  const char* kGetTitleScript =
      "function() {"
      "  if (document.title)"
      "    return document.title;"
      "  else"
      "    return document.URL;"
      "}";
  base::ListValue args;
  return web_view->CallFunction(session->frame, kGetTitleScript, args, value);
}

Status ExecuteGetPageSource(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  const char* kGetPageSource =
      "function() {"
      "  return new XMLSerializer().serializeToString(document);"
      "}";
  base::ListValue args;
  return web_view->CallFunction(session->frame, kGetPageSource, args, value);
}

Status ExecuteFindElement(
    int interval_ms,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return FindElement(interval_ms, true, NULL, session, web_view, params, value);
}

Status ExecuteFindElements(
    int interval_ms,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return FindElement(
      interval_ms, false, NULL, session, web_view, params, value);
}

Status ExecuteGetCurrentUrl(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string url;
  Status status = GetUrl(web_view, session->frame, &url);
  if (status.IsError())
    return status;
  value->reset(new base::StringValue(url));
  return Status(kOk);
}

Status ExecuteGoBack(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->EvaluateScript("", "window.history.back();", value);
}

Status ExecuteGoForward(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->EvaluateScript("", "window.history.forward();", value);
}

Status ExecuteRefresh(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->Reload();
}

Status ExecuteMouseMoveTo(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string element_id;
  bool has_element = params.GetString("element", &element_id);
  int x_offset = 0;
  int y_offset = 0;
  bool has_offset = params.GetInteger("xoffset", &x_offset) &&
      params.GetInteger("yoffset", &y_offset);
  if (!has_element && !has_offset)
    return Status(kUnknownError, "at least an element or offset should be set");

  WebPoint location;
  if (has_element) {
    Status status = ScrollElementIntoView(
        session, web_view, element_id, &location);
    if (status.IsError())
      return status;
  } else {
    location = session->mouse_position;
  }

  if (has_offset) {
    location.offset(x_offset, y_offset);
  } else {
    WebSize size;
    Status status = GetElementSize(session, web_view, element_id, &size);
    if (status.IsError())
      return status;
    location.offset(size.width / 2, size.height / 2);
  }

  std::list<MouseEvent> events;
  events.push_back(
      MouseEvent(kMovedMouseEventType, kNoneMouseButton,
                 location.x, location.y, 0));
  Status status = web_view->DispatchMouseEvents(events);
  if (status.IsOk())
    session->mouse_position = location;
  return status;
}

Status ExecuteMouseClick(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  MouseButton button;
  Status status = GetMouseButton(params, &button);
  if (status.IsError())
    return status;
  std::list<MouseEvent> events;
  events.push_back(
      MouseEvent(kPressedMouseEventType, button,
                 session->mouse_position.x, session->mouse_position.y, 1));
  events.push_back(
      MouseEvent(kReleasedMouseEventType, button,
                 session->mouse_position.x, session->mouse_position.y, 1));
  return web_view->DispatchMouseEvents(events);
}

Status ExecuteMouseButtonDown(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  MouseButton button;
  Status status = GetMouseButton(params, &button);
  if (status.IsError())
    return status;
  std::list<MouseEvent> events;
  events.push_back(
      MouseEvent(kPressedMouseEventType, button,
                 session->mouse_position.x, session->mouse_position.y, 1));
  return web_view->DispatchMouseEvents(events);
}

Status ExecuteMouseButtonUp(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  MouseButton button;
  Status status = GetMouseButton(params, &button);
  if (status.IsError())
    return status;
  std::list<MouseEvent> events;
  events.push_back(
      MouseEvent(kReleasedMouseEventType, button,
                 session->mouse_position.x, session->mouse_position.y, 1));
  return web_view->DispatchMouseEvents(events);
}

Status ExecuteMouseDoubleClick(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  MouseButton button;
  Status status = GetMouseButton(params, &button);
  if (status.IsError())
    return status;
  std::list<MouseEvent> events;
  events.push_back(
      MouseEvent(kPressedMouseEventType, button,
                 session->mouse_position.x, session->mouse_position.y, 2));
  events.push_back(
      MouseEvent(kReleasedMouseEventType, button,
                 session->mouse_position.x, session->mouse_position.y, 2));
  return web_view->DispatchMouseEvents(events);
}

Status ExecuteGetActiveElement(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  base::ListValue args;
  return web_view->CallFunction(
      session->frame,
      "function() { return document.activeElement || document.body }",
      args,
      value);
}

Status ExecuteGetAppCacheStatus(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->EvaluateScript(
      session->frame,
      "applicationCache.status",
      value);
}

Status ExecuteIsBrowserOnline(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->EvaluateScript(
      session->frame,
      "navigator.onLine",
      value);
}

Status ExecuteGetStorageItem(
    const char* storage,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string key;
  if (!params.GetString("key", &key))
    return Status(kUnknownError, "'key' must be a string");
  base::ListValue args;
  args.Append(new base::StringValue(key));
  return web_view->CallFunction(
      session->frame,
      base::StringPrintf("function(key) { return %s[key]; }", storage),
      args,
      value);
}

Status ExecuteGetStorageKeys(
    const char* storage,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  const char script[] =
      "var keys = [];"
      "for (var key in %s) {"
      "  keys.push(key);"
      "}"
      "keys";
  return web_view->EvaluateScript(
      session->frame,
      base::StringPrintf(script, storage),
      value);
}

Status ExecuteSetStorageItem(
    const char* storage,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string key;
  if (!params.GetString("key", &key))
    return Status(kUnknownError, "'key' must be a string");
  std::string storage_value;
  if (!params.GetString("value", &storage_value))
    return Status(kUnknownError, "'value' must be a string");
  base::ListValue args;
  args.Append(new base::StringValue(key));
  args.Append(new base::StringValue(storage_value));
  return web_view->CallFunction(
      session->frame,
      base::StringPrintf("function(key, value) { %s[key] = value; }", storage),
      args,
      value);
}

Status ExecuteRemoveStorageItem(
    const char* storage,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string key;
  if (!params.GetString("key", &key))
    return Status(kUnknownError, "'key' must be a string");
  base::ListValue args;
  args.Append(new base::StringValue(key));
  return web_view->CallFunction(
      session->frame,
      base::StringPrintf("function(key) { %s.removeItem(key) }", storage),
      args,
      value);
}

Status ExecuteClearStorage(
    const char* storage,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->EvaluateScript(
      session->frame,
      base::StringPrintf("%s.clear()", storage),
      value);
}

Status ExecuteGetStorageSize(
    const char* storage,
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  return web_view->EvaluateScript(
      session->frame,
      base::StringPrintf("%s.length", storage),
      value);
}

Status ExecuteScreenshot(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string screenshot;
  Status status = web_view->CaptureScreenshot(&screenshot);
  if (status.IsError())
    return status;
  value->reset(new base::StringValue(screenshot));
  return Status(kOk);
}

Status ExecuteGetCookies(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::list<Cookie> cookies;
  Status status = GetVisibleCookies(web_view, &cookies);
  if (status.IsError())
    return status;
  scoped_ptr<base::ListValue> cookie_list(new base::ListValue());
  for (std::list<Cookie>::const_iterator it = cookies.begin();
       it != cookies.end(); ++it) {
    cookie_list->Append(CreateDictionaryFrom(*it));
  }
  value->reset(cookie_list.release());
  return Status(kOk);
}

Status ExecuteAddCookie(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  const base::DictionaryValue* cookie;
  if (!params.GetDictionary("cookie", &cookie))
    return Status(kUnknownError, "missing 'cookie'");
  base::ListValue args;
  args.Append(cookie->DeepCopy());
  scoped_ptr<base::Value> result;
  return web_view->CallFunction(
      session->frame, kAddCookieScript, args, &result);
}

Status ExecuteDeleteCookie(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::string name;
  if (!params.GetString("name", &name))
    return Status(kUnknownError, "missing 'name'");
  base::DictionaryValue params_url;
  scoped_ptr<base::Value> value_url;
  std::string url;
  Status status = GetUrl(web_view, session->frame, &url);
  if (status.IsError())
    return status;
  return web_view->DeleteCookie(name, url);
}

Status ExecuteDeleteAllCookies(
    Session* session,
    WebView* web_view,
    const base::DictionaryValue& params,
    scoped_ptr<base::Value>* value) {
  std::list<Cookie> cookies;
  Status status = GetVisibleCookies(web_view, &cookies);
  if (status.IsError())
    return status;

  if (!cookies.empty()) {
    base::DictionaryValue params_url;
    scoped_ptr<base::Value> value_url;
    std::string url;
    status = GetUrl(web_view, session->frame, &url);
    if (status.IsError())
      return status;
    for (std::list<Cookie>::const_iterator it = cookies.begin();
         it != cookies.end(); ++it) {
      status = web_view->DeleteCookie(it->name, url);
      if (status.IsError())
        return status;
    }
  }

  return Status(kOk);
}
