# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import sys
import unittest

from telemetry import test
from telemetry.core.platform import factory
from telemetry.core.platform import posix_platform_backend


class TestBackend(posix_platform_backend.PosixPlatformBackend):

  # pylint: disable=W0223

  def __init__(self):
    super(TestBackend, self).__init__()
    self._mock_ps_output = None

  def SetMockPsOutput(self, output):
    self._mock_ps_output = output

  def _GetPsOutput(self, columns, pid=None):
    return self._mock_ps_output


class PosixPlatformBackendTest(unittest.TestCase):

  def testGetChildPidsWithGrandChildren(self):
    backend = TestBackend()
    backend.SetMockPsOutput(['1 0 S', '2 1 R', '3 2 S', '4 1 R', '5 4 R'])
    result = backend.GetChildPids(1)
    self.assertEquals(set(result), set([2, 3, 4, 5]))

  def testGetChildPidsWithNoSuchPid(self):
    backend = TestBackend()
    backend.SetMockPsOutput(['1 0 S', '2 1 R', '3 2 S', '4 1 R', '5 4 R'])
    result = backend.GetChildPids(6)
    self.assertEquals(set(result), set())

  def testGetChildPidsWithZombieChildren(self):
    backend = TestBackend()
    backend.SetMockPsOutput(['1 0 S', '2 1 R', '3 2 S', '4 1 R', '5 4 Z'])
    result = backend.GetChildPids(1)
    self.assertEquals(set(result), set([2, 3, 4]))

  def testGetChildPidsWithMissingState(self):
    backend = TestBackend()
    backend.SetMockPsOutput(['  1 0 S  ', '  2 1', '3 2 '])
    result = backend.GetChildPids(1)
    self.assertEquals(set(result), set([2, 3]))

  @test.Enabled('linux', 'mac')
  def testIsApplicationRunning(self):
    backend = factory.GetPlatformBackendForCurrentOS()

    self.assertFalse(backend.IsApplicationRunning('This_Is_A_Bad___App__Name'))
    sys_exe = os.path.basename(sys.executable)
    self.assertTrue(backend.IsApplicationRunning(sys_exe))
    self.assertFalse(
        backend.IsApplicationRunning('%s append_bad_after_space' % sys_exe))
