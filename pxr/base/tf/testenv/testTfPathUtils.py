#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
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
