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

        uvTextureNode = r.GetShaderNodeByName('UsdUVTexture', ['OSL'])
        self.assertTrue(uvTextureNode)
        self.assertEqual(
            uvTextureNode.GetShaderInputNames(),
            ['file', 'st', 'wrapS', 'wrapT', 'fallback', 'scale', 'bias'])

        primvarReaderNode = r.GetShaderNodeByName('UsdPrimvarReader_float', ['OSL'])
        self.assertTrue(primvarReaderNode)
        self.assertEqual(
            primvarReaderNode.GetShaderInputNames(),
            ['varname', 'fallback'])

if __name__ == '__main__':
    unittest.main()
