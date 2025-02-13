#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.


import unittest
from pxr import Kind, Sdf, Tf, Usd

class TestUsdObjectsChangedNoticesSublayerOps(unittest.TestCase):
    def _listenForNotices(self, stage):
        # Each ObjectsChanged notice is converted to a dictionary and appended
        # to self._notices.
        self._notices = []
        self._noticeKey = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged,
            self._onObjectsChanged,
            stage)

    def _onObjectsChanged(self, notice, sender):
        asDict = {}
        resynced = notice.GetResyncedPaths()
        changedInfoOnly = notice.GetChangedInfoOnlyPaths()
        resolvedAssetPathsResynced = notice.GetResolvedAssetPathsResyncedPaths()
        if resynced:
            asDict['Resynced'] = {
                str(path): notice.GetChangedFields(path)
                for path in resynced
            }
        if changedInfoOnly:
            asDict['ChangedInfoOnly'] = {
                str(path): notice.GetChangedFields(path)
                for path in changedInfoOnly
            }
        if resolvedAssetPathsResynced:
            asDict['ResolvedAssetPathsResynced'] = {
                str(path): notice.GetChangedFields(path)
                for path in resolvedAssetPathsResynced
            }
        self._notices.append(asDict)

    def test_InsertPackageSublayer(self):
        stage = Usd.Stage.CreateInMemory()
        self._listenForNotices(stage)
        stage.GetRootLayer().subLayerPaths = ["./package.usdz"]
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/': [] 
            }
        })

    def test_RemovePackageSublayer(self):
        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.ImportFromString("""#usda 1.0
                                (
                                    subLayers = [@./package.usdz@]
                                )
                            """)
        stage = Usd.Stage.Open(root)
        self._listenForNotices(stage)
        stage.GetRootLayer().subLayerPaths = []
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/': [] 
            }
        })

    def test_MutePackageSublayer(self):
        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.ImportFromString("""#usda 1.0
                                (
                                    subLayers = [@./package.usdz@]
                                )
                            """)
        stage = Usd.Stage.Open(root)
        self._listenForNotices(stage)
        stage.MuteLayer('./package.usdz')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/': [] 
            }
        })

    def test_UnmutePackageSublayer(self):
        stage = Usd.Stage.CreateInMemory()
        stage.MuteLayer('./package.usdz')
        stage.GetRootLayer().subLayerPaths = ["./package.usdz"]
        self._listenForNotices(stage)
        stage.UnmuteLayer('./package.usdz')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/': [] 
            }
        })

if __name__ == "__main__":
    unittest.main()