# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess

from telemetry.core import util
from telemetry.core.chrome import android_browser_finder
from telemetry.core.platform import profiler

class AndroidMemReportProfiler(profiler.Profiler):
  """Android-specific, collects 'memreport' graphs."""

  def __init__(self, browser_backend, platform_backend, output_path):
    super(AndroidMemReportProfiler, self).__init__(
        browser_backend, platform_backend, output_path)
    self._html_file = output_path + '.html'
    self._memreport = subprocess.Popen(
        [os.path.join(util.GetChromiumSrcDir(),
                      'tools', 'android', 'memdump', 'memreport.py'),
         '--manual-graph', '--package', browser_backend.package],
         stdout=file(self._html_file, 'w'),
         stdin=subprocess.PIPE)

  @classmethod
  def name(cls):
    return 'android-memreport'

  @classmethod
  def is_supported(cls, options):
    if not options:
      return android_browser_finder.CanFindAvailableBrowsers()
    return options.browser_type.startswith('android')

  def CollectProfile(self):
    self._memreport.communicate(input='\n')
    self._memreport.wait()
    print 'To view the memory report, open:'
    print self._html_file
    return [self._html_file]
