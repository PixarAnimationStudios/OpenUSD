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
        # to self._objectsChangedNotices.
        self._objectsChangedNotices = []
        self._objectsChangedNoticeKey = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged,
            self._onObjectsChanged,
            stage)

        self._stageContentsChangedCount = 0
        self._stageContentsChangedKey = Tf.Notice.Register(
            Usd.Notice.StageContentsChanged,
            self._onStageContentsChanged,
            stage)
        
    def _onStageContentsChanged(self, *args):
        self._stageContentsChangedCount += 1

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
        self._objectsChangedNotices.append(asDict)

    def test_InsertPackageSublayer(self):
        stage = Usd.Stage.CreateInMemory()
        self._listenForNotices(stage)
        stage.GetRootLayer().subLayerPaths = ["./package.usdz"]
        self.assertEqual(self._stageContentsChangedCount, 1)
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
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
        self.assertEqual(self._stageContentsChangedCount, 1)
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
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
        self.assertEqual(self._stageContentsChangedCount, 1)
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
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
        self.assertEqual(self._stageContentsChangedCount, 1)
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
            'Resynced': {
                '/': [] 
            }
        })

    def test_muteEmptyLayer(self):
        """Tests that a StageContentsChanged and an empty ObjectsChanged notice
        are triggered when an empty layer is muted"""

        stage = Usd.Stage.CreateInMemory()
        l1 = Sdf.Layer.CreateAnonymous()

        stage.GetRootLayer().subLayerPaths.append(l1.identifier)
        self._listenForNotices(stage)
        stage.MuteLayer(l1.identifier)
        self.assertEqual(self._stageContentsChangedCount, 1)
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {})

    def test_UnmuteEmptyLayer(self):
        """Tests that a StageContentsChanged and an empty ObjectsChanged notice
        are triggered when an empty layer is unmuted"""

        stage = Usd.Stage.CreateInMemory()
        l1 = Sdf.Layer.CreateAnonymous()

        stage.GetRootLayer().subLayerPaths.append(l1.identifier)
        stage.MuteLayer(l1.identifier)
        self._listenForNotices(stage)
        stage.UnmuteLayer(l1.identifier)
        self.assertEqual(self._stageContentsChangedCount, 1)
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {})

    def test_SublayerWithDef(self):
        """Tests that proper change notifications are generated when a
        sublayer is added containing a def"""
    
        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.ImportFromString("""#usda 1.0
                                 def "World" { }
                              """)
        
        sub = Sdf.Layer.CreateAnonymous('sub.usda')
        sub.ImportFromString("""#usda 1.0
                                over "World" { 
                                    def "Prim" { }
                                }
                            """)
        
        stage = Usd.Stage.Open(root)
        self._listenForNotices(stage)

        # Test sublayer insertion/removal
        root.subLayerPaths = [sub.identifier]
        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
            'ChangedInfoOnly': {
                '/': ['subLayerOffsets', 'subLayers'],
                '/World': []
            },
            'Resynced': {
                '/World/Prim': ['specifier'] 
            }
        })

        del root.subLayerPaths[0]

        self.assertEqual(len(self._objectsChangedNotices), 2)
        self.assertDictEqual(self._objectsChangedNotices[1], {
            'ChangedInfoOnly': {
                '/': ['subLayerOffsets', 'subLayers'],
                '/World': []
            },
            'Resynced': {
                '/World/Prim': [] 
            }
        })

        # Test sublayer muting/unmuting.
        # Put our test sublayer back in temporarily, then use that for
        # testing. Note this will increment self._objectsChangedNotices
        root.subLayerPaths.insert(0, sub.identifier)
        self.assertEqual(len(self._objectsChangedNotices), 3)

        stage.MuteLayer(sub.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 4)
        self.assertDictEqual(self._objectsChangedNotices[3], {
            'ChangedInfoOnly': {
                '/World': []
            },
            'Resynced': {
                '/World/Prim': [] 
            }
        })

        stage.UnmuteLayer(sub.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 5)
        self.assertDictEqual(self._objectsChangedNotices[4], {
            'ChangedInfoOnly': {
                '/World': []
            },
            'Resynced': {
                '/World/Prim': ['specifier'] 
            }
        })

    def test_SublayerWithAttr(self):
        """Tests that proper notifications are generated when muting a sublayer
        which contains a prim which overrides an attribute"""

        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.ImportFromString("""#usda 1.0
                                 def "World" { 
                                   def "Sets" {
                                     def "GlassLens" {
                                     }
                                   }
                                }
                            """)
        

        glowLayer = Sdf.Layer.CreateAnonymous('glowLayer.usda')
        glowLayer.ImportFromString("""#usda 1.0
                                 over "World" { 
                                   over "Sets" {
                                     over "GlassLens" {
                                        color3f primvars:lightBulbColor = (0, 0, 0)
                                     }
                                   }
                                }
                              """)

        root.subLayerPaths = [glowLayer.identifier]

        stage = Usd.Stage.Open(root)
        self._listenForNotices(stage)

        expectedNotices = {
            'ChangedInfoOnly': {
                '/World': [],
                '/World/Sets': [],
                '/World/Sets/GlassLens': []
            },
            'Resynced': {
                '/World/Sets/GlassLens.primvars:lightBulbColor': [] 
            }
        }

        # Test sublayer muting/unmuting

        stage.MuteLayer(glowLayer.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], expectedNotices)

        stage.UnmuteLayer(glowLayer.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 2)
        self.assertDictEqual(self._objectsChangedNotices[1], expectedNotices)

        # Test sublayer removal/insertion
        del root.subLayerPaths[0]

        self.assertEqual(len(self._objectsChangedNotices), 3)
        self.assertDictEqual(self._objectsChangedNotices[2], {
            'ChangedInfoOnly': {
                '/': ['subLayerOffsets', 'subLayers'],
                '/World': [],
                '/World/Sets': [],
                '/World/Sets/GlassLens': []
            },
            'Resynced': {
                '/World/Sets/GlassLens.primvars:lightBulbColor': [] 
            }
        })

        root.subLayerPaths.insert(0, glowLayer.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 4)
        self.assertDictEqual(self._objectsChangedNotices[3], {
            'ChangedInfoOnly': {
                '/': ['subLayerOffsets', 'subLayers'],
                '/World': [],
                '/World/Sets': [],
                '/World/Sets/GlassLens': []
            },
            'Resynced': {
                '/World/Sets/GlassLens.primvars:lightBulbColor': [] 
            }
        })

    def test_SublayerWithSublayer(self):
        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.ImportFromString("""#usda 1.0
                                      def "World" { 
                                        def "Sets" {
                                            def "GlassLens" {
                                            }
                                        }
                                      }
                                   """)
        over = Sdf.Layer.CreateAnonymous('over.usda')
        over.ImportFromString("""#usda 1.0
                                      over "World" { 
                                        over "Sets" {
                                            over "GlassLens"(
                                               active = false
                                            )
                                            {
                                            }
                                        }
                                      }
                                   """)

        sub = Sdf.Layer.CreateAnonymous('sub.usda')
        sub.subLayerPaths = [over.identifier]
        root.subLayerPaths = [sub.identifier]

        stage = Usd.Stage.Open(root)
        self._listenForNotices(stage)

        # Test sublayer removal/insertion
        del root.subLayerPaths[0]

        expectedChanges = {
            'ChangedInfoOnly': {
                '/': ['subLayerOffsets', 'subLayers'],
                '/World': [],
                '/World/Sets': [],
            },
            'Resynced': {
                '/World/Sets/GlassLens': ['active'] 
            }
        }

        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], expectedChanges)

        root.subLayerPaths = [sub.identifier]
        self.assertEqual(len(self._objectsChangedNotices), 2)
        self.assertDictEqual(self._objectsChangedNotices[1], expectedChanges)
        
        # Test sublayer muting/unmuting
        stage.MuteLayer(sub.identifier)
        self.assertEqual(len(self._objectsChangedNotices), 3)
        self.assertDictEqual(self._objectsChangedNotices[2], expectedChanges)

        stage.UnmuteLayer(sub.identifier)
        self.assertEqual(len(self._objectsChangedNotices), 4)
        self.assertDictEqual(self._objectsChangedNotices[3], expectedChanges)

    def test_MutingSublayerThoughReference(self):
        """Tests that proper change notifications are generated when a
        sublayer containing a def is muted cor unmuted"""

        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.ImportFromString("""#usda 1.0
                                      def "World" { }
                                   """)
        
        ref = Sdf.Layer.CreateAnonymous('ref.usda')
        ref.ImportFromString("""#usda 1.0
                             (
                                defaultPrim="Base"
                             )
                                def "Base" {
                                    def "Refprim" {
                                    }
                                }
                            """)
        sub = Sdf.Layer.CreateAnonymous('sub.usda')
        sub.ImportFromString("""#usda 1.0
                                over "Base" {
                                    def "Subprim" {
                                    }
                                }
                            """)
        
        ref.subLayerPaths = [sub.identifier]
        root.GetPrimAtPath("/World").referenceList.Add(Sdf.Reference(ref.identifier))

        stage = Usd.Stage.Open(root)

        # Test Change notifications for muting
        self._listenForNotices(stage)
        stage.MuteLayer(sub.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
            'ChangedInfoOnly': {
                '/World': []
            },
            'Resynced': {
                '/World/Subprim': [] 
            }
        })

        # Test Change notifications for unmuting

        self._listenForNotices(stage)
        stage.UnmuteLayer(sub.identifier)

        self.assertEqual(len(self._objectsChangedNotices), 1)
        self.assertDictEqual(self._objectsChangedNotices[0], {
            'ChangedInfoOnly': {
                '/World': []
            },
            'Resynced': {
                '/World/Subprim': ['specifier'] 
            }
        })

if __name__ == "__main__":
    unittest.main()