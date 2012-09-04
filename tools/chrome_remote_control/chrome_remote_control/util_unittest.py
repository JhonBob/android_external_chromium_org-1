# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import unittest

import util

class TestWait(unittest.TestCase):
  def testNonTimeout(self):
    def test():
      return True
    util.WaitFor(test, 0.1)

  def testTimeout(self):
    def test():
      return False
    self.assertRaises(util.TimeoutException, lambda: util.WaitFor(test, 0.1))
