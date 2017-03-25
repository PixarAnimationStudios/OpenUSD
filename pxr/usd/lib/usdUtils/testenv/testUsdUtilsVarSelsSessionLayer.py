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
from pxr import UsdUtils, Sdf
import unittest

class TestUsdUtilsVarSelsSessionLayer(unittest.TestCase):
    def test_Basic(self):
        # Verify contents of session layer for the requested variant selections.
        lyr1 = UsdUtils.StageCache.GetSessionLayerForVariantSelections(
            'model', [('modelingVariant', 'RockA'), ('shadingVariant', 'Mossy'),
                      ('lodVariant', 'Low')])
        self.assertEqual(lyr1.GetPrimAtPath('/model').variantSelections['modelingVariant'], 'RockA')
        self.assertEqual(lyr1.GetPrimAtPath('/model').variantSelections['shadingVariant'], 'Mossy')
        self.assertEqual(lyr1.GetPrimAtPath('/model').variantSelections['lodVariant'], 'Low')

        # Specifying variants in a different order should still give a cache hit.
        lyr2 = UsdUtils.StageCache.GetSessionLayerForVariantSelections(
            'model', [('shadingVariant', 'Mossy'),
                      ('lodVariant', 'Low'), ('modelingVariant', 'RockA')])
        self.assertEqual(lyr1, lyr2)

        # Different variant selection should produce a new layer.
        lyr3 = UsdUtils.StageCache.GetSessionLayerForVariantSelections(
            'model', [('shadingVariant', 'Mossy'),
                      ('lodVariant', 'Low'), ('modelingVariant', 'RockB')])
        self.assertNotEqual(lyr2, lyr3)

if __name__=="__main__":
    unittest.main()
