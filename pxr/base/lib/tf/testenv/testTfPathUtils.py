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

            self.log.info('leaf dir is symlink')
            self.assertEqual(os.path.abspath('subdir'), Tf.RealPath('d', True))
            self.log.info('symlinks through to dir')
            self.assertEqual(os.path.abspath('subdir/e'), Tf.RealPath('d/e', True))
            self.log.info('symlinks through to nonexistent dirs')
            self.assertEqual(os.path.abspath('subdir/e/f/g/h'),
                Tf.RealPath('d/e/f/g/h', True))
            self.log.info('symlinks through to broken link')
            self.assertEqual('', Tf.RealPath('g', True))

            self.log.info('symlinks through to broken link, raiseOnError=True')
            with self.assertRaises(RuntimeError):
                Tf.RealPath('g', True, raiseOnError=True)


if __name__ == '__main__':
    unittest.main()

