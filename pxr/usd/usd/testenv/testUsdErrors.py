#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Usd, Tf

class TestErrors(unittest.TestCase):
    def test_Errors(self):
        with self.assertRaises(Tf.CppException):
            Usd._UnsafeGetStageForTesting(Usd.Prim())

if __name__ == "__main__":
    unittest.main()
