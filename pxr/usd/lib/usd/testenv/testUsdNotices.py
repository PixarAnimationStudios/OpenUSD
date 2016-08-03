#!/pxrpythonsubst

import sys
from pxr import Sdf,Usd,Tf
from Mentor.Runtime import (AssertEqual, AssertNotEqual, FindDataFile,
                            ExpectedErrors, ExpectedWarnings, RequiredException)

allFormats = ['usd' + x for x in 'abc']

class NoticeTester(object):
    def __init__(self):
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
            AssertEqual(self._changeCount, 2)
            AssertEqual(self._contentsCount, 1)
            AssertEqual(self._objectsCount, 1)
            AssertEqual(self._editTargetsCount, 0)

            self._ResetCounters()
            s.SetEditTarget(s.GetSessionLayer())
            AssertEqual(self._changeCount, 1)
            AssertEqual(self._contentsCount, 0)
            AssertEqual(self._objectsCount, 0)
            AssertEqual(self._editTargetsCount, 1)

            self._ResetCounters()
            s.GetPrimAtPath("/Foo").SetMetadata("comment", "")
            AssertEqual(self._changeCount, 4)
            AssertEqual(self._contentsCount, 2)     # Why 2? I expected 1. 
            AssertEqual(self._objectsCount, 2)      # We get an additional
                                                    # object resync notice when
                                                    # we first drop an over, in
                                                    # addition to the info-only
                                                    # change notice.
            AssertEqual(self._editTargetsCount, 0)

            self._ResetCounters()
            s.GetPrimAtPath("/Foo").SetMetadata("comment", "x")
            # Now that the over(s) have been established, setting a value
            # behaves as expected.
            AssertEqual(self._changeCount, 2)
            AssertEqual(self._contentsCount, 1)
            AssertEqual(self._objectsCount, 1)
            AssertEqual(self._editTargetsCount, 0)

        # testing for payload specific updates previously, load/unload calls
        # didn't trigger notices
        self._ResetCounters()
        assert self._objectsCount == 0
        payloadBasedFile = 'testUsdNotices.testenv/payload_base.usda'
        payloadBasedStage = Usd.Stage.Open(FindDataFile(payloadBasedFile))
        
        # the payload will be already loaded since we didn't supply
        # the additional parameter to the stage constructor
        payloadBasedStage.Unload('/Foo')
        AssertEqual(self._objectsCount, 1)
        payloadBasedStage.Load('/Foo')
        AssertEqual(self._objectsCount, 2)

        del contentsChanged
        del objectsChanged
        del stageEditTargetChanged 


    def ObjectsChangedNotice(self):
        def OnResync(notice, stage):
            assert notice.GetStage() == stage
            assert notice.GetResyncedPaths() == [Sdf.Path("/Foo")]
            assert notice.GetChangedInfoOnlyPaths() == []
            assert notice.AffectedObject(stage.GetPrimAtPath("/Foo"))
            assert notice.ResyncedObject(stage.GetPrimAtPath("/Foo"))
            assert not notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo"))

        def OnUpdate(notice, stage):
            assert notice.GetStage() == stage
            assert notice.GetResyncedPaths() == []
            assert notice.GetChangedInfoOnlyPaths() == [Sdf.Path("/Foo")]
            assert notice.AffectedObject(stage.GetPrimAtPath("/Foo"))
            assert not notice.ResyncedObject(stage.GetPrimAtPath("/Foo"))
            assert notice.ChangedInfoOnly(stage.GetPrimAtPath("/Foo"))

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


def Main(argv):
    tester = NoticeTester()
    tester.Basics()
    tester.ObjectsChangedNotice()


if __name__ == "__main__":
    Main(sys.argv)

