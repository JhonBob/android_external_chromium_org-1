// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/device/android_device_manager.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread.h"
#include "chrome/browser/devtools/device/adb/adb_client_socket.h"
#include "chrome/browser/devtools/device/usb/android_rsa.h"
#include "chrome/browser/devtools/device/usb/android_usb_device.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_client_socket.h"

using content::BrowserThread;

namespace {

const char kHostTransportCommand[] = "host:transport:%s|%s";
const char kHostDevicesCommand[] = "host:devices";
const char kLocalAbstractCommand[] = "localabstract:%s";

const int kAdbPort = 5037;
const int kBufferSize = 16 * 1024;

const char kDeviceModelCommand[] = "shell:getprop ro.product.model";
const char kOpenedUnixSocketsCommand[] = "shell:cat /proc/net/unix";
const char kOpenedUnixSocketsResponse[] =
    "Num       RefCount Protocol Flags    Type St Inode Path\n"
    "00000000: 00000002 00000000 00010000 0001 01 20894 @%s\n";
const char kRemoteDebuggingSocket[] = "chrome_devtools_remote";
const char kLocalChrome[] = "Local Chrome";
const char kLocalhost[] = "127.0.0.1";

static const char kWebSocketUpgradeRequest[] = "GET %s HTTP/1.1\r\n"
    "Upgrade: WebSocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n";

class HttpRequest {
 public:
  typedef AndroidDeviceManager::CommandCallback CommandCallback;
  typedef AndroidDeviceManager::SocketCallback SocketCallback;

  static void CommandRequest(const std::string& request,
                           const CommandCallback& callback,
                           int result,
                           net::StreamSocket* socket) {
    if (result != net::OK) {
      callback.Run(result, std::string());
      return;
    }
    new HttpRequest(socket, request, callback);
  }

  static void SocketRequest(const std::string& request,
                          const SocketCallback& callback,
                          int result,
                          net::StreamSocket* socket) {
    if (result != net::OK) {
      callback.Run(result, NULL);
      return;
    }
    new HttpRequest(socket, request, callback);
  }

 private:
  HttpRequest(net::StreamSocket* socket,
                      const std::string& request,
                      const CommandCallback& callback)
    : socket_(socket),
      command_callback_(callback),
      body_pos_(0) {
    SendRequest(request);
  }

  HttpRequest(net::StreamSocket* socket,
                      const std::string& request,
                      const SocketCallback& callback)
    : socket_(socket),
      socket_callback_(callback),
      body_pos_(0) {
    SendRequest(request);
  }

  ~HttpRequest() {
  }

  void SendRequest(const std::string& request) {
    scoped_refptr<net::StringIOBuffer> request_buffer =
        new net::StringIOBuffer(request);

    int result = socket_->Write(
        request_buffer.get(),
        request_buffer->size(),
        base::Bind(&HttpRequest::ReadResponse, base::Unretained(this)));
    if (result != net::ERR_IO_PENDING)
      ReadResponse(result);
  }

  void ReadResponse(int result) {
    if (!CheckNetResultOrDie(result))
      return;
    scoped_refptr<net::IOBuffer> response_buffer =
        new net::IOBuffer(kBufferSize);

    result = socket_->Read(
        response_buffer.get(),
        kBufferSize,
        base::Bind(&HttpRequest::OnResponseData, base::Unretained(this),
                  response_buffer,
                  -1));
    if (result != net::ERR_IO_PENDING)
      OnResponseData(response_buffer, -1, result);
  }

