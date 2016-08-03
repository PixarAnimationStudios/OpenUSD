#!/pxrpythonsubst

import sys
from pxr import Sdf,Usd,Tf
from Mentor.Runtime import RequiredException

allFormats = ['usd' + c for c in 'abc']

def TestUsedLayers():
    for fmt in allFormats:
        sMain = Usd.Stage.CreateInMemory('testUsedLayers.'+fmt)
        # includes a session layer...
        assert len(sMain.GetUsedLayers()) == 2

        lMain = sMain.GetRootLayer()
        lSub = Sdf.Layer.CreateAnonymous()
        lMain.subLayerPaths.append(lSub.identifier)
        # Picks up newly added sublayer
        usedLayers = sMain.GetUsedLayers()
        assert len(usedLayers) == 3
        assert lSub in usedLayers

        # Now make a layer that is only referenced in one
        # variant of a variantSet
        sVar = Usd.Stage.CreateInMemory('testUsedLayers-sVar.'+fmt)
        lVar = sVar.GetRootLayer()
        varPrim = sVar.DefinePrim('/varPrim')
        sVar.SetDefaultPrim(varPrim)
        p = sMain.DefinePrim('/World')
        fooSet = p.GetVariantSet('fooSet')
        fooSet.FindOrCreateVariant('default')
        fooSet.FindOrCreateVariant('varied')
        fooSet.SetVariantSelection('varied')
        with fooSet.GetVariantEditContext():
            refs = p.GetReferences()
            refs.Add(lVar.identifier)
        usedLayers = sMain.GetUsedLayers()
        assert len(usedLayers) == 4
        assert lVar in usedLayers

        fooSet.SetVariantSelection('default')    
        usedLayers = sMain.GetUsedLayers()
        assert len(usedLayers) == 3
        assert not (lVar in usedLayers)

