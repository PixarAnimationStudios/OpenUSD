#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, UsdAbc
import unittest, os

class TestUsdAbcIndexedProperties(unittest.TestCase):
    def test_IndexedTextureCoordinates(self):
        name = "indexedTextureCoordinates"
        layer = Sdf.Layer.FindOrOpen('%s.abc' % (name, ))
        self.assertTrue(layer)
        layer.Export('%s.%s.usda' % (name, os.environ['USD_ABC_TESTSUFFIX']))

if __name__ == "__main__":
    unittest.main()
