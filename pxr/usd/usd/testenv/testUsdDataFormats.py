#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import unittest
from pxr import Sdf

allFormats = ['usda', 'usdc']

class TestUsdDataFormats(unittest.TestCase):
    def test_PseudoRoot(self):
        """Verify that pseudoroots report that they does not have specifiers with all file formats"""
        for fmt in allFormats:
            layer = Sdf.Layer.CreateAnonymous('pseudoroot.' + fmt)
            assert layer, 'Could not create ' + fmt + ' layer'
            assert not layer.pseudoRoot.HasInfo('specifier'), \
                fmt + ' layer reports unexpected pseudoroot specifier'
            prim = Sdf.PrimSpec(layer, 'root', Sdf.SpecifierDef)
            assert prim
            assert prim.HasInfo('specifier')
            assert 'specifier' in prim.ListInfoKeys()
            self.assertEqual(prim.GetInfo('specifier'), Sdf.SpecifierDef)
            prim.ClearInfo('specifier')
            self.assertEqual(prim.GetInfo('specifier'), Sdf.SpecifierOver)
            assert prim.HasInfo('specifier')
            assert 'specifier' in prim.ListInfoKeys()

    def test_Sublayers(self):
        """Regression test to ensure that all formats can read and write 
        sublayer paths.
        """
        for fmt in allFormats:
            a = Sdf.Layer.CreateAnonymous('test.' + fmt)
            layerName = "hello.usd"
            a.subLayerPaths.append(layerName)
            print(a.ExportToString())
            self.assertEqual(1, len(a.subLayerPaths))
            self.assertEqual(layerName, a.subLayerPaths[0])

    def test_AssetPath(self):
        """ Regression test to ensure that VtDictionaries holding SdfAssetPath
            can be represented in all formats.
        """
        from pxr import Usd
        for fmt in allFormats:
            s = Usd.Stage.CreateNew('test.' + fmt)
            p = s.DefinePrim("/X", 'Scope')
            p.SetCustomDataByKey('identifier', Sdf.AssetPath('asset.usd'))
            self.assertEqual(p.GetCustomDataByKey('identifier'),
                        Sdf.AssetPath('asset.usd'))

if __name__ == '__main__':
    unittest.main()