def TestMutedLocalLayers():
    for fmt in allFormats:
        sublayer_1 = Sdf.Layer.CreateNew('localLayers_sublayer_1.'+fmt)
        primSpec_1 = Sdf.CreatePrimInLayer(sublayer_1, '/A')
        attrSpec_1 = Sdf.AttributeSpec(primSpec_1, 'attr', 
                                       Sdf.ValueTypeNames.String,
                                       declaresCustom = True)
        attrSpec_1.default = 'from_sublayer_1'

        sublayer_2 = Sdf.Layer.CreateNew('localLayers_sublayer_2.'+fmt)
        primSpec_2 = Sdf.CreatePrimInLayer(sublayer_2, '/A')
        attrSpec_2 = Sdf.AttributeSpec(primSpec_2, 'attr', 
                                       Sdf.ValueTypeNames.String,
                                       declaresCustom = True)
        attrSpec_2.default = 'from_sublayer_2'
        
        sessionLayer = Sdf.Layer.CreateNew('localLayers_session.'+fmt)
        primSpec_session = Sdf.CreatePrimInLayer(sessionLayer, '/A')
        attrSpec_session = Sdf.AttributeSpec(primSpec_session, 'attr', 
                                             Sdf.ValueTypeNames.String,
                                             declaresCustom = True)
        attrSpec_session.default = 'from_session'

        rootLayer = Sdf.Layer.CreateNew('localLayers_root.'+fmt)
        rootLayer.subLayerPaths = [sublayer_1.identifier, sublayer_2.identifier]

        stage = Usd.Stage.Open(rootLayer, sessionLayer)
        prim = stage.GetPrimAtPath('/A')
        attr = prim.GetAttribute('attr')
        assert attr

        # Muting the stage's root layer is disallowed and results
        # in a coding error.
        with RequiredException(Tf.ErrorException):
            stage.MuteLayer(rootLayer.identifier)

        assert attr.Get() == 'from_session'
        assert (set(stage.GetUsedLayers()) ==
                set([sublayer_1, sublayer_2, sessionLayer, rootLayer]))
        assert set(stage.GetMutedLayers()) == set([])
        assert not stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(sublayer_2.identifier)
        assert not stage.IsLayerMuted(sessionLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.MuteLayer(sessionLayer.identifier)
        assert attr.Get() == 'from_sublayer_1'
        assert (set(stage.GetUsedLayers()) ==
                set([sublayer_1, sublayer_2, rootLayer]))
        assert set(stage.GetMutedLayers()) == set([sessionLayer.identifier])
        assert not stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(sublayer_2.identifier)
        assert stage.IsLayerMuted(sessionLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.MuteLayer(sublayer_1.identifier)
        assert attr.Get() == 'from_sublayer_2'
        assert set(stage.GetUsedLayers()) == set([sublayer_2, rootLayer])
        assert (set(stage.GetMutedLayers()) ==
                set([sessionLayer.identifier, sublayer_1.identifier]))
        assert stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(sublayer_2.identifier)
        assert stage.IsLayerMuted(sessionLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)
        
        stage.UnmuteLayer(sessionLayer.identifier)
        assert attr.Get() == 'from_session'
        assert (set(stage.GetUsedLayers()) == 
                set([sublayer_2, sessionLayer, rootLayer]))
        assert set(stage.GetMutedLayers()) == set([sublayer_1.identifier])
        assert stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(sublayer_2.identifier)
        assert not stage.IsLayerMuted(sessionLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.MuteAndUnmuteLayers([sessionLayer.identifier, 
                                   sublayer_2.identifier],
                                  [sublayer_1.identifier])
        assert attr.Get() == 'from_sublayer_1'
        assert set(stage.GetUsedLayers()) == set([sublayer_1, rootLayer])
        assert (set(stage.GetMutedLayers()) ==
                set([sublayer_2.identifier, sessionLayer.identifier]))
        assert not stage.IsLayerMuted(sublayer_1.identifier)
        assert stage.IsLayerMuted(sublayer_2.identifier)
        assert stage.IsLayerMuted(sessionLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

def TestMutedReferenceLayers():
    for fmt in allFormats:
        sublayer_1 = Sdf.Layer.CreateNew('refLayers_sublayer_1.'+fmt)
        primSpec_1 = Sdf.CreatePrimInLayer(sublayer_1, '/A')
        attrSpec_1 = Sdf.AttributeSpec(primSpec_1, 'attr', 
                                       Sdf.ValueTypeNames.String,
                                       declaresCustom = True)
        attrSpec_1.default = 'from_sublayer_1'

        refLayer = Sdf.Layer.CreateNew('refLayers_ref.'+fmt)
        primSpec_ref = Sdf.CreatePrimInLayer(refLayer, '/A')
        refLayer.subLayerPaths = [sublayer_1.identifier]

        rootLayer = Sdf.Layer.CreateNew('refLayers_root.'+fmt)
        primSpec_root = Sdf.CreatePrimInLayer(rootLayer, '/A')
        primSpec_root.referenceList.Add(
            Sdf.Reference(refLayer.identifier, '/A'))

        stage = Usd.Stage.Open(rootLayer, sessionLayer=None)
        prim = stage.GetPrimAtPath('/A')
        attr = prim.GetAttribute('attr')
        assert attr

        assert attr.Get() == 'from_sublayer_1'
        assert (set(stage.GetUsedLayers()) == 
                set([sublayer_1, refLayer, rootLayer]))
        assert set(stage.GetMutedLayers()) == set([])
        assert not stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(refLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.MuteLayer(sublayer_1.identifier)
        assert attr.Get() == None
        assert set(stage.GetUsedLayers()) == set([refLayer, rootLayer])
        assert set(stage.GetMutedLayers()) == set([sublayer_1.identifier])
        assert stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(refLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.UnmuteLayer(sublayer_1.identifier)
        assert attr.Get() == 'from_sublayer_1'
        assert (set(stage.GetUsedLayers()) == 
                set([sublayer_1, refLayer, rootLayer]))
        assert set(stage.GetMutedLayers()) == set([])
        assert not stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(refLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.MuteLayer(refLayer.identifier)
        assert not attr
        assert set(stage.GetUsedLayers()) == set([rootLayer])
        assert set(stage.GetMutedLayers()) == set([refLayer.identifier])
        assert not stage.IsLayerMuted(sublayer_1.identifier)
        assert stage.IsLayerMuted(refLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

        stage.MuteAndUnmuteLayers([sublayer_1.identifier],
                                  [refLayer.identifier])
        assert attr.Get() == None
        assert set(stage.GetUsedLayers()) == set([refLayer, rootLayer])
        assert set(stage.GetMutedLayers()) == set([sublayer_1.identifier])
        assert stage.IsLayerMuted(sublayer_1.identifier)
        assert not stage.IsLayerMuted(refLayer.identifier)
        assert not stage.IsLayerMuted(rootLayer.identifier)

def TestUsdStageIsSupportedFile():
    validFileNames = ['foo.usda', '/baz/bar/foo.usd', 'foo.usd', 'xxx.usdc']
    invalidFileNames = ['hello.alembic', 'hello.usdx', 'ill.never.work']
    assert all([Usd.Stage.IsSupportedFile(fl) for fl in validFileNames]) 
    assert not all([Usd.Stage.IsSupportedFile(fl) for fl in invalidFileNames])

def TestUsdStageTimeMetadata():
    for fmt in allFormats:
        f = lambda base: base + '.' + fmt

        sessionLayer = Sdf.Layer.CreateNew(f('sessionLayer'), f('sessionLayer'))
        rootLayer = Sdf.Layer.CreateNew(f("rootLayer"), f("rootLayer"))
        subLayer = Sdf.Layer.CreateNew(f("subLayer"), f("subLayer"))

        rootLayer.subLayerPaths = [f("./subLayer")]
        subLayer.Save()

        stage = Usd.Stage.Open(rootLayer, sessionLayer)
        assert (not stage.HasAuthoredTimeCodeRange())

        # Test (startFrame,endFrame) in rootLayer XXX: bug 123508 Once
        # StartFrame,EndFrame are migrated to Sd, the test cases involving them
        # should be removed.
        stage.SetMetadata("startFrame", 10.0)
        stage.SetMetadata("endFrame", 20.0)
        assert(stage.GetStartTimeCode() == 10.0)
        assert(stage.GetEndTimeCode() == 20.0)
        assert (stage.HasAuthoredTimeCodeRange())

        # Test (startFrame,endFrame) in sessionLayer
        with Usd.EditContext(stage, sessionLayer):
            stage.SetMetadata('startFrame', 30.0)
            stage.SetMetadata('endFrame', 40.0)
        assert(stage.GetStartTimeCode() == 30.0)
        assert(stage.GetEndTimeCode() == 40.0)
        assert (stage.HasAuthoredTimeCodeRange())

        # Test (startTimeCode,endTimeCode) in rootLayer with (startFrame,
        # endFrame) in session layer This should author to the root layer.
        stage.SetStartTimeCode(50.0)
        stage.SetEndTimeCode(60.0)
        assert(rootLayer.startTimeCode == 50.0)
        assert(rootLayer.endTimeCode == 60.0)

        # (startFrame, endFrame) in the session layer is stronger than 
        # (startTimeCode, endTimeCode) in the rootLayer.
        assert(stage.GetStartTimeCode() == 30.0)
        assert(stage.GetEndTimeCode() == 40.0)
        assert (stage.HasAuthoredTimeCodeRange())

        # Clear the (startFrame, endFrame) opinions in the session layer and
        # verify that (startTimeCode, endTimeCode) in root layer win over the
        # (startFrame, endFrame) in the root layer.
        with Usd.EditContext(stage, sessionLayer):
            stage.ClearMetadata('startFrame')
            stage.ClearMetadata('endFrame')

        assert(stage.GetStartTimeCode() == 50.0)
        assert(stage.GetEndTimeCode() == 60.0)
        assert (stage.HasAuthoredTimeCodeRange())

        # Test (startTimeCode,endTimeCode) in sessionLayer with
        # (startFrame,endFrame) in 
        # both root and session layers.
        with Usd.EditContext(stage, sessionLayer):
            stage.SetStartTimeCode(70.0)
            stage.SetEndTimeCode(80.0)

        assert(sessionLayer.startTimeCode == 70.0)
        assert(sessionLayer.endTimeCode == 80.0)

        assert(stage.GetStartTimeCode() == 70.0)
        assert(stage.GetEndTimeCode() == 80.0)
        assert (stage.HasAuthoredTimeCodeRange())

        # Test that setting start/endTimeCode in a layer that's not the root
        # layer or the session layer will result in a warning and no authoring
        # is performed.
        with Usd.EditContext(stage, subLayer):
            with RequiredException(Tf.ErrorException):
                stage.SetStartTimeCode(90.0)
            with RequiredException(Tf.ErrorException):
                stage.SetEndTimeCode(100.0)
        assert(stage.GetStartTimeCode() == 70.0)
        assert(stage.GetEndTimeCode() == 80.0)
        assert (stage.HasAuthoredTimeCodeRange())

        # Now ensure that we have fallbacks for the 'perSecond' metadata, using
        # both generic and specific API's.  SdfSchema is not wrapped to python,
        # so we use the psuedoRoot-spec to get fallbacks
        schemaSpec = rootLayer.pseudoRoot
        fallbackFps = schemaSpec.GetFallbackForInfo("framesPerSecond")
        fallbackTps = schemaSpec.GetFallbackForInfo("timeCodesPerSecond")
        assert(stage.GetFramesPerSecond() == fallbackFps)
        assert(stage.HasMetadata("framesPerSecond") and 
               not stage.HasAuthoredMetadata("framesPerSecond"))
        with Usd.EditContext(stage, sessionLayer):
            stage.SetFramesPerSecond(48.0)
        assert(stage.GetMetadata("framesPerSecond") == 48.0)
        assert(stage.HasAuthoredMetadata("framesPerSecond"))
        assert(schemaSpec.GetInfo("framesPerSecond") == fallbackFps)

        assert(stage.GetTimeCodesPerSecond() == fallbackTps)
        assert(stage.HasMetadata("timeCodesPerSecond") and 
               not stage.HasAuthoredMetadata("timeCodesPerSecond"))
        with Usd.EditContext(stage, sessionLayer):
            stage.SetTimeCodesPerSecond(48.0)
        assert(stage.GetMetadata("timeCodesPerSecond") == 48.0)
        assert(stage.HasAuthoredMetadata("timeCodesPerSecond"))
        assert(schemaSpec.GetInfo("timeCodesPerSecond") == fallbackTps)

def Main(argv):
    TestUsedLayers()
    TestMutedLocalLayers()
    TestMutedReferenceLayers()
    TestUsdStageIsSupportedFile()
    TestUsdStageTimeMetadata()

if __name__ == "__main__":
    Main(sys.argv)

