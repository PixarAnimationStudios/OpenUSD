#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Sdf, Usd, Tf
from testUsdNamespaceEditorDependentEditsBase \
      import TestUsdNamespaceEditorDependentEditsBase

class TestUsdNamespaceEditorDependentEditsProperties(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency namespace edits to properties across all the 
    composition arcs.'''

    # Calls CanApplyEdits and ApplyEdits on the given editor and verifies both
    # succeed. If expectedObjectsChangedRenamedProperties is provided, this also 
    # verifies that listening to the ObjectsChanged notice will send a notice 
    # holding the expected renamed properties specified.
    def _ApplyEditWithVerification(self, editor, 
            expectedObjectsChangedRenamedProperties = None, expectWarnings=False):
        # receivedObjectsChanged is used for sanity checking that the notice
        # handler was indeed called as expected.
        receivedObjectsChanged = False
        def _OnObjectsChangedVerifyRenamedPropertiesNotices(notice, sender):
            nonlocal receivedObjectsChanged
            receivedObjectsChanged = True

            # Compare the notice's renamed properties with the expected
            # properties or verify that renamed properties is empty if we don't
            # expect renamed properties
            if expectedObjectsChangedRenamedProperties is None:
                self.assertEqual(notice.GetRenamedProperties(), [])
            else:
                stageName = notice.GetStage().GetRootLayer().GetDisplayName()
                self.assertEqual(notice.GetRenamedProperties(),
                                 expectedObjectsChangedRenamedProperties[stageName])
            self._OnObjectsChanged(notice, sender)

        # Register the ObjectsChange listener; we revoke it after applying the
        # edits
        self.resyncedObjectsPerStage = {}
        objectsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, 
            _OnObjectsChangedVerifyRenamedPropertiesNotices)

        try:
            if expectWarnings:
                print("\n=== EXPECT WARNINGS ===", file=sys.stderr)
            # Verify CanApply and Apply
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            # Sanity check on the notice listener being called.
            self.assertTrue(receivedObjectsChanged)
            
        finally:
            if expectWarnings:
                print("\n=== END EXPECTED WARNINGS ===", file=sys.stderr)
            objectsChanged.Revoke()

    def _VerifyDefaultAndCustomMetadata(self, stage, propertyPath, 
                                        expectedDefaultValue=None, 
                                        expectedCustomMetadata={}):
        prop = stage.GetPropertyAtPath(propertyPath)
      
        self.assertEqual(prop.Get(), expectedDefaultValue)
        self.assertEqual(prop.GetCustomData(), expectedCustomMetadata)

    def _VerifyStagePropertyResyncNotices(self, stage, expectedResyncList):
        # Property changes aren't prim changes, so although a prim resync 
        # notification is sent, we expect the type to be invalid.
        if expectedResyncList is None:
            expectedResyncs = None
        else:
            expectedResyncs = \
                {value: self.PrimResyncType.Invalid for value in expectedResyncList}
        self._VerifyStageResyncNotices(stage, expectedResyncs)


    def test_BasicReferences(self):
        """Test downstream dependency namespace edits to properties across basic 
        single references, both internal and in separate layers """

        # Setup:
        # Layer 1 has /Ref, which is referenced from layer 2 and has a 
        # property (childAttr) that will be namespace edited. It also has a prim
        # that internally references /Ref. /InternalRef.childAttr will be 
        # affected by downstream dependency edits.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref" {
                string childAttr  = "fromRef" (customData = {int fromRef = 0})

            }

            def "RefSibling" {
            }

            def "InternalRef" (
                references = </Ref>
            ) {
                string childAttr (customData = {int fromInternalRef = 0})

            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 has a prim that references /Ref from layer 1 and will be
        # affected by downstream dependency edits.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            def "Prim" (
                references = @''' + layer1.identifier + '''@</Ref>
            ) {
                string childAttr (customData = {int fromPrim = 0})
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as an additional dependent
        # stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the expected contents of stage 1
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['childAttr'],
            },
            'RefSibling' : {},
            'InternalRef' : {
                '.' : ['childAttr'],
            },
        })

        # Verify the expected contents of stage 2.
        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['childAttr'],
            },
        })

        # Check the composed property metadata of childAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromRef", {'fromRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage1, '/InternalRef.childAttr', 
            "fromRef", {'fromRef' : 0, 'fromInternalRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            "fromRef", {'fromRef' : 0, 'fromPrim' : 0})

        # Edit: Rename /Ref.childAttr to /Ref.renamedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref.childAttr', '/Ref.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = 
            {'layer1.usda' : [('/InternalRef.childAttr', 'renamedChildAttr'), 
                              ('/Ref.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Prim.childAttr', 'renamedChildAttr')]})
        
        # Verify the updated stage contents for both stages.
        #
        # The childAttr property is renamed to renamedChildAttr. 
        # Note that this means the local override specs
        # for the referencing prims have been renamed to renamedChildAttr too.

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['renamedChildAttr'],
            },
            'RefSibling' : {},
            'InternalRef' : {
                '.' : ['renamedChildAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])

        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['renamedChildAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [])
        
        # Check the composed property metadata of renamedChildAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.renamedChildAttr', 
            "fromRef", {'fromRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage1, '/InternalRef.renamedChildAttr', 
            "fromRef", {'fromRef' : 0, 'fromInternalRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.renamedChildAttr', 
            "fromRef", {'fromRef' : 0, 'fromPrim' : 0})

        # Edit: Reparent and rename /Ref.childAttr to /RefSibling.movedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref.renamedChildAttr', '/RefSibling.movedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = 
            {'layer1.usda' : [],
            'layer2.usda' : []})

        # Verify the updated stage contents for both stages.
        #
        # For both prims that directly reference /Ref, since renamedChildAttr has
        # been moved out of /Ref, it is no longer a composed property of the
        # referencing prims. Note that the overs to renamedChildAttr have been
        # deleted to prevent the reintroduction of a partially specced
        # renamedChildAttr.

        self._VerifyStageContents(stage1, {
            'Ref': {},
            'RefSibling' : {
                '.' : ['movedChildAttr'],
            },
            'InternalRef' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            '/InternalRef.renamedChildAttr',
            '/RefSibling.movedChildAttr',
            '/Ref.renamedChildAttr'
        ])

        self._VerifyStageContents(stage2, {
            'Prim' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            '/Prim.renamedChildAttr'
        ])
        
        # Check the composed property metadata of movedChildAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/RefSibling.movedChildAttr', 
            "fromRef", {'fromRef' : 0})

        # Edit: Reparent and rename /RefSibling.movedChildAttr back to its 
        # original path /Ref.childAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/RefSibling.movedChildAttr', '/Ref.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : []
        })

        # Verify the updated stage contents for both stages.
        #
        # For both prims that directly reference /Ref, they are returned to
        # their original contents with one notable exception: the overs on
        # childAttr are NOT restored from being deleted so the metadata they had
        # introduced is not present like it was in the initial state of the stages.

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['childAttr'],
            },
            'RefSibling' : {},
            'InternalRef' : {
                '.' : ['childAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            '/InternalRef.childAttr',
            '/RefSibling.movedChildAttr',
            '/Ref.childAttr'
        ])

        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['childAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            '/Prim.childAttr'
        ])
        
        # Check the composed property metadata of childAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromRef", {'fromRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage1, '/InternalRef.childAttr', 
            "fromRef", {'fromRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            "fromRef", {'fromRef' : 0})

        # Reinitialize to reset the overs on childAttr for the next test case.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Check the composed property metadata of childAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromRef", {'fromRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage1, '/InternalRef.childAttr', 
            "fromRef", {'fromRef' : 0, 'fromInternalRef' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            "fromRef", {'fromRef' : 0, 'fromPrim' : 0})

        # Edit: Delete the property at /Ref.childAttr
        self.assertTrue(editor.DeletePropertyAtPath('/Ref.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : []
        })

        # Verify the updated stage contents for both stages.
        #
        # For both prims that directly reference /Ref, since childAttr has been 
        # deleted, the overs to childAttr have been deleted to prevent the 
        # reintroduction of a partially specced childAttr.

        self._VerifyStageContents(stage1, {
            'Ref' : {},
            'RefSibling' : {},
            'InternalRef' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            '/InternalRef.childAttr',
            '/Ref.childAttr'
        ])

        self._VerifyStageContents(stage2, {
            'Prim' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            '/Prim.childAttr'
        ])
        
    def test_BasicRelocates(self):
        '''Tests downstream dependency namespace edits to properties across a 
        reference where a child of the referencing prim is then relocated.'''

        # Setup: 
        # Layer1 is has a /World/Ref that will be referenced by various
        # prims in the next layer. /World/Ref.childAttr will be namespace edited.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "World" {                                
                def "Ref" {
                    string childAttr = "fromRef" (customData = {int fromRef = 0})
                }
            }
        ''')

        # Layer2 has /Prim that references /World, and a relocates that
        # moves /Prim/Ref to /Relocated and provides local opinions at the 
        # post-relocates location.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            (
                relocates = {
                    </Prim/Ref> : </Relocated>,
                }
            )

            def "Prim" (
                references = @''' + layer1.identifier + '''@</World>
            ) {
            }

            def "Relocated"
            {
                string childAttr (customData = {int fromRelocated = 0})
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Create a stage for each layer we created.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for editing the base layer via stage1.
        editor = Usd.NamespaceEditor(stage1)

        # Add stage2 as a dependent stage of the editor.
        editor.AddDependentStage(stage2)

        # Verify initial contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref': {
                    '.' : ['childAttr'],
                }
            }
        })

        # Verify initial contents of stage 2
        self._VerifyStageContents(stage2, {
            'Prim' : {},
            'Relocated' : {
                '.' : ['childAttr']
            }
        })
        
        # Check the composed property metadata of childAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/World/Ref.childAttr', 
            "fromRef", {'fromRef': 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Relocated.childAttr', 
            "fromRef", {'fromRef': 0, 'fromRelocated' : 0})
        
        # Edit: Rename /World/Ref.childAttr to /World/Ref.renamedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/World/Ref.childAttr', '/World/Ref.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = 
            {'layer1.usda' : [('/World/Ref.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Relocated.childAttr', 'renamedChildAttr')]})

        # Verify the rename of childAttr to renamedChildAttr on stage1.
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref': {
                    '.' : ['renamedChildAttr']
                }
            }
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])

        # On stage2, the contents of the pre-relocation prim (/Prim) has not 
        # changed because the contents that would've changed are relocated to 
        # /Relocated and reflected there.
        #
        # As expected, /Relocated's contents change to reflect the rename of
        # childAttr to renamedChildAttr (the specs at /Relocated.childAttr are 
        # moved to  /Relocated.renamedChildAttr for the rename). 
        self._VerifyStageContents(stage2, {
            'Prim' : {},
            'Relocated' : {
                '.' : ['renamedChildAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [])

        # Check the composed property metadata of renamedChildAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/World/Ref.renamedChildAttr', 
            "fromRef", {'fromRef': 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Relocated.renamedChildAttr', 
            "fromRef", {'fromRef': 0, 'fromRelocated' : 0})

        # Edit: Reparent and rename /World/Ref.renamedChildAttr to /World.movedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/World/Ref.renamedChildAttr', '/World.movedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = 
            {'layer1.usda' : [], 'layer2.usda' : []})
            
        # Verify the reparent and rename of /World/Ref.renamedChildAttr to 
        # /World.movedChildAttr on stage1.
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['movedChildAttr'],
                'Ref': {
                    '.' : [],
                },
            }
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/World/Ref.renamedChildAttr",
            "/World.movedChildAttr"
        ])
 
        # On stage2, the pre-relocation prim /Prim now has the attribute 
        # movedChildAttr (with its ancestral reference metadata) because 
        # movedChildAttr is no longer a property of /Prim/Ref, and as such is 
        # not ancestrally relocated via the relocation of /Prim/Ref to 
        # /Relocated. Similarly, /Relocated no longer has the attribute 
        # renamedChildAttr since it is no longer a descendant of the relocation 
        # source. The fully composed movedChildAttr still matches the specs of
        # renamedChildAttr - all the contributing specs have been moved together.
        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['movedChildAttr']
            },
            'Relocated' : {}
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Relocated.renamedChildAttr",
            "/Prim.movedChildAttr"
        ])

        # Check the composed property metadata of movedChildAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/World.movedChildAttr', 
            "fromRef", {'fromRef': 0})
        # We keep metadata from /Relocated as the goal is to moved the composed
        # childAttr when we namespace edit it. 
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.movedChildAttr', 
            "fromRef", {'fromRef': 0, 'fromRelocated' : 0})
        
        # Edit: Move /World.movedChildAttr back to its original path 
        # /World/Ref.childAttr.
        self.assertTrue(editor.MovePropertyAtPath(
            '/World.movedChildAttr', '/World/Ref.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = 
            {'layer1.usda' : [], 'layer2.usda' : []})

        # Verify that stage1 is back to its original contents.
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref': {
                    '.' : ['childAttr'],
                },
            }
        })

        self._VerifyStagePropertyResyncNotices(stage1, [
            "/World.movedChildAttr",
            "/World/Ref.childAttr"
        ])

        # On stage2, /Prim again has its contents relocated away 
        # since childAttr is back to being from of /Prim/Ref, and as such is
        # ancestrally relocated via the relocation from /Prim/Ref to 
        # /Relocated. And similarly, /Relocated now has childAttr
        # since it is a descendant of the relocation source. This matches the
        # initial state of /Prim and /Relocated.
        self._VerifyStageContents(stage2, {
            'Prim' : {},
            'Relocated' : {
                '.' : ['childAttr']
            }
        })

        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Prim.movedChildAttr",
            "/Relocated.childAttr"
        ])

        # Check the composed property metadata of childAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/World/Ref.childAttr', 
            "fromRef", {'fromRef': 0})
        # We keep metadata from /Relocated as the goal is to moved the composed
        # childAttr when we namespace edit it. 
        self._VerifyDefaultAndCustomMetadata(stage2, '/Relocated.childAttr', 
            "fromRef", {'fromRef': 0, 'fromRelocated' : 0})
        
        # Edit: Delete /World/Ref.childAttr
        self.assertTrue(editor.DeletePropertyAtPath('/World/Ref.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = 
            {'layer1.usda' : [],
            'layer2.usda' : []})

        # Verify the deletion of Child on stage1.
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref': {
                },
            }
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/World/Ref.childAttr"
        ])

        # On stage2, /Prim's contents haven't changed because its child is
        # relocated, but /Relocated no longer has childAttr as it was deleted.
        self._VerifyStageContents(stage2, {
            'Prim' : {},
            'Relocated' : {},
        })
        
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Relocated.childAttr"
        ])

    def test_BasicSublayers(self):
        """Tests downstream dependency namespace edits to properties across 
        sublayers from other dependent stages."""

        # Setup: 
        # Layer1 is a simple base layer with /Ref and /RefSibling. This layer 
        # will be opened as the base stage on which direct namespace edits will 
        # be performed on /Ref.childAttr. Each stage that sublayers layer1 will
        # provide opinions for childAttr's metadata that we can use to check
        # whether the property composes correctly.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Ref" (
            ) {                
                string childAttr = "fromLayer1" (customData = {int fromLayer1 = 0})
            }
            
            def "RefSibling" {}                             
        ''')

        # Layer2 includes layer1 as a sublayer and provides opinions for /Ref.childAttr
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            (
                subLayers = [@''' + layer1.identifier + '''@]
            )

            over "Ref" (
            ) {                
                string childAttr (customData = {int fromLayer2 = 0})
            }
        ''')

        # Layer3Sub will be a sublayer of the next layer, layer3 and provides 
        # opinions for /Ref.childAttr.
        layer3Sub = Sdf.Layer.CreateAnonymous("layer3-sub.usda")
        layer3Sub.ImportFromString('''#usda 1.0
            over "Ref" (
            ) {                
                string childAttr (customData = {int fromLayer3Sub = 0})
            }
        ''')

        # Layer3 includes layer2 (which includes layer1) and layer3Sub as 
        # sublayer and also includes opinions for /Ref.childAttr.
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3.ImportFromString('''#usda 1.0
            (
                subLayers = [
                    @''' + layer2.identifier + '''@,
                    @''' + layer3Sub.identifier + '''@
                ]
            )

            over "Ref" (
            ) {
                string childAttr (customData = {int fromlayer3 = 0})
            }
        ''')

        # Layer4 includes layer3 as a sublayer as well as its own local 
        # opinions for /Ref.childAttr.
        layer4 = Sdf.Layer.CreateAnonymous("layer4.usda")
        layer4.ImportFromString('''#usda 1.0
            (
                subLayers = [@''' + layer3.identifier + '''@]
            )

            over "Ref" (
            ) {
                string childAttr (customData = {int fromlayer4 = 0})
            }
        ''')
        
        # Open stages for the 4 main layers (excludes layer3Sub)
        stage1 = Usd.Stage.Open(layer1)
        stage2 = Usd.Stage.Open(layer2)
        stage3 = Usd.Stage.Open(layer3)
        stage4 = Usd.Stage.Open(layer4)
        stages = [stage1, stage2, stage3, stage4]

        # Create a namespace editor for stage1 which only includes layer1.
        editor = Usd.NamespaceEditor(stage1)

        # Add ONLY stage3 as a dependent stage. This is to specifically show
        # how layer2 will be affected by edits because stage3 depends on it 
        # but layer4 will not be affected as stage4 would've needed to be added
        # to introduce any dependencies on layer4.
        editor.AddDependentStage(stage3)
        
        # To start, all four stages have the same contents, which we verify below.
        for stage in stages:
            self._VerifyStageContents(stage, {
                'Ref': {
                    '.' : ['childAttr'],
                },
                'RefSibling' : {},
            })            

        # The only difference is /Ref.childAttr's metadata - each layer adds a 
        # piece of customData indicating where it was composed from.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0})
        self._VerifyDefaultAndCustomMetadata(stage3, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 
                           'fromLayer3Sub': 0, 'fromlayer3': 0})
        self._VerifyDefaultAndCustomMetadata(stage4, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0, 'fromlayer4': 0})
        
        # Edit: Rename /Ref.childAttr to /Ref.renamedChildAttr.
        # Stages 1 and 3 receive object changed notifications for the rename as
        # they are registered with the namespace editor. Stage 2 is not registered
        # as a dependent stage, so although the property is renamed through layer 2's
        # dependency in stage 3, stage 2 does not receive an object changed
        # notification for it. Stage 4 is not updated and does not receive a 
        # notification.
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref.childAttr', '/Ref.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [('/Ref.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [],
            'layer3.usda' : [('/Ref.childAttr', 'renamedChildAttr')],
            'layer4.usda' : []
        })
        
        stage1to3Contents = {
            'Ref': {
                '.' : ['renamedChildAttr'],
            },
            'RefSibling' : {},
        }
        
        # Verify the direct rename on stage1
        self._VerifyStageContents(stage1, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage1, [])

        # Stage2 was not added as a dependent stage, but layer2 is dependent on
        # layer1 edits via stage3 so stage2 reflects the rename in layer1 and
        # layer2.
        self._VerifyStageContents(stage2, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage2, [])
        
        # Stage3 is a dependent stage of the editor and depends on layer1 edits
        # through sublayers, so all sublayers in the dependent layer stack of
        # layer3 (this includes layer3Sub) are affected by the rename and 
        # childAttr is fully renamed in stage3.
        self._VerifyStageContents(stage3, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage3, [])

        # Stage4 is not a dependent stage so layer4 is not updated even though
        # all its other sublayers have been edited. Thus stage4 has both childAttr
        # and renamedChildAttr where childAttr has only opinions from layer4 while 
        # renamedChildAttr has the opinions from all other layers.        
        self._VerifyStageContents(stage4, {
            'Ref': {
                    '.' : ['childAttr', 'renamedChildAttr'],
                },
            'RefSibling' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage4, [])
        
        # /Ref.renamedchildAttr's metadata stays the same for stages 1-3, but 
        # stage 4 now has /Ref.childAttr with layer 4's opinions and 
        # /Ref.renamedChildAttr with the opinions from the other layers.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.renamedChildAttr', 
            "fromLayer1", {'fromLayer1': 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Ref.renamedChildAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0})
        self._VerifyDefaultAndCustomMetadata(stage3, '/Ref.renamedChildAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0})
        self._VerifyDefaultAndCustomMetadata(stage4, '/Ref.renamedChildAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0})
        self._VerifyDefaultAndCustomMetadata(stage4, '/Ref.childAttr', 
            None, {'fromlayer4': 0})
        
        # Edit: Reparent and rename /Ref.renamedChildAttr to /RefSibling.movedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref.renamedChildAttr', '/RefSibling.movedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : [],
            'layer3.usda' : [],
            'layer4.usda' : []
        })
        
        stage1to3Contents = {
            'Ref': {},
            'RefSibling' : {
                '.' : ['movedChildAttr'],
            },
        }
        
        # Verify the direct reparent and rename on stage1
        self._VerifyStageContents(stage1, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/Ref.renamedChildAttr",
            "/RefSibling.movedChildAttr"
        ])

        # Like with the rename edit previously, renamedChildAttr is moved to 
        # /RefSibling.movedChildAttr in all layers and is reflected as a move 
        # in both stage2 and stage3.
        self._VerifyStageContents(stage2, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Ref.renamedChildAttr",
            "/RefSibling.movedChildAttr"
        ])

        self._VerifyStageContents(stage3, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage3, [
            "/Ref.renamedChildAttr",
            "/RefSibling.movedChildAttr"
        ])

        # And /Ref.renamedChildAttr is also fully moved to 
        # /RefSibling.movedChildAttr in stage4. But note that the specs for 
        # childAttr in layer4 were not renamed in the first edit so childAttr 
        # still exists with the same "layer4 only" contents as before.
        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['childAttr'],
            },
            'RefSibling' : {
                '.' : ['movedChildAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage4, [
            "/Ref.renamedChildAttr",
            "/RefSibling.movedChildAttr"
        ])
        
        # Similarly, stage 4 still has /Ref.childAttr with the same "layer4 only"
        # metadata and /RefSibling.movedChildAttr with opinions from layers 1-3.
        # Layers 1-3 have the same composed opinions as before, just moved to 
        # /RefSibling.movedChildAttr
        self._VerifyDefaultAndCustomMetadata(stage1, '/RefSibling.movedChildAttr', 
            "fromLayer1", {'fromLayer1': 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/RefSibling.movedChildAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0})
        self._VerifyDefaultAndCustomMetadata(stage3, '/RefSibling.movedChildAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0})
        self._VerifyDefaultAndCustomMetadata(stage4, '/RefSibling.movedChildAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0})
        self._VerifyDefaultAndCustomMetadata(stage4, '/Ref.childAttr', 
            None, {'fromlayer4': 0})
        
        # Edit: Reparent and rename /RefSibling.movedChildAttr back to /Ref.childAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/RefSibling.movedChildAttr', '/Ref.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : [],
            'layer3.usda' : [],
            'layer4.usda' : []
        })

        # All stages return to their exact original contents. 
        # This includes stage4 where layer4's childAttr opinions are once again
        # composed with the childAttr opinions from the other sublayers.
        for stage in stages:
            self._VerifyStageContents(stage, {
                'Ref': {
                    '.' : ['childAttr'],
                },
                'RefSibling' : {},
            })
            self._VerifyStagePropertyResyncNotices(stage1, [
                "/Ref.childAttr",
                "/RefSibling.movedChildAttr"
            ])
            
        # Check the composed property metadata of childAttr has been reset.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0})
        self._VerifyDefaultAndCustomMetadata(stage3, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0})
        self._VerifyDefaultAndCustomMetadata(stage4, '/Ref.childAttr', 
            "fromLayer1", {'fromLayer1': 0, 'fromLayer2': 0, 'fromLayer3Sub': 0, 
                           'fromlayer3': 0,  'fromlayer4': 0})
        
        # Edit: Delete the prim at /Ref.childAttr
        self.assertTrue(editor.DeletePropertyAtPath('/Ref.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [], 'layer2.usda' : [], 
            'layer3.usda' : [], 'layer4.usda' : []
        })
        
        stage1to3Contents = {
            'Ref': {},
            'RefSibling' : {},
        }
        
        # Verify the direct delete on stage1
        self._VerifyStageContents(stage1, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/Ref.childAttr"
        ])

        # Stage2 was not added as a dependent stage, but layer2 is dependent on
        # layer1 edits via stage3 so stage2 reflects the delete of childAttr in 
        # layer1 and layer2.
        self._VerifyStageContents(stage2, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Ref.childAttr"
        ])

        # Stage3 is a dependent stage of the editor and depends on layer1 edits
        # through sublayers, so all sublayers in the dependent layer stack of
        # layer3 are affected by the deletion and childAttr is fully deleted in 
        # stage3.
        self._VerifyStageContents(stage3, stage1to3Contents)
        self._VerifyStagePropertyResyncNotices(stage3, [
            "/Ref.childAttr"
        ])

        # Stage4 is not a dependent stage so layer4 is not updated even though
        # all its other sublayers have been edited. Thus stage4 still has 
        # childAttr which now only has opinions from layer4 as the childAttr 
        # opinions from all other sublayers have been deleted.
        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['childAttr'],
            },
            'RefSibling' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage4, [
            "/Ref.childAttr"
        ])
        
        # The composed property metadata of childAttr in stage 4 should only
        # have opinions from layer 4.
        self._VerifyDefaultAndCustomMetadata(stage4, '/Ref.childAttr', None, {
           'fromlayer4': 0
        })

    def _RunTestBasicDependentGlobalClassArcs(self, classArcType):
        """Helper for testing downstream dependency namespace edits across
        global class arcs and their implied class specs. classArcType can
        be either 'inherits' or 'specializes'"""

        # Create the base layer which we will open as a stage and use to make
        # namespace edits to a global class
        #
        # The global class is /Class and has childAttr. This layer also has 
        # /Prim, which inherits (or specializes) /Class, and an empty 
        # /ClassSibling. /Prim will be referenced in other layers to show how we
        # handle namespace edits that affect implied class arcs.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Class" {
                string childAttr = "fromClass" (customData = {int fromClass = 0})
            }
            
            def "ClassSibling" {}

            def "Prim" (
                ''' + classArcType + ''' = </Class>
            ) {
                string childAttr (customData = {int fromPrim = 0})
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer2 has a prim that references /Prim from the first layer 
        # which inherits or specializes /Class and propagates the implied 
        # class arc to /Class in this layer.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Prim>
            ) {
                string childAttr (customData = {int fromPrim2 = 0})
            }

            over "Class" {
                string childAttr (customData = {int fromImpliedClass = 0})
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Layer2_A has a prim that references /Prim2 from the layer2 
        # which continues to propagate the implied class arc to /Class in this 
        # layer.
        layer2_A = Sdf.Layer.CreateAnonymous("layer2_A.usda")
        layer2_AImportString = '''#usda 1.0
            def "Prim2_A" (
                references = @''' + layer2.identifier + '''@</Prim2>
            ) {
                string childAttr (customData = {int fromPrim2_A = 0}) 
            }

            over "Class" {
                string childAttr (customData = {int fromImpliedClass2_A = 0})
            }
        '''
        layer2_A.ImportFromString(layer2_AImportString)
        
        # Create a stage for each of the layers we created.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)
        stage2_A = Usd.Stage.Open(layer2_A, Usd.Stage.LoadAll)

        # Create an editor for just stage1 so we can edit the base class.
        editor = Usd.NamespaceEditor(stage1)

        # Add the other stages as dependent stages of the editor.
        editor.AddDependentStage(stage2)
        editor.AddDependentStage(stage2_A)
        
        # Verify the initial contents of stage1.
        self._VerifyStageContents(stage1, {
            'Class' : {
                '.' : ['childAttr']
            },
            'ClassSibling' : {},
            'Prim' : {
                '.' : ['childAttr']
            },
        })

        # Verify initial contents of stage2.
        self._VerifyStageContents(stage2, {
            'Class' : {
                '.' : ['childAttr']
            },
            'Prim2' : {
                '.' : ['childAttr']
            }
        })

        # Verify initial contents of stage2_A.
        self._VerifyStageContents(stage2_A, {
            'Class' : {
                '.' : ['childAttr']
            },
            'Prim2_A' : {
                '.' : ['childAttr']
            },
        })
        
        # We use metadata to check that opinions from all layers compose correctly.
        # In stage 1, we get opinions from /Prim and /Class as expected.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Prim.childAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass' : 0})
        # In stage 2, we get the opinions from layer 1 with local opinions from 
        # /Prim2 and implied opinions from /Class in layer 2
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim2.childAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass': 0, 'fromPrim2': 0, 
                          'fromImpliedClass': 0, })
        # In stage 2_A, we get all the opinions from layer 2 with local opinions 
        # from /Prim2_A and implied opinions from /Class in layer 2_A.
        self._VerifyDefaultAndCustomMetadata(stage2_A, '/Prim2_A.childAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass': 0, 'fromPrim2': 0, 
            'fromImpliedClass': 0, 'fromPrim2_A': 0, 'fromImpliedClass2_A': 0})        
        
        # Edit: Rename /Class.childAttr to /Class.renamedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Class.childAttr', '/Class.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [('/Class.childAttr', 'renamedChildAttr'), 
                             ('/Prim.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Class.childAttr', 'renamedChildAttr'), 
                             ('/Prim2.childAttr', 'renamedChildAttr')],
            'layer2_A.usda' : [('/Class.childAttr', 'renamedChildAttr'), 
                               ('/Prim2_A.childAttr', 'renamedChildAttr')],
        })
        
        # On stage1, the class contents are updated with childAttr renamed to 
        # renamedChildAttr in both /Class and /Prim.
        self._VerifyStageContents(stage1, {
            'Class' : {
                '.' : ['renamedChildAttr']
            },
            'ClassSibling' : {},
            'Prim' : {
                '.' : ['renamedChildAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])

        # On stage2 the implied class specs on layer2 are also updated with the
        # rename of childAttr to renamedChildAttr. This results in Prim2's 
        # contents having changed to fully reflect the renaming of childAttr to 
        # renamedChildAttr that still composes all the same metadata) as before 
        # the rename.
        self._VerifyStageContents(stage2, {
            'Class': {
                '.' : ['renamedChildAttr']
            },
            'Prim2' : {
                '.' : ['renamedChildAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [])
        
        # On stage2_A the implied class specs on layer2_A are also updated with
        # the rename of childAttr to renamedChildAttr. This results in Prim2_A's 
        # contents having changed to fully reflect the renaming of childAttr to 
        # renamedChildAttr for all specs that originally contributed to childAttr 
        # (which includes implied classes from layer2 and layer2_A).
        self._VerifyStageContents(stage2_A, {
            'Class': {
                '.' : ['renamedChildAttr']
            },
            'Prim2_A' : {
                '.' : ['renamedChildAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2_A, [])
        
        # The composed metadata remains the same as before the rename.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Prim.renamedChildAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim2.renamedChildAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass': 0, 'fromPrim2': 0, 
                          'fromImpliedClass': 0})
        self._VerifyDefaultAndCustomMetadata(stage2_A, '/Prim2_A.renamedChildAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass': 0, 'fromPrim2': 0, 
            'fromImpliedClass': 0, 'fromPrim2_A': 0, 'fromImpliedClass2_A': 0})
       
        # Edit: Reparent and rename /Class.renamedChild to /Class.movedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Class.renamedChildAttr', '/ClassSibling.movedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : [],
            'layer2_A.usda' : [],
        })
        
        # On stage1 the class contents are updated with movedChildAttr being 
        # moved to /ClassSibling - a prim outside of /Class.  
        # This is effectively a delete of renamedChildAttr from /Prim as it no
        # longer ancestrally inherits or specializes at its new path. 
        self._VerifyStageContents(stage1, {
            'Class': {},
            'ClassSibling' : {
                '.' : ['movedChildAttr']
            },
            'Prim' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/Class.renamedChildAttr",
            "/Prim.renamedChildAttr",
            "/ClassSibling.movedChildAttr"
        ])

        # On stage2 the implied class is also updated for the move which 
        # manifests as a deletion of the specs for /Class.renamedChildAttr on 
        # layer2. The specs are deleted because the class arc to /Class doesn't 
        # propagate movedChildAttr as an ancestral implied class arc like it 
        # did with /Class.renamedChildAttr. Prim2's contents have changed to 
        # reflect the effective deletion of renamedChildAttr.
        # 
        # Similarly on stage2_A, the further implied class specs on layer2_A are
        # updated in the same way (i.e. deleted), and Prim2_A's contents have
        # changed to also reflect the full deletion of renamedChildAttr.
        self._VerifyStageContents(stage2, {
            'Class' : {},
            'Prim2' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Class.renamedChildAttr",
            "/Prim2.renamedChildAttr"
        ])

        self._VerifyStageContents(stage2_A, {
            'Class' : {},
            'Prim2_A' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage2_A, [
            "/Class.renamedChildAttr",
            "/Prim2_A.renamedChildAttr"
        ])
        
        # Verify movedChildAttr's metadata in layer 1. The other layers have had
        # their specs for renamedChildAttr deleted.
        self._VerifyDefaultAndCustomMetadata(stage1, '/ClassSibling.movedChildAttr', 
            "fromClass", {'fromClass' : 0})
        
        # Edit: Reparent and Rename /ClassSibling.movedChildAttr back to its 
        # original path /Class.childAttr.
        self.assertTrue(editor.MovePropertyAtPath(
            '/ClassSibling.movedChildAttr', '/Class.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : [],
            'layer2_A.usda' : [],
        })

        # On stage1 /Class itself is returned to its original contents from
        # the direct edit. 
        # 
        # For /Prim, it is returned to its original 
        # contents with the one notable exception: the overs on childAttr  
        # are NOT restored from being deleted when renamedChildAttr was moved 
        # out from being a descendant of the inherited class. 
        # We never restore deleted specs.
        self._VerifyStageContents(stage1, {
            'Class' : {
                '.' : ['childAttr']
            },
            'ClassSibling' : {},
            'Prim' : {
                '.' : ['childAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/Class.childAttr",
            "/ClassSibling.movedChildAttr",
            "/Prim.childAttr"
        ])

        # On stage2 the implied class specs are NOT updated as the deleted specs
        # for renamedChildAttr on layer2 from the prior move are not restored.
        #
        # Prim2's contents return to the original contents with the notable 
        # exception of metadata. Its local specs, implied specs in layer 2, 
        # and specs from /Prim in layer 1 were all deleted by the prior move and
        # cannot be restored.
        #
        # Similarly on stage2_A, the further implied class specs on layer2_A
        # cannot be restored from deletion, and Prim2_A's contents only been
        # partially restored in the same manner.
        self._VerifyStageContents(stage2, {
            'Class' : {},
            'Prim2' : {
                '.' : ['childAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Prim2.childAttr"
        ])

        self._VerifyStageContents(stage2_A, {
            'Class' : {},
            'Prim2_A' : {
                '.' : ['childAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2_A, [
            "/Prim2_A.childAttr"
        ])
        
        # Although childAttr is restored on /Prim from the inherits to layer 1's 
        # /Class, the specs for childAttr in layers 2 and 2_A are not restored 
        # and therefore thhe composed metadata only contains opinions from 
        # layer 1's /Class.childAttr.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Prim.childAttr', 
            "fromClass", {'fromClass' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim2.childAttr', 
            "fromClass", {'fromClass': 0})
        self._VerifyDefaultAndCustomMetadata(stage2_A, '/Prim2_A.childAttr', 
            "fromClass", {'fromClass': 0})
        
        # Reinitialize to reset the overs on childAttr for the next test case.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)
        layer2_A.ImportFromString(layer2_AImportString)

        # Check the composed property metadata of childAttr includes opinions
        # from all three layers again.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Prim.childAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass' : 0})
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim2.childAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass': 0, 'fromPrim2': 0, 
                          'fromImpliedClass': 0})
        self._VerifyDefaultAndCustomMetadata(stage2_A, '/Prim2_A.childAttr', 
            "fromClass", {'fromPrim': 0, 'fromClass': 0, 'fromPrim2': 0, 
            'fromImpliedClass': 0, 'fromPrim2_A': 0, 'fromImpliedClass2_A': 0})       

        # Edit: Delete /Class.childAttr
        self.assertTrue(editor.DeletePropertyAtPath('/Class.childAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [],
            'layer2.usda' : [],
            'layer2_A.usda' : [],
        })

        # Verify the updated stage contents for stage1. Note that the overs to 
        # childAttr on /Prim have been deleted as well which is why we don't 
        # end up with a reintroduction of a partially specced childAttr.
        self._VerifyStageContents(stage1, {
            'Class': {},
            'ClassSibling' : {},
            'Prim' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/Class.childAttr",
            "/Prim.childAttr"
        ])

        # On stage2 Prim2's contents have changed to reflect the deletion of 
        # childAttr, as have the implied class specs.
        # 
        # Similarly on stage2_A, Prim2_A's contents and the implied class 
        # contents have changed to also reflect the full deletion of childAttr.
        self._VerifyStageContents(stage2, {
            'Class' : {},
            'Prim2' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Class.childAttr",
            "/Prim2.childAttr"
        ])

        self._VerifyStageContents(stage2_A, {
            'Class' : {},
            'Prim2_A' : {},
        })
        self._VerifyStagePropertyResyncNotices(stage2_A, [
            "/Class.childAttr",
            "/Prim2_A.childAttr"
        ])
       
    def test_BasicDependentInherits(self):
        self._RunTestBasicDependentGlobalClassArcs("inherits")

    def test_BasicDependentSpecializes(self):
        self._RunTestBasicDependentGlobalClassArcs("specializes")

    def test_BasicDependentVariants(self):
        '''Tests downstream dependency namespace edits to a property across a 
        reference contained within a variant both when the variant is selected 
        and when it is not.
        '''
        # Setup: 
        # Layer1 just has a simple /Ref.childAttr, which is where the primary
        # edits will occur.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref" {
                string childAttr = "fromRef" (customData = {int "fromRef" = 0})
            }
        '''
        layer1.ImportFromString(layer1ImportString)
        
        # Layer2 sets up a variant. 
        # The prim /PropVariant has a variant set "propVariant" with one 
        # variant: "one" references layer1's /Ref and provides overs for it and 
        # its namespace descendants.
        # Then we have /Prim which references /PropVariant and provides local 
        # opinions for the contents of variant "one" in the form of metadata for 
        # childAttr. However, /Prim does NOT provide a variant selection here so 
        # its opinions would not normally compose with those of the variant when 
        # layer2 is opened as a stage itself.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            def "PropVariant" (
                variantSets = ["propVariant"]
            ) {
                variantSet "propVariant" = {
                    "one" (
                        references = @''' + layer1.identifier + '''@</Ref>
                    ) {
                        string childAttr (customData = {int "fromVariant" = 0})
                    }
                }
            }

            def "Prim" (
                references = </PropVariant>
            ) {
                string childAttr (customData = {int "fromPrim" = 0})
            }
         '''
        layer2.ImportFromString(layer2ImportString)
        
        # The session layer defines overs for /Prim that set the "propVariant" 
        # variant selection to "one".
        sessionLayer  = Sdf.Layer.CreateAnonymous("session.usda")
        sessionLayer.ImportFromString('''#usda 1.0
            over "Prim" (
                variants = {
                    string propVariant = "one"
                }
            ) {
            }
        ''')
        
        # Open both layer1 and layer2 as stages. Specifically layer2's stage is
        # opened with the session layer we just defined that provides the 
        # variant selections for layer2's prims.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, sessionLayer)

        # Create a namespace editor for the first stage with stage2 as a 
        # dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)
        
                
        # Verify the initial contents of stage1 which is just the simple /Ref 
        # and childAttr.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['childAttr'],
            },
        })
        
        # Verify the initial contents of stage2. 
        # /PropVariant is empty because it has no variant selection so its 
        # propVariant variants are not composed. 
        # /Prim references /PropVariant and the session layer sets its 
        # propVariant to "one" so its contents are composed opinions from the 
        # local /Prim specs, the /PropVariant{propVariant=one} variant, and 
        # the referenced opinions for /Ref in layer1 (brought in by the 
        # variant).
        self._VerifyStageContents(stage2, {
            'PropVariant' : {},
            'Prim': {
                '.' : ['childAttr'],
            },
        })
        
        # We use metadata to check that opinions from all layers compose correctly.
        # In stage 1, we get just opinions from /Ref as expected.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromRef", {'fromRef': 0})
        # In stage 2, we get the opinions from /Ref in layer 1 composed with 
        # local opinions from /Prim and variant "one"
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            "fromRef", {'fromRef': 0, 'fromPrim': 0, 'fromVariant': 0})
        
        # Edit: Rename /Ref.childAttr to /Ref.renamedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref.childAttr', '/Ref.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [('/Ref.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Prim.childAttr', 'renamedChildAttr')],
            'session.usda' : [],
        })
        
        # Verify the contents of stage1 are updated to reflect the simple rename
        # of childAttr to renamedChildAttr
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['renamedChildAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])
        
        # On stage2 the contents of /Prim have changed to reflect the full 
        # rename /Prim.childAttr to /Prim.renamedChildAttr as all specs that 
        # originally contributed to childAttr have been moved to renamedChildAttr. 
        self._VerifyStageContents(stage2, {
            'PropVariant' : {},
            'Prim': {
                '.' : ['renamedChildAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [])
        
        # We use metadata to check that opinions from all the layers compose 
        # correctly. In stage 1, we stil just get opinions from /Ref.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.renamedChildAttr', 
            "fromRef", {'fromRef': 0})
        # All the opinions from /Ref, /Prim, and the variant as still present 
        # since all specs that contribute have been updated to renamedChildAttr.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.renamedChildAttr', 
            "fromRef", {'fromRef': 0, 'fromPrim': 0, 'fromVariant': 0})
        
        # Edit: Delete /Ref.renamedChildAttr
        self.assertTrue(editor.DeletePropertyAtPath('/Ref.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
                'layer1.usda' : [],
                'layer2.usda' : [],
                'session.usda' : [],
            })

        # Verify the contents of stage1 are updated to reflect the deletion of 
        # /Ref.renamedChildAttr.
        self._VerifyStageContents(stage1, {
            'Ref': {},
        })
        self._VerifyStagePropertyResyncNotices(stage1, [
            "/Ref.renamedChildAttr"
        ])
        
        # On stage2 the contents of Prim have changed to reflect the full that
        # /Prim.renamedChildAttr has been fully deleted as all specs that 
        # originally contributed to it have bene deleted. 
        #
        self._VerifyStageContents(stage2, {
            'PropVariant' : {},
            'Prim': {},
        })
        self._VerifyStagePropertyResyncNotices(stage2, [
            "/Prim.renamedChildAttr"
        ])
        
        # Reset both layer1 and layer2 contents for the next test case and then
        # mute stage2's session layer. This will remove the variant selection
        # for stage2's /Prim so that we can test the effects of the same
        # namespace edits on variants that are not selected.
        with Sdf.ChangeBlock():
            layer2.ImportFromString(layer2ImportString)
            layer1.ImportFromString(layer1ImportString)
        stage2.MuteLayer(sessionLayer.identifier)
    
        # Verify the returned initial contents of stage1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['childAttr'],
                }
        })

        # Verify the contents of stage2. This time, because the session layer is
        # muted, there are no variant selections on any of the prims. So none of
        # the prims compose any opinions from any variants in /PropVariant and
        # therefore cannot compose any opinions from layer1 as the references to
        # layer1 are are defined in the variants.
        self._VerifyStageContents(stage2, {
            'PropVariant' : {},
            'Prim': {
                '.' : ['childAttr'],
            },
        })

        # We use metadata to check that opinions from all the layers compose 
        # correctly. In stage 1, we still get opinions from /Ref.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.childAttr', 
            "fromRef", {'fromRef': 0})
        # However, in stage 2, we no longer get opinions from the variant and as 
        # result none from /Ref either. We only see /Prim's local opinions.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            None, {'fromPrim': 0})
       
        # Edit: Rename /Ref.childAttr to /Ref.renamedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref.childAttr', '/Ref.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [('/Ref.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Prim.childAttr', 'renamedChildAttr')],
            'session.usda' : [],
        })
        
        # Verify the contents of stage1 are updated to reflect the simple
        # rename of childAttr to renamedChildAttr, same as before.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['renamedChildAttr'],
            },   
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])
        
        # Verify the contents of stage2 are completely unchanged as there are no
        # composed prim dependencies on the specs in layer1 without the variant
        # selections.
        self._VerifyStageContents(stage2, {
            'PropVariant' : {},
            'Prim': {
                '.' : ['childAttr'],
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, None)
        
        # In stage 1, /Ref.renamedChildAttr keeps its metadata.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.renamedChildAttr', 
            "fromRef", {'fromRef': 0})

        # In stage 2, we do not have opinions for childAttr from layer 1, so 
        # /Prim.childAttr remains unchanged.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            None, {'fromPrim': 0})

        # Unmute the session layer on stage2 which reapplies the variant
        # selection in composition.
        stage2.UnmuteLayer(sessionLayer.identifier)

        # Verify the contents of post-edit stage2 with the variant selections
        # applied again. The results are different from the results from when
        # the same edit was performed with the variant selections active. 
        # Specifically: for /Prim, the specs from across the reference to 
        # layer1's /Ref bring in renamedChildAttr, but the local spec and
        # variant specs still bring in childAttr as a different property since
        # those specs could not be updated without the active variant selection.
        #
        # This all demonstrates how we do not attempt to fix namespace edited
        # paths in variants that aren't currently selected.
        self._VerifyStageContents(stage2, {
            'PropVariant' : {},
            'Prim': {
                '.' : ['childAttr', 'renamedChildAttr'],
            },
        })
        
        # Stage 1 is unchanged from before we unmuted the session layer.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref.renamedChildAttr', 
            "fromRef", {'fromRef': 0})
        # In stage 2, childAttr gets opinions from the variant and from /Prim.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            None, {'fromPrim': 0, 'fromVariant': 0})
        # In stage 2, we now get renamedChildAttr with only opinions from /Ref. 
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.renamedChildAttr', 
            "fromRef", {'fromRef': 0})
        
    def test_SiblingReferenceWithSameHierarchy(self):
        """Test downstream dependency name space edits across basic references,
        when there is a namespace conflict with a sibling composition arc."""

        # Layer1 has two prims, /Ref1 and /Ref2, with the same basic property
        # childAttr but different defaults and metadata so we can tell which 
        # specs are contributing when both prims are referenced.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref1" {
                string childAttr = "fromRef1" (customData = {int fromRef1 = 0})

            }

            def "Ref2" {
                string childAttr = "fromRef2" (customData = {int fromRef2 = 0})
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer2 just has /Prim, which references both /Ref1 and /Ref2 and 
        # provides local opinions for childAttr.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0

            def "Prim" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1>,
                    @''' + layer1.identifier + '''@</Ref2>
                ]
            ) {
                string childAttr (customData = {int fromPrim = 0})
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # We'll edit the first stage with stage2 as a dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)
        
        # Verify initial contents of stage 1.
        self._VerifyStageContents(stage1, {
            'Ref1' : {
                '.' : ['childAttr']
            },
            'Ref2' : {
                '.' : ['childAttr']
            },
        })
        
        # Verify the initial contents of stage2. 
        # /Prim is composed from specs under /Ref1, /Ref2, and local specs.
        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['childAttr']
            },
        })
        
        # We use metadata to check that opinions from all the layers compose correctly.
        # In stage 1, /Ref1 only has local opinions.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref1.childAttr', 
            "fromRef1", {'fromRef1': 0})
        # /Ref2 only has local opinions as well.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref2.childAttr', 
            "fromRef2", {'fromRef2': 0})
        # In stage 2, we get the opinions from /Ref1 and /Ref 2 in layer 1 
        # composed with local opinions from /Prim.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            "fromRef1", {'fromPrim': 0, 'fromRef1': 0, 'fromRef2': 0})

        # Edit: Rename /Ref1.childAttr to /Ref1.renamedChildAttr
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref1.childAttr', '/Ref1.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [('/Ref1.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Prim.childAttr', 'renamedChildAttr')],
        }, expectWarnings = True)
        
        # Verify on stage1 that just /Ref1.childAttr is renamed.
        self._VerifyStageContents(stage1, {
            'Ref1' : {
                '.' : ['renamedChildAttr']
            },
            'Ref2' : {
                '.' : ['childAttr']
            }
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])
        
        # On stage2, the contents of /Prim change to reflect that the reference
        # to /Ref1 brings in renamedChildAttr instead of childAttr. However, the 
        # reference to /Ref2 still brings in childAttr. We do not update sibling 
        # arcs and do not author relocates to move sibling arcs when processing 
        # downstream dependencies. Additionally, we do NOT move the local specs 
        # for childAttr in layer2 because we've chosen to leave them attached to 
        # the remaining childAtttr property. Thus we have the split here in 
        # stage2 where /Prim.childAttr will compose opinions from /Ref2 and the 
        # local layer stack, while /Prim.renamedChildAttr will only have 
        # opinions from /Ref1.
        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['childAttr', 'renamedChildAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [])
        
        # We use metadata to check that opinions from all the layers compose correctly.
        # In stage 1, /Ref1.childAttr has been renamed to renamedChildAttr.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref1.renamedChildAttr', 
            "fromRef1", {'fromRef1': 0})
        # /Ref2.childAttr is unchanged.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref2.childAttr', 
            "fromRef2", {'fromRef2': 0})
        # In stage 2, /Prim.childAttr keeps the local opinions and opinions from /Ref2.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.childAttr', 
            "fromRef2", {'fromPrim': 0, 'fromRef2': 0})
        # /Prim.renamedChildAttr only has metadata from /Ref1.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.renamedChildAttr', 
            "fromRef1", {'fromRef1': 0})
         
        # Edit: Now rename /Ref2.childAttr to be /Ref2.renamedChildAttr to match the
        # namespace hierarchy of /Ref1 again. Unlike the previous edit, we do
        # not expect any warnings.
        self.assertTrue(editor.MovePropertyAtPath(
            '/Ref2.childAttr', '/Ref2.renamedChildAttr'))
        self._ApplyEditWithVerification(editor,
            expectedObjectsChangedRenamedProperties = {
            'layer1.usda' : [('/Ref2.childAttr', 'renamedChildAttr')],
            'layer2.usda' : [('/Prim.childAttr', 'renamedChildAttr')],
        }, expectWarnings = False)

        # Verify on stage1 that /Ref2.childAttr has been renamed.
        self._VerifyStageContents(stage1, {
            'Ref1' : {
                '.' : ['renamedChildAttr']
            },
            'Ref2' : {
                '.' : ['renamedChildAttr']
            }
        })
        self._VerifyStagePropertyResyncNotices(stage1, [])
        
        # On stage2, the contents of /Prim change to reflect that the reference
        # to /Ref2 brings in renamedChildAttr instead of childAttr. This now 
        # "remerges" the contents of childAttr and renamedChildAttr into the 
        # single property renamedChildAttr since both references now provide 
        # specs for renamedChildAttr. Note that this time we DO move the local 
        # specs for /Prim.childAttr to /Prim.renamedChildAttr because 
        # /Prim.childAttr has no more ancestral opinions from other arcs and has 
        # been officially moved. Thus, in two steps, we have moved the entire 
        # original contents of /Prim.childAttr to /Prim.renamedChildAttr.
        self._VerifyStageContents(stage2, {
            'Prim' : {
                '.' : ['renamedChildAttr']
            },
        })
        self._VerifyStagePropertyResyncNotices(stage2, [])
        
        # We use metadata to check that opinions from all the layers compose correctly.
        # In stage 1, /Ref1.renamedChildAttr is unchanged.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref1.renamedChildAttr', 
            "fromRef1", {'fromRef1': 0})
        # /Ref2.childAttr has been renamed to renamedChildAttr.
        self._VerifyDefaultAndCustomMetadata(stage1, '/Ref2.renamedChildAttr', 
            "fromRef2", {'fromRef2': 0})
        # In stage 2, we get all the composed metadata from /Ref1, /Ref2, and
        # local opinions from /Prim in renamedChildAttr.
        self._VerifyDefaultAndCustomMetadata(stage2, '/Prim.renamedChildAttr', 
            "fromRef1", {'fromPrim': 0, 'fromRef1': 0, 'fromRef2' : 0})

if __name__ == '__main__':
    unittest.main()