#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

"""
Tests for Tf's NamedTemporaryFile wrapper
"""

from __future__ import print_function

import unittest, os
from pxr import Tf

class TestNamedTemporaryFile(unittest.TestCase):
    def setUp(self):
        self.prefixes = ['a', 'b']
        self.suffixes = ['foobar.txt', 'baz.tar']
        self.aArgs = {'prefix':self.prefixes[0], 'suffix':self.suffixes[0]}
        self.bArgs = {'prefix':self.prefixes[1], 'suffix':self.suffixes[1]}

    def _testArgCapture(self, fs):
        print('Ensuring we can thread all arguments forward')

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
        print('Running context manager test')
    
        with Tf.NamedTemporaryFile(**self.aArgs) as a, \
             Tf.NamedTemporaryFile(**self.bArgs) as b:
       
            self._runTests([a, b])

if __name__ == "__main__":
    unittest.main()
