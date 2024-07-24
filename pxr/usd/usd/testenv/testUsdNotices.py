#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import contextlib, os, platform, sys, unittest
from pxr import Ar,Sdf,Usd,Tf

allFormats = ['usd' + x for x in 'ac']

class TestUsdNotices(unittest.TestCase):
    def setUp(self):
        self._ResetCounters()

    def _ResetCounters(self):
        self._changeCount = 0
        self._contentsCount = 0
        self._objectsCount = 0
        self._editTargetsCount = 0
        self._layerMutingCount = 0

    def OnStageContentsChanged(self, *args):
        self._changeCount += 1
        self._contentsCount += 1 

    def OnObjectsChanged(self, *args):
        self._changeCount += 1
        self._objectsCount += 1

    def OnStageEditTargetChanged(self, *args):
        self._changeCount += 1
        self._editTargetsCount += 1

    def OnLayerMutingChanged(self, *args):
        self._changeCount += 1
        self._layerMutingCount += 1

    def test_Basics(self):
        contentsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.StageContentsChanged, 
            self.OnStageContentsChanged)
        objectsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, 
            self.OnObjectsChanged)
        stageEditTargetChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.StageEditTargetChanged, 
            self.OnStageEditTargetChanged)
        layerMutingChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.LayerMutingChanged,
            self.OnLayerMutingChanged)

        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('Basics.'+fmt)
            s.DefinePrim("/Foo")
            self.assertEqual(self._changeCount, 2)
            self.assertEqual(self._contentsCount, 1)
            self.assertEqual(self._objectsCount, 1)
            self.assertEqual(self._editTargetsCount, 0)

            self._ResetCounters()
            s.SetEditTarget(s.GetSessionLayer())
            self.assertEqual(self._changeCount, 1)
            self.assertEqual(self._contentsCount, 0)
            self.assertEqual(self._objectsCount, 0)
            self.assertEqual(self._editTargetsCount, 1)

            self._ResetCounters()
            s.GetPrimAtPath("/Foo").SetMetadata("comment", "")
            self.assertEqual(self._changeCount, 4)
            self.assertEqual(self._contentsCount, 2)     # Why 2? I expected 1. 
            self.assertEqual(self._objectsCount, 2)      # We get an additional
                                                    # object resync notice when
                                                    # we first drop an over, in
                                                    # addition to the info-only
                                                    # change notice.
            self.assertEqual(self._editTargetsCount, 0)

            self._ResetCounters()
            s.GetPrimAtPath("/Foo").SetMetadata("comment", "x")
            # Now that the over(s) have been established, setting a value
            # behaves as expected.
            self.assertEqual(self._changeCount, 2)
            self.assertEqual(self._contentsCount, 1)
            self.assertEqual(self._objectsCount, 1)
            self.assertEqual(self._editTargetsCount, 0)

            self._ResetCounters()
            s.MuteAndUnmuteLayers([s.GetSessionLayer().identifier],
                                    [s.GetRootLayer().identifier])
            self.assertEqual(self._changeCount, 3)
            self.assertEqual(self._objectsCount, 1)
            self.assertEqual(self._contentsCount, 1)
            self.assertEqual(self._layerMutingCount, 1)

            # no notice should be called
            self._ResetCounters()
            with self.assertRaises(Tf.ErrorException):
                s.MuteAndUnmuteLayers([s.GetRootLayer().identifier],
                                        [s.GetRootLayer().identifier])
            self.assertEqual(self._changeCount, 0)
            self.assertEqual(self._objectsCount, 0)
            self.assertEqual(self._contentsCount, 0)
            self.assertEqual(self._layerMutingCount, 0)
        # testing for payload specific updates previously, load/unload calls
        # didn't trigger notices
        self._ResetCounters()
        self.assertTrue(self._objectsCount == 0)
        payloadBasedFile = 'payload_base.usda'
        payloadBasedStage = Usd.Stage.Open(payloadBasedFile)
        
        # the payload will be already loaded since we didn't supply
        # the additional parameter to the stage constructor
        payloadBasedStage.Unload('/Foo')
        self.assertEqual(self._objectsCount, 1)
        payloadBasedStage.Load('/Foo')
        self.assertEqual(self._objectsCount, 2)

        del contentsChanged
        del objectsChanged
        del stageEditTargetChanged 
        del layerMutingChanged


    def test_ObjectsChangedNotice(self):
        def OnResync(notice, stage):
            self.assertEqual(notice.GetStage(), stage)
            self.assertEqual(notice.GetResyncedPaths(), [Sdf.Path("/Foo")])
            self.assertEqual(notice.GetResolvedAssetPathsResyncedPaths(), [])
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), [])
            self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(notice.ResyncedObject(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ResolvedAssetPathsResynced(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo")))

        def OnUpdate(notice, stage):
            self.assertEqual(notice.GetStage(), stage)
            self.assertEqual(notice.GetResyncedPaths(), [])
            self.assertEqual(notice.GetResolvedAssetPathsResyncedPaths(), [])
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), [Sdf.Path("/Foo")])
            self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.AffectedObject(stage.GetPrimAtPath("/Foo/Bar")))
            self.assertFalse(notice.ResyncedObject(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ResyncedObject(stage.GetPrimAtPath("/Foo/Bar")))
            self.assertFalse(notice.ResolvedAssetPathsResynced(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ResolvedAssetPathsResynced(stage.GetPrimAtPath("/Foo/Bar")))
            self.assertTrue(notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo/Bar")))

        def OnAssetPathResync(notice, stage):
            self.assertEqual(notice.GetStage(), stage)
            self.assertEqual(notice.GetResyncedPaths(), [])
            self.assertEqual(notice.GetResolvedAssetPathsResyncedPaths(), [Sdf.Path("/")])
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), [Sdf.Path("/")])
            self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo/Bar")))
            self.assertFalse(notice.ResyncedObject(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ResyncedObject(stage.GetPrimAtPath("/Foo/Bar")))
            self.assertTrue(notice.ResolvedAssetPathsResynced(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(notice.ResolvedAssetPathsResynced(stage.GetPrimAtPath("/Foo/Bar")))
            self.assertFalse(notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo")))
            self.assertFalse(notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo/Bar")))

        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('ObjectsChangedNotice.'+fmt)

            objectsChanged = Tf.Notice.Register(Usd.Notice.ObjectsChanged, 
                                                       OnResync, s)
            s.DefinePrim("/Foo")

            del objectsChanged
            s.DefinePrim("/Foo/Bar")

            objectsChanged = Tf.Notice.Register(Usd.Notice.ObjectsChanged, 
                                                OnUpdate, s)
            s.GetPrimAtPath("/Foo").SetMetadata("comment", "")

            objectsChanged = Tf.Notice.Register(Usd.Notice.ObjectsChanged,
                                                OnAssetPathResync, s)
            s.SetMetadata("expressionVariables", {"X":"Y"})

        del objectsChanged

    def test_ObjectsChangedNoticeForAttributes(self):
        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('ObjectsChangedNoticeForProps.'+fmt)
            prim = s.DefinePrim("/Foo")

            def OnAttributeCreation(notice, stage):
                self.assertEqual(notice.GetStage(), stage)
                self.assertEqual(notice.GetResyncedPaths(), [Sdf.Path("/Foo.attr")])
                self.assertEqual(notice.GetChangedInfoOnlyPaths(), [])
                self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo").GetAttribute("attr")))
                self.assertTrue(notice.ResyncedObject(stage.GetPrimAtPath("/Foo").GetAttribute("attr")))
                self.assertTrue(not notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo").GetAttribute("attr")))

            objectsChanged = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, OnAttributeCreation, s)
            attr = prim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)

            def OnAttributeValueChange(notice, stage):
                self.assertEqual(notice.GetStage(), stage)
                self.assertEqual(notice.GetResyncedPaths(), [])
                self.assertTrue(notice.GetChangedInfoOnlyPaths() == \
                    [Sdf.Path("/Foo.attr")])
                self.assertTrue(notice.AffectedObject(
                    stage.GetPrimAtPath("/Foo").GetAttribute("attr")))
                self.assertTrue(not notice.ResyncedObject(
                    stage.GetPrimAtPath("/Foo").GetAttribute("attr")))
                self.assertTrue(notice.ChangedInfoOnly(
                    stage.GetPrimAtPath("/Foo").GetAttribute("attr")))

            objectsChanged = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, OnAttributeValueChange, s)
            attr.Set(42)

            del objectsChanged

    def test_ObjectsChangedNoticeForRelationships(self):
        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('ObjectsChangedNoticeForRels.'+fmt)
            prim = s.DefinePrim("/Foo")

            def OnRelationshipCreation(notice, stage):
                self.assertEqual(notice.GetStage(), stage)
                self.assertEqual(notice.GetResyncedPaths(), [Sdf.Path("/Foo.rel")])
                self.assertEqual(notice.GetChangedInfoOnlyPaths(), [])
                self.assertTrue(notice.AffectedObject(
                    stage.GetPrimAtPath("/Foo").GetRelationship("rel")))
                self.assertTrue(notice.ResyncedObject(
                    stage.GetPrimAtPath("/Foo").GetRelationship("rel")))
                self.assertTrue(not notice.ChangedInfoOnly(
                    stage.GetPrimAtPath("/Foo").GetRelationship("rel")))

            objectsChanged = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, OnRelationshipCreation, s)
            rel = prim.CreateRelationship("rel")

            def OnRelationshipTargetChange(notice, stage):
                self.assertEqual(notice.GetStage(), stage)

                self.assertEqual(notice.GetResyncedPaths(), [])
                self.assertEqual(notice.GetChangedInfoOnlyPaths(),
                                 [Sdf.Path("/Foo.rel")])
                self.assertTrue(notice.AffectedObject(
                    stage.GetPrimAtPath("/Foo").GetRelationship("rel")))
                self.assertTrue(not notice.ResyncedObject(
                    stage.GetPrimAtPath("/Foo").GetRelationship("rel")))
                self.assertTrue(notice.ChangedInfoOnly(
                    stage.GetPrimAtPath("/Foo").GetRelationship("rel")))

            objectsChanged = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, OnRelationshipTargetChange, s)
            rel.AddTarget("/Bar")

            del objectsChanged

    def test_LayerMutingChange(self):
        expectedResult = {}
        def OnLayerMutingChange(notice, stage):
            self.assertEqual(notice.GetStage(), stage)
            self.assertEqual(notice.GetMutedLayers(), expectedResult["muted"])
            self.assertEqual(notice.GetUnmutedLayers(), 
                    expectedResult["unmuted"])
            
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('LayerMutingChange.'+fmt)

            # mute session layer and try unmute root layer
            # note that root layer cannot be muted or unmuted
            expectedResult = {
                    "muted": [s.GetSessionLayer().identifier],
                    "unmuted": [] 
                    }
            layerMutingChanged = Tf.Notice.Register(
                Usd.Notice.LayerMutingChanged,
                OnLayerMutingChange, s)
            s.MuteAndUnmuteLayers([s.GetSessionLayer().identifier],
                                    [s.GetRootLayer().identifier])

            # undo session layer muting and try to mute root layer
            expectedResult = {
                    "muted": [],
                    "unmuted": [s.GetSessionLayer().identifier]
                    }
            with self.assertRaises(Tf.ErrorException):
                s.MuteAndUnmuteLayers([s.GetRootLayer().identifier],
                        [s.GetSessionLayer().identifier])

            # test to make sure callback is still called for muting of layers
            # not part of the stage
            layer1 = Sdf.Layer.CreateNew('nonStageLayer.'+fmt)
            expectedResult = {
                    "muted": [layer1.identifier],
                    "unmuted": []
                    }
            s.MuteAndUnmuteLayers([layer1.identifier],[])

            del layerMutingChanged

    def test_Reload(self):
        s = Usd.Stage.CreateInMemory()
        s.DefinePrim('/Root')

        def OnReload(notice, sender):
            self.assertEqual(notice.GetStage(), s)
            self.assertEqual(notice.GetResyncedPaths(), ['/Root'])
            self.assertFalse(notice.HasChangedFields('/Root'))
            self.assertEqual(notice.GetChangedFields('/Root'), [])

            # XXX: The pseudo-root is marked as having info changed
            # because of the didReloadContent bit in the layer changelist.
            # This doesn't seem correct.
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), ['/'])
            self.assertFalse(notice.HasChangedFields('/'))
            self.assertFalse(notice.GetChangedFields('/'), [])

        notice = Tf.Notice.Register(Usd.Notice.ObjectsChanged, OnReload, s)

        s.Reload()

    @unittest.skipIf(platform.system() == "Windows" and
                     not hasattr(Ar.Resolver, "CreateIdentifier"),
                     "This test case currently fails on Windows due to "
                     "path canonicalization issues except with Ar 2.0.")
    def test_InvalidLayerReloadChange(self):
        s = Usd.Stage.CreateNew('LayerReloadChange.usda')

        # Create a prim with a reference to a non-existent layer.
        # This should result in a composition error.
        prim = s.DefinePrim('/ModelReference')
        prim.GetReferences().AddReference(
            assetPath='./Model.usda', primPath='/Model')

        s.Save()

        # Create a new layer at the referenced path and reload the referencing
        # stage. This should cause the layer to be opened and a resync for the
        # referencing prim.
        try:
            layer = Sdf.Layer.CreateNew('Model.usda')
            Sdf.CreatePrimInLayer(layer, '/Model')

            class OnReload(object):
                def __init__(self, stage, fixture):
                    super(OnReload, self).__init__()
                    self.receivedNotice = False
                    self.fixture = fixture
                    self.stage = stage
                    self._key = Tf.Notice.Register(
                        Usd.Notice.ObjectsChanged, self._HandleChange, stage)
                
                def _HandleChange(self, notice, sender):
                    self.receivedNotice = True
                    self.fixture.assertEqual(notice.GetStage(), self.stage)
                    self.fixture.assertEqual(notice.GetResyncedPaths(), 
                                             [Sdf.Path('/ModelReference')])
                    self.fixture.assertFalse(
                        notice.HasChangedFields('/ModelReference'))
                    self.fixture.assertEqual(
                        notice.GetChangedFields('/ModelReference'), [])
                    self.fixture.assertEqual(notice.GetChangedInfoOnlyPaths(),
                                             [])

            l = OnReload(s, self)
            s.Reload()

            # Should have received a resync notice and composed the referenced
            # /Model prim into /ModelReference.
            self.assertTrue(l.receivedNotice)
            self.assertEqual(
                s.GetPrimAtPath('/ModelReference').GetPrimStack()[1],
                layer.GetPrimAtPath('/Model'))

        finally:
            # Make sure to remove Model.usda so subsequent test runs don't
            # pick it up, otherwise test case won't work.
            try:
                os.remove('Model.usda')
            except:
                pass

    def test_StageVariableExpressionChange(self):
        refLayer = Sdf.Layer.CreateAnonymous('.usda')
        refLayer.ImportFromString('''
        #usda 1.0
        (
            expressionVariables = {
                string REF = "A"
            }
        )

        def "Ref"
        {
            asset attr = @`"${REF}.jpg"`@
        }
        '''.strip())

        rootLayer = Sdf.Layer.CreateAnonymous('.usda')
        rootLayer.ImportFromString('''
        #usda 1.0
        (
            expressionVariables = {{
                string ROOT = "A"
            }}
        )

        def "Test"
        {{
            asset attr = @`"${{A}}.jpg"`@
        }}

        def "Ref1" (
            references = @{refId}@</Ref>
        )
        {{
        }}

        def "Ref2" (
            references = @{refId}@</Ref>
        )
        {{
        }}
        '''.format(refId=refLayer.identifier).strip())

        s = Usd.Stage.Open(rootLayer)

        @contextlib.contextmanager
        def ExpectedNotice(stage, callback):
            received = False
            def _RunTest(notice, sender):
                callback(notice)
                nonlocal received
                received = True
            key = Tf.Notice.Register(Usd.Notice.ObjectsChanged, _RunTest, stage)
            yield
            self.assertTrue(received, "Did not receive notice")

        # Author a change to the expression variables in the root layer
        # stack. This should not send a resync for any objects, but it
        # should send a resolved asset path resync covering the entire stage
        # since there may be asset-valued attributes (that we may have never 
        # pulled a value from) that depend on those variables.
        def RootResync(notice):
            self.assertEqual(notice.GetResyncedPaths(), [])
            self.assertEqual(notice.GetResolvedAssetPathsResyncedPaths(), ['/'])
            self.assertEqual(notice.GetChangedInfoOnlyPaths(), ['/'])
            self.assertEqual(notice.GetChangedFields('/'), ['expressionVariables'])

        with ExpectedNotice(s, RootResync):
            rootLayer.expressionVariables = {'ROOT':'B'}

        # Author a change to the expression variables in a referenced layer
        # stack. This should not send a resync for the prim(s) that are
        # referencing that layer, but it should send a resolved asset path
        # resync covering those two prims.
        def ReferencingPrimsResync(notice):
            self.assertEqual(notice.GetResyncedPaths(), [])
            self.assertEqual(notice.GetResolvedAssetPathsResyncedPaths(), 
                             ['/Ref1', '/Ref2'])

            # XXX:
            # 'expressionVariables' should not show up as changed fields on
            # /Ref1 and /Ref2 since that field was actually authored on the
            # pseudo-root of the referenced layer. This is a pre-existing bug
            # involving layer metadata fields that cause resyncs for referencing
            # prims.  'defaultPrim' is another example of such a field.
            self.assertEqual(
                notice.GetChangedInfoOnlyPaths(), ['/Ref1', '/Ref2'])
            self.assertEqual(
                notice.GetChangedFields('/Ref1'), ['expressionVariables'])
            self.assertEqual(
                notice.GetChangedFields('/Ref2'), ['expressionVariables'])

        with ExpectedNotice(s, ReferencingPrimsResync):
            refLayer.expressionVariables = {'REF':'B'}

if __name__ == "__main__":
    unittest.main()
