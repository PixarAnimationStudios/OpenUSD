#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Sdr

class TestRmanOslParser(unittest.TestCase):
    def test_Basic(self):
        r = Sdr.Registry()

        uvTextureNode = r.GetNodeByName('UsdUVTexture', ['OSL'])
        self.assertTrue(uvTextureNode)
        self.assertEqual(
            uvTextureNode.GetInputNames(),
            ['file', 'st', 'wrapS', 'wrapT', 'fallback', 'scale', 'bias'])

        primvarReaderNode = r.GetNodeByName('UsdPrimvarReader_float', ['OSL'])
        self.assertTrue(primvarReaderNode)
        self.assertEqual(
            primvarReaderNode.GetInputNames(),
            ['varname', 'fallback'])

if __name__ == '__main__':
    unittest.main()
