# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'content_common_mojo_bindings',
      'type': 'static_library',
      'dependencies': [
        '../mojo/mojo.gyp:mojo_bindings',
        '../mojo/mojo.gyp:mojo_environment_chromium',
        '../mojo/mojo.gyp:mojo_system',
      ],
      'sources': [
        'common/mojo/render_process.mojom',
      ],
      'includes': [ '../mojo/public/bindings/mojom_bindings_generator.gypi' ],
      'export_dependent_settings': [
        '../mojo/mojo.gyp:mojo_bindings',
        '../mojo/mojo.gyp:mojo_environment_chromium',
        '../mojo/mojo.gyp:mojo_system',
      ],
    },
  ],
}
