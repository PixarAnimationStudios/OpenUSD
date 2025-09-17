#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
