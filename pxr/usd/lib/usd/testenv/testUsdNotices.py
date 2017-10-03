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

allFormats = ['usd' + x for x in 'ac']

class NoticeTester(unittest.TestCase):
    def setUp(self):
        self._ResetCounters()

    def _ResetCounters(self):
        self._changeCount = 0
        self._contentsCount = 0
        self._objectsCount = 0
        self._editTargetsCount = 0

    def OnStageContentsChanged(self, *args):
        self._changeCount += 1
        self._contentsCount += 1 

    def OnObjectsChanged(self, *args):
        self._changeCount += 1
        self._objectsCount += 1

    def OnStageEditTargetChanged(self, *args):
        self._changeCount += 1
        self._editTargetsCount += 1

    def Basics(self):
        contentsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.StageContentsChanged, 
            self.OnStageContentsChanged)
        objectsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, 
            self.OnObjectsChanged)
        stageEditTargetChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.StageEditTargetChanged, 
            self.OnStageEditTargetChanged)

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


    def ObjectsChangedNotice(self):
        def OnResync(notice, stage):
            self.assertTrue(notice.GetStage() == stage)
            self.assertTrue(notice.GetResyncedPaths() == [Sdf.Path("/Foo")])
            self.assertTrue(notice.GetChangedInfoOnlyPaths() == [])
            self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(notice.ResyncedObject(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(not notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo")))

        def OnUpdate(notice, stage):
            self.assertTrue(notice.GetStage() == stage)
            self.assertTrue(notice.GetResyncedPaths() == [])
            self.assertTrue(notice.GetChangedInfoOnlyPaths() == [Sdf.Path("/Foo")])
            self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(not notice.ResyncedObject(stage.GetPrimAtPath("/Foo")))
            self.assertTrue(notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo")))

        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('ObjectsChangedNotice.'+fmt)

            objectsChanged = Tf.Notice.Register(Usd.Notice.ObjectsChanged, 
                                                       OnResync, s)
            s.DefinePrim("/Foo")

            objectsChanged = Tf.Notice.Register(Usd.Notice.ObjectsChanged, 
                                                       OnUpdate, s)
            s.GetPrimAtPath("/Foo").SetMetadata("comment", "")
        del objectsChanged

    def ObjectsChangedNoticeForAttributes(self):
        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('ObjectsChangedNoticeForProps.'+fmt)
            prim = s.DefinePrim("/Foo");

            def OnAttributeCreation(notice, stage):
                self.assertTrue(notice.GetStage() == stage)
                self.assertTrue(notice.GetResyncedPaths() == [Sdf.Path("/Foo.attr")])
                self.assertTrue(notice.GetChangedInfoOnlyPaths() == [])
                self.assertTrue(notice.AffectedObject(stage.GetPrimAtPath("/Foo").GetAttribute("attr")))
                self.assertTrue(notice.ResyncedObject(stage.GetPrimAtPath("/Foo").GetAttribute("attr")))
                self.assertTrue(not notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo").GetAttribute("attr")))

            objectsChanged = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, OnAttributeCreation, s)
            attr = prim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)

            def OnAttributeValueChange(notice, stage):
                self.assertTrue(notice.GetStage() == stage)
                self.assertTrue(notice.GetResyncedPaths() == [])
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

    def ObjectsChangedNoticeForRelationships(self):
        for fmt in allFormats:
            self._ResetCounters()
            s = Usd.Stage.CreateInMemory('ObjectsChangedNoticeForRels.'+fmt)
            prim = s.DefinePrim("/Foo");

            def OnRelationshipCreation(notice, stage):
                self.assertTrue(notice.GetStage() == stage)
                self.assertTrue(notice.GetResyncedPaths() == [Sdf.Path("/Foo.rel")])
                self.assertTrue(notice.GetChangedInfoOnlyPaths() == [])
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
                self.assertTrue(notice.GetStage() == stage)

                # XXX: This is a bug. We should not get a resync involving the
                # target path, since no such object exists in the USD
                # scenegraph.
                self.assertTrue(notice.GetResyncedPaths() == [Sdf.Path("/Foo.rel[/Bar]")])
                self.assertTrue(notice.GetChangedInfoOnlyPaths() == [Sdf.Path("/Foo.rel")])
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

    def test_Basic(self):
        self.Basics()
        self.ObjectsChangedNotice()
        self.ObjectsChangedNoticeForAttributes()
        self.ObjectsChangedNoticeForRelationships()

if __name__ == "__main__":
    unittest.main()
