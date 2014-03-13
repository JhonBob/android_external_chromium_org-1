// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/websocket_host.h"

#include "base/basictypes.h"
#include "base/strings/string_util.h"
#include "content/browser/renderer_host/websocket_dispatcher_host.h"
#include "content/common/websocket_messages.h"
#include "ipc/ipc_message_macros.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/websockets/websocket_channel.h"
#include "net/websockets/websocket_event_interface.h"
#include "net/websockets/websocket_frame.h"  // for WebSocketFrameHeader::OpCode
#include "net/websockets/websocket_handshake_request_info.h"
#include "net/websockets/websocket_handshake_response_info.h"
#include "url/origin.h"

namespace content {

namespace {

typedef net::WebSocketEventInterface::ChannelState ChannelState;

// Convert a content::WebSocketMessageType to a
// net::WebSocketFrameHeader::OpCode
net::WebSocketFrameHeader::OpCode MessageTypeToOpCode(
    WebSocketMessageType type) {
  DCHECK(type == WEB_SOCKET_MESSAGE_TYPE_CONTINUATION ||
         type == WEB_SOCKET_MESSAGE_TYPE_TEXT ||
         type == WEB_SOCKET_MESSAGE_TYPE_BINARY);
  typedef net::WebSocketFrameHeader::OpCode OpCode;
  // These compile asserts verify that the same underlying values are used for
  // both types, so we can simply cast between them.
  COMPILE_ASSERT(static_cast<OpCode>(WEB_SOCKET_MESSAGE_TYPE_CONTINUATION) ==
                     net::WebSocketFrameHeader::kOpCodeContinuation,
                 enum_values_must_match_for_opcode_continuation);
  COMPILE_ASSERT(static_cast<OpCode>(WEB_SOCKET_MESSAGE_TYPE_TEXT) ==
                     net::WebSocketFrameHeader::kOpCodeText,
                 enum_values_must_match_for_opcode_text);
  COMPILE_ASSERT(static_cast<OpCode>(WEB_SOCKET_MESSAGE_TYPE_BINARY) ==
                     net::WebSocketFrameHeader::kOpCodeBinary,
                 enum_values_must_match_for_opcode_binary);
  return static_cast<OpCode>(type);
}

WebSocketMessageType OpCodeToMessageType(
    net::WebSocketFrameHeader::OpCode opCode) {
  DCHECK(opCode == net::WebSocketFrameHeader::kOpCodeContinuation ||
         opCode == net::WebSocketFrameHeader::kOpCodeText ||
         opCode == net::WebSocketFrameHeader::kOpCodeBinary);
  // This cast is guaranteed valid by the COMPILE_ASSERT() statements above.
  return static_cast<WebSocketMessageType>(opCode);
}

ChannelState StateCast(WebSocketDispatcherHost::WebSocketHostState host_state) {
  const WebSocketDispatcherHost::WebSocketHostState WEBSOCKET_HOST_ALIVE =
      WebSocketDispatcherHost::WEBSOCKET_HOST_ALIVE;
  const WebSocketDispatcherHost::WebSocketHostState WEBSOCKET_HOST_DELETED =
      WebSocketDispatcherHost::WEBSOCKET_HOST_DELETED;

  DCHECK(host_state == WEBSOCKET_HOST_ALIVE ||
         host_state == WEBSOCKET_HOST_DELETED);
  // These compile asserts verify that we can get away with using static_cast<>
  // for the conversion.
  COMPILE_ASSERT(static_cast<ChannelState>(WEBSOCKET_HOST_ALIVE) ==
                     net::WebSocketEventInterface::CHANNEL_ALIVE,
                 enum_values_must_match_for_state_alive);
  COMPILE_ASSERT(static_cast<ChannelState>(WEBSOCKET_HOST_DELETED) ==
                     net::WebSocketEventInterface::CHANNEL_DELETED,
                 enum_values_must_match_for_state_deleted);
  return static_cast<ChannelState>(host_state);
}

// Implementation of net::WebSocketEventInterface. Receives events from our
// WebSocketChannel object. Each event is translated to an IPC and sent to the
// renderer or child process via WebSocketDispatcherHost.
class WebSocketEventHandler : public net::WebSocketEventInterface {
 public:
  WebSocketEventHandler(WebSocketDispatcherHost* dispatcher, int routing_id);
  virtual ~WebSocketEventHandler();

  // net::WebSocketEventInterface implementation

