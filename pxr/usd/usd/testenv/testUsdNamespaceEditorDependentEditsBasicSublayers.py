#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Sdf, Usd
from testUsdNamespaceEditorDependentEditsBase \
      import TestUsdNamespaceEditorDependentEditsBase

class TestUsdNamespaceEditorDependentEditsBasicRelocates(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency namespace edits across sublayers.
    '''
    
    def test_BasicDependentSublayers(self):
        """Tests downstream dependency namespace edits across sublayers from
        other dependent stages."""

        # Setup: 
        # Layer1 is a simple base layer with a prim hierarchy of /Ref,
        # Child, and GrandChild. This layer will be opened as the base stage
        # that direct namespace edits will be performed on.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Ref" (
            ) {
                int refAttr
                
                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }                              
        ''')

        # Layer2 includes layer1 as a sublayer and provides opinions for Ref
        # Child and GrandChild.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            (
                subLayers = [@''' + layer1.identifier + '''@]
            )

            over "Ref" (
            ) {
                int over2RefAttr
                
                over "Child" {
                    int over2ChildAttr

                    over "GrandChild" {
                        int over2GrandChildAttr
                    }
                }
            }
        ''')

        # Layer3Sub will be a sublayer of the next layer, layer3 and provides 
        # opinions for Ref Child and GrandChild.
        layer3Sub = Sdf.Layer.CreateAnonymous("layer3-sub.usda")
        layer3Sub.ImportFromString('''#usda 1.0
            over "Ref" (
            ) {
                int overSub3RefAttr
                
                over "Child" {
                    int overSub3ChildAttr

                    over "GrandChild" {
                        int overSub3GrandChildAttr
                    }
                }
            }
        ''')

        # Layer3 includes layer2 (which includes layer1) and layer3Sub as 
        # sublayer and also includes local opionion for the prims.
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
                int over3RefAttr
                
                over "Child" {
                    int over3ChildAttr

                    over "GrandChild" {
                        int over3GrandChildAttr
                    }
                }
            }
        ''')

        # Layer4 includes layer3 as a sublayer as well as its own local 
        # opinions for the same prims.
        layer4 = Sdf.Layer.CreateAnonymous("layer4.usda")
        layer4.ImportFromString('''#usda 1.0
            (
                subLayers = [@''' + layer3.identifier + '''@]
            )

            over "Ref" (
            ) {
                int over4RefAttr
                
                over "Child" {
                    int over4ChildAttr

                    over "GrandChild" {
                        int over4GrandChildAttr
                    }
                }
            }
        ''')

        # Open stages for the 4 main layers (excludes layer3Sub)
        stage1 = Usd.Stage.Open(layer1)
        stage2 = Usd.Stage.Open(layer2)
        stage3 = Usd.Stage.Open(layer3)
        stage4 = Usd.Stage.Open(layer4)

        # Create a namespace editor for stage1 which only includes layer1.
        editor = Usd.NamespaceEditor(stage1)

        # Add ONLY stage3 as a dependent stage. This is to specifically show
        # how layer2 will be affected by edits because stage3 depends on it 
        # but layer4 will not be affected as stage4 would've needed to be added
        # to introduce any dependencies on layer4.
        editor.AddDependentStage(stage3)

        # Verify initial contents of each stage.
        
        # Stage1 only includes opinions from layer1
        stage1ChildContents = {
            '.' : ['childAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr'],
            }           
        }

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : stage1ChildContents
            },
        })

        # Stage2's contents come from layer1 and layer2.
        stage2ChildContents = {
            '.' : ['childAttr', 'over2ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr'],
            }           
        }

        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
                'Child' : stage2ChildContents
            },
        })

        # Stage3's contents come from layers 1, 2, 3, and 3Sub.
        stage3ChildContents = {
            '.' : ['childAttr', 'over2ChildAttr', 'over3ChildAttr', 
                    'overSub3ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                        'over3GrandChildAttr', 'overSub3GrandChildAttr'],
            }           
        }

        self._VerifyStageContents(stage3, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr'],
                'Child' : stage3ChildContents
            },
        })

        # Stage4's contents come from all layers: 1, 2 3, 3Sub, and 4
        stage4CombinedChildContents = {
            '.' : ['childAttr', 'over2ChildAttr', 'over3ChildAttr', 
                    'overSub3ChildAttr', 'over4ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                        'over3GrandChildAttr', 'overSub3GrandChildAttr', 
                        'over4GrandChildAttr'],
            }           
        }

        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr', 'over4RefAttr'],
                'Child' : stage4CombinedChildContents
            },
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Move /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify the direct rename on stage1
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : stage1ChildContents
            },
        })

        # Stage2 was not added as a dependent stage, but layer2 is dependent on
        # layer1 edits via stage3 so stage2 reflects the rename in layer1 and
        # layer2.
        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
                'RenamedChild' : stage2ChildContents
            },
        })

        # Stage3 is a dependent stage of the editor and depends on layer1 edits
        # through sublayers, so all sublayers in the dependent layer stack of
        # layer3 (this includes layer3Sub) are affected by the rename and Child
        # is fully renamed in stage3.
        self._VerifyStageContents(stage3, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr'],
                'RenamedChild' : stage3ChildContents
            },
        })

        # Stage4 is not a dependent stage so layer4 is not updated even though
        # all its other sublayers have been edited. Thus stage4 has both Child
        # and RenamedChild where Child has only opinions from layer4 while 
        # RenamedChild has the opinions from all other layers.
        layer4OnlyChildContents = {
            '.' : ['over4ChildAttr'],
            'GrandChild' : {
                '.' : ['over4GrandChildAttr'],
            }           
        }
        
        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr', 'over4RefAttr'],
                'Child' : layer4OnlyChildContents,
                'RenamedChild' : stage3ChildContents
            },
        })

        # Edit: Reparent and rename /Ref/RenamedChild to /MovedChild 
        with self.ApplyEdits(editor, "Move /Ref/RenamedChild -> /MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/RenamedChild', '/MovedChild'))

        # Verify the direct reparent and rename on stage1
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
            'MovedChild' : stage1ChildContents
        })

        # Like with the rename edit previously, RenamedChild is moved to 
        # /MovedChild in all layers and is reflected as a move in both stage2
        # and stage3.
        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
            },
            'MovedChild' : stage2ChildContents
        })

        self._VerifyStageContents(stage3, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 'overSub3RefAttr'],
            },
            'MovedChild' : stage3ChildContents
        })

        # And RenamedChild is also fully moved to /MovedChild in stage4. But
        # note that the specs for Child in layer4 were not renamed in the first
        # edit so Child still exists with the same "layer4 only" contents as
        # before.
        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr', 'over4RefAttr'],
                'Child' : layer4OnlyChildContents,
            },
            'MovedChild' : stage3ChildContents
        })

        # Edit: Reparent and rename /MovedChild back to its original path 
        # /Ref/Child 
        with self.ApplyEdits(editor, "Move /MovedChild -> /Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath('/MovedChild', '/Ref/Child'))

        # All stages return to their exact original contents.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : stage1ChildContents
            },
        })

        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
                'Child' : stage2ChildContents
            },
        })

        self._VerifyStageContents(stage3, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr'],
                'Child' : stage3ChildContents
            },
        })

        # This includes stage4 where layer4's Child opinions are once again
        # composed with the Child opinions from the other sublayers.
        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr', 'over4RefAttr'],
                'Child' : stage4CombinedChildContents
            },
        })

        # Edit: Delete the prim at /Ref/Child 
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        # Verify the direct delete on stage1
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
        })

        # Stage2 was not added as a dependent stage, but layer2 is dependent on
        # layer1 edits via stage3 so stage2 reflects the delete of Child in 
        # layer1 and layer2.
        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
            },
        })

        # Stage3 is a dependent stage of the editor and depends on layer1 edits
        # through sublayers, so all sublayers in the dependent layer stack of
        # layer3 are affected by the deletion and Child is fully deleted in 
        # stage3.
        self._VerifyStageContents(stage3, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr'],
            },
        })

        # Stage4 is not a dependent stage so layer4 is not updated even though
        # all its other sublayers have been edited. Thus stage4 still has Child
        # which now only has opinions from layer4 as the Child opinions from all
        # other sublayers have been deleted.
        self._VerifyStageContents(stage4, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 
                       'overSub3RefAttr', 'over4RefAttr'],
                'Child' : layer4OnlyChildContents
            },
        })

    def test_DependentSublayersAcrossArcs(self):
        """Tests downstream dependency namespace edits across sublayers from
        within composition arcs."""

        # Setup: 
        # Layer1 is a simple base layer with a prim hierarchy of /Ref,
        # Child, and GrandChild. This layer will be opened as the base stage
        # that direct namespace edits will be performed on.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Ref" (
            ) {
                int refAttr
                
                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }
        ''')

        # Layer2 includes layer1 as a sublayer and provides opinions for Ref
        # Child and GrandChild.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            (
                subLayers = [@''' + layer1.identifier + '''@]
            )

            over "Ref" (
            ) {
                int over2RefAttr
                
                over "Child" {
                    int over2ChildAttr

                    over "GrandChild" {
                        int over2GrandChildAttr
                    }
                }
            }
        ''')

        # Layer3 has three prims that reference /Ref, /Ref/Child, and 
        # /Ref/Child/GrandChild from layer2 respectively. These will be
        # affected by downstream dependency edits.
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3.ImportFromString('''#usda 1.0
            def "Prim1" (
                references = @''' + layer2.identifier + '''@</Ref>
            ) {
                int over3RefAttr
                over "Child" {
                    int over3ChildAttr
                    over "GrandChild" {
                        int over3GrandChildAttr
                    }
                }
            }

            def "Prim2" (
                references = @''' + layer2.identifier + '''@</Ref/Child>
            ) {
                int over3ChildAttr
                over "GrandChild" {
                    int over3GrandChildAttr
                }
            }

            def "Prim3" (
                references = @''' + layer2.identifier + '''@</Ref/Child/GrandChild>
            ) {
                int over3GrandChildAttr
            }
        ''')

        # Layer4Sub will be a sublayer of the next layer, layer4, which holds
        # overs for all the prims that are defined in layer3.
        layer4Sub = Sdf.Layer.CreateAnonymous("layer4-sub.usda")
        layer4Sub.ImportFromString('''#usda 1.0
            over "Prim1" {
                int over4RefAttr
                over "Child" {
                    int over4ChildAttr
                    over "GrandChild" {
                        int over4GrandChildAttr
                    }
                }
            }

            over "Prim2" {
                int over4ChildAttr
                over "GrandChild" {
                    int over4GrandChildAttr
                }
            }

            over "Prim3" {
                int over4GrandChildAttr
            }
        ''')

        # Layer4 just has two sublayers, layer3 which defines prims with 
        # references and layer4Sub which provides local opinions for those 
        # prims. This layer has no specs of its own.
        layer4 = Sdf.Layer.CreateAnonymous("layer4.usda")
        layer4.ImportFromString('''#usda 1.0
            (
                subLayers = [
                    @''' + layer3.identifier + '''@,
                    @''' + layer4Sub.identifier + '''@
                ]
            )
        ''')

        # Open stages for the 4 main layers (excludes layer4Sub)
        stage1 = Usd.Stage.Open(layer1)
        stage2 = Usd.Stage.Open(layer2)
        stage3 = Usd.Stage.Open(layer3)
        stage4 = Usd.Stage.Open(layer4)

        # Create a namespace editor for stage1 which only includes layer1.
        editor = Usd.NamespaceEditor(stage1)

        # Add ONLY stage4 as a dependent stage. This is to specifically show
        # how all layers will be affected by edits because stage4 introduces 
        # dependencies on all layers through a combination of sublayers and 
        # references.
        editor.AddDependentStage(stage4)

        # Verify the initial composition fields for layer3 which is the only 
        # layer with non-sublayer composition fields.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/Ref/Child/GrandChild'),)
            },
        })

        # Verify initial contents of each stage.

        # Stage1 has just the contents of layer1
        stage1ChildContents = {
            '.' : ['childAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr'],
            }           
        }
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : stage1ChildContents
            },
        })

        # Stage2 composes layer2 with its only sublayer, layer1.
        stage2ChildContents = {
            '.' : ['childAttr', 'over2ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr'],
            }           
        }
        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
                'Child' : stage2ChildContents
            },
        })

        # Stage3 has each prim composed with the referenced opinions from layer2
        # (which includes sublayer layer1) and the local opinions in layer3.
        stage3Prim1ChildContents = {
            '.' : ['childAttr', 'over2ChildAttr', 'over3ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr',
                        'over3GrandChildAttr'],
            }           
        }
        stage3Prim2Contents = {
            '.' : ['childAttr', 'over2ChildAttr', 'over3ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                        'over3GrandChildAttr'],
            }           
        }
        stage3Prim3Contents = {
            '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                    'over3GrandChildAttr'],
        }

        self._VerifyStageContents(stage3, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr'],
                'Child' : stage3Prim1ChildContents
            },
            'Prim2' : stage3Prim2Contents,
            'Prim3' : stage3Prim3Contents           
        })

        # Stage4 has the same prims from stage3 composed with local opinions 
        # from sublayer layer4Sub
        stage4Prim1ChildContents = {
            '.' : ['childAttr', 'over2ChildAttr', 'over3ChildAttr', 
                    'over4ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                        'over3GrandChildAttr', 'over4GrandChildAttr'],
            }           
        }
        stage4Prim2Contents = {
            '.' : ['childAttr', 'over2ChildAttr', 'over3ChildAttr', 
                    'over4ChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                        'over3GrandChildAttr', 'over4GrandChildAttr'],
            }           
        }
        stage4Prim3Contents = {
            '.' : ['grandChildAttr', 'over2GrandChildAttr', 
                    'over3GrandChildAttr', 'over4GrandChildAttr'],
        }

        self._VerifyStageContents(stage4, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr',
                       'over4RefAttr'],
                'Child' : stage4Prim1ChildContents
            },
            'Prim2' : stage4Prim2Contents,
            'Prim3' : stage4Prim3Contents  
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Move /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify the updated composition fields for layer3 where Prim2 and Prim3
        # reference paths are updated to the renamed path.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/Ref/RenamedChild'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/Ref/RenamedChild/GrandChild'),)
            },
        })

        # Verify the direct rename of Child on stage1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : stage1ChildContents
            },
        })

        # On stage2, Child is also fully renamed because layer2's specs were
        # updated for the sublayer dependency on layer1. Note that stage4 is the
        # only dependent stage added to the namespace editor so this update
        # only occurs because stage4 has dependencies on these layer2 specs.
        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
                'RenamedChild' : stage2ChildContents
            },
        })

        # On stage3, the rename of Child is propagated across the reference for
        # Prim1 where the specs from layer3 are also renamed resulting in a full
        # rename of /Prim1/Child. Prim2 and Prim3's composed contents are 
        # unchanged as their reference fields were updated to the new paths.
        # Note that, like with stage2, these updates only occur because stage4 
        # has dependencies on layer3 specs.
        self._VerifyStageContents(stage3, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr'],
                'RenamedChild' : stage3Prim1ChildContents
            },
            'Prim2' : stage3Prim2Contents,
            'Prim3' : stage3Prim3Contents  
        })

        # On stage4, the rename of Child is propagated across the reference for
        # Prim1 as in stage3. The sublayer layer4Sub has its specs updated for
        # the rename resulting in /Prim1/RenamedChild having all the same 
        # opinions as /Prim1/Child did prior.
        # 
        # Prim2 and Prim3's composed contents are unchanged as their reference
        # fields were updated to the new paths (no changes are necessary to the
        # overs in sublayer layer4Sub).
        self._VerifyStageContents(stage4, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 'over4RefAttr'],
                'RenamedChild' : stage4Prim1ChildContents
            },
            'Prim2' : stage4Prim2Contents,
            'Prim3' : stage4Prim3Contents  
        })

        # Edit: Reparent and rename /Ref/RenamedChild to /MovedChild 
        with self.ApplyEdits(editor, "Move /Ref/RenamedChild -> /MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/RenamedChild', '/MovedChild'))

        # Verify the updated composition fields for layer3 where Prim2 and Prim3
        # reference paths are updated to the moved path.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/MovedChild'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/MovedChild/GrandChild'),)
            },
        })

        # Verify updated contents of stage1 and stage2. /Ref/RenamedChild is 
        # fully moved to /MovedChild on both stages.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
            'MovedChild' : stage1ChildContents
        })

        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
            },
            'MovedChild' : stage2ChildContents
        })

        # On stage3 and stage4, RenamedChild has been moved out from under the
        # referenced prim for Prim1 resulting in an effective deletion of 
        # RenamedChild. All specs in layer3 and layer4Sub referring to 
        # /Prim1/RenamedChild are deleted too resulting in a full deletion of
        # this prim on these stages.
        # 
        # Prim2 and Prim3's composed contents are unchanged as their reference
        # fields were updated to the new paths and no changes are necessary to
        # the local specs in layer3 and layer4Sub).
        self._VerifyStageContents(stage3, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr'],
            },
            'Prim2' : stage3Prim2Contents,
            'Prim3' : stage3Prim3Contents  
        })

        self._VerifyStageContents(stage4, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 'over4RefAttr'],
            },
            'Prim2' : stage4Prim2Contents,
            'Prim3' : stage4Prim3Contents  
        })

        # Edit: Reparent and rename /MovedChild back to its original path 
        # /Ref/Child 
        with self.ApplyEdits(editor, "Move /MovedChild -> /Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath('/MovedChild', '/Ref/Child'))

        # Verify the updated composition fields for layer3 where Prim2 and Prim3
        # reference paths are updated to the original paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/Ref/Child'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer2.identifier, 
                                              '/Ref/Child/GrandChild'),)
            },
        })

        # Verify updated contents of stage1 and stage2 where /Ref/Child has
        # been returned to its original contents.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : stage1ChildContents
            },
        })

        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
                'Child' : stage2ChildContents
            },
        })

        # On stage3 and stage4, Child has returned under Prim1 but now only
        # matches the contents of /Ref/Child as seen on stage2. This is because
        # the local specs on layer3 and layer4Sub for this prim were deleted 
        # by the prior edit and are not able to restored via this namespace 
        # edit. Thus /Prim1/Child only consists of specs from layer1 and layer2.
        # 
        # Prim2 and Prim3's composed contents remain unchanged as their 
        # reference fields were updated to the original paths.
        self._VerifyStageContents(stage3, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr'],
                'Child' : stage2ChildContents
            },
            'Prim2' : stage3Prim2Contents,
            'Prim3' : stage3Prim3Contents  
        })

        self._VerifyStageContents(stage4, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 'over4RefAttr'],
                'Child' : stage2ChildContents
            },
            'Prim2' : stage4Prim2Contents,
            'Prim3' : stage4Prim3Contents  
        })

        # Edit: Delete the prim at /Ref/Child 
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        # Verify the updated composition fields for layer3 where Prim2 and Prim3
        # references are deleted because prims at those paths have been deleted.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : ()
            },
            '/Prim3' : {
                'references' : ()
            },
        })

        # Verify updated contents of stage1 and stage2. /Ref/Child is fully 
        # deleted on both stages.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
        })

        self._VerifyStageContents(stage2, {
            'Ref': {
                '.' : ['refAttr', 'over2RefAttr'],
            },
        })

        # On stage3 and stage4, Child is fully deleted from under Prim1 again.
        # 
        # This time Prim2 and Prim3's composed contents have changed as they
        # no longer have references to layer2. Local opinions for /Prim2 and 
        # /Prim3 in layer3 and layer4Sub remain but opinions for 
        # /Prim2/GrandChild (which is considered an over for the deleted 
        # ancestral reference to /Ref/Child/GrandChild) are deleted.
        self._VerifyStageContents(stage3, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr'],
            },
            'Prim2' : {
                '.' : ['over3ChildAttr'],
            },
            'Prim3' : {
                '.' : ['over3GrandChildAttr'],
            }           
        })

        self._VerifyStageContents(stage4, {
            'Prim1': {
                '.' : ['refAttr', 'over2RefAttr', 'over3RefAttr', 'over4RefAttr'],
            },
            'Prim2' : {
                '.' : ['over3ChildAttr', 'over4ChildAttr'],
            },
            'Prim3' : {
                '.' : ['over3GrandChildAttr', 'over4GrandChildAttr'],
            }           
        })

if __name__ == '__main__':
    unittest.main()
