#!/pxrpythonsubst

from pxr import UsdUtils, Sdf

from Mentor.Framework.RunTime import AssertEqual, AssertContentsEqual, FindDataFile

def main():
    lyr1 = Sdf.Layer.FindOrOpen(FindDataFile('BottleMedicalDefaultPrim.usd'))
    assert lyr1
    AssertEqual(UsdUtils.GetModelNameFromRootLayer(lyr1), 'BottleMedical')

    lyr2 = Sdf.Layer.FindOrOpen(FindDataFile('BottleMedicalSameName.usd'))
    assert lyr2
    AssertEqual(UsdUtils.GetModelNameFromRootLayer(lyr2), 'BottleMedicalSameName')

    lyr3 = Sdf.Layer.FindOrOpen(FindDataFile('BottleMedicalRootPrim.usd'))
    assert lyr3
    AssertEqual(UsdUtils.GetModelNameFromRootLayer(lyr3), 'BottleMedical')

    pxVarSetNames = set([regVarSet.name
        for regVarSet in UsdUtils.GetRegisteredVariantSets()
        if regVarSet.selectionExportPolicy == UsdUtils.RegisteredVariantSet.SelectionExportPolicy.Always])
    AssertContentsEqual(pxVarSetNames,
                        set(['modelingVariant', 'lodVariant', 'hairmanVariant', 'shadingVariant']))

if __name__=="__main__":
    main()
