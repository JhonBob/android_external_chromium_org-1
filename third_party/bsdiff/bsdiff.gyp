# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'bsdiff',
      'type': 'executable',
      'dependencies': [
        '../bspatch/bspatch.gyp:bspatch',
      ],
      'link_settings': {
        'libraries': [
          '-lWs2_32.lib',
        ],
      },
      'sources': [
        'mbsdiff.cc',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
