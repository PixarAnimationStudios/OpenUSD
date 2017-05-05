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

import sys, unittest
from pxr import Sdf,Usd,Tf

allFormats = ['usd' + c for c in 'ac']

class TestUsdStage(unittest.TestCase):
    def test_UsedLayers(self):
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
            fooSet.AppendVariant('default')
            fooSet.AppendVariant('varied')
            fooSet.SetVariantSelection('varied')
            with fooSet.GetVariantEditContext():
                refs = p.GetReferences()
                refs.AppendReference(lVar.identifier)
            usedLayers = sMain.GetUsedLayers()
            assert len(usedLayers) == 4
            assert lVar in usedLayers

            fooSet.SetVariantSelection('default')    
            usedLayers = sMain.GetUsedLayers()
            assert len(usedLayers) == 3
            assert not (lVar in usedLayers)

    def test_MutedLocalLayers(self):
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
            with self.assertRaises(Tf.ErrorException):
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

    def test_MutedReferenceLayers(self):
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

    def test_UsdStageIsSupportedFile(self):
        validFileNames = ['foo.usda', '/baz/bar/foo.usd', 'foo.usd', 'xxx.usdc']
        invalidFileNames = ['hello.alembic', 'hello.usdx', 'ill.never.work']
        assert all([Usd.Stage.IsSupportedFile(fl) for fl in validFileNames]) 
        assert not all([Usd.Stage.IsSupportedFile(fl) for fl in invalidFileNames])

    def test_testUsdStageColorConfiguration(self):
        for fmt in allFormats:
            f = lambda base: base + '.' + fmt
            rootLayer = Sdf.Layer.CreateNew(f("colorConf"), f("colorConf"))
            stage = Usd.Stage.Open(rootLayer)
            
            colorConfigFallbacks = Usd.Stage.GetColorConfigFallbacks()
            assert len(colorConfigFallbacks) == 2
            fallbackColorConfiguration = colorConfigFallbacks[0]
            fallbackColorManagementSystem = colorConfigFallbacks[1]

            self.assertEqual(stage.GetColorConfiguration(), 
                             fallbackColorConfiguration)
            self.assertEqual(stage.GetColorManagementSystem(), 
                             fallbackColorManagementSystem)

            colorConfig = Sdf.AssetPath("https://github.com/imageworks/OpenColorIO-Configs/blob/master/aces_1.0.3/config.ocio")
            stage.SetColorConfiguration(colorConfig)
            self.assertEqual(stage.GetColorConfiguration(), colorConfig)
            
            # Need to drop down to sdf API to clear color configuration values.
            stage.GetRootLayer().ClearColorConfiguration()
            self.assertEqual(stage.GetColorConfiguration(), 
                             fallbackColorConfiguration)

            stage.GetRootLayer().ClearColorManagementSystem()
            self.assertEqual(stage.GetColorManagementSystem(), 
                             fallbackColorManagementSystem)

            # Change the fallback values via API.
            newColorConfig = Sdf.AssetPath("https://github.com/imageworks/OpenColorIO- Configs/blob/master/aces_1.0.5/config.ocio")
            Usd.Stage.SetColorConfigFallbacks(colorConfiguration=newColorConfig,
                colorManagementSystem="")
            self.assertEqual(stage.GetColorConfiguration(), newColorConfig)
            # Ensure that cms is unchanged although an empty string was passed 
            # in for it.
            self.assertEqual(stage.GetColorManagementSystem(), 
                             fallbackColorManagementSystem)
            
            # Test colorSpace metadata.
            # Note: this is here an not in a testUsdAttribute* because this 
            # API on UsdAttribute pertains to encoding of color spaces, which 
            # is closely related to the color configuration metadata on the 
            # stage.
            prim = stage.DefinePrim("/Prim")
            colorAttr = prim.CreateAttribute('displayColor', 
                                             Sdf.ValueTypeNames.Color3f)
            self.assertFalse(colorAttr.HasColorSpace())
            colorAttr.SetColorSpace('lin_srgb')
            self.assertTrue(colorAttr.HasColorSpace())
            self.assertEqual(colorAttr.GetColorSpace(), 'lin_srgb')
            self.assertTrue(colorAttr.ClearColorSpace())
            self.assertFalse(colorAttr.HasColorSpace())
            
    def test_UsdStageTimeMetadata(self):
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
                with self.assertRaises(Tf.ErrorException):
                    stage.SetStartTimeCode(90.0)
                with self.assertRaises(Tf.ErrorException):
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

    def test_BadGetPrimAtPath(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('testBadGetPrimAtPath.'+fmt)
            s.DefinePrim('/Foo')
            
            # Should get an invalid prim when using a relative path, even
            # if a root prim with that name exists.
            assert(not s.GetPrimAtPath('Foo'))
            assert(not s.GetPrimAtPath('NonExistentRelativePath'))

            # Should get an invalid prim if passed a non-prim path.
            assert(not s.GetPrimAtPath('/Foo.prop'))

            # Should get an invalid prim if passed an empty path
            assert(not s.GetPrimAtPath(Sdf.Path.emptyPath))

    def test_Save(self):
        for fmt in allFormats:
            def _CreateLayers(rootLayerName):
                f = lambda base: base + '.' + fmt
                rootLayer = Sdf.Layer.CreateNew(f(rootLayerName))
                subLayer = Sdf.Layer.CreateNew(f(rootLayerName + '_sublayer'))
                anonLayer = Sdf.Layer.CreateAnonymous(f(rootLayerName + '_anon'))
                refLayer = Sdf.Layer.CreateNew(f(rootLayerName + '_reflayer'))

                # Author some scene description so all layers start as dirty.
                rootLayer.subLayerPaths.append(subLayer.identifier)
                rootLayer.subLayerPaths.append(anonLayer.identifier)

                primPath = "/" + rootLayerName
                subLayerPrim = Sdf.CreatePrimInLayer(subLayer, primPath)
                subLayerPrim.referenceList.Add(
                    Sdf.Reference(refLayer.identifier, primPath))
                Sdf.CreatePrimInLayer(refLayer, primPath)

                Sdf.CreatePrimInLayer(anonLayer, primPath)

                return rootLayer, subLayer, anonLayer, refLayer

            # After calling Usd.Stage.Save(), all layers should be saved except
            # local session layers. The layer referenced from the session
            # layer is also saved, which is as intended.
            (rootLayer, rootSubLayer, rootAnonLayer, rootRefLayer) = \
                _CreateLayers('root')
            (sessionLayer, sessionSubLayer, sessionAnonLayer, sessionRefLayer) = \
                _CreateLayers('session')

            stage = Usd.Stage.Open(rootLayer, sessionLayer)
            assert all([l.dirty for l in 
                        [rootLayer, rootSubLayer, rootAnonLayer, rootRefLayer,
                         sessionLayer, sessionSubLayer, sessionAnonLayer,
                         sessionRefLayer]])
            stage.Save()
            assert not any([l.dirty for l in 
                            [rootLayer, rootSubLayer, rootRefLayer, 
                             sessionRefLayer]])
            assert all([l.dirty for l in [rootAnonLayer, sessionLayer, 
                                          sessionSubLayer, sessionAnonLayer]])

            # After calling Usd.Stage.SaveSessionLayers(), only the local session
            # layers should be saved. 
            (rootLayer, rootSubLayer, rootAnonLayer, rootRefLayer) = \
                _CreateLayers('root_2')
            (sessionLayer, sessionSubLayer, sessionAnonLayer, sessionRefLayer) = \
                _CreateLayers('session_2')

            stage = Usd.Stage.Open(rootLayer, sessionLayer)
            assert all([l.dirty for l in 
                        [rootLayer, rootSubLayer, rootAnonLayer, rootRefLayer,
                         sessionLayer, sessionSubLayer, sessionAnonLayer,
                         sessionRefLayer]])
            stage.SaveSessionLayers()
            assert all([l.dirty for l in 
                        [rootLayer, rootSubLayer, rootAnonLayer, rootRefLayer,
                         sessionAnonLayer, sessionRefLayer]])
            assert not any([l.dirty for l in [sessionLayer, sessionSubLayer]])

if __name__ == "__main__":
    unittest.main()