  void OnResponseData(scoped_refptr<net::IOBuffer> response_buffer,
                      int bytes_total,
                      int result) {
    if (!CheckNetResultOrDie(result))
      return;
    if (result == 0) {
      CheckNetResultOrDie(net::ERR_CONNECTION_CLOSED);
      return;
    }

    response_ += std::string(response_buffer->data(), result);
    int expected_length = 0;
    if (bytes_total < 0) {
      // TODO(kaznacheev): Use net::HttpResponseHeader to parse the header.
      size_t content_pos = response_.find("Content-Length:");
      if (content_pos != std::string::npos) {
        size_t endline_pos = response_.find("\n", content_pos);
        if (endline_pos != std::string::npos) {
          std::string len = response_.substr(content_pos + 15,
                                             endline_pos - content_pos - 15);
          base::TrimWhitespace(len, base::TRIM_ALL, &len);
          if (!base::StringToInt(len, &expected_length)) {
            CheckNetResultOrDie(net::ERR_FAILED);
            return;
          }
        }
      }

      body_pos_ = response_.find("\r\n\r\n");
      if (body_pos_ != std::string::npos) {
        body_pos_ += 4;
        bytes_total = body_pos_ + expected_length;
      }
    }

    if (bytes_total == static_cast<int>(response_.length())) {
      if (!command_callback_.is_null())
        command_callback_.Run(net::OK, response_.substr(body_pos_));
      else
        socket_callback_.Run(net::OK, socket_.release());
      delete this;
      return;
    }

    result = socket_->Read(
        response_buffer.get(),
        kBufferSize,
        base::Bind(&HttpRequest::OnResponseData,
                   base::Unretained(this),
                   response_buffer,
                   bytes_total));
    if (result != net::ERR_IO_PENDING)
      OnResponseData(response_buffer, bytes_total, result);
  }

  bool CheckNetResultOrDie(int result) {
    if (result >= 0)
      return true;
    if (!command_callback_.is_null())
      command_callback_.Run(result, std::string());
    else
      socket_callback_.Run(result, NULL);
    delete this;
    return false;
  }

  scoped_ptr<net::StreamSocket> socket_;
  std::string response_;
  AndroidDeviceManager::CommandCallback command_callback_;
  AndroidDeviceManager::SocketCallback socket_callback_;
  size_t body_pos_;
};


// AdbDeviceImpl --------------------------------------------------------------

class AdbDeviceImpl : public AndroidDeviceManager::Device {
 public:
  AdbDeviceImpl(const std::string& serial, bool is_connected);
  virtual void RunCommand(const std::string& command,
                          const CommandCallback& callback) OVERRIDE;
  virtual void OpenSocket(const std::string& name,
                          const SocketCallback& callback) OVERRIDE;
 private:
  virtual ~AdbDeviceImpl() {}
};

AdbDeviceImpl::AdbDeviceImpl(const std::string& serial, bool is_connected)
    : Device(serial, is_connected) {
}

void AdbDeviceImpl::RunCommand(const std::string& command,
                               const CommandCallback& callback) {
  DCHECK(CalledOnValidThread());
  std::string query = base::StringPrintf(kHostTransportCommand,
                                         serial().c_str(), command.c_str());
  AdbClientSocket::AdbQuery(kAdbPort, query, callback);
}

void AdbDeviceImpl::OpenSocket(const std::string& name,
                               const SocketCallback& callback) {
  DCHECK(CalledOnValidThread());
  std::string socket_name =
      base::StringPrintf(kLocalAbstractCommand, name.c_str());
  AdbClientSocket::TransportQuery(kAdbPort, serial(), socket_name, callback);
}

// UsbDeviceImpl --------------------------------------------------------------

class UsbDeviceImpl : public AndroidDeviceManager::Device {
 public:
  explicit UsbDeviceImpl(AndroidUsbDevice* device);
  virtual void RunCommand(const std::string& command,
                          const CommandCallback& callback) OVERRIDE;
  virtual void OpenSocket(const std::string& name,
                          const SocketCallback& callback) OVERRIDE;
 private:
  void OnOpenSocket(const SocketCallback& callback,
                    net::StreamSocket* socket,
                    int result);
  void OpenedForCommand(const CommandCallback& callback,
                        net::StreamSocket* socket,
                        int result);
  void OnRead(net::StreamSocket* socket,
              scoped_refptr<net::IOBuffer> buffer,
              const std::string& data,
              const CommandCallback& callback,
              int result);

