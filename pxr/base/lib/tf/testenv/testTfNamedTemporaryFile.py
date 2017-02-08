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

"""
Tests for Tf's NamedTemporaryFile wrapper
"""

import unittest, os
from pxr import Tf

class TestNamedTemporaryFile(unittest.TestCase):
    def setUp(self):
        self.prefixes = ['a', 'b']
        self.suffixes = ['foobar.txt', 'baz.tar']
        self.aArgs = {'prefix':self.prefixes[0], 'suffix':self.suffixes[0]}
        self.bArgs = {'prefix':self.prefixes[1], 'suffix':self.suffixes[1]}

    def _testArgCapture(self, fs):
        print 'Ensuring we can thread all arguments forward'

        base = os.path.basename
        argError = '%s argument (%s) not captured in file name: %s.'
        
        for i, f in enumerate(fs):
            n = f.name
            p = self.prefixes[i]
            s = self.suffixes[i]
            assert base(n).startswith(p), argError % ('prefix', p, n)
            assert base(n).endswith(s), argError % ('suffix', s, n)

    def _runTests(self, fs):
        self._testArgCapture(fs)

    def test_ContextUsage(self):
        print 'Running context manager test'
    
        with Tf.NamedTemporaryFile(**self.aArgs) as a, \
             Tf.NamedTemporaryFile(**self.bArgs) as b:
       
            self._runTests([a, b])

if __name__ == "__main__":
    unittest.main()
