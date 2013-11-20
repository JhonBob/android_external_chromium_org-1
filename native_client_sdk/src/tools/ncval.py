#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper script for running ncval.
"""

import optparse
import os
import subprocess
import sys

import getos

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_SDK_ROOT = os.path.dirname(SCRIPT_DIR)

if sys.version_info < (2, 6, 0):
  sys.stderr.write("python 2.6 or later is required run this script\n")
  sys.exit(1)

class Error(Exception):
  pass

def Log(msg):
  if Log.verbose:
    sys.stderr.write(str(msg) + '\n')
Log.verbose = False

def main(argv):
  usage = 'Usage: %prog [options] <.nexe | .so>'
  epilog = 'Example: ncval.py my_nexe.nexe'
  parser = optparse.OptionParser(usage, description=__doc__, epilog=epilog)
  parser.add_option('-v', '--verbose', action='store_true',
                    help='Verbose output')

  # To enable bash completion for this command first install optcomplete
  # and then add this line to your .bashrc:
  #  complete -F _optcomplete ncval.py
  try:
    import optcomplete
    optcomplete.autocomplete(parser)
  except ImportError:
    pass

  options, args = parser.parse_args(argv)
  if not args:
    parser.error('No executable file specified')

  nexe = args[0]
  if options.verbose:
    Log.verbose = True

  # TODO(binji): http://crbug.com/321791. Fix ncval upstream to reduce the
  # amount of additional checking done here.
  osname = getos.GetPlatform()
  if not os.path.exists(nexe):
    raise Error('executable not found: %s' % nexe)
  if not os.path.isfile(nexe):
    raise Error('not a file: %s' % nexe)

  ncval = os.path.join(SCRIPT_DIR, 'ncval')
  if osname == 'win':
    ncval += '.exe'

  cmd = [ncval, nexe]
  Log('Running %s' % ' '.join(cmd))
  proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  proc_out, _ = proc.communicate()
  if proc.returncode:
    # ncval doesn't print anything to stderr.
    sys.stderr.write('Validating %s failed:\n' % nexe)
    sys.stderr.write(proc_out + '\n')

    Log('Changing the modification time of %s to 0.' % nexe)
    # "untouch" the executable; that is, change the modification time to be so
    # old that it will be remade when `make` runs.
    statinfo = os.stat(nexe)
    mtime = 0
    os.utime(nexe, (statinfo.st_atime, mtime))

    return proc.returncode
  elif options.verbose:
    # By default, don't print anything on success.
    Log(proc_out)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except Error as e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)
