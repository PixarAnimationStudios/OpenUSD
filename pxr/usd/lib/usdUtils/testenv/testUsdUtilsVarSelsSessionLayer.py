#!/pxrpythonsubst

from pxr import UsdUtils, Sdf

from Mentor.Framework.RunTime import AssertEqual, AssertNotEqual

def main():
    # Verify contents of session layer for the requested variant selections.
    lyr1 = UsdUtils.StageCache.GetSessionLayerForVariantSelections(
        'model', [('modelingVariant', 'RockA'), ('shadingVariant', 'Mossy'),
                  ('lodVariant', 'Low')])
    AssertEqual(lyr1.GetPrimAtPath('/model').variantSelections['modelingVariant'], 'RockA')
    AssertEqual(lyr1.GetPrimAtPath('/model').variantSelections['shadingVariant'], 'Mossy')
    AssertEqual(lyr1.GetPrimAtPath('/model').variantSelections['lodVariant'], 'Low')

    # Specifying variants in a different order should still give a cache hit.
    lyr2 = UsdUtils.StageCache.GetSessionLayerForVariantSelections(
        'model', [('shadingVariant', 'Mossy'),
                  ('lodVariant', 'Low'), ('modelingVariant', 'RockA')])
    AssertEqual(lyr1, lyr2)

    # Different variant selection should produce a new layer.
    lyr3 = UsdUtils.StageCache.GetSessionLayerForVariantSelections(
        'model', [('shadingVariant', 'Mossy'),
                  ('lodVariant', 'Low'), ('modelingVariant', 'RockB')])
    AssertNotEqual(lyr2, lyr3)

if __name__=="__main__":
    main()
