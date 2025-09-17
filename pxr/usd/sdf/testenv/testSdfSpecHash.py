#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Sdf

class TestSdfSpecHash(unittest.TestCase):
    def test_Basic(self):
        # Test using a python dictionary to associate values with specs.
        # This relies on the hash function for spec handles.
        test_dict = {}
        test_value = 'foo'

        layer = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.PrimSpec(layer, "foo", Sdf.SpecifierOver)
        test_dict[primSpec] = test_value
        for i in range(10):
            self.assertEqual(test_dict[ layer.GetObjectAtPath(primSpec.path) ], 
                             test_value)

if __name__ == "__main__":
    unittest.main()
