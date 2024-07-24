#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os, unittest
from pxr import Usd, Tf

class TestUsdReadOutOfBounds(unittest.TestCase):

    def test_ReadOutOfBounds(self):
        # Opening a corrupt file
        with self.assertRaises(Tf.ErrorException):
            stage = Usd.Stage.Open("corrupt.usd")
 
if __name__ == "__main__":
    unittest.main()