  virtual ~UsbDeviceImpl() {}
  scoped_refptr<AndroidUsbDevice> device_;
};


UsbDeviceImpl::UsbDeviceImpl(AndroidUsbDevice* device)
    : Device(device->serial(), device->is_connected()),
      device_(device) {
  device_->InitOnCallerThread();
}

void UsbDeviceImpl::RunCommand(const std::string& command,
                               const CommandCallback& callback) {
  DCHECK(CalledOnValidThread());
  net::StreamSocket* socket = device_->CreateSocket(command);
  if (!socket) {
    callback.Run(net::ERR_CONNECTION_FAILED, std::string());
    return;
  }
  int result = socket->Connect(base::Bind(&UsbDeviceImpl::OpenedForCommand,
                                          this, callback, socket));
  if (result != net::ERR_IO_PENDING)
    callback.Run(result, std::string());
}

void UsbDeviceImpl::OpenSocket(const std::string& name,
                               const SocketCallback& callback) {
  DCHECK(CalledOnValidThread());
  std::string socket_name =
      base::StringPrintf(kLocalAbstractCommand, name.c_str());
  net::StreamSocket* socket = device_->CreateSocket(socket_name);
  if (!socket) {
    callback.Run(net::ERR_CONNECTION_FAILED, NULL);
    return;
  }
  int result = socket->Connect(base::Bind(&UsbDeviceImpl::OnOpenSocket, this,
                                          callback, socket));
  if (result != net::ERR_IO_PENDING)
    callback.Run(result, NULL);
}

void UsbDeviceImpl::OnOpenSocket(const SocketCallback& callback,
                  net::StreamSocket* socket,
                  int result) {
  callback.Run(result, result == net::OK ? socket : NULL);
}

void UsbDeviceImpl::OpenedForCommand(const CommandCallback& callback,
                                     net::StreamSocket* socket,
                                     int result) {
  if (result != net::OK) {
    callback.Run(result, std::string());
    return;
  }
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kBufferSize);
  result = socket->Read(buffer, kBufferSize,
                        base::Bind(&UsbDeviceImpl::OnRead, this,
                                   socket, buffer, std::string(), callback));
  if (result != net::ERR_IO_PENDING)
    OnRead(socket, buffer, std::string(), callback, result);
}

void UsbDeviceImpl::OnRead(net::StreamSocket* socket,
                           scoped_refptr<net::IOBuffer> buffer,
                           const std::string& data,
                           const CommandCallback& callback,
                           int result) {
  if (result <= 0) {
    callback.Run(result, result == 0 ? data : std::string());
    delete socket;
    return;
  }

  std::string new_data = data + std::string(buffer->data(), result);
  result = socket->Read(buffer, kBufferSize,
                        base::Bind(&UsbDeviceImpl::OnRead, this,
                                   socket, buffer, new_data, callback));
  if (result != net::ERR_IO_PENDING)
    OnRead(socket, buffer, new_data, callback, result);
}

// AdbDeviceProvider -------------------------------------------

class AdbDeviceProvider : public AndroidDeviceManager::DeviceProvider {
 public:
  virtual void QueryDevices(const QueryDevicesCallback& callback) OVERRIDE;
 private:
  void ReceivedAdbDevices(const QueryDevicesCallback& callback, int result,
                          const std::string& response);

  virtual ~AdbDeviceProvider();
};

AdbDeviceProvider::~AdbDeviceProvider() {
}

void AdbDeviceProvider::QueryDevices(const QueryDevicesCallback& callback) {
  AdbClientSocket::AdbQuery(
      kAdbPort, kHostDevicesCommand,
      base::Bind(&AdbDeviceProvider::ReceivedAdbDevices, this, callback));
}

void AdbDeviceProvider::ReceivedAdbDevices(const QueryDevicesCallback& callback,
                                           int result_code,
                                           const std::string& response) {
  AndroidDeviceManager::Devices result;
  std::vector<std::string> serials;
  Tokenize(response, "\n", &serials);
  for (size_t i = 0; i < serials.size(); ++i) {
    std::vector<std::string> tokens;
    Tokenize(serials[i], "\t ", &tokens);
    bool offline = tokens.size() > 1 && tokens[1] == "offline";
    result.push_back(new AdbDeviceImpl(tokens[0], !offline));
  }
  callback.Run(result);
}

// UsbDeviceProvider -------------------------------------------

class UsbDeviceProvider : public AndroidDeviceManager::DeviceProvider {
 public:
  explicit UsbDeviceProvider(Profile* profile);

  virtual void QueryDevices(const QueryDevicesCallback& callback) OVERRIDE;
 private:
  virtual ~UsbDeviceProvider();
  void EnumeratedDevices(const QueryDevicesCallback& callback,
                         const AndroidUsbDevices& devices);

  scoped_ptr<crypto::RSAPrivateKey>  rsa_key_;
};

UsbDeviceProvider::UsbDeviceProvider(Profile* profile){
  rsa_key_.reset(AndroidRSAPrivateKey(profile));
}

UsbDeviceProvider::~UsbDeviceProvider() {
}

