#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import logging
import os
from pxr import Tf
import unittest
import platform

class TestPathUtils(unittest.TestCase):

    def setUp(self):
        self.log = logging.getLogger()

    def test_TfRealPath(self):
        if not os.path.isdir('subdir/e'):
            os.makedirs('subdir/e')
        self.log.info('no symlinks')
        self.assertEqual(os.path.abspath('subdir'), Tf.RealPath('subdir', True))

        if hasattr(os, 'symlink'):
            try:
                if not os.path.islink('b'):
                    os.symlink('subdir', 'b')
                if not os.path.islink('c'):
                    os.symlink('b', 'c')
                if not os.path.islink('d'):
                    os.symlink('c', 'd')
                if not os.path.islink('e'):
                    os.symlink('missing', 'e')
                if not os.path.islink('f'):
                    os.symlink('e', 'f')
                if not os.path.islink('g'):
                    os.symlink('f', 'g')

                # XXX:
                # On Windows, TfRealPath explicitly lower-cases drive letters
                # if the given path contains symlinks, otherwise it returns
                # the same as os.path.abspath. This is inconsistent but
                # fixing it is a bit riskier. So for now, we just test the
                # the current behavior.
                def _TestSymlink(expected, got):
                    if platform.system() == 'Windows':
                        drive, tail = os.path.splitdrive(expected)
                        expected = drive.lower() + tail
                    self.assertEqual(expected, got)

                self.log.info('leaf dir is symlink')
                _TestSymlink(os.path.abspath('subdir'),
                             Tf.RealPath('d', True))
                self.log.info('symlinks through to dir')
                _TestSymlink(os.path.abspath('subdir/e'),
                             Tf.RealPath('d/e', True))
                self.log.info('symlinks through to nonexistent dirs')
                _TestSymlink(os.path.abspath('subdir/e/f/g/h'),
                             Tf.RealPath('d/e/f/g/h', True))
                self.log.info('symlinks through to broken link')
                _TestSymlink('', Tf.RealPath('g', True))

                self.log.info('symlinks through to broken link, '
                              'raiseOnError=True')
                with self.assertRaises(RuntimeError):
                    Tf.RealPath('g', True, raiseOnError=True)

                if platform.system() == 'Windows':
                    try:
                        # Test repro from USD-6557
                        if not os.path.isdir(r'C:/symlink-test'):
                            os.makedirs(r'C:/symlink-test')

                        if not os.path.islink(r'C:/symlink-test-link'):
                            os.symlink(r'C:/symlink-test', 'C:/symlink-test-link')
                            cwd = os.getcwd()
                            try:
                                os.chdir(r'C:/symlink-test-link')
                                _TestSymlink(os.path.abspath('C:/symlink-test'),
                                             Tf.RealPath(r'C:/symlink-test-link'))
                            finally:
                                # Restore cwd before trying to remove the test
                                # dirs, otherwise Windows will disallow it.
                                os.chdir(cwd)
                    finally:
                        if os.path.isdir(r'C:/symlink-test-link'):
                            os.rmdir(r'C:/symlink-test-link')
                        if os.path.isdir(r'C:/symlink-test'):
                            os.rmdir(r'C:/symlink-test')

            except OSError:
                # On windows this is expected if run by a non-administrator
                if platform.system() == 'Windows':
                    pass
                else:
                    raise

if __name__ == '__main__':
    unittest.main()
