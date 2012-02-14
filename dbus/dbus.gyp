# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'dbus',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../build/linux/system.gyp:dbus',
        '../third_party/protobuf/protobuf.gyp:protobuf_lite',
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
      ],
      'sources': [
        'bus.cc',
        'bus.h',
        'exported_object.cc',
        'exported_object.h',
        'message.cc',
        'message.h',
        'object_path.cc',
        'object_path.h',
        'object_proxy.cc',
        'object_proxy.h',
        'scoped_dbus_error.h',
      ],
    },
    {
      # Protobuf compiler / generator test protocol buffer
      'target_name': 'dbus_test_proto',
      'type': 'static_library',
      'sources': [ 'test_proto.proto' ],
      'variables': {
        'proto_out_dir': 'dbus',
      },
      'includes': [ '../build/protoc.gypi' ],
    },
    {
      # This target contains mocks that can be used to write unit tests
      # without issuing actual D-Bus calls.
      'target_name': 'dbus_test_support',
      'type': 'static_library',
      'dependencies': [
        '../build/linux/system.gyp:dbus',
        '../testing/gmock.gyp:gmock',
        'dbus',
      ],
      'sources': [
        'mock_bus.cc',
        'mock_bus.h',
        'mock_exported_object.cc',
        'mock_exported_object.h',
        'mock_object_proxy.cc',
        'mock_object_proxy.h',
      ],
      'include_dirs': [
        '..',
      ],
    },
    {
      'target_name': 'dbus_unittests',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:test_support_base',
        '../build/linux/system.gyp:dbus',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        'dbus',
        'dbus_test_proto',
        'dbus_test_support',
      ],
      'sources': [
        '../base/test/run_all_unittests.cc',
        'bus_unittest.cc',
        'end_to_end_async_unittest.cc',
        'end_to_end_sync_unittest.cc',
        'message_unittest.cc',
        'mock_unittest.cc',
        'test_service.cc',
        'test_service.h',
      ],
      'include_dirs': [
        '..',
      ],
    },
  ],
}