void UsbDeviceProvider::QueryDevices(const QueryDevicesCallback& callback) {
  AndroidUsbDevice::Enumerate(rsa_key_.get(),
      base::Bind(&UsbDeviceProvider::EnumeratedDevices, this, callback));
}

void UsbDeviceProvider::EnumeratedDevices(const QueryDevicesCallback& callback,
                                          const AndroidUsbDevices& devices) {
  AndroidDeviceManager::Devices result;
  for (AndroidUsbDevices::const_iterator it = devices.begin();
      it != devices.end(); ++it)
    result.push_back(new UsbDeviceImpl(*it));
  callback.Run(result);
}

} // namespace

AndroidDeviceManager::Device::Device(const std::string& serial,
                                     bool is_connected)
    : serial_(serial),
      is_connected_(is_connected) {
}

AndroidDeviceManager::Device::~Device() {
}

AndroidDeviceManager::DeviceProvider::DeviceProvider() {
}

AndroidDeviceManager::DeviceProvider::~DeviceProvider() {
}

// static
scoped_refptr<AndroidDeviceManager::DeviceProvider>
    AndroidDeviceManager::GetUsbDeviceProvider(Profile* profile) {
  return new UsbDeviceProvider(profile);
}

// static
scoped_refptr<AndroidDeviceManager::DeviceProvider>
    AndroidDeviceManager::GetAdbDeviceProvider() {
  return new AdbDeviceProvider();
}


class SelfAsDevice : public AndroidDeviceManager::Device {
 public:
  explicit SelfAsDevice(int port);

  virtual void RunCommand(const std::string& command,
                          const CommandCallback& callback) OVERRIDE;
  virtual void OpenSocket(const std::string& socket_name,
                          const SocketCallback& callback) OVERRIDE;
 private:
  void RunCommandCallback(const CommandCallback& callback,
                          const std::string& response,
                          int result);

  void RunSocketCallback(const SocketCallback& callback,
                         net::StreamSocket* socket,
                         int result);
  virtual ~SelfAsDevice() {}

  int port_;
};

SelfAsDevice::SelfAsDevice(int port)
    : Device("local", true),
      port_(port)
{}

void SelfAsDevice::RunCommandCallback(const CommandCallback& callback,
                                      const std::string& response,
                                      int result) {
  callback.Run(result, response);
}

void SelfAsDevice::RunSocketCallback(const SocketCallback& callback,
                                     net::StreamSocket* socket,
                                     int result) {
  callback.Run(result, socket);
}

void SelfAsDevice::RunCommand(const std::string& command,
                              const CommandCallback& callback) {
  DCHECK(CalledOnValidThread());
  std::string response;
  if (command == kDeviceModelCommand) {
    response = kLocalChrome;
  } else if (command == kOpenedUnixSocketsCommand) {
    response = base::StringPrintf(kOpenedUnixSocketsResponse,
                                  kRemoteDebuggingSocket);
  }

  base::MessageLoop::current()->PostTask(FROM_HERE,
                base::Bind(&SelfAsDevice::RunCommandCallback, this, callback,
                           response, 0));
}

void SelfAsDevice::OpenSocket(const std::string& socket_name,
                              const SocketCallback& callback) {
  DCHECK(CalledOnValidThread());
  // Use plain socket for remote debugging and port forwarding on Desktop
  // (debugging purposes).
  net::IPAddressNumber ip_number;
  net::ParseIPLiteralToNumber(kLocalhost, &ip_number);

  int port = 0;
  if (socket_name == kRemoteDebuggingSocket)
    port = port_;
  else
    base::StringToInt(socket_name, &port);

  net::AddressList address_list =
      net::AddressList::CreateFromIPAddress(ip_number, port);
  net::TCPClientSocket* socket = new net::TCPClientSocket(
      address_list, NULL, net::NetLog::Source());
  socket->Connect(base::Bind(&SelfAsDevice::RunSocketCallback, this, callback,
                             socket));
}

class SelfAsDeviceProvider : public AndroidDeviceManager::DeviceProvider {
 public:
  explicit SelfAsDeviceProvider(int port);

  virtual void QueryDevices(const QueryDevicesCallback& callback) OVERRIDE;
 private:
  virtual ~SelfAsDeviceProvider(){}

  int port_;
};

SelfAsDeviceProvider::SelfAsDeviceProvider(int port)
    : port_(port) {
}

