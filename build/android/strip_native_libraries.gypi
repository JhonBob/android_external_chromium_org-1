# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is meant to be included into an action to provide a rule that strips
# native libraries.
#
# To use this, create a gyp target with the following form:
#  {
#    'action_name': 'strip_native_libraries',
#    'actions': [
#      'variables': {
#        'ordered_libraries_file': 'file generated by write_ordered_libraries'
#        'input_paths': 'files to be added to the list of inputs'
#        'stamp': 'file to touch when the action is complete'
#        'stripped_libraries_dir': 'directory to store stripped libraries',
#      },
#      'includes': [ '../../build/android/strip_native_libraries.gypi' ],
#    ],
#  },
#

{
  'message': 'Stripping libraries for <(_target_name)',
  'variables': {
    'input_paths': [],
  },
  'inputs': [
    '<(DEPTH)/build/android/gyp/util/build_utils.py',
    '<(DEPTH)/build/android/gyp/strip_library_for_device.py',
    '<(ordered_libraries_file)',
    '>@(input_paths)',
  ],
  'outputs': [
    '<(stamp)',
  ],
  'conditions': [
    ['component == "shared_library"', {
      # Add a fake output to force the build to always re-run this step. This
      # is required because the real inputs are not known at gyp-time and
      # changing base.so may not trigger changes to dependent libraries.
      'outputs': [ '<(stamp).fake' ]
    }],
  ],
  'action': [
    'python', '<(DEPTH)/build/android/gyp/strip_library_for_device.py',
    '--android-strip=<(android_strip)',
    '--android-strip-arg=--strip-unneeded',
    '--stripped-libraries-dir=<(stripped_libraries_dir)',
    '--libraries-dir=<(SHARED_LIB_DIR)',
    '--libraries-file=<(ordered_libraries_file)',
    '--stamp=<(stamp)',
  ],
}
