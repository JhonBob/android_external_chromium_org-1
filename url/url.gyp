# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    'url_srcs.gypi',
  ],
  'targets': [
    {
      # Note, this target_name cannot be 'url', because that will generate
      # 'url.dll' for a Windows component build, and that will confuse Windows,
      # which has a system DLL with the same name.
      'target_name': 'url_lib',
      'type': '<(component)',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
      ],
      'sources': [
        '<@(gurl_sources)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'defines': [
        'URL_IMPLEMENTATION',
      ],
      'conditions': [
        ['use_icu_alternatives_on_android==1', {
          'sources!': [
            'url_canon_icu.cc',
            'url_canon_icu.h',
          ],
          'dependencies!': [
            '../third_party/icu/icu.gyp:icui18n',
            '../third_party/icu/icu.gyp:icuuc',
          ],
        }],
        ['use_icu_alternatives_on_android==1 and OS=="android"', {
          'dependencies': [
            'url_java',
            'url_jni_headers',
          ],
          'sources': [
            'url_canon_icu_alternatives_android.cc',
            'url_canon_icu_alternatives_android.h',
          ],
        }],
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [4267, ],
    },
    {
      'target_name': 'url_unittests',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:run_all_unittests',
        '../testing/gtest.gyp:gtest',
        '../third_party/icu/icu.gyp:icuuc',
        'url_lib',
      ],
      'sources': [
        'gurl_unittest.cc',
        'origin_unittest.cc',
        'url_canon_icu_unittest.cc',
        'url_canon_unittest.cc',
        'url_parse_unittest.cc',
        'url_test_utils.h',
        'url_util_unittest.cc',
      ],
      'conditions': [
        # TODO(dmikurube): Kill linux_use_tcmalloc. http://crbug.com/345554
        ['os_posix==1 and OS!="mac" and OS!="ios" and ((use_allocator!="none" and use_allocator!="see_use_tcmalloc") or (use_allocator=="see_use_tcmalloc" and linux_use_tcmalloc==1))',
          {
            'dependencies': [
              '../base/allocator/allocator.gyp:allocator',
            ],
          }
        ],
        ['use_icu_alternatives_on_android==1',
          {
            'sources!': [
              'url_canon_icu_unittest.cc',
            ],
            'dependencies!': [
              '../third_party/icu/icu.gyp:icuuc',
            ],
          }
        ],
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [4267, ],
    },
  ],
  'conditions': [
    ['use_icu_alternatives_on_android==1 and OS=="android"', {
      'targets': [
        {
          'target_name': 'url_jni_headers',
          'type': 'none',
          'sources': [
            'android/java/src/org/chromium/url/IDNStringUtil.java'
          ],
          'variables': {
            'jni_gen_package': 'url',
          },
          'includes': [ '../build/jni_generator.gypi' ],
        },
        {
          'target_name': 'url_java',
          'type': 'none',
          'variables': {
            'java_in_dir': '../url/android/java',
          },
          'dependencies': [
            '../base/base.gyp:base',
          ],
          'includes': [ '../build/java.gypi' ],
        },
      ],
    }],
  ],
}