void SelfAsDeviceProvider::QueryDevices(const QueryDevicesCallback& callback) {
  AndroidDeviceManager::Devices result;
  result.push_back(new SelfAsDevice(port_));
  callback.Run(result);
}

// static
scoped_refptr<AndroidDeviceManager::DeviceProvider>
AndroidDeviceManager::GetSelfAsDeviceProvider(int port) {
  return new SelfAsDeviceProvider(port);
}

// static
scoped_refptr<AndroidDeviceManager> AndroidDeviceManager::Create() {
  return new AndroidDeviceManager();
}

void AndroidDeviceManager::QueryDevices(
    const DeviceProviders& providers,
    const QueryDevicesCallback& callback) {
  DCHECK(CalledOnValidThread());
  stopped_ = false;
  Devices empty;
  QueryNextProvider(callback, providers, empty, empty);
}

void AndroidDeviceManager::Stop() {
  DCHECK(CalledOnValidThread());
  stopped_ = true;
  devices_.clear();
}

bool AndroidDeviceManager::IsConnected(const std::string& serial) {
  DCHECK(CalledOnValidThread());
  Device* device = FindDevice(serial);
  return device && device->is_connected();
}

void AndroidDeviceManager::RunCommand(
    const std::string& serial,
    const std::string& command,
    const CommandCallback& callback) {
  DCHECK(CalledOnValidThread());
  Device* device = FindDevice(serial);
  if (device)
    device->RunCommand(command, callback);
  else
    callback.Run(net::ERR_CONNECTION_FAILED, std::string());
}

void AndroidDeviceManager::OpenSocket(
    const std::string& serial,
    const std::string& socket_name,
    const SocketCallback& callback) {
  DCHECK(CalledOnValidThread());
  Device* device = FindDevice(serial);
  if (device)
    device->OpenSocket(socket_name, callback);
  else
    callback.Run(net::ERR_CONNECTION_FAILED, NULL);
}

void AndroidDeviceManager::HttpQuery(
    const std::string& serial,
    const std::string& socket_name,
    const std::string& request,
    const CommandCallback& callback) {
  DCHECK(CalledOnValidThread());
  Device* device = FindDevice(serial);
  if (device)
    device->OpenSocket(socket_name,
        base::Bind(&HttpRequest::CommandRequest, request, callback));
  else
    callback.Run(net::ERR_CONNECTION_FAILED, std::string());
}

void AndroidDeviceManager::HttpUpgrade(
    const std::string& serial,
    const std::string& socket_name,
    const std::string& url,
    const SocketCallback& callback) {
  DCHECK(CalledOnValidThread());
  Device* device = FindDevice(serial);
  if (device) {
    device->OpenSocket(
        socket_name,
        base::Bind(&HttpRequest::SocketRequest,
                   base::StringPrintf(kWebSocketUpgradeRequest, url.c_str()),
                   callback));
  } else {
    callback.Run(net::ERR_CONNECTION_FAILED, NULL);
  }
}

AndroidDeviceManager::AndroidDeviceManager()
    : stopped_(false) {
}

AndroidDeviceManager::~AndroidDeviceManager() {
}

void AndroidDeviceManager::QueryNextProvider(
    const QueryDevicesCallback& callback,
    const DeviceProviders& providers,
    const Devices& total_devices,
    const Devices& new_devices) {
  DCHECK(CalledOnValidThread());

  if (stopped_)
    return;

  Devices more_devices(total_devices);
  more_devices.insert(
      more_devices.end(), new_devices.begin(), new_devices.end());

  if (providers.empty()) {
    std::vector<std::string> serials;
    devices_.clear();
    for (Devices::const_iterator it = more_devices.begin();
        it != more_devices.end(); ++it) {
      devices_[(*it)->serial()] = *it;
      serials.push_back((*it)->serial());
    }
    callback.Run(serials);
    return;
  }

  scoped_refptr<DeviceProvider> current_provider = providers.back();
  DeviceProviders less_providers = providers;
  less_providers.pop_back();
  current_provider->QueryDevices(
      base::Bind(&AndroidDeviceManager::QueryNextProvider,
                 this, callback, less_providers, more_devices));
}

AndroidDeviceManager::Device*
AndroidDeviceManager::FindDevice(const std::string& serial) {
  DCHECK(CalledOnValidThread());
  DeviceMap::const_iterator it = devices_.find(serial);
  if (it == devices_.end())
    return NULL;
  return (*it).second.get();
}
