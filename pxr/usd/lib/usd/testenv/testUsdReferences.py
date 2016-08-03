#!/pxrpythonsubst

import sys
from pxr import Sdf,Usd,Tf
from Mentor.Runtime import (AssertEqual, AssertNotEqual, FindDataFile,
                            ExpectedErrors, ExpectedWarnings, RequiredException)

allFormats = ['usd' + x for x in 'abc']

def API():
    for fmt in allFormats:
        s1 = Usd.Stage.CreateInMemory('API-s1.'+fmt)
        s2 = Usd.Stage.CreateInMemory('API-s2.'+fmt)
        srcPrim = s1.OverridePrim('/src')
        trgPrimInternal = s1.OverridePrim('/trg_internal')
        srcPrimSpec = s1.GetRootLayer().GetPrimAtPath('/src')    
        trgPrim = s2.OverridePrim('/trg')
        s2.GetRootLayer().defaultPrim = 'trg'

        # Identifier only.
        srcPrim.GetReferences().Add(s2.GetRootLayer().identifier)
        AssertEqual(srcPrimSpec.referenceList.addedOrExplicitItems[0],
                    Sdf.Reference(s2.GetRootLayer().identifier))
        srcPrim.GetReferences().Clear()

        # Identifier and layerOffset.
        srcPrim.GetReferences().Add(s2.GetRootLayer().identifier,
                                    Sdf.LayerOffset(1.25, 2.0))
        AssertEqual(srcPrimSpec.referenceList.addedOrExplicitItems[0],
                    Sdf.Reference(s2.GetRootLayer().identifier,
                                  layerOffset=Sdf.LayerOffset(1.25, 2.0)))
        srcPrim.GetReferences().Clear()

        # Identifier and primPath.
        srcPrim.GetReferences().Add(s2.GetRootLayer().identifier, '/trg')
        AssertEqual(srcPrimSpec.referenceList.addedOrExplicitItems[0],
                    Sdf.Reference(s2.GetRootLayer().identifier,
                                  primPath='/trg'))
        srcPrim.GetReferences().Clear()

        # Identifier and primPath and layerOffset.
        srcPrim.GetReferences().Add(s2.GetRootLayer().identifier, '/trg',
                                    Sdf.LayerOffset(1.25, 2.0))
        AssertEqual(srcPrimSpec.referenceList.addedOrExplicitItems[0],
                    Sdf.Reference(s2.GetRootLayer().identifier, primPath='/trg',
                                  layerOffset=Sdf.LayerOffset(1.25, 2.0)))
        srcPrim.GetReferences().Clear()

        # primPath only.
        srcPrim.GetReferences().AddInternal('/trg_internal')
        AssertEqual(srcPrimSpec.referenceList.addedOrExplicitItems[0],
                    Sdf.Reference('', primPath='/trg_internal'))
        srcPrim.GetReferences().Clear()

        # primPath and layer offset.
        srcPrim.GetReferences().AddInternal(
            '/trg_internal', layerOffset=Sdf.LayerOffset(1.25, 2.0))
        AssertEqual(srcPrimSpec.referenceList.addedOrExplicitItems[0],
                    Sdf.Reference('', primPath='/trg_internal',
                                  layerOffset=Sdf.LayerOffset(1.25, 2.0)))
        srcPrim.GetReferences().Clear()

def DefaultPrimBasics():
    # create a layer, set DefaultPrim, then reference it.
    for fmt in allFormats:
        targLyr = Sdf.Layer.CreateAnonymous('DefaultPrimBasics.'+fmt)

        def makePrim(name, attrDefault):
            primSpec = Sdf.CreatePrimInLayer(targLyr, name)
            primSpec.specifier = Sdf.SpecifierDef
            attr = Sdf.AttributeSpec(
                primSpec, 'attr', Sdf.ValueTypeNames.Double)
            attr.default = attrDefault

        makePrim('target1', 1.234)
        makePrim('target2', 2.345)

        targLyr.defaultPrim = 'target1'

        # create a new layer and reference the first.
        srcLyr = Sdf.Layer.CreateAnonymous('DefaultPrimBasics-new.'+fmt)
        srcPrimSpec = Sdf.CreatePrimInLayer(srcLyr, '/source')

        # create a stage with srcLyr.
        stage = Usd.Stage.Open(srcLyr)
        prim = stage.GetPrimAtPath('/source')
        assert prim

        # author a prim-path-less reference to the targetLyr.
        prim.GetReferences().Add(targLyr.identifier)

        # should now pick up 'attr' from across the reference.
        assert prim.GetAttribute('attr').Get() == 1.234


def DefaultPrimChangeProcessing():
    for fmt in allFormats:
        # create a layer, set DefaultPrim, then reference it.
        targLyr = Sdf.Layer.CreateAnonymous('DefaultPrimChangeProcessing.'+fmt)

        def makePrim(name, attrDefault):
            primSpec = Sdf.CreatePrimInLayer(targLyr, name)
            primSpec.specifier = Sdf.SpecifierDef
            attr = Sdf.AttributeSpec(
                primSpec, 'attr', Sdf.ValueTypeNames.Double)
            attr.default = attrDefault

        makePrim('target1', 1.234)
        makePrim('target2', 2.345)

        targLyr.defaultPrim = 'target1'

        # create a new layer and reference the first.
        srcLyr = Sdf.Layer.CreateAnonymous(
            'DefaultPrimChangeProcessing-new.'+fmt)
        srcPrimSpec = Sdf.CreatePrimInLayer(srcLyr, '/source')
        srcPrimSpec.referenceList.Add(Sdf.Reference(targLyr.identifier))

        # create a stage with srcLyr.
        stage = Usd.Stage.Open(srcLyr)

        prim = stage.GetPrimAtPath('/source')
        assert prim.GetAttribute('attr').Get() == 1.234

        # Clear defaultPrim.  This should issue a composition
        # error, and fail to pick up the attribute from the referenced prim.
        targLyr.ClearDefaultPrim()
        assert prim
        assert not prim.GetAttribute('attr')

        # Change defaultPrim to other target.  This should pick up the reference
        # again, but to the new prim with the new attribute value.
        targLyr.defaultPrim = 'target2'
        assert prim
        assert prim.GetAttribute('attr').Get() == 2.345


def InternalReferences():
    for fmt in allFormats:
        targLyr = Sdf.Layer.CreateAnonymous('InternalReferences.'+fmt)

        def makePrim(name, attrDefault):
            primSpec = Sdf.CreatePrimInLayer(targLyr, name)
            primSpec.specifier = Sdf.SpecifierDef
            attr = Sdf.AttributeSpec(
                primSpec, 'attr', Sdf.ValueTypeNames.Double)
            attr.default = attrDefault

        makePrim('target1', 1.234)
        makePrim('target2', 2.345)

        targLyr.defaultPrim = 'target1'

        stage = Usd.Stage.Open(targLyr)
        prim = stage.DefinePrim('/ref1')
        prim.GetReferences().AddInternal('/target2')
        assert prim
        assert prim.GetAttribute('attr').Get() == 2.345

        prim.GetReferences().Clear()
        assert prim
        assert not prim.GetAttribute('attr')

        prim.GetReferences().AddInternal('/target1')
        assert prim
        assert prim.GetAttribute('attr').Get() == 1.234


def Main(argv):
    API()
    DefaultPrimBasics()
    DefaultPrimChangeProcessing()
    InternalReferences()

if __name__ == "__main__":
    Main(sys.argv)
    print 'OK'

