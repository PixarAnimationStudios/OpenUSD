#!/pxrpythonsubst

from pxr import UsdUtils, Sdf

from Mentor.Framework.RunTime import AssertEqual, FindDataFile

def main():
    # Test CopyLayerMetadata()
    source = Sdf.Layer.FindOrOpen(FindDataFile('layerWithMetadata.usda'))
    assert source

    keysToCompare = [x for x in source.pseudoRoot.ListInfoKeys() if (x not in ['subLayers', 'subLayerOffsets'])]

    cpy = Sdf.Layer.CreateNew("cpy.usda")
    assert cpy
    UsdUtils.CopyLayerMetadata(source, cpy)
    
    for key in ['subLayers'] + keysToCompare:
        AssertEqual(source.pseudoRoot.GetInfo(key),
                    cpy.pseudoRoot.GetInfo(key))
    # bug #127687 - can't use GetInfo() for subLayerOffsets
    AssertEqual(source.subLayerOffsets, cpy.subLayerOffsets)

    cpyNoSublayers = Sdf.Layer.CreateNew("cpyNoSublayers.usda")
    assert cpyNoSublayers
    UsdUtils.CopyLayerMetadata(source, cpyNoSublayers, skipSublayers=True)
    assert not cpyNoSublayers.pseudoRoot.HasInfo('subLayers')
    assert not cpyNoSublayers.pseudoRoot.HasInfo('subLayerOffsets')
    for key in keysToCompare:
        AssertEqual(source.pseudoRoot.GetInfo(key),
                    cpyNoSublayers.pseudoRoot.GetInfo(key))

if __name__=="__main__":
    main()
