#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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
            print a.ExportToString()
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
