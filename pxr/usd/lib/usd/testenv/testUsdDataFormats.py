#!/pxrpythonsubst

from pxr import Sdf

from Mentor.Runtime import (AssertEqual)

allFormats = ['usda', 'usdb', 'usdc']

def TestPseudoRoot():
    """Verify that pseudoroots report that they does not have specifiers with all file formats"""
    for fmt in ['usda', 'usdb', 'usdc']:
        layer = Sdf.Layer.CreateAnonymous('pseudoroot.' + fmt)
        assert layer, 'Could not create ' + fmt + ' layer'
        assert not layer.pseudoRoot.HasInfo('specifier'), \
            fmt + ' layer reports unexpected pseudoroot specifier'
        prim = Sdf.PrimSpec(layer, 'root', Sdf.SpecifierDef)
        assert prim
        assert prim.HasInfo('specifier')
        assert 'specifier' in prim.ListInfoKeys()
        AssertEqual(prim.GetInfo('specifier'), Sdf.SpecifierDef)
        prim.ClearInfo('specifier')
        AssertEqual(prim.GetInfo('specifier'), Sdf.SpecifierOver)
        assert prim.HasInfo('specifier')
        assert 'specifier' in prim.ListInfoKeys()

def TestSublayers():
    """Regression test to ensure that all formats can read and write 
    sublayer paths.
    """
    for fmt in allFormats:
        a = Sdf.Layer.CreateAnonymous('test.' + fmt)
        layerName = "hello.usd"
        a.subLayerPaths.append(layerName)
        print a.ExportToString()
        AssertEqual(1, len(a.subLayerPaths))
        AssertEqual(layerName, a.subLayerPaths[0])

def TestAssetPath():
    """ Regression test to ensure that VtDictionaries holding SdfAssetPath
        can be represented in all formats.
    """
    from pxr import Usd
    for fmt in allFormats:
        s = Usd.Stage.CreateNew('test.' + fmt)
        p = s.DefinePrim("/X", 'Scope')
        p.SetCustomDataByKey('identifier', Sdf.AssetPath('asset.usd'))
        AssertEqual(p.GetCustomDataByKey('identifier'),
                    Sdf.AssetPath('asset.usd'))

if __name__ == '__main__':
    TestPseudoRoot()
    TestSublayers()
    TestAssetPath()
