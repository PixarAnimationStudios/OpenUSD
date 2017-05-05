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
from pxr import UsdUtils, Sdf, Usd
import unittest

class TestUsdUtilsAuthoring(unittest.TestCase):
    def test_Basic(self):
        # Test CopyLayerMetadata()
        source = Sdf.Layer.FindOrOpen('layerWithMetadata.usda')
        self.assertTrue(source)

        keysToCompare = [x for x in source.pseudoRoot.ListInfoKeys() if 
                         (x not in ['subLayers', 'subLayerOffsets'])]

        cpy = Sdf.Layer.CreateNew("cpy.usda")
        self.assertTrue(cpy)
        UsdUtils.CopyLayerMetadata(source, cpy)
        
        for key in ['subLayers'] + keysToCompare:
            self.assertEqual(source.pseudoRoot.GetInfo(key),
                             cpy.pseudoRoot.GetInfo(key))
        # bug #127687 - can't use GetInfo() for subLayerOffsets
        self.assertEqual(source.subLayerOffsets, cpy.subLayerOffsets)

        cpyNoSublayers = Sdf.Layer.CreateNew("cpyNoSublayers.usda")
        self.assertTrue(cpyNoSublayers)
        UsdUtils.CopyLayerMetadata(source, cpyNoSublayers, skipSublayers=True, 
            bakeUnauthoredFallbacks=True)
        self.assertFalse(cpyNoSublayers.pseudoRoot.HasInfo('subLayers'))
        self.assertFalse(cpyNoSublayers.pseudoRoot.HasInfo('subLayerOffsets'))
        for key in keysToCompare:
            self.assertEqual(source.pseudoRoot.GetInfo(key),
                             cpyNoSublayers.pseudoRoot.GetInfo(key))
        
        # Ensure that the color config fallbacks get stamped out when
        # bakeUnauthoredFallbacks is set to true.
        fallbackKeysToCompare = ['colorConfiguration', 'colorManagementSystem']
        colorConfigFallbacks = Usd.Stage.GetColorConfigFallbacks()
        self.assertEqual(colorConfigFallbacks,
            (cpyNoSublayers.pseudoRoot.GetInfo(Sdf.Layer.ColorConfigurationKey), 
             cpyNoSublayers.pseudoRoot.GetInfo(Sdf.Layer.ColorManagementSystemKey)))

if __name__=="__main__":
    unittest.main()