  virtual ChannelState OnAddChannelResponse(
      bool fail,
      const std::string& selected_subprotocol,
      const std::string& extensions) OVERRIDE;
  virtual ChannelState OnDataFrame(bool fin,
                                   WebSocketMessageType type,
                                   const std::vector<char>& data) OVERRIDE;
  virtual ChannelState OnClosingHandshake() OVERRIDE;
  virtual ChannelState OnFlowControl(int64 quota) OVERRIDE;
  virtual ChannelState OnDropChannel(bool was_clean,
                                     uint16 code,
                                     const std::string& reason) OVERRIDE;
  virtual ChannelState OnFailChannel(const std::string& message) OVERRIDE;
  virtual ChannelState OnStartOpeningHandshake(
      scoped_ptr<net::WebSocketHandshakeRequestInfo> request) OVERRIDE;
  virtual ChannelState OnFinishOpeningHandshake(
      scoped_ptr<net::WebSocketHandshakeResponseInfo> response) OVERRIDE;

 private:
  WebSocketDispatcherHost* const dispatcher_;
  const int routing_id_;

  DISALLOW_COPY_AND_ASSIGN(WebSocketEventHandler);
};

WebSocketEventHandler::WebSocketEventHandler(
    WebSocketDispatcherHost* dispatcher,
    int routing_id)
    : dispatcher_(dispatcher), routing_id_(routing_id) {}

WebSocketEventHandler::~WebSocketEventHandler() {
  DVLOG(1) << "WebSocketEventHandler destroyed routing_id=" << routing_id_;
}

ChannelState WebSocketEventHandler::OnAddChannelResponse(
    bool fail,
    const std::string& selected_protocol,
    const std::string& extensions) {
  DVLOG(3) << "WebSocketEventHandler::OnAddChannelResponse"
           << " routing_id=" << routing_id_ << " fail=" << fail
           << " selected_protocol=\"" << selected_protocol << "\""
           << " extensions=\"" << extensions << "\"";
  return StateCast(dispatcher_->SendAddChannelResponse(
      routing_id_, fail, selected_protocol, extensions));
}

ChannelState WebSocketEventHandler::OnDataFrame(
    bool fin,
    net::WebSocketFrameHeader::OpCode type,
    const std::vector<char>& data) {
  DVLOG(3) << "WebSocketEventHandler::OnDataFrame"
           << " routing_id=" << routing_id_ << " fin=" << fin
           << " type=" << type << " data is " << data.size() << " bytes";
  return StateCast(dispatcher_->SendFrame(
      routing_id_, fin, OpCodeToMessageType(type), data));
}

ChannelState WebSocketEventHandler::OnClosingHandshake() {
  DVLOG(3) << "WebSocketEventHandler::OnClosingHandshake"
           << " routing_id=" << routing_id_;
  return StateCast(dispatcher_->SendClosing(routing_id_));
}

ChannelState WebSocketEventHandler::OnFlowControl(int64 quota) {
  DVLOG(3) << "WebSocketEventHandler::OnFlowControl"
           << " routing_id=" << routing_id_ << " quota=" << quota;
  return StateCast(dispatcher_->SendFlowControl(routing_id_, quota));
}

ChannelState WebSocketEventHandler::OnDropChannel(bool was_clean,
                                                  uint16 code,
                                                  const std::string& reason) {
  DVLOG(3) << "WebSocketEventHandler::OnDropChannel"
           << " routing_id=" << routing_id_ << " was_clean=" << was_clean
           << " code=" << code << " reason=\"" << reason << "\"";
  return StateCast(
      dispatcher_->DoDropChannel(routing_id_, was_clean, code, reason));
}

ChannelState WebSocketEventHandler::OnFailChannel(const std::string& message) {
  DVLOG(3) << "WebSocketEventHandler::OnFailChannel"
           << " routing_id=" << routing_id_
           << " message=\"" << message << "\"";
  return StateCast(dispatcher_->NotifyFailure(routing_id_, message));
}

ChannelState WebSocketEventHandler::OnStartOpeningHandshake(
    scoped_ptr<net::WebSocketHandshakeRequestInfo> request) {
  // TODO(yhirano) Do nothing if the inspector is not attached.
  DVLOG(3) << "WebSocketEventHandler::OnStartOpeningHandshake";
  WebSocketHandshakeRequest request_to_pass;
  request_to_pass.url.Swap(&request->url);
  net::HttpRequestHeaders::Iterator it(request->headers);
  while (it.GetNext())
    request_to_pass.headers.push_back(std::make_pair(it.name(), it.value()));
  request_to_pass.headers_text =
      base::StringPrintf("GET %s HTTP/1.1\r\n",
                         request_to_pass.url.spec().c_str()) +
      request->headers.ToString();

  request_to_pass.request_time = request->request_time;
  return StateCast(dispatcher_->SendStartOpeningHandshake(routing_id_,
                                                          request_to_pass));
}

ChannelState WebSocketEventHandler::OnFinishOpeningHandshake(
    scoped_ptr<net::WebSocketHandshakeResponseInfo> response) {
  // TODO(yhirano) Do nothing if the inspector is not attached.
  DVLOG(3) << "WebSocketEventHandler::OnFinishOpeningHandshake";
  WebSocketHandshakeResponse response_to_pass;
  response_to_pass.url.Swap(&response->url);
  response_to_pass.status_code = response->status_code;
  response_to_pass.status_text.swap(response->status_text);
  void* iter = NULL;
  std::string name, value;
  while (response->headers->EnumerateHeaderLines(&iter, &name, &value))
    response_to_pass.headers.push_back(std::make_pair(name, value));
  response_to_pass.headers_text =
      net::HttpUtil::ConvertHeadersBackToHTTPResponse(
          response->headers->raw_headers());
  response_to_pass.response_time = response->response_time;
  return StateCast(dispatcher_->SendFinishOpeningHandshake(routing_id_,
                                                           response_to_pass));
}

}  // namespace

WebSocketHost::WebSocketHost(int routing_id,
                             WebSocketDispatcherHost* dispatcher,
                             net::URLRequestContext* url_request_context)
    : routing_id_(routing_id) {
  DVLOG(1) << "WebSocketHost: created routing_id=" << routing_id;
  scoped_ptr<net::WebSocketEventInterface> event_interface(
      new WebSocketEventHandler(dispatcher, routing_id));
  channel_.reset(
      new net::WebSocketChannel(event_interface.Pass(), url_request_context));
}

WebSocketHost::~WebSocketHost() {}

bool WebSocketHost::OnMessageReceived(const IPC::Message& message,
                                      bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(WebSocketHost, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(WebSocketHostMsg_AddChannelRequest, OnAddChannelRequest)
    IPC_MESSAGE_HANDLER(WebSocketMsg_SendFrame, OnSendFrame)
    IPC_MESSAGE_HANDLER(WebSocketMsg_FlowControl, OnFlowControl)
    IPC_MESSAGE_HANDLER(WebSocketMsg_DropChannel, OnDropChannel)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()
  return handled;
}

void WebSocketHost::OnAddChannelRequest(
    const GURL& socket_url,
    const std::vector<std::string>& requested_protocols,
    const url::Origin& origin) {
  DVLOG(3) << "WebSocketHost::OnAddChannelRequest"
           << " routing_id=" << routing_id_ << " socket_url=\"" << socket_url
           << "\" requested_protocols=\""
           << JoinString(requested_protocols, ", ") << "\" origin=\""
           << origin.string() << "\"";

  channel_->SendAddChannelRequest(
      socket_url, requested_protocols, origin);
}

void WebSocketHost::OnSendFrame(bool fin,
                                WebSocketMessageType type,
                                const std::vector<char>& data) {
  DVLOG(3) << "WebSocketHost::OnSendFrame"
           << " routing_id=" << routing_id_ << " fin=" << fin
           << " type=" << type << " data is " << data.size() << " bytes";

  channel_->SendFrame(fin, MessageTypeToOpCode(type), data);
}

void WebSocketHost::OnFlowControl(int64 quota) {
  DVLOG(3) << "WebSocketHost::OnFlowControl"
           << " routing_id=" << routing_id_ << " quota=" << quota;

  channel_->SendFlowControl(quota);
}

void WebSocketHost::OnDropChannel(bool was_clean,
                                  uint16 code,
                                  const std::string& reason) {
  DVLOG(3) << "WebSocketHost::OnDropChannel"
           << " routing_id=" << routing_id_ << " was_clean=" << was_clean
           << " code=" << code << " reason=\"" << reason << "\"";

  // TODO(yhirano): Handle |was_clean| appropriately.
  channel_->StartClosingHandshake(code, reason);
}

}  // namespace content
