#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Sdf, Usd
from testUsdNamespaceEditorDependentEditsBase \
      import TestUsdNamespaceEditorDependentEditsBase

class TestUsdNamespaceEditorDependentEditsBasicRelocates(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency namespace edits across relocates.
    '''

    def test_BasicRelocatedReferenceChildren(self):
        '''Tests downstream dependency namespace edits across a reference where
        a child of the referencing prim is then relocated.
        '''

        # Setup: 
        # Layer1 is the layer of the stage to edited and has a simple hierarchy
        # of /World/Ref/Child/GrandChild that will be referenced by various
        # prims in the next layer.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "World" {
                int worldAttr
                                
                def "Ref" {
                    int refAttr
                    
                    def "Child" {
                        int childAttr
                
                        def "GrandChild" {
                            int grandChildAttr
                        }
                    }    
                }
            }
        ''')

        # Layer2 has three prims that reference /World, /World/Ref, and 
        # /World/Ref/Child respectively. Also for each, there's a relocates that
        # moves the next immediate namespace child of each of those prims to
        # /Relocated1, /Relocated2, and /Relocated3 respectively and provides
        # local opinions at those post-relocates locations.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            (
                relocates = {
                    </Prim1/Ref> : </Relocated1>,
                    </Prim2/Child> : </Relocated2>,
                    </Prim3/GrandChild> : </Relocated3>
                }
            )

            def "Prim1" (
                references = @''' + layer1.identifier + '''@</World>
            ) {
                def "LocalPreReloChild" {}
                int localPreReloAttr
            }

            def "Prim2" (
                references = @''' + layer1.identifier + '''@</World/Ref>
            ) {
                def "LocalPreReloChild" {}
                int localPreReloAttr
            }

            def "Prim3" (
                references = @''' + layer1.identifier + '''@</World/Ref/Child>
            ) {
                def "LocalPreReloChild" {}
                int localPreReloAttr
            }

            def "Relocated1"
            {
                over "Child" {
                    int overChildAttr
                    over "GrandChild" {
                        int overGrandChildAttr
                    }
                }
                def "LocalPostReloChild" {}
                int localPostReloAttr
            }

            def "Relocated2" 
            {
                over "GrandChild" {
                    int overGrandChildAttr
                }
                def "LocalPostReloChild" {}
                int localPostReloAttr
            }

            def "Relocated3" 
            {
                def "LocalPostReloChild" {}
                int localPostReloAttr
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

        # Verify initial composition fields
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim1/Ref', '/Relocated1'), 
                              ('/Prim2/Child', '/Relocated2'), 
                              ('/Prim3/GrandChild', '/Relocated3')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/Ref/Child'),)
            },
        })

        # Verify initial contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                    'Child' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                        }           
                    }
                }
            }
        })

        # Verify initial contents of stage 2

        # Initial contents of the pre-relocation prims. The child from across
        # the reference for each prim is a relocation source so they are not 
        # children of these prims in the composed stage.
        prim1Contents = {
            '.' : ['worldAttr', 'localPreReloAttr'],
            'LocalPreReloChild' : {},
        }

        prim2Contents = {
            '.' : ['refAttr', 'localPreReloAttr'],
            'LocalPreReloChild' : {},
        }

        prim3Contents = {
            '.' : ['childAttr', 'localPreReloAttr'],
            'LocalPreReloChild' : {},
        }

        # Initial contents of the post relocation prims. These prims compose the
        # relocation source opinions from across their ancestral references with
        # the local opinions at the relocation's target path.
        relocated1Contents = {
            '.' : ['refAttr', 'localPostReloAttr'],
            'Child' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr'],
                },
            },
            'LocalPostReloChild' : {}
        }

        relocated2Contents = {
            '.' : ['childAttr', 'localPostReloAttr'],
            'GrandChild' : {
                '.' : [ 'grandChildAttr', 'overGrandChildAttr'],
            },
            'LocalPostReloChild' : {}
        }

        relocated3Contents = {
            '.' : ['grandChildAttr', 'localPostReloAttr'],
            'LocalPostReloChild' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Relocated1' : relocated1Contents,
            'Relocated2' : relocated2Contents,
            'Relocated3' : relocated3Contents
        })

        # Edit: Rename /World/Ref/Child to /World/Ref/RenamedChild
        with self.ApplyEdits(editor, 
                "Move /World/Ref/Child -> /World/Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/Ref/Child', '/World/Ref/RenamedChild'))

        # Verify the updated composition fields in layer2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            # /Prim2's reference to /World/Ref means that the composed 
            # /Prim2/Child would be renamed to /Prim2/RenamedChild on the 
            # referencing side. The direct relocation of this child of /Prim2 
            # is maintained by updating the source of the relocation to be
            # /Prim2/RenamedChild 
            '/': {
                'relocates': [('/Prim1/Ref', '/Relocated1'), 
                              ('/Prim2/RenamedChild', '/Relocated2'), 
                              ('/Prim3/GrandChild', '/Relocated3')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            # /Prim3 originally referenced /World/Ref/Child directly so the
            # reference is updated to the new path.
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/Ref/RenamedChild'),)
            },
        })

        # Verify the rename of Child to RenamedChild on stage1.
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                    'RenamedChild' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                        }           
                    }
                }
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/Ref/Child" : self.PrimResyncType.RenameSource,
            "/World/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # On stage2, none of the contents of the pre-relocation prims have 
        # changed but each for different reasons. 
        # - Prim1's contents don't change because the contents that would've
        #   changed are relocated to /Relocated1 and reflected there.
        # - Prim2's contents don't change because the relocates were updated
        #   to continue to relocate RenamedChild out of Prim2
        # - Prim3's contents don't change because its reference was updated to
        #   refer to the renamed prim. 
        #
        # As expected, /Relocated1's contents change to reflect the rename of
        # Child to RenamedChild (the specs at /Relocated1/Child are moved to 
        # /Relocated1/RenamedChild for the rename). The contents of /Relocated2
        # and /Relocated3 are unchanged as the composition field changes
        # maintain their same contents.
        relocated1Contents = {
            '.' : ['refAttr', 'localPostReloAttr'],
            'RenamedChild' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['overGrandChildAttr', 'grandChildAttr'],
                },
            },
            'LocalPostReloChild' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Relocated1' : relocated1Contents,
            'Relocated2' : relocated2Contents,
            'Relocated3' : relocated3Contents
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Relocated1/Child" : self.PrimResyncType.RenameSource,
            "/Relocated1/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Relocated2" : self.PrimResyncType.UnchangedPrimStack,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
            "/Relocated3" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Prim2/Child" : self.PrimResyncType.Delete,
            "/Prim2/RenamedChild" : self.PrimResyncType.Delete,
        })

        # Edit: Reparent and rename /World/Ref/RenamedChild to /World/MovedChild
        with self.ApplyEdits(editor, 
                "Move /World/Ref/RenamedChild -> /World/MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/Ref/RenamedChild', '/World/MovedChild'))

        # Verify the updated composition fields in layer2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                # /Prim2's reference to /World/Ref means that the composed path
                # /Prim2/RenamedChild no longer exists since the new path 
                # /World/MovedChild is not a descendant of /World/Ref and cannot
                # be mapped across that reference. This effective deletion of
                # /Prim2/RenamedChild causes us to have to delete the relocation
                # from /Prim/RenamedChild to /Relocated2 since there is no valid
                # source path to update it with.
                'relocates': [('/Prim1/Ref', '/Relocated1'), 
                              ('/Prim3/GrandChild', '/Relocated3')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            # In /Prim3, the direct reference to /World/Ref/RenamedChild can be
            # and is updated to the new path.
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/MovedChild'),)
            },
        })

        # Verify the reparent and rename of RenamedChild /World/MovedChild on
        # stage1.
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                },
                'MovedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/Ref/RenamedChild" : self.PrimResyncType.RenameAndReparentSource,
            "/World/MovedChild" : self.PrimResyncType.RenameAndReparentDestination,
        })

        # On stage2, the pre-relocation prim /Prim1 now has the child prim 
        # MovedChild (with its ancestral reference opinions) because MovedChild
        # is not a child of /Prim1/Ref, and as such is not ancestrally relocated
        # via the relocation of /Prim1/Ref to /Relocated1. Similarly, 
        # /Relocated1 no longer has the child prim RenamedChild since it is no
        # longer a descendant of the relocation source.
        #
        # The contents of /Relocated2 have lost the referenced opinions from
        # /Prim2/RenamedChild because there is no relocation anymore. But unlike
        # /Prim1, /Prim2 remains unchanged because even though its child is no
        # longer relocated away, the child prim no longer exists across the
        # reference and is not composed as a child of /Prim2 anyway.
        #
        # the contents of /Prim3 and /Relocated3 remain unchanged because the
        # compositon arcs were able to be updated to maintain the same composed
        # specs.
        #
        prim1Contents = {
            '.' : ['worldAttr', 'localPreReloAttr'],
            'LocalPreReloChild' : {},
            'MovedChild' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr'],
                },
            },
        }

        relocated1Contents = {
            '.' : ['refAttr', 'localPostReloAttr'],
            'LocalPostReloChild' : {}
        }

        relocated2Contents = {
            '.' : ['localPostReloAttr'],
            'LocalPostReloChild' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Relocated1' : relocated1Contents,
            'Relocated2' : relocated2Contents,
            'Relocated3' : relocated3Contents
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Relocated1/RenamedChild" : self.PrimResyncType.RenameAndReparentSource,
            "/Prim1/MovedChild" : self.PrimResyncType.RenameAndReparentDestination,
            "/Relocated2" : self.PrimResyncType.Other,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
            "/Relocated3" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Prim2/RenamedChild" : self.PrimResyncType.Delete,
        })

        # Edit: Move /World/MovedChild back to its original path 
        # /World/Ref/Child.
        with self.ApplyEdits(editor, 
                "Move /World/MovedChild -> /World/Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/MovedChild', '/World/Ref/Child'))

        # Verify the updated composition fields in layer2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            # The relocates are not updated this time since the only relocate 
            # that has been affected by moving this particular prim around was
            # deleted in the last edit when the relocation source was caused to
            # not exist. We don't store any state that would allow this relocate
            # to be restored in this event.
            '/': {
                'relocates': [('/Prim1/Ref', '/Relocated1'), 
                              ('/Prim3/GrandChild', '/Relocated3')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            # The move updates the direct reference in /Prim3 back to the 
            # initial path /World/Ref/Child
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/World/Ref/Child'),)
            },
        })

        # Verify stage1; it's back to its original contents.
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                    'Child' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                        }           
                    }
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/MovedChild" : self.PrimResyncType.RenameAndReparentSource,
            "/World/Ref/Child" : self.PrimResyncType.RenameAndReparentDestination,
        })

        # On stage2, /Prim1 now again has its child contents relocated away 
        # since Child is back to being a child of /Prim1/Ref, and as such is
        # ancestrally relocated via the relocation from /Prim1/Ref to 
        # /Relocated1. And similarly, /Relocated1 now has the child prim Child
        # since it is a descendant of the relocation source. This matches the
        # initial state of /Prim1 and /Relocated1
        #
        # The contents of /Relocated2 have not changed from the previous case 
        # because we have not restored the deleted relocate. However, at the 
        # "pre-relocation" source path, /Prim2 now has the contents of 
        # /World/Ref/Child composed into /Prim2/Child because it exists across
        # the ancestral reference but is no longer relocated. These prims do not
        # match the initial state of the stage.
        #
        # the contents of /Prim3 and /Relocated3 remain unchanged again because
        # the compositon arcs were able to be updated to maintain the same
        # composed specs.
        #
        prim1Contents = {
            '.' : ['worldAttr', 'localPreReloAttr'],
            'LocalPreReloChild' : {},
        }

        prim2Contents = {
            '.' : ['refAttr', 'localPreReloAttr'],
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr'],
                },
            },
            'LocalPreReloChild' : {},
        }

        relocated1Contents = {
            '.' : ['refAttr', 'localPostReloAttr'],
            'Child' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr'],
                },
            },
            'LocalPostReloChild' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Relocated1' : relocated1Contents,
            'Relocated2' : relocated2Contents,
            'Relocated3' : relocated3Contents
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim1/MovedChild" : self.PrimResyncType.RenameAndReparentSource,
            "/Relocated1/Child" : self.PrimResyncType.RenameAndReparentDestination,
            "/Prim2/Child" : self.PrimResyncType.Other,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
            "/Relocated3" : self.PrimResyncType.UnchangedPrimStack,
        })
            
        # Reinitialize layer2 to reset /Prim2 and /Relocated2 for the next test
        # case.
        layer2.ImportFromString(layer2ImportString)

        # Verify the composition field of layer2 have been restored to their
        # original state.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim1/Ref', '/Relocated1'), 
                              ('/Prim2/Child', '/Relocated2'),
                              ('/Prim3/GrandChild', '/Relocated3')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/Ref/Child'),)
            },
        })

        # Verify the contents of stage2 have all been restored to the original
        # state.
        prim2Contents = {
            '.' : ['refAttr', 'localPreReloAttr'],
            'LocalPreReloChild' : {},
        }

        relocated2Contents = {
            '.' : ['childAttr', 'localPostReloAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'overGrandChildAttr'],
            },
            'LocalPostReloChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Relocated1' : relocated1Contents,
            'Relocated2' : relocated2Contents,
            'Relocated3' : relocated3Contents
        })

        # Edit: Delete /World/Ref/Child
        with self.ApplyEdits(editor, "Delete /World/Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/World/Ref/Child'))

        # Verify the updated composition fields in layer2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            # /Prim2's reference to /World/Ref means that /Prim2/Child no longer
            # exists and thus the ancestral reference to /World/Ref/Child
            # is deleted. /Prim3's deleted reference means that its child from
            # across the reference, GrandChild, also no longer exists. These
            # deletions cause us to have to delete both the relocations of 
            # /Prim2/Child and /Prim3/GrandChild.
            '/': {
                'relocates': [('/Prim1/Ref', '/Relocated1')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            # The direct reference to /World/Ref/RenamedChild in /Prim3 is
            # deleted now that the prim is deleted.
            '/Prim3' : {
                'references' : ()
            },
        })

        # Verify the deletion of Child on stage1.
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/Ref/Child" : self.PrimResyncType.Delete,
        })

        # On stage2, /Prim1's contents haven't changed because its child is
        # relocated, but /Relocated1 no longer has child prim "Child" as it was
        # deleted.
        # 
        # /Prim2's contents haven't changed because it was relocated as well. 
        # But /Relocated2 no longer has contents other than its local opinions
        # because of the deleted relocation.
        #
        # /Prim3 and /Relocated3 have changed because both of their affecting
        # composition arcs (the reference for Prim3 and the relocation for 
        # Relocated3) were deleted.
        prim3Contents = {
            '.' : ['localPreReloAttr'],
            'LocalPreReloChild' : {},
        }

        relocated1Contents = {
            '.' : ['refAttr', 'localPostReloAttr'],
            'LocalPostReloChild' : {}
        }

        relocated2Contents = {
            '.' : ['localPostReloAttr'],
            'LocalPostReloChild' : {},
        }

        relocated3Contents = {
            '.' : ['localPostReloAttr'],
            'LocalPostReloChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Relocated1' : relocated1Contents,
            'Relocated2' : relocated2Contents,
            'Relocated3' : relocated3Contents
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Relocated1/Child" : self.PrimResyncType.Delete,
            "/Relocated2" : self.PrimResyncType.Other,
            "/Prim3" : self.PrimResyncType.Other,
            "/Relocated3" : self.PrimResyncType.Other,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Prim2/Child" : self.PrimResyncType.Delete,
        })

    def test_NestedRelocates(self):
        '''Tests downstream dependency namespace edits across a reference where
        all descendants of the referencing prim are relocated through nested
        relocates.
        '''

        # Setup: 
        # The layer to be referenced has a simple hierarchy of 
        # /World/Ref/Child/GrandChild/Foo. ChildSibling is an empty sibling of
        # Child to use as a parent for some of the reparenting cases below.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "World" {
                int worldAttr

                def "Ref" {
                    int refAttr
                    
                    def "Child" {
                        int childAttr
                
                        def "GrandChild" {
                            int grandChildAttr
                            
                            def "Foo" {
                                int fooAttr
                            }
                        }
                    }
                    
                    def "ChildSibling" {
                    }
                }
            }
        ''')

        # Layer2 has a prim that references /World in layer1 and then 
        # relocates each of Ref, Child, and GrandChild to be individual root
        # prims (essentially flattening most of the hierarchy). These relocates
        # are nested as per the requirements for how to express valid 
        # relocations of descendants of other relocated prims.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            (
                relocates = {
                    </Prim/Ref> : </Ref>,
                    </Ref/Child> : </Child>,
                    </Child/GrandChild> : </GrandChild>
                }
            )

            def "Prim" (
                references = @''' + layer1.identifier + '''@</World>
            ) {
                def "LocalChild" {}
                int locatAttr
            }

            over "Ref"
            {
                def "LocalChild" {}
                int localAttr
            }

            over "Child" 
            {
                def "LocalChild" {}
                int localAttr
            }

            over "GrandChild" 
            {
                def "LocalChild" {}
                int localAttr
                
                over "Foo" {
                    int overFooAttr
                }
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage1
        editor = Usd.NamespaceEditor(stage1)

        # Add stage2 as a dependent stage.
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/Ref', '/Ref'), 
                              ('/Ref/Child', '/Child'), 
                              ('/Child/GrandChild', '/GrandChild')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify initial contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                    'Child' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                            'Foo' : {
                                '.' : ['fooAttr']
                            }
                        }           
                    },
                    'ChildSibling' : {}
                }
            }
        })

        # Verify initial contents of stage 2 where the Ref, Child, and 
        # GrandChild descendants in the hierarchy are each relocated to be a 
        # root prim.
        primContents = {
            '.' : ['worldAttr', 'locatAttr'],
            'LocalChild' : {},
        }

        refContents = {
            '.' : ['refAttr', 'localAttr'],
            'LocalChild' : {},
            'ChildSibling' : {}
        }

        childContents = {
            '.' : ['childAttr', 'localAttr'],
            'LocalChild' : {},
        }

        grandChildContents = {
            '.' : ['grandChildAttr', 'localAttr'],
            'LocalChild' : {},
            'Foo' : {
                '.' : ['fooAttr', 'overFooAttr']
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
            
        # Edit: Rename /World/Ref/Child to /World/Ref/RenamedChild
        with self.ApplyEdits(editor, 
                "Move /World/Ref/Child -> /World/Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/Ref/Child', '/World/Ref/RenamedChild'))

        # Verify the composition fields on layer2
        # The rename of /World/Ref/Child propagates across the ancestral 
        # reference from /World to /Prim and then again across the relocate from
        # /Prim/Ref to /Ref, so we have to update the source of the relocate
        # from /Ref/Child to /Child to be from /Ref/RenamedChild instead.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/Ref', '/Ref'), 
                              ('/Ref/RenamedChild', '/Child'), 
                              ('/Child/GrandChild', '/GrandChild')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify the rename in the contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'Ref': {
                    '.' : ['refAttr'],
                    'RenamedChild' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                            'Foo' : {
                                '.' : ['fooAttr']
                            }
                        }           
                    },
                    'ChildSibling' : {}
                }
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/Ref/Child" : self.PrimResyncType.RenameSource,
            "/World/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # Verify the contents of stage2 have not changed as the update of the
        # relocates maintains the same composed prims.
        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Child" : self.PrimResyncType.UnchangedPrimStack,
            "/GrandChild" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Ref/Child" : self.PrimResyncType.Delete,
            "/Ref/RenamedChild" : self.PrimResyncType.Delete,
        })

        # Edit: Rename /World/Ref to /World/RenamedRef
        with self.ApplyEdits(editor, 
                "Move /World/Ref -> /World/RenamedRef"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/Ref', '/World/RenamedRef'))

        # Verify the composition fields on layer2
        # This is just a rename of a more ancestral prim than the prior edit. It
        # propagates across the ancestral reference from /World to /Prim, so we
        # only have to update the source of the relocate from /Prim/Ref to /Ref
        # to be from /Prim/RenamedRef instead.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/RenamedRef', '/Ref'), 
                              ('/Ref/RenamedChild', '/Child'), 
                              ('/Child/GrandChild', '/GrandChild')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify the rename in the contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'RenamedRef': {
                    '.' : ['refAttr'],
                    'RenamedChild' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                            'Foo' : {
                                '.' : ['fooAttr']
                            }
                        }           
                    },
                    'ChildSibling' : {}
                }
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/Ref" : self.PrimResyncType.RenameSource,
            "/World/RenamedRef" : self.PrimResyncType.RenameDestination,
        })

        # Verify the contents of stage2 have again not changed as the update of 
        # the relocates maintains the same composed prims.
        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Ref" : self.PrimResyncType.UnchangedPrimStack,
            "/Child" : self.PrimResyncType.UnchangedPrimStack,
            "/GrandChild" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Prim/Ref" : self.PrimResyncType.Delete,
            "/Prim/RenamedRef" : self.PrimResyncType.Delete,
        })

        # Edit: Reparent /World/RenamedRef/RenamedChild to be a child of 
        # /World/RenamedRef/ChildSibling
        with self.ApplyEdits(editor, 
                "Move /World/RenamedRef/RenamedChild -> "
                "/World/RenamedRef/ChildSibling/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/RenamedRef/RenamedChild', 
                '/World/RenamedRef/ChildSibling/RenamedChild'))

        # Verify the composition fields on layer2
        # Because this reparent causes RenamedChild to still be a descendant of
        # RenamedRef, the changed path can still propagate across the reference
        # and the first relocate, so we just have to update the source of the
        # relocate from /Ref/RenamedChild to /Child to be from
        # /Ref/ChildSibling/RenamedChild instead.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/RenamedRef', '/Ref'), 
                              ('/Ref/ChildSibling/RenamedChild', '/Child'), 
                              ('/Child/GrandChild', '/GrandChild')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify the reparent in the contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'RenamedRef': {
                    '.' : ['refAttr'],
                    'ChildSibling' : {
                        'RenamedChild' : {
                            '.' : ['childAttr'],
                            'GrandChild' : {
                                '.' : ['grandChildAttr'],
                                'Foo' : {
                                    '.' : ['fooAttr']
                                }
                            }           
                        }
                    }
                }
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/RenamedRef/RenamedChild" : 
                self.PrimResyncType.ReparentSource,
            "/World/RenamedRef/ChildSibling/RenamedChild" : 
                self.PrimResyncType.ReparentDestination,
        })

        # Verify the contents of stage2 have not changed as the update of the
        # relocates maintains the same composition.
        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Child" : self.PrimResyncType.UnchangedPrimStack,
            "/GrandChild" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Ref/RenamedChild" : self.PrimResyncType.Delete,
            "/Ref/ChildSibling/RenamedChild" : self.PrimResyncType.Delete,
        })

        # Rename the leaf prim Foo to Bar
        with self.ApplyEdits(editor, 
                "Move /World/RenamedRef/ChildSibling/RenamedChild/GrandChild/Foo "
                "-> /World/RenamedRef/ChildSibling/RenamedChild/GrandChild/Bar"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/RenamedRef/ChildSibling/RenamedChild/GrandChild/Foo', 
                '/World/RenamedRef/ChildSibling/RenamedChild/GrandChild/Bar'))

        # Verify the composition fields on layer2
        # None of the composition fields change as this a descendant of all the
        # prim paths involved in composition.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/RenamedRef', '/Ref'), 
                              ('/Ref/ChildSibling/RenamedChild', '/Child'), 
                              ('/Child/GrandChild', '/GrandChild')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify the rename in the contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'RenamedRef': {
                    '.' : ['refAttr'],
                    'ChildSibling' : {
                        'RenamedChild' : {
                            '.' : ['childAttr'],
                            'GrandChild' : {
                                '.' : ['grandChildAttr'],
                                'Bar' : {
                                    '.' : ['fooAttr']
                                }
                            }           
                        }
                    }
                }
            }
        })

        # This time the contents of /GrandChild has changed to account for the
        # rename of Foo to Bar. This includes the change of the directly edited
        # spec in layer1 (which brings .fooAttr) as well as the move of the
        # dependent spec in layer2 (across a reference and 3 relocates) of 
        # /GrandChild/Foo to /GrandChild/Bar (which brings in 'overFooAttr')
        grandChildContents = {
            '.' : ['grandChildAttr', 'localAttr'],
            'LocalChild' : {},
            'Bar' : {
                '.' : ['fooAttr', 'overFooAttr']
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/GrandChild/Foo" : self.PrimResyncType.RenameSource,
            "/GrandChild/Bar" : self.PrimResyncType.RenameDestination,
        })

        # Edit: Reparent ChildSibling to now be a child of /World instead of
        # /World/RenamedRef.
        with self.ApplyEdits(editor, 
                "Move /World/RenamedRef/ChildSibling -> /World/ChildSibling"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/RenamedRef/ChildSibling', '/World/ChildSibling'))

        # Verify the composition fields on layer2
        # This reparent moves ChildSibling (and therefore RenamedChild) so that
        # it is no longer a descendant of RenamedRef. Therefore it is no longer
        # affected by the relocate from /Prim/RenamedRef to /Ref. However, since
        # RenamedChild is still a descendant of /World and maps across the
        # ancestral reference to /Prim, we can still adjust the relocation of 
        # [/Ref/ChildSibling/RenamedChild -> /Child] so that its source is
        # /Prim/ChildSibling/RenamedChild to maintain the composition of /Child 
        # depending on RenamedChild. Note that the relocation to /Child is now
        # no longer a nested relocate in relation to the relocation to /Ref.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/RenamedRef', '/Ref'), 
                              ('/Prim/ChildSibling/RenamedChild', '/Child'), 
                              ('/Child/GrandChild', '/GrandChild')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify the reparent in the contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'RenamedRef': {
                    '.' : ['refAttr'],
                },
                'ChildSibling' : {
                    'RenamedChild' : {
                        '.' : ['childAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr'],
                            'Bar' : {
                                '.' : ['fooAttr']
                            }
                        }           
                    }
                }
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/RenamedRef/ChildSibling" : self.PrimResyncType.ReparentSource,
            "/World/ChildSibling" : self.PrimResyncType.ReparentDestination,
        })

        # There are small changes to the contents of /Prim and /Ref as 
        # ChildSibling is now a child of /Prim instead of /Ref via ancestral
        # reference. Note that ChildSibling does not have Child as a child prim
        # because it is still relocated away. Outside of that, no other contents
        # of stage 2 have changed.
        primContents = {
            '.' : ['worldAttr', 'locatAttr'],
            'LocalChild' : {},
            'ChildSibling' : {}
        }

        refContents = {
            '.' : ['refAttr', 'localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Ref/ChildSibling" : self.PrimResyncType.ReparentSource,
            "/Prim/ChildSibling" : self.PrimResyncType.ReparentDestination,
            "/Child" : self.PrimResyncType.UnchangedPrimStack,
            "/GrandChild" : self.PrimResyncType.UnchangedPrimStack,
        })

        # Edit: Delete the prim /World/ChildSibling
        with self.ApplyEdits(editor, "Delete /World/ChildSibling"):
            self.assertTrue(editor.DeletePrimAtPath('/World/ChildSibling'))

        # Verify the composition fields on layer2
        # This deletion causes Child and GrandChild to be deleted so any
        # relocates (nested or otherwise) that originate from those specs are no
        # longer valid and get deleted, thus only the single relocate to /Ref
        # remains.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [('/Prim/RenamedRef', '/Ref')]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
        })

        # Verify the post-deletion contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                '.' : ['worldAttr'],
                'RenamedRef': {
                    '.' : ['refAttr'],
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World/ChildSibling" : self.PrimResyncType.Delete,
        })

        # On stage2 /Prim has changed to no longer have the deleted ChildSibling 
        primContents = {
            '.' : ['worldAttr', 'locatAttr'],
            'LocalChild' : {},
        }

        # /Child's contents have changed to no longer have the relocated 
        # reference contents. Note that the local specs for /Child have not been
        # deleted (which is why it still exists).
        childContents = {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        # /GrandChild contents have changed to no longer have the relocated
        # reference contents. Note that the local specs for /GrandChild have
        # not been deleted (which is why it still exists), but the local specs
        # for /GrandChild/Bar HAVE been deleted because Bar previously had specs
        # that were composed from across the relocated ancestral reference.
        #
        # XXX Does this make sense that the spec overring an ancestral
        # dependency gets deleted when the spec overring the direct dependency
        # does not?
        grandChildContents = {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'Ref' : refContents,
            'Child' : childContents,
            'GrandChild' : grandChildContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim/ChildSibling" : self.PrimResyncType.Delete,
            "/Child" : self.PrimResyncType.Other,
            "/GrandChild" : self.PrimResyncType.Other,
        })

    def test_InheritsWithRelocate(self):
        '''Tests downstream dependency namespace edits across inherits arcs that
        are under referenced prims where a child of the referencing prim is then
        relocated. This also verifies the effect on the "spooky" implied inherit
        specs composed with relocated prims that have inherit dependencies.
        '''
        # Setup: 
        # Layer 1 has a prim Class with a Child and GrandChild which will be
        # namespace edited. It also has the prim /World with the child prim 
        # Ref that inherits Class and provides local overs. /World and its 
        # descendants will be referenced and relocated in layer 2.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
        class "Class" {
            int classAttr
            
            def "Child" {
                int classChildAttr
                
                def "GrandChild" {
                    int classGrandChildAttr
                }
            }
        }

        def "World" {
            def "Ref" (
                inherits = </Class>
            ) {
                int refAttr
                
                over "Child" {
                    int childAttr
            
                    over "GrandChild" {
                        int grandChildAttr
                    }
                }
            }
        }
        ''')

        # Layer 2 has a three prims, Prim1 Prim2 and Prim3, that each reference
        # /World, /World/Ref, and /World/Ref/Child in layer1 respectively. It 
        # also has a relocates for each of the three prims that renames its
        # first namespace child that is introduced by its reference. Lastly it
        # has opinions for /Class, /Class/Child, and /Class/Child/GrandChild 
        # which will contribute as implied inherits to the relocated prims
        # due the "spooky" inherits across the references and relocates.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
        (
            relocates = {
                </Prim1/Ref> : </Prim1/RelocatedRef>,
                </Prim2/Child> : </Prim2/RelocatedChild>,
                </Prim3/GrandChild> : </Prim3/RelocatedGrandChild>
            }
        )

        def "Prim1" (
            references = @''' + layer1.identifier + '''@</World>
        ) {
            over "RelocatedRef" {
                int localRefAttr
                over "Child" {
                    int locaChildAttr
                    over "GrandChild" {
                        int localGrandChildAttr
                    }
                }
            }
        }

        def "Prim2" (
            references = @''' + layer1.identifier + '''@</World/Ref>
        ) {
            int localRefAttr
            over "RelocatedChild" {
                int locaChildAttr
                over "GrandChild" {
                    int localGrandChildAttr
                }
            }
        }

        def "Prim3" (
            references = @''' + layer1.identifier + '''@</World/Ref/Child>
        ) {
            int locaChildAttr
            over "RelocatedGrandChild" {
                int localGrandChildAttr
            }
        }

        class "Class" {
            int impliedClassAttr
            
            over "Child" {
                int impliedClassChildAttr
                
                over "GrandChild" {
                    int impliedClassGrandChildAttr
                }
            }
        }

        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # We'll edit the first stage with stage2 as a dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify initial composition fields
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/World/Ref' : {
                'inherits' : ('/Class',)
            }   
        })

        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [
                    ('/Prim1/Ref', '/Prim1/RelocatedRef'), 
                    ('/Prim2/Child', '/Prim2/RelocatedChild'),
                    ('/Prim3/GrandChild', '/Prim3/RelocatedGrandChild')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            '/Prim3' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/World/Ref/Child'),)
            },
        })

        # Verify initial contents of stage 1
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref' : {
                    '.' : ['refAttr', 'classAttr'],
                    'Child' : {
                        '.' : ['childAttr', 'classChildAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr', 'classGrandChildAttr'],
                        }           
                    },
                },
            },
            'Class' : {
                '.' : ['classAttr'],
                'Child' : {
                    '.' : ['classChildAttr'],
                    'GrandChild' : {
                        '.' : ['classGrandChildAttr'],
                    }           
                },
            }
        })

        # Verify the initial contents of stage2.
        # The prims that have opinions composed from their references (either 
        # direct or ancestral) to the prims in the layer1 hierarchy 
        # /World/Ref/Child/GrandChild compose opinions from four locations
        # (NOT listed in strength order):
        # 1. The local opinions at the namespace path which may be a post 
        #    relocation path (localXXAttr)
        # 2. The ancestral reference opinions from across the reference 
        #    (xxxAttr)
        # 3. The class opinions within layer1 where the inherit is defined by
        #    reference prim (classXXXAttr)
        # 4. The implied class opinions in layer2 which still apply to relocated
        #    paths (impliedClassXXXAttr)
        composedRefAttrs = [
            'localRefAttr', 'refAttr', 
            'classAttr', 'impliedClassAttr']
        composedChildAttrs = [
            'locaChildAttr', 'childAttr', 
            'classChildAttr', 'impliedClassChildAttr']
        composedGrandChildAttrs = [
            'localGrandChildAttr', 'grandChildAttr',
            'classGrandChildAttr', 'impliedClassGrandChildAttr']

        # For convenience, we have the expected content of the "Child" prim 
        # which throughout this test case won't changed outside of the fact that
        # multiple prim paths (that will change themselves) will hold these
        # "Child contents."
        composedChildContents = {
            '.' : composedChildAttrs,
            'GrandChild' : {
                '.' : composedGrandChildAttrs
            }
        }

        # Prim1 contents: 
        # /Prim1: references /World, has no composed attributes
        #   RelocatedRef: relocated from Ref, fully composed Ref attributes
        #      Child: fully composed Child attributes
        #         GrandChild: fully composed GrandChild attributes.
        prim1Contents = {
            'RelocatedRef' : {
                '.' : composedRefAttrs,
                'Child' : composedChildContents,
            }
        }

        # Prim2 contents:
        # /Prim2: references /World/Ref, fully composed Ref attributes.
        #   RelocatedChild: relocated from Child, fully composed Child attributes
        #     GrandChild: fully composed GrandChild attributes.
        prim2Contents = {
            '.' : composedRefAttrs,
            'RelocatedChild' : composedChildContents,
        }

        # Prim3 contents:
        # /Prim3: references /World/Ref/Child, fully composed Child attributes
        #   RelocatedGrandChild: relocated from GrandChild, fully composed 
        #                        GrandChild attributes.
        prim3Contents = {
            '.' : composedChildAttrs,
            'RelocatedGrandChild' : {
                '.' : composedGrandChildAttrs
            }
        }

        # Finally stage2's full contents are each of the 3 compose prims. and
        # the implied class specs in the stage2's root layer.
        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedClassChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedClassGrandChildAttr'],
                    }           
                },
            },
        })

        # Edit: Rename /Class/Child to /Class/RenamedChild
        with self.ApplyEdits(editor, "Move /Class/Child -> /Class/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath('/Class/Child', 
                                            '/Class/RenamedChild'))

        # Verify the composition fields on layer2
        # The rename of /Class/Child propagates across the direct inherit from
        # /World/Ref in layer1, renaming /World/Ref/Child to 
        # /World/Ref/RenamedChild. Then the rename of /World/Ref/Child 
        # propagates twice. 
        # 
        # First is across the ancestral reference of /World/Ref to /Prim2 so that 
        # /Prim2/Child is now /Prim2/RenamedChild. And since /Prim2/Child was
        # the source of a relocate (to /Prim2/RelocatedChild), that relocate's
        # source is updated to the renamed path.
        # 
        # Second is to the the direct reference that /Prim3 has to 
        # /World/Ref/Child which just results in updating /Prim3's reference
        # to use the new path.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [
                    ('/Prim1/Ref', '/Prim1/RelocatedRef'), 
                    ('/Prim2/RenamedChild', '/Prim2/RelocatedChild'),
                    ('/Prim3/GrandChild', '/Prim3/RelocatedGrandChild')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/Ref/RenamedChild'),)
            },
        })

        # Verify the edited contents of stage1 where Child is renamed under
        # /Class (the direct edit) as well as under /World/Ref because of the 
        # prapagation of the edit across the inherit.
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref' : {
                    '.' : ['refAttr', 'classAttr'],
                    'RenamedChild' : {
                        '.' : ['childAttr', 'classChildAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr', 'classGrandChildAttr'],
                        }           
                    },
                },
            },
            'Class' : {
                '.' : ['classAttr'],
                'RenamedChild' : {
                    '.' : ['classChildAttr'],
                    'GrandChild' : {
                        '.' : ['classGrandChildAttr'],
                    }           
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Class/Child" : self.PrimResyncType.RenameSource,
            "/Class/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/World/Ref/Child" : self.PrimResyncType.RenameSource,
            "/World/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # Verify the updated contents of stage2 where /Prim1/RelocatedRef's 
        # Child is now named RenamedChild with the same composed contents as the
        # original prim.
        # Prim2 and Prim3 are unchanged because the composition field changes
        # keep their composed contents the same. The implied class specs reflect
        # the rename of /Class/Child to /Class/RenamedChild as well.
        prim1Contents = {
            'RelocatedRef' : {
                '.' : composedRefAttrs,
                'RenamedChild' : composedChildContents,
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Class' : {
                '.' : ['impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedClassChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedClassGrandChildAttr'],
                    }           
                },
            },
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Class/Child" : self.PrimResyncType.RenameSource,
            "/Class/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Prim1/RelocatedRef/Child" : self.PrimResyncType.RenameSource,
            "/Prim1/RelocatedRef/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Prim2/RelocatedChild" : self.PrimResyncType.UnchangedPrimStack,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Prim2/Child" : self.PrimResyncType.Delete,
            "/Prim2/RenamedChild" : self.PrimResyncType.Delete,
            # XXX: The presence of this resync notification is due to a separate
            # bug related to implied ancestral inherits nodes having map 
            # functions mapping ancestral paths to incorrect locations. This 
            # bug subsequently impacts Pcp change processing causing this to
            # come up as a resync even though there is no prim on the composed
            # stage at this path neither before or after the edit
            "/World/Ref/RenamedChild" : self.PrimResyncType.Delete,
        })

        # Edit: Rename /Class/RenamedChild to /Class/RelocatedChild
        with self.ApplyEdits(editor, 
                "Move /Class/RenamedChild -> /Class/RelocatedChild"):
            self.assertTrue(editor.MovePrimAtPath('/Class/RenamedChild', 
                                                  '/Class/RelocatedChild'))

        # Verify the updated composition fields in layer2.
        # Just like the previous rename edit, the rename of /Class/RenamedChild 
        # propagates across the direct inherit from /World/Ref in layer1, 
        # renaming /World/Ref/RenamedChild to /World/Ref/RelocatedChild. Then 
        # the rename of /World/Ref/RenamedChild propagates twice. 
        # 
        # First is again across the ancestral reference of /World/Ref to /Prim2 
        # so that /Prim2/RenamedChild is now /Prim2/RelocatedChild. And since 
        # /Prim2/RenamedChild was the source of a relocate, that relocate needs
        # to be updated. However, this time since the original relocate is to
        # the target path /Prim2/RelocatedChild, the relocate source would be
        # the same as the target after the update. So in this case, we actually
        # delete this relocate entirely as it is no longer needed (or valid).
        # 
        # The second is to the the direct reference that /Prim3 has to 
        # /World/Ref/RenamedChild which just results in updating /Prim3's 
        # reference to use the new path just like in the previous edit.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [
                    ('/Prim1/Ref', '/Prim1/RelocatedRef'), 
                    ('/Prim3/GrandChild', '/Prim3/RelocatedGrandChild')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/Ref/RelocatedChild'),)
            },
        })

        # Verify the edited contents of stage1 where RenamedChild is renamed 
        # under /Class (the direct edit) as well as under /World/Ref because of
        # the prapagation of the edit across the inherit.
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref' : {
                    '.' : ['refAttr', 'classAttr'],
                    'RelocatedChild' : {
                        '.' : ['childAttr', 'classChildAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr', 'classGrandChildAttr'],
                        }           
                    },
                },
            },
            'Class' : {
                '.' : ['classAttr'],
                'RelocatedChild' : {
                    '.' : ['classChildAttr'],
                    'GrandChild' : {
                        '.' : ['classGrandChildAttr'],
                    }           
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Class/RenamedChild" : self.PrimResyncType.RenameSource,
            "/Class/RelocatedChild" : self.PrimResyncType.RenameDestination,
            "/World/Ref/RenamedChild" : self.PrimResyncType.RenameSource,
            "/World/Ref/RelocatedChild" : self.PrimResyncType.RenameDestination,
        })

        # Verify the updated contents of stage2 where Prim1's RenamedChild is 
        # now named RelocatedChild with the same composed contents as the
        # original prim. Prim2 and Prim3 are unchanged because the composition
        # field changes keep their composed contents the same. The implied class
        # specs reflect the rename of /Class/RenamedChild to 
        # /Class/RelocatedChild as well.
        prim1Contents = {
            'RelocatedRef' : {
                '.' : composedRefAttrs,
                'RelocatedChild' : composedChildContents,
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Class' : {
                '.' : ['impliedClassAttr'],
                'RelocatedChild' : {
                    '.' : ['impliedClassChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedClassGrandChildAttr'],
                    }           
                },
            },
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Class/RenamedChild" : self.PrimResyncType.RenameSource,
            "/Class/RelocatedChild" : self.PrimResyncType.RenameDestination,
            "/Prim1/RelocatedRef/RenamedChild" : self.PrimResyncType.RenameSource,
            "/Prim1/RelocatedRef/RelocatedChild" : self.PrimResyncType.RenameDestination,
            "/Prim2/RelocatedChild" : self.PrimResyncType.UnchangedPrimStack,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The old and new relocation source will both show up as 
            # resyncs that are interpreted as deletes even though neither prim
            # existed before the edit. It would be nice to not have these 
            # unchanged non-existemnt prims show up as a resync (or at least be
            # understood to be a UnchangedPrimStack resync) but it's tricky to tease this 
            # apart in the current change processing. This same effect will 
            # apply to most of the other test cases in this test file.
            "/Prim2/RenamedChild" : self.PrimResyncType.Delete,
            # XXX: The presence of this resync notification is due to a separate
            # bug related to implied ancestral inherits nodes having map 
            # functions mapping ancestral paths to incorrect locations. This 
            # bug subsequently impacts Pcp change processing causing this to
            # come up as a resync even though there is no prim on the composed
            # stage at this path neither before or after the edit
            "/World/Ref/RelocatedChild" : self.PrimResyncType.Delete,
        })

        # Edit: Rename /Class/RelocatedChild to /Class/RenamedChild, effectively
        # an "undo" of the previous edit.
        with self.ApplyEdits(editor, 
                "Move /Class/RelocatedChild -> /Class/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath('/Class/RelocatedChild', 
                                                  '/Class/RenamedChild'))

        # Verify the updated composition fields in layer2.
        # When compared with the state of the layer before the previous edit, 
        # Prim3 has returned to having its reference point to RenamedChild.
        # But for /Prim2, the relocate that was removed during the prior edit
        # is NOT restored as there is no currently existing relocate dependency
        # for Prim2 that we could find to update.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/': {
                'relocates': [
                    ('/Prim1/Ref', '/Prim1/RelocatedRef'), 
                    ('/Prim3/GrandChild', '/Prim3/RelocatedGrandChild')]
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World/Ref'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                              '/World/Ref/RenamedChild'),)
            },
        })

        # Verify the edited contents of stage1 where RenamedChild is now the 
        # name of the child under /Class again (the direct edit) as well as 
        # under /World/Ref because of the prapagation of the edit across the
        # inherit.
        self._VerifyStageContents(stage1, {
            'World' : {
                'Ref' : {
                    '.' : ['refAttr', 'classAttr'],
                    'RenamedChild' : {
                        '.' : ['childAttr', 'classChildAttr'],
                        'GrandChild' : {
                            '.' : ['grandChildAttr', 'classGrandChildAttr'],
                        }           
                    },
                },
            },
            'Class' : {
                '.' : ['classAttr'],
                'RenamedChild' : {
                    '.' : ['classChildAttr'],
                    'GrandChild' : {
                        '.' : ['classGrandChildAttr'],
                    }           
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Class/RelocatedChild" : self.PrimResyncType.RenameSource,
            "/Class/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/World/Ref/RelocatedChild" : self.PrimResyncType.RenameSource,
            "/World/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # Verify the updated contents of stage2 where Prim1's RelocatedChild is
        # now named RenamedChild with the same composed contents as the original
        # prim. Prim3 is unchanged because the composition field changes keeps 
        # its composed contents the same. But unlike the first time we renamed
        # the child prim to RenamedChild, Prim2's contents DO change to reflect
        # that its child is now RenamedChild instead of RelocatedChild. This is
        # because the lack of a relocate means there was no composition arc
        # present to maintain that the final child prim still be named 
        # RelocatedChild in the composed stage.
        prim1Contents = {
            'RelocatedRef' : {
                '.' : composedRefAttrs,
                'RenamedChild' : composedChildContents,
            }
        }

        prim2Contents = {
            '.' : composedRefAttrs,
            'RenamedChild' : composedChildContents,
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Class' : {
                '.' : ['impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedClassChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedClassGrandChildAttr'],
                    }           
                },
            },
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Class/RelocatedChild" : self.PrimResyncType.RenameSource,
            "/Class/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Prim1/RelocatedRef/RelocatedChild" : self.PrimResyncType.RenameSource,
            "/Prim1/RelocatedRef/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Prim2/RelocatedChild" : self.PrimResyncType.RenameSource,
            "/Prim2/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
            # XXX: The presence of this resync notification is due to a separate
            # bug related to implied ancestral inherits nodes having map 
            # functions mapping ancestral paths to incorrect locations. This 
            # bug subsequently impacts Pcp change processing causing this to
            # come up as a resync even though there is no prim on the composed
            # stage at this path neither before or after the edit
            "/World/Ref/RenamedChild" : self.PrimResyncType.Delete,
        })

    def test_PartiallyRelocatedNewPath(self):
        '''This is a particularly crafted case to exercise the code that maps
        namespace destinations into dependent introducing nodes new spec paths 
        to show that we need to map the destination path back to its fully 
        unrelocated source path in the introducing node before then mapping it
        to its fully relocated path.'''

        # Setup:
        # Layer1 has two similar hierarchies of 
        # World_1/Ref_1/Child_1/GrandChild_1 and 
        # World_2/Ref_2/Child_2/GrandChild_2.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "World_1" {
                int world_1_Attr
                
                def "Ref_1" {
                    int ref_1_Attr
                    
                    def "Child_1" {
                        int child_1_Attr
                
                        def "GrandChild_1" {
                            int grandChild_1_Attr
                        }
                    }    
                }
            }

            def "World_2" {
                int world_2_Attr
                
                def "Ref_2" {
                    int ref_2_Attr
                    
                    def "Child_2" {
                        int child_2_Attr
                
                        def "GrandChild_2" {
                            int grandChild_2_Attr
                        }
                    }    
                }
            }
        ''')

        # Layer2 is specifically constructed to exercise the case where a local
        # opinion on a relocated prim introduces a new reference and then a prim
        # descendant introduced by that reference is then itself relocated. 
        # 
        # /Prim references /World_1 in layer1 and then /Prim/Ref is relocated to 
        # /RelocatedRef_1. Then in the child prim of /RelocatedRef_1 we have 
        # another reference to /World_2 in layer1 and we relocate the child it
        # brings in, Ref_2, out to /RelocatedRef_2.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            (
                relocates = {
                    </Prim/Ref_1> : </RelocatedRef_1>,
                    </RelocatedRef_1/Child_1/Ref_2> : </RelocatedRef_2>
                }
            )

            def "Prim" (
                references = @''' + layer1.identifier + '''@</World_1>
            ) {
            }

            over "RelocatedRef_1" {
                int reloRef_1_Attr
                
                over "Child_1" (
                    references = @''' + layer1.identifier + '''@</World_2>
                ) {
                    int reloChild_1_Attr

                    over "GrandChild_1" {
                        int reloGrandChild_1_Attr
                    }
                }    
            }

            over "RelocatedRef_2" {
                int reloRef_2_Attr
                
                over "Child_2" {
                    int reloChild_2_Attr

                    over "GrandChild_2" {
                        int reloGrandChild_2_Attr
                    }
                }    
            }
        ''')

        # Open both layers as separates stages
        stage1 = Usd.Stage.Open(layer1)
        stage2 = Usd.Stage.Open(layer2)

        # Create an editor for editing the base layer via stage1.
        editor = Usd.NamespaceEditor(stage1)

        # Add stage2 as a dependent stage of the editor.
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields in layer 2
        layer2CompositionContents = {
            '/' : {
                'relocates' : [
                    ('/Prim/Ref_1', '/RelocatedRef_1'),
                    ('/RelocatedRef_1/Child_1/Ref_2', '/RelocatedRef_2')
                ]
            },
            '/Prim' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World_1'),)
            },
            '/RelocatedRef_1/Child_1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/World_2'),)
            },
        }
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), 
                         layer2CompositionContents)

        # Verify the initial contents on stage1
        world1Contents = {
            '.' : ['world_1_Attr'],
            'Ref_1' : {
                '.' : ['ref_1_Attr'],
                'Child_1' : {
                    '.' : ['child_1_Attr'],
                    'GrandChild_1' : {
                        '.' : ['grandChild_1_Attr']
                    }
                }    
            },
        }

        self._VerifyStageContents(stage1, {
            'World_1' : world1Contents,
            'World_2' : {
                '.' : ['world_2_Attr'],
                'Ref_2' : {
                    '.' : ['ref_2_Attr'],
                    'Child_2' : {
                        '.' : ['child_2_Attr'],
                        'GrandChild_2' : {
                            '.' : ['grandChild_2_Attr']
                        }
                    }    
                },
            }
        })

        # Verify the initial contents on stage2.
        # The initial prim that references /World_1 only contains the single
        # attribute from world since its child, Ref_1, is relocated away.
        primContents = {
            '.' : ['world_1_Attr'],
        }

        # The relocated Ref_1 prim has its composed hierarchy from 
        # /World_1/Ref_1 and local opinions. Child_1, additionally has the
        # single attribute from /World_2, which it references locally, as the 
        # child Ref_2 is relocates away.
        ref1Contents = {
            '.' : ['ref_1_Attr', 'reloRef_1_Attr'],
            'Child_1' : {
                '.' : ['child_1_Attr', 'reloChild_1_Attr', 'world_2_Attr'],
                'GrandChild_1' : {
                    '.' : ['grandChild_1_Attr', 'reloGrandChild_1_Attr']
                }
            }
        }

        # The relocated Ref_2 prim has its composed hierarchy from 
        # /World_2/Ref_2 and local opinions
        ref2Contents = {
            '.' : ['ref_2_Attr', 'reloRef_2_Attr'],
            'Child_2' : {
                '.' : ['child_2_Attr', 'reloChild_2_Attr'],
                'GrandChild_2' : {
                    '.' : ['grandChild_2_Attr', 'reloGrandChild_2_Attr']
                }   
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'RelocatedRef_1' : ref1Contents,
            'RelocatedRef_2' : ref2Contents,
        })

        # Edit:
        # Rename /World_2/Ref_2/Child_2 to RenamedChild_2
        with self.ApplyEdits(editor, 
                "Rename /World_2/Ref_2/Child_2 -> /World_2/Ref_2/RenamedChild_2",
                expectWarnings = False):
            self.assertTrue(editor.MovePrimAtPath(
                "/World_2/Ref_2/Child_2", 
                "/World_2/Ref_2/RenamedChild_2"))

        # The composition contents of layer2 haven't changed as none of the 
        # composition arcs refer to paths that are affected by this rename.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), 
                         layer2CompositionContents)

        # Verify that Child_2 is renamed to RenamedChild_2 on stage1
        self._VerifyStageContents(stage1, {
            'World_1' : world1Contents,
            'World_2' : {
                '.' : ['world_2_Attr'],
                'Ref_2' : {
                    '.' : ['ref_2_Attr'],
                    'RenamedChild_2' : {
                        '.' : ['child_2_Attr'],
                        'GrandChild_2' : {
                            '.' : ['grandChild_2_Attr']
                        }
                    }    
                },
            }
        })
        self._VerifyStageResyncNotices(stage1, {
            "/World_2/Ref_2/Child_2" : self.PrimResyncType.RenameSource,
            "/World_2/Ref_2/RenamedChild_2" : self.PrimResyncType.RenameDestination,
        })

        # On stage2 the contents of RelocatedRef_2 have changed to reflect that
        # the entirety of composed specs for Child_2 have been renamed to 
        # RenamedChild_2. This result is dependent on the fact that we correctly
        # map the new spec path across the introducing node and its relocates 
        # regardless of whether the initial mapping across the node results in a
        # partially relocated path versus the more typical case of the path
        # being fully unrelocated.
        # 
        # In detail for this particular example, the new spec path at the 
        # initial edit target in layer1 is /World_2/Ref_2/RenamedChild_2. The
        # stage2 dependency on the original path, /World_2/Ref_2/Child_2,
        # is introduced by the reference on layer2's /RelocatedRef_1/Child_1 to
        # layer1's /World_2. When we map the new path 
        # /World_2/Ref_2/RenamedChild_2 into the introducing node to get the new
        # spec path in layer2, we get the new path 
        # /RelocatedRef_1/Child_1/Ref_2/RenamedChild_2. This is a partially 
        # relocated path (/Prim/Ref_1 -> /RelocatedRef_1) but is not the fully
        # relocated path for the moved spec because it's missing the relocate
        # /RelocatedRef_1/Child_1/Ref_2 -> /RelocatedRef_2. We get the final 
        # correct new spec path, /RelocatedRef_2/RenamedChild_2 by applying the
        # rest of the relocates to the partially relocated new path.
        # 
        # All the other examples in this test file result in fully unrelocated
        # paths during the initial new spec path mapping across the introducing
        # node which is why this example is included to specifically make sure
        # we account for partially relocated paths in the guts of the namespace
        # editing code.
        ref2Contents = {
            '.' : ['ref_2_Attr', 'reloRef_2_Attr'],
            'RenamedChild_2' : {
                '.' : ['child_2_Attr', 'reloChild_2_Attr'],
                'GrandChild_2' : {
                    '.' : ['grandChild_2_Attr', 'reloGrandChild_2_Attr']
                }   
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim' : primContents,
            'RelocatedRef_1' : ref1Contents,
            'RelocatedRef_2' : ref2Contents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/RelocatedRef_2/Child_2" : self.PrimResyncType.RenameSource,
            "/RelocatedRef_2/RenamedChild_2" : self.PrimResyncType.RenameDestination,
        })

if __name__ == '__main__':
    unittest.main()
