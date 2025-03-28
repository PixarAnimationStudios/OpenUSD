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

class TestUsdNamespaceEditorDependentEditsBasicClassArcs(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests for dependent edits across inherits and specializes.
    '''

    def _RunTestBasicDependentGlobalClassArcs(self, classArcType):
        """Helper for testing downstream dependency namespace edits across
        global class arcs and their implied class specs. classArcType can
        be either 'inherits' or 'specializes'"""

        # Create the base layer which we will open as a stage and use to make
        # namespace edits to a global class
        #
        # The global class is /Class and has a Child with GrandChild hierarchy.
        # This layer also has three Instance prims that each inherit (or 
        # specialize) /Class, /Class/Child, and /Class/Child/GrandChild 
        # respectively which will be referenced in other layers to show how we
        # handle namespace edits that affect implied class arcs.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Class" {
                int classAttr
                
                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }    
            }

            def "Instance2" (
                ''' + classArcType + ''' = </Class>
            ) {
                int instanceLocalAttr
                over "Child" {
                    int instanceLocalChildAttr
                    over "GrandChild" {
                        int instanceLocalGrandChildAttr
                    }
                }
                def "InstanceLocalChild" {}
            }

            def "Instance3" (
                ''' + classArcType + ''' = </Class/Child>
            ) {
                int instanceLocalChildAttr
                over "GrandChild" {
                    int instanceLocalGrandChildAttr
                }
                def "InstanceLocalChild" {}
            }

            def "Instance4" (
                ''' + classArcType + ''' = </Class/Child/GrandChild>
            ) {
                int instanceLocalGrandChildAttr
                def "InstanceLocalChild" {}
            }
        ''')

        # Layer2 has a prim that references /Instance2 from the first layer 
        # which inherits or specializes /Class and propagates the implied 
        # class arc to /Class in this layer.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Instance2>
            ) {
                over "Child" {
                    int overRefChildAttr
                    over "GrandChild" {
                        int overRefGrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            over "Class" {
                int impliedClassAttr
                
                over "Child" {
                    int impliedChildAttr

                    over "GrandChild" {
                        int impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer2_A has a prim that references /Prim2 from the layer2 
        # which continues to propagate the implied class arc to /Class in this 
        # layer.
        layer2_A = Sdf.Layer.CreateAnonymous("layer2_A.usda")
        layer2_A.ImportFromString('''#usda 1.0
            def "Prim2_A" (
                references = @''' + layer2.identifier + '''@</Prim2>
            ) {
            }

            over "Class" {
                int A_impliedClassAttr
                
                over "Child" {
                    int A_impliedChildAttr

                    over "GrandChild" {
                        int A_impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer3 has a prim that references /Instance3 from the first layer 
        # which inherits or specializes /Class/Child and propagates the implied
        # class arc to /Class/Child in this layer.
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3.ImportFromString('''#usda 1.0
            def "Prim3" (
                references = @''' + layer1.identifier + '''@</Instance3>
            ) {
                int overRefChildAttr
                over "GrandChild" {
                    int overRefGrandChildAttr
                }
                def "LocalChild" {}
            }

            over "Class" {
                int impliedClassAttr
                
                over "Child" {
                    int impliedChildAttr

                    over "GrandChild" {
                        int impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer3_A has a prim that references /Prim3 from layer3 which continues
        # to propagate the implied class arc to /Class/Child in this layer.
        layer3_A = Sdf.Layer.CreateAnonymous("layer3_A.usda")
        layer3_A.ImportFromString('''#usda 1.0
            def "Prim3_A" (
                references = @''' + layer3.identifier + '''@</Prim3>
            ) {
            }

            over "Class" {
                int A_impliedClassAttr
                
                over "Child" {
                    int A_impliedChildAttr

                    over "GrandChild" {
                        int A_impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer4 has a prim that references /Instance4 from the first layer 
        # which inherits or specializes /Class/Child/GrandChild and propagates
        # the implied class arc to /Class/Child/GrandChild in this layer.
        layer4 = Sdf.Layer.CreateAnonymous("layer4.usda")
        layer4.ImportFromString('''#usda 1.0
            def "Prim4" (
                references = @''' + layer1.identifier + '''@</Instance4>
            ) {
                int overRefGrandChildAttr
                def "LocalChild" {}
            }

            over "Class" {
                int impliedClassAttr
                
                over "Child" {
                    int impliedChildAttr

                    over "GrandChild" {
                        int impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer4_A has a prim that references /Prim4 from layer4 which continues
        # to propagate the implied class arc to /Class/Child/GrandChild in this
        # layer.
        layer4_A = Sdf.Layer.CreateAnonymous("layer4_A.usda")
        layer4_A.ImportFromString('''#usda 1.0
            def "Prim4_A" (
                references = @''' + layer4.identifier + '''@</Prim4>
            ) {
            }

            over "Class" {
                int A_impliedClassAttr
                
                over "Child" {
                    int A_impliedChildAttr

                    over "GrandChild" {
                        int A_impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer5 is has a combination of all three prims from layer2, layer3,
        # and layer4 to show the interaction when three different references
        # are accessing the same implied class structure (i.e. each prim
        # has an implied class arc to /Class, /Class/Child, and
        # /Class/Child/GrandChild respectively so they share implied class 
        # specs.)
        layer5 = Sdf.Layer.CreateAnonymous("layer5.usda")
        layer5.ImportFromString('''#usda 1.0
            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Instance2>
            ) {
                over "Child" {
                    int overRefChildAttr
                    over "GrandChild" {
                        int overRefGrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim3" (
                references = @''' + layer1.identifier + '''@</Instance3>
            ) {
                int overRefChildAttr
                over "GrandChild" {
                    int overRefGrandChildAttr
                }
                def "LocalChild" {}
            }

            def "Prim4" (
                references = @''' + layer1.identifier + '''@</Instance4>
            ) {
                int overRefGrandChildAttr
                def "LocalChild" {}
            }

            over "Class" {
                int impliedClassAttr
                
                over "Child" {
                    int impliedChildAttr

                    over "GrandChild" {
                        int impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Layer5_A has three prims that each reference the prims from the layer5
        # which continue to propagate the implied class arcs to /Class, 
        # /Class/Child, and /Class/Child/GrandChild in this layer.
        layer5_A = Sdf.Layer.CreateAnonymous("layer5_A.usda")
        layer5_A.ImportFromString('''#usda 1.0
            def "Prim2_A" (
                references = @''' + layer5.identifier + '''@</Prim2>
            ) {
            }

            def "Prim3_A" (
                references = @''' + layer5.identifier + '''@</Prim3>
            ) {
            }

            def "Prim4_A" (
                references = @''' + layer5.identifier + '''@</Prim4>
            ) {
            }


            over "Class" {
                int A_impliedClassAttr
                
                over "Child" {
                    int A_impliedChildAttr

                    over "GrandChild" {
                        int A_impliedGrandChildAttr
                    }
                }    
            }
        ''')

        # Create a stage for each of the layers we created.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)
        stage2_A = Usd.Stage.Open(layer2_A, Usd.Stage.LoadAll)
        stage3 = Usd.Stage.Open(layer3, Usd.Stage.LoadAll)
        stage3_A = Usd.Stage.Open(layer3_A, Usd.Stage.LoadAll)
        stage4 = Usd.Stage.Open(layer4, Usd.Stage.LoadAll)
        stage4_A = Usd.Stage.Open(layer4_A, Usd.Stage.LoadAll)
        stage5 = Usd.Stage.Open(layer5, Usd.Stage.LoadAll)
        stage5_A = Usd.Stage.Open(layer5_A, Usd.Stage.LoadAll)

        # Create an editor for just stage1 so we can edit the base class.
        editor = Usd.NamespaceEditor(stage1)

        # Add all of the other stages as dependent stages of the editor.
        editor.AddDependentStage(stage2)
        editor.AddDependentStage(stage2_A)
        editor.AddDependentStage(stage3)
        editor.AddDependentStage(stage3_A)
        editor.AddDependentStage(stage4)
        editor.AddDependentStage(stage4_A)
        editor.AddDependentStage(stage5)
        editor.AddDependentStage(stage5_A)

        # Verify the initial composition fields for layer1 which will change
        # as we perform edits.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Instance2' : {
                classArcType : ('/Class',)
            },
            '/Instance3' : {
                classArcType : ('/Class/Child',)
            },
            '/Instance4' : {
                classArcType : ('/Class/Child/GrandChild',)
            },
        })

        # The rest of the dependent layers will not have any composition field
        # changes for this test, but for sanity's sake, we'll still verify this
        # after every edit via this helper function
        def _VerifyUnchangedLayerCompositionFields():
            self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
                '/Prim2' : {
                    'references' : (Sdf.Reference(layer1.identifier, 
                                                  '/Instance2'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer2_A), {
                '/Prim2_A' : {
                    'references' : (Sdf.Reference(layer2.identifier, '/Prim2'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
                '/Prim3' : {
                    'references' : (Sdf.Reference(layer1.identifier, 
                                                  '/Instance3'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer3_A), {
                '/Prim3_A' : {
                    'references' : (Sdf.Reference(layer3.identifier, '/Prim3'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer4), {
                '/Prim4' : {
                    'references' : (Sdf.Reference(layer1.identifier, 
                                                  '/Instance4'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer4_A), {
                '/Prim4_A' : {
                    'references' : (Sdf.Reference(layer4.identifier, '/Prim4'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer5), {
                '/Prim2' : {
                    'references' : (Sdf.Reference(layer1.identifier, 
                                                  '/Instance2'),)
                },
                '/Prim3' : {
                    'references' : (Sdf.Reference(layer1.identifier, 
                                                  '/Instance3'),)
                },
                '/Prim4' : {
                    'references' : (Sdf.Reference(layer1.identifier, 
                                                  '/Instance4'),)
                },
            })

            self.assertEqual(self._GetCompositionFieldsInLayer(layer5_A), {
                '/Prim2_A' : {
                    'references' : (Sdf.Reference(layer5.identifier, 
                                                  '/Prim2'),)
                },
                '/Prim3_A' : {
                    'references' : (Sdf.Reference(layer5.identifier, 
                                                  '/Prim3'),)
                },
                '/Prim4_A' : {
                    'references' : (Sdf.Reference(layer5.identifier, 
                                                  '/Prim4'),)
                },
            })

        _VerifyUnchangedLayerCompositionFields()

        # Verify the initial contents of stage1, which includes the three 
        # composed instance prims and the class spec prims.
        instanceAttrs = ['classAttr', 'instanceLocalAttr']
        instanceChildAttrs = ['childAttr', 'instanceLocalChildAttr']
        instanceGrandChildAttrs = [
            'grandChildAttr', 'instanceLocalGrandChildAttr']
        instance2Contents = {
            '.' : instanceAttrs,
            'Child' : {
                '.' : instanceChildAttrs,
                'GrandChild' : {
                    '.' : instanceGrandChildAttrs
                },
            },
            'InstanceLocalChild' : {},
        }

        instance3Contents =  {
            '.' : ['childAttr', 'instanceLocalChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'instanceLocalGrandChildAttr']
            },
            'InstanceLocalChild' : {},
        }

        instance4Contents = {
            '.' : ['grandChildAttr', 'instanceLocalGrandChildAttr'],
            'InstanceLocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }           
                }
            },
            'Instance2' : instance2Contents,
            'Instance3' : instance3Contents,
            'Instance4' : instance4Contents
        })

        # Verify initial contents of stage2 where /Prim2 has the contents of 
        # the referenced Instance2 prim composed with local opinions and implied
        # class opinions from layer2.
        composedRefAttrs = instanceAttrs + ['localAttr', 'impliedClassAttr']
        composedRefChildAttrs = instanceChildAttrs + [
            'overRefChildAttr', 'impliedChildAttr']
        composedRefGrandChildAttrs = instanceGrandChildAttrs + [
            'overRefGrandChildAttr', 'impliedGrandChildAttr']
        prim2Contents = {
            '.' : composedRefAttrs,
            'Child' : {
                '.' : composedRefChildAttrs,
                'GrandChild' : {
                    '.' : composedRefGrandChildAttrs,
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2' : prim2Contents,
        })

        # Verify initial contents of stage2_A where /Prim2_A has the contents 
        # of the referenced Prim2 prim composed with additional implied class 
        # opinions from layer2_A.
        composed_A_RefAttrs = composedRefAttrs + ['A_impliedClassAttr']
        composed_A_ChildAttrs = composedRefChildAttrs + ['A_impliedChildAttr']
        composed_A_GrandChildAttrs =\
            composedRefGrandChildAttrs + ['A_impliedGrandChildAttr']
        prim2_AContents = {
            '.' : composed_A_RefAttrs,
            'Child' : {
                '.' : composed_A_ChildAttrs,
                'GrandChild' : {
                    '.' : composed_A_GrandChildAttrs,
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2_A' : prim2_AContents,
        })

        # Verify initial contents of stage3 where /Prim3 has the contents of 
        # the referenced Instance3 prim composed with local opinions and implied
        # class opinions from layer3.
        prim3Contents = {
            '.' : composedRefChildAttrs,
            'GrandChild' : {
                '.' : composedRefGrandChildAttrs,
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim3' : prim3Contents,
        })

        # Verify initial contents of stage3_A where /Prim3_A has the contents 
        # of the referenced Prim3 prim composed with additional implied class 
        # opinions from layer3_A.
        prim3_AContents = {
            '.' : composed_A_ChildAttrs,
            'GrandChild' : {
                '.' : composed_A_GrandChildAttrs,
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim3_A' : prim3_AContents,
        })

        # Verify initial contents of stage4 where /Prim4 has the contents of 
        # the referenced Instance4 prim composed with local opinions and implied
        # class opinions from layer4.
        prim4Contents = {
            '.' : composedRefGrandChildAttrs,
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage4, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim4' : prim4Contents,
        })

        # Verify initial contents of stage4_A where /Prim4_A has the contents 
        # of the referenced Prim4 prim composed with additional implied class 
        # opinions from layer4_A.
        prim4_AContents = {
            '.' : composed_A_GrandChildAttrs,
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage4_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim4_A' : prim4_AContents,
        })

        # Verify initial contents of stage5 where /Prim2, /Prim3, and /Prim4 
        # all have the same contents as they do in the individual stage2, 
        # stage3, and stage4.
        self._VerifyStageContents(stage5, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Prim4' : prim4Contents,
        })

        # Verify initial contents of stage5_A where /Prim2_A, /Prim3_A, and 
        # /Prim4_A all have the same contents as they do in the individual 
        # stage2_A, stage3_A, and stage4_A.
        self._VerifyStageContents(stage5_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2_A' : prim2_AContents,
            'Prim3_A' : prim3_AContents,
            'Prim4_A' : prim4_AContents,
        })


        # Edit: Rename /Class/Child to /Class/RenamedChild
        with self.ApplyEdits(editor, "Move /Class/Child -> /Class/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath('/Class/Child', 
                                                '/Class/RenamedChild'))

        # Verify the updated composition fields in layer1. The inherit fields to
        # /Class/Child and its descendant /Class/Child/GrandChild have been
        # updated to use the renamed paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Instance2' : {
                classArcType : ('/Class',)
            },
            '/Instance3' : {
                classArcType : ('/Class/RenamedChild',)
            },
            '/Instance4' : {
                classArcType : ('/Class/RenamedChild/GrandChild',)
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1, the class contents are updated with Child renamed to 
        # RenamedChild. Only the contents of /Instance2 changed since we renamed
        # a descendant of the inherited prim. The contents of the other
        # instances remain the same as their inherits were just updated to point
        # to the new path.
        instance2Contents = {
            '.' : instanceAttrs,
            'RenamedChild' : {
                '.' : instanceChildAttrs,
                'GrandChild' : {
                    '.' : instanceGrandChildAttrs
                },
            },
            'InstanceLocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }           
                }
            },
            'Instance2' : instance2Contents,
            'Instance3' : instance3Contents,
            'Instance4' : instance4Contents
        })

        # On stage2 the implied class specs on layer2 are also updated with the
        # rename of Child to RenamedChild. This results in Prim2's contents 
        # having changed to fully reflect the renaming of Child to RenamedChild
        # that still composes all the same specs (and therefore properties) as
        # before the rename.
        prim2Contents = {
            '.' : composedRefAttrs,
            'RenamedChild' : {
                '.' : composedRefChildAttrs,
                'GrandChild' : {
                    '.' : composedRefGrandChildAttrs,
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2' : prim2Contents,
        })

        # On stage2_A the implied class specs on layer2_A are also updated with
        # the rename of Child to RenamedChild. This results in Prim2_A's 
        # contents having changed to fully reflect the renaming of Child to 
        # RenamedChild for all specs that originally contributed to Child (which
        # includes implied classes from layer2 and layer2_A).
        prim2_AContents = {
            '.' : composed_A_RefAttrs,
            'RenamedChild' : {
                '.' : composed_A_ChildAttrs,
                'GrandChild' : {
                    '.' : composed_A_GrandChildAttrs,
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2_A' : prim2_AContents,
        })

        # On stage3 the implied class specs on layer3 are also updated with the
        # rename of Child to RenamedChild. However, Prim3's contents have NOT 
        # changed as the inherits (direct and implied) now all refer to 
        # /Class/RenamedChild instead of /Class/Child.
        #
        # Similarly on stage3_A, the further implied class specs are updated
        # with the rename, but Prim3_A's contents have not changed for the same
        # reason.
        self._VerifyStageContents(stage3, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim3' : prim3Contents,
        })

        self._VerifyStageContents(stage3_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim3_A' : prim3_AContents,
        })

        # On stage4, the implied class specs on layer4 are updated so that 
        # /Class/Child/GrandChild now resides at /Class/RenamedChild/GrandChild.
        # Yes, this case splits the /Class/Child spec as only GrandChild needs
        # to be moved to RenamedChild/GrandChild; the rest of the contents of
        # /Class/Child are not part of the implied inherit so it doesn't make
        # sense to move them. Prim4's contents have NOT changed as the inherits
        # (direct and implied) now all refer to /Class/RenamedChild/GrandChild.
        #
        # Similarly on stage4_A, the further implied class specs on layer4_A are
        # updated in the same way, but Prim4_A's contents have not changed for
        # the same reason.
        self._VerifyStageContents(stage4, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                },
                'RenamedChild' : {
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim4' : prim4Contents,
        })

        self._VerifyStageContents(stage4_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                },
                'RenamedChild' : {
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim4_A' : prim4_AContents,
        })

        # On stage5, the combined stage, the implied class specs on layer5 are
        # updated exactly the same way as they were for stages 2 and 3 as both
        # /Prim2 and /Prim3 required us to move /Class/Child to 
        # /Class/RenamedChild. /Prim4 only required /Class/Child/GrandChild to
        # be moved to /Class/RenamedChild/GrandChild so that move was subsumed
        # by the ancestral move of /Class/Child to /Class/RenamedChild and is 
        # why the implied class here contrasts with the resulting implied class
        # on stage4.
        #
        # Each prim's contents on stage5 are the same as their equivalent prims
        # on the other "single prim" stages.
        #
        # Similarly on stage5_A, the further implied class specs on layer5_A are
        # updated in the same way, and the prim contents are the same as the 
        # "single prim" _A stages.
        self._VerifyStageContents(stage5, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Prim4' : prim4Contents,
        })

        self._VerifyStageContents(stage5_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                }    
            },
            'Prim2_A' : prim2_AContents,
            'Prim3_A' : prim3_AContents,
            'Prim4_A' : prim4_AContents,
        })

        # Edit: Reparent and rename /Class/RenamedChild to /MovedChild
        with self.ApplyEdits(editor, "Move /Class/RenamedChild -> /MovedChild"):
            self.assertTrue(editor.MovePrimAtPath('/Class/RenamedChild', '/MovedChild'))

        # Verify the updated composition fields in layer1. The class arc fields 
        # to /Class/RenamedChild and its descendant 
        # /Class/RenamedChild/GrandChild have been updated to use the moved 
        # paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Instance2' : {
                classArcType : ('/Class',)
            },
            '/Instance3' : {
                classArcType : ('/MovedChild',)
            },
            '/Instance4' : {
                classArcType : ('/MovedChild/GrandChild',)
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1 the class contents are updated with /MovedChild being a root
        # prim outside of /Class. Only the contents of /Instance2 changed again
        # since we moved a namespace descendant of the class out of the class. 
        # This is effectively a delete of RenamedChild from Instance2 as it no
        # longer ancestrally inherits or specializes it at its new path. The 
        # contents of the other instances remain the same as their class arcs
        # were just updated to point to the new path.
        instance2Contents = {
            '.' : instanceAttrs,
            'InstanceLocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
            },
            'MovedChild' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                }           
            },
            'Instance2' : instance2Contents,
            'Instance3' : instance3Contents,
            'Instance4' : instance4Contents
        })

        # On stage2 the implied class is also updated for the move which 
        # manifests as a deletion of the specs for /Class/RenamedChild on 
        # layer2. The specs are deleted instead of moved to /MovedChild because
        # the class arc to /Class doesn't propagate /MovedChild as an ancestral
        # implied class arc like it did with /Class/RenamedChild. Prim2's 
        # contents have changed to reflect the effective deletion of RenamedChild.
        # 
        # Similarly on stage2_A, the further implied class specs on layer2_A are
        # updated in the same way (i.e. deleted), and Prim2_A's contents have
        # changed to also reflect the full deletion of RenamedChild.
        prim2Contents = {
            '.' : composedRefAttrs,
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'Prim2' : prim2Contents,
        })

        prim2_AContents = {
            '.' : composed_A_RefAttrs,
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'Prim2_A' : prim2_AContents,
        })

        # On stage3 the implied class specs on layer3 are updated with the
        # reparent/rename of /Child/Renamed to /MovedChild. However, Prim3's 
        # contents have NOT changed as the class arcs (direct and implied) now 
        # all refer to /MovedChild instead of /Class/RenamedChild.
        #
        # Similarly on stage3_A, the further implied class specs are updated
        # with the move, but Prim3_A's contents have not changed for the same
        # reason.
        self._VerifyStageContents(stage3, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'MovedChild' : {
                '.' : ['impliedChildAttr'],
                'GrandChild' : {
                    '.' : ['impliedGrandChildAttr']
                }
            },
            'Prim3' : prim3Contents,
        })

        self._VerifyStageContents(stage3_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'MovedChild' : {
                '.' : ['A_impliedChildAttr'],
                'GrandChild' : {
                    '.' : ['A_impliedGrandChildAttr']
                }
            },
            'Prim3_A' : prim3_AContents,
        })

        # On stage4 the implied class specs on layer4 are updated so that 
        # /Class/RenamedChild/GrandChild now resides at /Moved/GrandChild. In
        # constrast with the prior edit's effect on stage4 the RenamedChild spec
        # does NOT stick around even though only RenamedChild/GrandChild needed 
        # to be moved. This is because the remaining RenamedChild would've been
        # just an empty over, and we remove those when specs are moved. The 
        # specs for /Class/Child still remain as they did after the previous 
        # edit.
        #
        # However, Prim4's contents have NOT changed as the class arcs (direct 
        # and implied) now all refer to /MovedChild/GrandChild instead of 
        # /Class/RenamedChild/GrandChild.
        #
        # Similarly on stage4_A, the further implied class specs are updated
        # with the move in the same way, but Prim4_A's contents have also not
        # changed for the same reason.
        self._VerifyStageContents(stage4, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                },
            },
            'MovedChild' : {
                'GrandChild' : {
                    '.' : ['impliedGrandChildAttr']
                }
            },
            'Prim4' : prim4Contents,
        })

        self._VerifyStageContents(stage4_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                },
            },
            'MovedChild' : {
                'GrandChild' : {
                    '.' : ['A_impliedGrandChildAttr']
                }
            },
            'Prim4_A' : prim4_AContents,
        })

        # On stage5, the combined stage, the implied class specs on layer5 are
        # updated exactly the same way as they were on layer3 for stage3 as 
        # that encompasses all the edit requirements for all three prims'
        # dependencies:
        # - Prim2 just requires that /Class no longer has RenamedChild as
        #   namespace child.
        # - Prim3 requires that /Class/RenamedChild be moved to /MovedChild
        # - Prim4 just requires /Class/RenamedChild/GrandChild be moved to 
        #   /MovedChild/GrandChild
        # Moving /Class/RenamedChild to /MovedChild satisfies all three.
        #
        # Each prim's contents on stage5 are the same as their equivalent prims
        # on the other "single prim" stages.
        #
        # Similarly on stage5_A, the further implied class specs on layer5_A are
        # updated in the same way, and the prim contents are the same as the 
        # "single prim" _A stages.
        self._VerifyStageContents(stage5, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'MovedChild' : {
                '.' : ['impliedChildAttr'],
                'GrandChild' : {
                    '.' : ['impliedGrandChildAttr']
                }
            },
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Prim4' : prim4Contents,
        })

        self._VerifyStageContents(stage5_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'MovedChild' : {
                '.' : ['A_impliedChildAttr'],
                'GrandChild' : {
                    '.' : ['A_impliedGrandChildAttr']
                }
            },
            'Prim2_A' : prim2_AContents,
            'Prim3_A' : prim3_AContents,
            'Prim4_A' : prim4_AContents,
        })

        # Edit: Reparent and Rename /MovedChild back to its original path 
        # /Class/Child 
        with self.ApplyEdits(editor, "Move /MovedChild -> /Class/Child"):
            self.assertTrue(editor.MovePrimAtPath('/MovedChild', '/Class/Child'))

        # Verify the updated composition fields in layer1. The inherit fields to
        # /MovedChild and its descendant /MovedChild/GrandChild have been 
        # updated to use the moved paths which are now the original paths again.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Instance2' : {
                classArcType : ('/Class',)
            },
            '/Instance3' : {
                classArcType : ('/Class/Child',)
            },
            '/Instance4' : {
                classArcType : ('/Class/Child/GrandChild',)
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1 the class itself is returned to its original contents from
        # the direct edit. 
        # 
        # For Instance2, it is returned to its original 
        # contents with the one notable exception: the overs on Child and 
        # GrandChild that originally introduced instanceLocalChildAttr and 
        # instanceLocalGrandChildAttr are NOT restored from being deleted when 
        # RenamedChild was moved out from being a descendant of the inherited
        # class. We never restore deleted specs.
        #
        # The contents of the other instances continue to remain the same as 
        # their class arcs have just updated to point to the original path again.
        instance2Contents = {
            '.' : instanceAttrs,
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                }           
            },
            'InstanceLocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }           
                },
            },
            'Instance2' : instance2Contents,
            'Instance3' : instance3Contents,
            'Instance4' : instance4Contents
        })

        # On stage2 the implied class specs are NOT updated as the deleted specs
        # for RenamedChild on layer2 from the prior move are not restored.
        #
        # Prim2's contents return to the original contents with the notable 
        # exceptions that instanceLocalChildAttr, instanceLocalGrandChildAttr,
        # impliedChildAttr, and impliedGrandChildAttr are all missing as the 
        # specs that introduced them were all deleted by the prior move and
        # cannot be restored.
        #
        # Similarly on stage2_A, the further implied class specs on layer2_A
        # cannot be restored from deletion, and Prim2_A's contents only been
        # partially restored in the same manner.
        prim2Contents = {
            '.' : composedRefAttrs,
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr'],
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'Prim2' : prim2Contents,
        })

        prim2_AContents = {
            '.' : composed_A_RefAttrs,
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr'],
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'Prim2_A' : prim2_AContents,
        })

        # On stage3 the implied class specs on layer3 are updated with the
        # reparent/rename that returns stage3 to its original contents. Prim3's 
        # contents have NOT changed (as has been the case through all prior 
        # edits) as the class arcs (direct and implied) now all refer to 
        # /Class/Child.
        #
        # Similarly on stage3_A, the further implied class specs are updated
        # with the move, but Prim3_A's contents have not changed for the same
        # reason.
        self._VerifyStageContents(stage3, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                },
            },
            'Prim3' : prim3Contents,
        })

        self._VerifyStageContents(stage3_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                },
            },
            'Prim3_A' : prim3_AContents,
        })

        # On stage4 the implied class specs on layer4 are updated with the
        # reparent/rename that returns stage4 to its original contents. Prim4's 
        # contents have NOT changed (as has been the case through all prior 
        # edits) as the class arcs (direct and implied) now all refer to 
        # /Class/Child/GrandChild.
        #
        # Similarly on stage4_A, the further implied class specs are updated
        # with the move, but Prim4_A's contents have not changed for the same
        # reason.
        self._VerifyStageContents(stage4, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                },
            },
            'Prim4' : prim4Contents,
        })

        self._VerifyStageContents(stage4_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                },
            },
            'Prim4_A' : prim4_AContents,
        })

        # On stage5, the combined stage, the implied class specs on layer5 are
        # updated exactly the same way as they were on layer3 for stage3 as 
        # that encompasses all the edit requirements for all three prims'
        # dependencies and therefore restores stage5 to its original implied 
        # class contents.
        #
        # Each prim's contents on stage5 are the same as their equivalent prims
        # on the other "single prim" stages EXCEPT for Prim2 which has had its
        # implied attributes restored because the dependency from Prim3 and 
        # Prim4 restore the implied class structure. This is a different 
        # behavior than had occurred on Prim2 on stage2 which doesn't have the
        # addtional dependencies from the other two prims to maintain and 
        # restore the implied class specs originally contributing to 
        # /Prim2/Child and /Prim2/Child/GrandChild. Note, however, that not 
        # all specs contributing to /Prim2/Child and /Prim2/Child/GrandChild
        # were restore to the original state from this stage as the local specs
        # for descendants in the both the referenced layer and the referencing
        # layer were deleted by the prior edit and cannot be restored.
        #
        # Similarly on stage5_A, the further implied class specs on layer5_A are
        # updated in the same way, and the prim contents are the same as the 
        # "single prim" _A stages with the same called out exception of 
        # /Prim2_A.
        prim2Contents = {
            '.' : composedRefAttrs,
            'Child' : {
                '.' : ['childAttr', 'impliedChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'impliedGrandChildAttr']
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage5, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }
                },
            },
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Prim4' : prim4Contents,
        })

        prim2_AContents = {
            '.' : composed_A_RefAttrs,
            'Child' : {
                '.' : ['childAttr', 'impliedChildAttr', 'A_impliedChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'impliedGrandChildAttr', 
                           'A_impliedGrandChildAttr']
                },
            },
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage5_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['A_impliedGrandChildAttr']
                    }
                },
            },
            'Prim2_A' : prim2_AContents,
            'Prim3_A' : prim3_AContents,
            'Prim4_A' : prim4_AContents,
        })

        # Edit: Delete /Class/Child 
        with self.ApplyEdits(editor, "Delete /Class/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Class/Child'))

        # Verify the updated composition fields in layer1. All the class 
        # fields that refer to /Ref/Child or its descendants have had those
        # class arc removed. Note the composition fields remain but the values 
        # are empty.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Instance2' : {
                classArcType : ('/Class',)
            },
            '/Instance3' : {
                classArcType : ()
            },
            '/Instance4' : {
                classArcType : ()
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # Verify the updated stage contents for stage1.
        #
        # For Instance2 which directly inherits /Class, since Child has been
        # deleted from /Class, it is no longer a child of the class instancing
        # prim. Note that the overs to Child (which brought in the 
        # attributes instanceLocalChildAttr and 
        # GrandChild.instanceLocalGrandChildAttr) have been deleted which is why
        # we don't end up with a reintroduction of a partially specced Child.
        #
        # Unlike all the prior edits in this test case, the contents of
        # Instance3 and Instance4 DO change because the class arcs have been 
        # deleted and can no longer compose contents from across them. 
        # Addtionally any child specs in the instancign prims that would've 
        # contributed to the prim stacks of the now deleted referenced prim
        # children are also deleted to "truly delete" all prims from the deleted
        # class arc.
        #
        # XXX: There is an open question as to whether this is the correct 
        # behavior or if we should go a step further and delete the specs that
        # introduced the now class prims (instead of just removing the class 
        # arcs but not the instancing prim itself like we do now). This expanded
        # approach would result in the Instance3 and Instance4 prims being
        # completely deleted from the stage.
        instance2Contents = {
            '.' : instanceAttrs,
            'InstanceLocalChild' : {},
        }

        instance3Contents =  {
            '.' : ['instanceLocalChildAttr'],
            'InstanceLocalChild' : {},
        }

        instance4Contents = {
            '.' : ['instanceLocalGrandChildAttr'],
            'InstanceLocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
            },
            'Instance2' : instance2Contents,
            'Instance3' : instance3Contents,
            'Instance4' : instance4Contents
        })

        # On stage2 the implied class is also updated to delete the specs for
        # /Class/Child on layer2. Prim2's contents have changed to reflect the
        # deletion of Child.
        # 
        # Similarly on stage2_A, the further implied class specs on layer2_A are
        # updated in the same way (i.e. deleted), and Prim2_A's contents have
        # changed to also reflect the full deletion of Child.
        prim2Contents = {
            '.' : composedRefAttrs,
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'Prim2' : prim2Contents,
        })

        prim2_AContents = {
            '.' : composed_A_RefAttrs,
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'Prim2_A' : prim2_AContents,
        })

        # On stage3 the implied class is also updated to delete the specs for
        # /Class/Child on layer3. This is not strictly necessary as the deletion
        # caused the Instance4's class arc to /Class/Child to be deleted which
        # automatically severs the implied clas arc for Prim3. But it makes 
        # sense to delete the implied class specs as they are intended to be
        # overrides to the direct class specs that have been deleted.
        # 
        # Prim3's contents have changed to reflect that there are no composition
        # opinions from class instancing /Class/Child. This includes the 
        # deletion of any local opinions for GrandChild that would otherwise
        # have reintroduced GrandChild without its class opinions.
        # 
        # Similarly on stage3_A, the further implied class specs on layer3_A are
        # updated in the same way (i.e. deleted), and Prim3_A's contents have
        # changed to also reflect the deletion of its classe opinions.
        prim3Contents = {
            '.' : ['instanceLocalChildAttr', 'overRefChildAttr'],
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'Prim3' : prim3Contents,
        })

        prim3_AContents = {
            '.' : ['instanceLocalChildAttr', 'overRefChildAttr'],
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'Prim3_A' : prim3_AContents,
        })

        # On stage4 the implied class is also updated to delete the specs for
        # /Class/Child/GrandChild on layer4 but this does NOT delete 
        # /Class/Child itself since Prim4's implied class arc is only to
        # /Class/Child/GrandChild. Like with stage3, it is not strictly 
        # necessary to delete the specs as the class arc was severed, but we do
        # it for the same reason.
        # 
        # Prim4's contents have changed to reflect that there are no composition
        # opinions from a class arc to /Class/Child/GrandChild.
        # 
        # Similarly on stage4_A, the further implied class specs on layer4_A are
        # updated in the same way (i.e. deleted), and Prim4_A's contents have
        # changed to also reflect the deletion of its class opinions.
        prim4Contents = {
            '.' : ['instanceLocalGrandChildAttr', 'overRefGrandChildAttr'],
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage4, {
            'Class' : {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                },
            },
            'Prim4' : prim4Contents,
        })

        prim4_AContents = {
            '.' : ['instanceLocalGrandChildAttr', 'overRefGrandChildAttr'],
            'InstanceLocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage4_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
                'Child' : {
                    '.' : ['A_impliedChildAttr'],
                },
            },
            'Prim4_A' : prim4_AContents,
        })

        # On stage5, the combined stage, the implied class specs on layer5 are
        # updated to delete /Class/Child which is strictly necessary for Prim2
        # and harmless but appropriate for Prim3 and Prim4
        #
        # Each prim's contents on stage5 are the same as their equivalent prims
        # on the other "single prim" stages.
        #
        # Similarly on stage5_A, the further implied class specs on layer5_A are
        # updated in the same way, and the prim contents are the same as the 
        # "single prim" _A stages.
        self._VerifyStageContents(stage5, {
            'Class' : {
                '.' : ['impliedClassAttr'],
            },
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents,
            'Prim4' : prim4Contents,
        })

        self._VerifyStageContents(stage5_A, {
            'Class' : {
                '.' : ['A_impliedClassAttr'],
            },
            'Prim2_A' : prim2_AContents,
            'Prim3_A' : prim3_AContents,
            'Prim4_A' : prim4_AContents,
        })

    def test_BasicDependentGlobalInherits(self):
        """Test downstream dependency namespace edits across global inherits and
        their implied class specs."""
        self._RunTestBasicDependentGlobalClassArcs("inherits")

    def test_BasicDependentGlobalSpecializes(self):
        """Test downstream dependency namespace edits across global specializes
        and their implied class specs."""
        self._RunTestBasicDependentGlobalClassArcs("specializes")

    def _RunTestBasicDependentLocalClassArcs(self, classArcType):
        """Helper for testing downstream dependency namespace edits across local
        classes and their implied class specs."""

        # "Local inherits" (or "local specializes" in the case of specializes 
        # arcs) refer to the situation where a prim is referenced and the 
        # referenced prim, or any of its namespace descendants, has an inherits 
        # to a class that is also a namespace child of the referenced prim. This
        # is opposed to "global inherits" where the inherited prim is outside
        # the namespace of the prim being referenced. There are two key
        # differences between global and local inherits:
        #   1. The path of any implied inherit specs are mapped across the 
        #      reference for local inherits. For global inherits, the paths of
        #      implied inherits are always identity mapped, i.e. they will used
        #      the same path in the implied layer as they do in the referenced
        #      layer.
        #   2. For local inherits, the inherited class will exist as a composed
        #      prim that is a namespace descendant of the referencing prim 
        #      because of normal ancestral references. For global inherits, the
        #      prim at the class path in the referencing layer will NOt be 
        #      composed with the class spec in the referenced layer since there
        #      isn't an actual ancestral composition arc that does so at that 
        #      path.
        # The effect of the latter on namespace editing is that local inherits
        # mean that the implied class specs have a reference dependency on the
        # origin class whereas for global inherits, the implied class specs have
        # no such dependency and have to be handled explicitly. While this is 
        # all an implementation detail, it explains the purpose of this whole 
        # test case which is to demonstrate that we correctly handle that local
        # inherits introduce both a reference and an implied inherits dependency
        # for the same downstream layer specs.

        # The base layer for editing.
        # Setup: This is the base layer that will be edited. The root prims 
        # /Ref1, /Ref2, and /Ref3 will all be referenced in another layer. Each
        # defines the same "local class" with the hierarchy 
        # LocalClass/Child/GrandChild. Then each defines a local instance with
        # the follow local inherits (or specializes):
        #   Ref1/Instance1 inherits /Ref1/LocalClass
        #   Ref2/Instance2 inherits /Ref2/LocalClass/Child
        #   Ref3/Instance3 inherits /Ref3/LocalClass/Child/GrandChild
        #
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0

            def "Ref1" {
                def "LocalClass" {
                    int classAttr
                    
                    def "Child" {
                        int childAttr
                
                        def "GrandChild" {
                            int grandChildAttr
                        }
                    }
                }

                def "Instance1" (
                    ''' + classArcType + ''' = <../LocalClass>
                ) {
                }
            }

            def "Ref2" {
                def "LocalClass" {
                    int classAttr
                    
                    def "Child" {
                        int childAttr
                
                        def "GrandChild" {
                            int grandChildAttr
                        }
                    }
                }

                def "Instance2" (
                    ''' + classArcType + ''' = <../LocalClass/Child>
                ) {
                }
            }

            def "Ref3" {
                def "LocalClass" {
                    int classAttr
                    
                    def "Child" {
                        int childAttr
                
                        def "GrandChild" {
                            int grandChildAttr
                        }
                    }
                }

                def "Instance3" (
                    ''' + classArcType + ''' = <../LocalClass/Child/GrandChild>
                ) {
                }
            }           
        ''')

        # Layer2 has a three prims that each reference /Ref1, /Ref2, and /Ref3
        # respectively from layer1. Each prim has overs for the implied local
        # class.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            def "Prim1" (
                references = @''' + layer1.identifier + '''@</Ref1>
            ) {
                over "LocalClass" {
                    int impliedClassAttr
                    
                    over "Child" {
                        int impliedChildAttr
                
                        over "GrandChild" {
                            int impliedGrandChildAttr
                        }
                    }    
                }
            }

            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Ref2>
            ) {
                over "LocalClass" {
                    int impliedClassAttr
                    
                    over "Child" {
                        int impliedChildAttr
                
                        over "GrandChild" {
                            int impliedGrandChildAttr
                        }
                    }    
                }
            }

            def "Prim3" (
                references = @''' + layer1.identifier + '''@</Ref3>
            ) {
                over "LocalClass" {
                    int impliedClassAttr
                    
                    over "Child" {
                        int impliedChildAttr
                
                        over "GrandChild" {
                            int impliedGrandChildAttr
                        }
                    }    
                }
            }
        ''')

        # Open stages for both layers.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage1 so we can edit the base local classes.
        editor = Usd.NamespaceEditor(stage1)

        # Add stage2 as a dependent stage.
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields for layer1 which will change
        # as we perform edits.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Ref1/Instance1' : {
                classArcType : ('/Ref1/LocalClass',)
            },
            '/Ref2/Instance2' : {
                classArcType : ('/Ref2/LocalClass/Child',)
            },
            '/Ref3/Instance3' : {
                classArcType : ('/Ref3/LocalClass/Child/GrandChild',)
            },
        })

        # Layer2 will not have any composition field changes for this test, but
        # for sanity's sake, we'll still verify this after every edit.
        def _VerifyUnchangedLayerCompositionFields():
            self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
                '/Prim1' : {
                    'references' : (Sdf.Reference(layer1.identifier, '/Ref1'),)
                },
                '/Prim2' : {
                    'references' : (Sdf.Reference(layer1.identifier, '/Ref2'),)
                },
                '/Prim3' : {
                    'references' : (Sdf.Reference(layer1.identifier, '/Ref3'),)
                },
            })
        _VerifyUnchangedLayerCompositionFields()

        # Verify the initial contents of stage1. Note, the instance contents
        # each match the appropriate class contents as there are no local specs
        # on the instances themselves for this particular test.
        classGrandChildContents = {
            '.' : ['grandChildAttr'],
        }

        classChildContents = {
            '.' : ['childAttr'],
            'GrandChild' : classGrandChildContents
        }

        classContents = {
            '.' : ['classAttr'],
            'Child' : classChildContents
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : {
                'LocalClass' : classContents,
                'Instance1' : classContents,
            },
            'Ref2' : {
                'LocalClass' : classContents,
                'Instance2' : classChildContents,
            },
            'Ref3' : {
                'LocalClass' : classContents,
                'Instance3' : classGrandChildContents,
            },
        })

        # Verify the initial contents of stage2. Note, the instance contents
        # each match the appropriate composed implied class contents as there 
        # are no local specs on the instances themselves for this particular
        # test.
        composedImpliedClassGrandChildContents = {
            '.' : ['grandChildAttr', 'impliedGrandChildAttr'],
        }

        composedImpliedClassChildContents = {
            '.' : ['childAttr', 'impliedChildAttr'],
            'GrandChild' : composedImpliedClassGrandChildContents
        }

        composedImpliedClassContents = {
            '.' : ['classAttr', 'impliedClassAttr'],
            'Child' : composedImpliedClassChildContents
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance1' : composedImpliedClassContents,
            },
            'Prim2' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance2' : composedImpliedClassChildContents,
            },
            'Prim3' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance3' : composedImpliedClassGrandChildContents,
            },
        })

        # Edit Rename LocalClass/Child to LocalClass/RenamedChild under all
        # three referenced prims.
        with self.ApplyEdits(editor, 
                "Move /Ref1/LocalClass/Child -> /Ref1/LocalClass/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref1/LocalClass/Child', '/Ref1/LocalClass/RenamedChild'))
        with self.ApplyEdits(editor, 
                "Move /Ref2/LocalClass/Child -> /Ref2/LocalClass/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/LocalClass/Child', '/Ref2/LocalClass/RenamedChild'))
        with self.ApplyEdits(editor, 
                "Move /Ref3/LocalClass/Child -> /Ref3/LocalClass/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref3/LocalClass/Child', '/Ref3/LocalClass/RenamedChild'))

        # Verify the updated composition fields in layer1. The class arc fields 
        # to /Ref2/LocalClass/Child and /Ref3/LocalClass/Child/GrandChild have 
        # been updated to use the renamed paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Ref1/Instance1' : {
                classArcType : ('/Ref1/LocalClass',)
            },
            '/Ref2/Instance2' : {
                classArcType : ('/Ref2/LocalClass/RenamedChild',)
            },
            '/Ref3/Instance3' : {
                classArcType : ('/Ref3/LocalClass/RenamedChild/GrandChild',)
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1, the local class contents of each Ref prim are updated with
        # Child renamed to RenamedChild. Only the contents of Instance1 changed
        # since the class contents have changed to reflect the rename. The
        # contents of the other instances remain the same as their inherits were
        # just updated to point to the new paths.
        classContents = {
            '.' : ['classAttr'],
            'RenamedChild' : classChildContents
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : {
                'LocalClass' : classContents,
                'Instance1' : classContents,
            },
            'Ref2' : {
                'LocalClass' : classContents,
                'Instance2' : classChildContents,
            },
            'Ref3' : {
                'LocalClass' : classContents,
                'Instance3' : classGrandChildContents,
            },
        })

        # On stage2 the implied local class specs under all three prims in 
        # layer2 are also updated with the rename of Child to RenamedChild 
        # because of ancestral reference dependency. However, only Prim1's 
        # instance's contents have changed reflect the rename of its namespace
        # child.
        composedImpliedClassContents = {
            '.' : ['classAttr', 'impliedClassAttr'],
            'RenamedChild' : composedImpliedClassChildContents
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance1' : composedImpliedClassContents,
            },
            'Prim2' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance2' : composedImpliedClassChildContents,
            },
            'Prim3' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance3' : composedImpliedClassGrandChildContents,
            },
        })

        # Edit: Move LocalClass/RenamedChild to MovedChild under all three
        # referenced prims.
        with self.ApplyEdits(editor, 
                "Move /Ref1/LocalClass/RenamedChild -> /Ref1/MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref1/LocalClass/RenamedChild', '/Ref1/MovedChild'))
        with self.ApplyEdits(editor, 
                "Move /Ref2/LocalClass/RenamedChild -> /Ref2/MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/LocalClass/RenamedChild', '/Ref2/MovedChild'))
        with self.ApplyEdits(editor, 
                "Move /Ref3/LocalClass/RenamedChild -> /Ref3/MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref3/LocalClass/RenamedChild', '/Ref3/MovedChild'))

        # Verify the updated composition fields in layer1. The class arc fields
        # to /Ref2/LocalClass/RenamedChild and 
        # /Ref3/LocalClass/RenamedChild/GrandChild have been updated to use the
        # new local class paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Ref1/Instance1' : {
                classArcType : ('/Ref1/LocalClass',)
            },
            '/Ref2/Instance2' : {
                classArcType : ('/Ref2/MovedChild',)
            },
            '/Ref3/Instance3' : {
                classArcType : ('/Ref3/MovedChild/GrandChild',)
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1, the LocalClass contents of each Ref prim no longer have a
        # RenamedChild prim. Each Ref prim has MovedChild a child prim with the 
        # original contents of RenamedChild. 
        # 
        # Instance1's contents have changed as they match the new class 
        # contents without RenamedChild. The contents of the other instances 
        # remain the same as their class arcs were just updated to point to the
        # new paths.
        classContents = {
            '.' : ['classAttr'],
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : {
                'LocalClass' : classContents,
                'MovedChild' : classChildContents,
                'Instance1' : classContents,
            },
            'Ref2' : {
                'LocalClass' : classContents,
                'MovedChild' : classChildContents,
                'Instance2' : classChildContents,
            },
            'Ref3' : {
                'LocalClass' : classContents,
                'MovedChild' : classChildContents,
                'Instance3' : classGrandChildContents,
            },
        })

        # On stage2 the implied local class specs on all three prims in layer2 
        # are also updated with the move of RenamedChild out of the class and 
        # into a sibling prim MovedChild because of ancestral reference
        # dependencies.
        #
        # However, only Prim1's instance's contents have changed to match the
        # change to the new class contents without RenamedChild.
        composedImpliedClassContents = {
            '.' : ['classAttr', 'impliedClassAttr'],
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : {
                'LocalClass' : composedImpliedClassContents,
                'MovedChild' : composedImpliedClassChildContents,
                'Instance1' : composedImpliedClassContents,
            },
            'Prim2' : {
                'LocalClass' : composedImpliedClassContents,
                'MovedChild' : composedImpliedClassChildContents,
                'Instance2' : composedImpliedClassChildContents,
            },
            'Prim3' : {
                'LocalClass' : composedImpliedClassContents,
                'MovedChild' : composedImpliedClassChildContents,
                'Instance3' : composedImpliedClassGrandChildContents,
            },
        })

        # Edit: Move MovedChild back to LocalClass/Child (the original path) 
        # under all three referenced prims.
        with self.ApplyEdits(editor, 
                "Move /Ref1/MovedChild -> /Ref1/LocalClass/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref1/MovedChild', '/Ref1/LocalClass/Child'))
        with self.ApplyEdits(editor, 
                "Move /Ref2/MovedChild -> /Ref2/LocalClass/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/MovedChild', '/Ref2/LocalClass/Child'))
        with self.ApplyEdits(editor, 
                "Move /Ref3/MovedChild -> /Ref3/LocalClass/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref3/MovedChild', '/Ref3/LocalClass/Child'))

        # Verify the updated composition fields in layer1. The class arc fields 
        # to /Ref2/MovedChild and /Ref3/MovedChild/GrandChild have been updated 
        # back to the original paths from the start of this test.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Ref1/Instance1' : {
                classArcType : ('/Ref1/LocalClass',)
            },
            '/Ref2/Instance2' : {
                classArcType : ('/Ref2/LocalClass/Child',)
            },
            '/Ref3/Instance3' : {
                classArcType : ('/Ref3/LocalClass/Child/GrandChild',)
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1, the local class contents of each Ref prim are updated to
        # the have child contents back under the prim Child again like in the
        # original stage. 
        # 
        # All intance's contents are back to their original contents which is
        # only a change to Instance1 as the other 2 instances haven't changed
        # over any of the prior edits.
        classContents = {
            '.' : ['classAttr'],
            'Child' : classChildContents
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : {
                'LocalClass' : classContents,
                'Instance1' : classContents,
            },
            'Ref2' : {
                'LocalClass' : classContents,
                'Instance2' : classChildContents,
            },
            'Ref3' : {
                'LocalClass' : classContents,
                'Instance3' : classGrandChildContents,
            },
        })

        # On stage2 the implied local class specs on all three prims in layer2 
        # are also updated to have child contents back under the prim Child
        # again like in the original stage because of ancestral reference 
        # dependencies.
        #
        # All instance's contents are back to their original contents which is
        # only a change to Instance1 as the other two instances haven't changed
        # over any of the prior edits either.
        composedImpliedClassContents = {
            '.' : ['classAttr', 'impliedClassAttr'],
            'Child' : composedImpliedClassChildContents
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance1' : composedImpliedClassContents,
            },
            'Prim2' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance2' : composedImpliedClassChildContents,
            },
            'Prim3' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance3' : composedImpliedClassGrandChildContents,
            },
        })

        # Edit: Delete LocalClass/Child under all three referenced prims.
        with self.ApplyEdits(editor, "Delete /Ref1/LocalClass/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref1/LocalClass/Child'))
        with self.ApplyEdits(editor, "Delete /Ref2/LocalClass/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref2/LocalClass/Child'))
        with self.ApplyEdits(editor, "Delete /Ref3/LocalClass/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref3/LocalClass/Child'))

        # Verify the updated composition fields in layer1. The class arc fields 
        # to /Ref2/LocalClass/Child and /Ref3/LocalClass/Child/GrandChild have 
        # been set to empty.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/Ref1/Instance1' : {
                classArcType : ('/Ref1/LocalClass',)
            },
            '/Ref2/Instance2' : {
                classArcType : ()
            },
            '/Ref3/Instance3' : {
                classArcType : ()
            },
        })
        _VerifyUnchangedLayerCompositionFields()

        # On stage1, the local class contents of each Ref prim are updated to
        # have Child completely removed from the LocalClass. 
        # 
        # Instance1 now also matches the class contents with the Child deleted.
        # This time Instance2 and Instance3 have been updated to have no 
        # contents as their class arcs were deleted.
        classContents = {
            '.' : ['classAttr'],
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : {
                'LocalClass' : classContents,
                'Instance1' : classContents,
            },
            'Ref2' : {
                'LocalClass' : classContents,
                'Instance2' : {},
            },
            'Ref3' : {
                'LocalClass' : classContents,
                'Instance3' : {},
            },
        })

        # On stage2 the implied local class specs on all three prims in layer2 
        # are also updated to have Child completely deleted because of the
        # ancestral reference dependencies.
        #
        # Instance1 now also matches the composed class contents with the Child
        # deleted. But this time, Instance2 and Instance3 have been updated
        # to have no contents all their class arcs (direct and implied) were 
        # deleted within the referenced prim.
        composedImpliedClassContents = {
            '.' : ['classAttr', 'impliedClassAttr'],
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance1' : composedImpliedClassContents,
            },
            'Prim2' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance2' : {},
            },
            'Prim3' : {
                'LocalClass' : composedImpliedClassContents,
                'Instance3' : {},
            },
        })

    def test_BasicDependentLocalInherits(self):
        """Test downstream dependency namespace edits across local inherits and
        their implied class specs."""
        self._RunTestBasicDependentLocalClassArcs("inherits")

    def test_BasicDependentLocalSpecializes(self):
        """Test downstream dependency namespace edits across local specializes
        and their implied class specs."""
        self._RunTestBasicDependentLocalClassArcs("specializes")

    def _RunTestNestedClassClassArcs(self, classArcType):
        """Helper for testing downstream dependency namespace edits across 
        class arcs and their implied classes when class arcs are nested inside
        other class arcs."""

        # Setup: Layer1 has the following 
        # - Base class prim /Class that has a Child and GrandChild hierarchy.
        #   This will be where we perform the explicit namespace edits.
        # - /NestedClass which inherits /Class giving us global class hierarchy
        #   with opinions for Child and GrandChild.
        # - /Model which will be referenced by the second layer and contains:
        #    - LocalClass which inherits /NestedClass giving us a class 
        #      hierarchy of one local class and two global classes.
        #    - Instance1 which inherits /Model/LocalClass
        #    - Instance2 which inherits /Model/LocalClass/Child
        #    - Instance3 which inherits /Model/LocalClass/Child/GrandChild
        #
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Class" {
                int classAttr
                
                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }    
            }

            def "NestedClass" (
                ''' + classArcType + ''' = </Class>
            ) {
                int nestedClassAttr
                
                def "Child" {
                    int nestedChildAttr

                    def "GrandChild" {
                        int nestedGrandChildAttr
                    }
                }    
            }

            def "Model" {
                def "LocalClass" (
                    ''' + classArcType + ''' = </NestedClass>
                ) {
                    int localClassAttr
                    
                    def "Child" {
                        int localChildAttr
                
                        def "GrandChild" {
                            int localGrandChildAttr
                        }
                    }    
                }
                
                def "Instance1" (
                    ''' + classArcType + ''' = </Model/LocalClass>
                ) {
                }
                
                def "Instance2" (
                    ''' + classArcType + ''' = </Model/LocalClass/Child>
                ) {
                }
                
                def "Instance3" (
                    ''' + classArcType + ''' = </Model/LocalClass/Child/GrandChild>
                ) {
                }
            }
        ''')

        # Layer2 has a prim /Char that references /Model from layer1 and 
        # provides opinions for all three implied inherit classes: 
        # /Char/LocalClass (mapped from /Model/LocalClass), /NestedClass, and
        # /Class
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            def "Char" (
                references = @''' + layer1.identifier + '''@</Model>
            ) {
                over "LocalClass" {
                    int impliedLocalClassAttr
                    
                    over "Child" {
                        int impliedLocalChildAttr
                
                        over "GrandChild" {
                            int impliedLocalGrandChildAttr
                        }
                    }    
                }
            }

            over "Class" {
                int impliedClassAttr
                
                over "Child" {
                    int impliedChildAttr

                    over "GrandChild" {
                        int impliedGrandChildAttr
                    }
                }    
            }

            over "NestedClass" {
                int impliedNestedClassAttr
                
                over "Child" {
                    int impliedNestedChildAttr

                    over "GrandChild" {
                        int impliedNestedGrandChildAttr
                    }
                }    
            }
        ''')

        # Open stages for both layers.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage1 so we can edit the base global class.
        editor = Usd.NamespaceEditor(stage1)

        # Add stage2 as a dependent stage.
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields for layer1 which will change
        # as we perform edits.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), 
            {
                '/NestedClass' : {
                    classArcType : ('/Class',)
                },
                '/Model/LocalClass' : {
                    classArcType : ('/NestedClass',)
                },
                '/Model/Instance1' : {
                    classArcType : ('/Model/LocalClass',)
                },
                '/Model/Instance2' : {
                    classArcType : ('/Model/LocalClass/Child',)
                },
                '/Model/Instance3' : {
                    classArcType : ('/Model/LocalClass/Child/GrandChild',)
                },
            })

        # Verify layer2's composition fields which will not change during this
        # test.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), 
            {
                '/Char' : {
                    'references' : (Sdf.Reference(layer1.identifier, '/Model'),)
                },
            })

        # Verify the initial contents of stage1 where, in particular, each 
        # Instance prim has opinions from each of its three nested classes.
        composedLocalClassGrandChildContents = {
            '.' : ['grandChildAttr', 'nestedGrandChildAttr', 
                   'localGrandChildAttr'],
        }

        composedLocalClassChildContents = {
            '.' : ['childAttr', 'nestedChildAttr', 'localChildAttr'],
            'GrandChild' : composedLocalClassGrandChildContents
        }

        composedLocalClassContents = {
            '.' : ['classAttr', 'nestedClassAttr', 'localClassAttr'],
            'Child' : composedLocalClassChildContents
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }           
                }
            },
            'NestedClass': {
                '.' : ['classAttr', 'nestedClassAttr'],
                'Child' : {
                    '.' : ['childAttr', 'nestedChildAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr', 'nestedGrandChildAttr']
                    }           
                }
            },
            'Model' : {
                'LocalClass' : composedLocalClassContents,
                'Instance1' : composedLocalClassContents,
                'Instance2' : composedLocalClassChildContents,
                'Instance3' : composedLocalClassGrandChildContents
            }
        })

        # Verify the initial contents of stage2 where, in particular, each 
        # Instance prim has opionions from the three direct nested classes as
        # well as from each corresponding implied class.
        composedImpliedLocalClassGrandChildContents = {
            '.' : ['grandChildAttr', 'nestedGrandChildAttr', 
                   'localGrandChildAttr', 'impliedGrandChildAttr', 
                   'impliedNestedGrandChildAttr', 'impliedLocalGrandChildAttr'],
        }

        composedImpliedLocalClassChildContents = {
            '.' : ['childAttr', 'nestedChildAttr', 'localChildAttr',
                   'impliedChildAttr', 'impliedNestedChildAttr', 
                   'impliedLocalChildAttr'],
            'GrandChild' : composedImpliedLocalClassGrandChildContents
        }

        composedImpliedLocalClassContents = {
            '.' : ['classAttr', 'nestedClassAttr', 'localClassAttr',
                   'impliedClassAttr', 'impliedNestedClassAttr', 
                   'impliedLocalClassAttr'],
            'Child' : composedImpliedLocalClassChildContents
        }

        self._VerifyStageContents(stage2, {
            'Class': {
                '.' : ['impliedClassAttr'],
                'Child' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }           
                }
            },
            'NestedClass': {
                '.' : ['impliedNestedClassAttr'],
                'Child' : {
                    '.' : ['impliedNestedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedNestedGrandChildAttr']
                    }           
                }
            },
            'Char' : {
                'LocalClass' : composedImpliedLocalClassContents,
                'Instance1' : composedImpliedLocalClassContents,
                'Instance2' : composedImpliedLocalClassChildContents,
                'Instance3' : composedImpliedLocalClassGrandChildContents
            }
        })

        # Edit: Rename /Class/Child to /Class/RenamedChild  
        with self.ApplyEdits(editor, 
                "Move /Class/Child -> /Class/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Class/Child', '/Class/RenamedChild'))

        # Verify the updated composition fields in layer1. The edit is 
        # propagated from base class /Class through /NestedClass and 
        # /Model/LocalClass leading to Instance2 and Instance3 having their 
        # class arcs updated to paths that use RenamedChild.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), 
            {
                '/NestedClass' : {
                    classArcType : ('/Class',)
                },
                '/Model/LocalClass' : {
                    classArcType : ('/NestedClass',)
                },
                '/Model/Instance1' : {
                    classArcType : ('/Model/LocalClass',)
                },
                '/Model/Instance2' : {
                    classArcType : ('/Model/LocalClass/RenamedChild',)
                },
                '/Model/Instance3' : {
                    classArcType : ('/Model/LocalClass/RenamedChild/GrandChild',)
                },
            })

        # On stage1, the propagated edits across class arcs mean that Child is
        # renamed to RenamedChild under /Class, /NestedClass, /Model/LocalClass,
        # and finally /Model/Instance1. The contents of Instance2 and Instance3
        # are unchanged because the class arc paths were updated.
        composedLocalClassContents = {
            '.' : ['classAttr', 'nestedClassAttr', 'localClassAttr'],
            'RenamedChild' : composedLocalClassChildContents
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }           
                }
            },
            'NestedClass': {
                '.' : ['classAttr', 'nestedClassAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr', 'nestedChildAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr', 'nestedGrandChildAttr']
                    }           
                }
            },
            'Model' : {
                'LocalClass' : composedLocalClassContents,
                'Instance1' : composedLocalClassContents,
                'Instance2' : composedLocalClassChildContents,
                'Instance3' : composedLocalClassGrandChildContents
            }
        })

        # On stage2, the propagated edits across implied inherits mean that 
        # Child is renamed to RenamedChild under the implied /Class, 
        # /NestedClass and /Char/LocalClass. /Char/Instance1 has Child renamed
        # to Renamed child because the rename has occurred in all its 
        # contributing specs. The contents of Instance2 and Instance3 are 
        # unchanged because their class arc paths (direct and implied) were
        # updated.
        composedImpliedLocalClassContents = {
            '.' : ['classAttr', 'nestedClassAttr', 'localClassAttr',
                   'impliedClassAttr', 'impliedNestedClassAttr', 
                   'impliedLocalClassAttr'],
            'RenamedChild' : composedImpliedLocalClassChildContents
        }

        self._VerifyStageContents(stage2, {
            'Class': {
                '.' : ['impliedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedGrandChildAttr']
                    }           
                }
            },
            'NestedClass': {
                '.' : ['impliedNestedClassAttr'],
                'RenamedChild' : {
                    '.' : ['impliedNestedChildAttr'],
                    'GrandChild' : {
                        '.' : ['impliedNestedGrandChildAttr']
                    }           
                }
            },
            'Char' : {
                'LocalClass' : composedImpliedLocalClassContents,
                'Instance1' : composedImpliedLocalClassContents,
                'Instance2' : composedImpliedLocalClassChildContents,
                'Instance3' : composedImpliedLocalClassGrandChildContents
            }
        })

        # Edit: Rename and reparent /Class/RenamedChild to /MovedChild
        with self.ApplyEdits(editor, "Move /Class/RenamedChild -> /MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Class/RenamedChild', '/MovedChild'))

        # Verify the updated composition fields in layer1. The edit is 
        # propagated from base class /Class through /NestedClass and 
        # /Model/LocalClass leading to Instance2 and Instance3 having their 
        # class arcs updated just like the last edit. But because MovedChild is
        # outside the root of the inherit from /NestedClass to /Class, this edit
        # is propagated as a deletion of /NestedClass/RenamedChild which 
        # then deletes /Model/LocalClass/RenamedChild. And thus we delete the
        # class arc targets for Instance2 and Instance3.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), 
            {
                '/NestedClass' : {
                    classArcType : ('/Class',)
                },
                '/Model/LocalClass' : {
                    classArcType : ('/NestedClass',)
                },
                '/Model/Instance1' : {
                    classArcType : ('/Model/LocalClass',)
                },
                '/Model/Instance2' : {
                    classArcType : ()
                },
                '/Model/Instance3' : {
                    classArcType : ()
                },
            })

        # On stage1, RenamedChild is moved out of /Class to /MovedChild. So, the
        # propagated edits across class arcs mean that RenamedChild is deleted
        # from /NestedClass, /Model/LocalClass, and finally /Model/Instance1. 
        # The contents of Instance2 and Instance3 are now empty because they no
        # longer have a class arc to the deleted descendants of
        # /Model/LocalClass.
        composedLocalClassContents = {
            '.' : ['classAttr', 'nestedClassAttr', 'localClassAttr'],
        }

        self._VerifyStageContents(stage1, {
            'Class': {
                '.' : ['classAttr'],
            },
            'MovedChild' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                }           
            },
            'NestedClass': {
                '.' : ['classAttr', 'nestedClassAttr'],
            },
            'Model' : {
                'LocalClass' : composedLocalClassContents,
                'Instance1' : composedLocalClassContents,
                'Instance2' : {},
                'Instance3' : {}
            }
        })

        # On stage2, the propagated edits across implied class arcs mean that 
        # RenamedChild is now deleted under the implied /Class, /NestedClass 
        # and /Char/LocalClass. /Char/Instance1 has RenamedChild removed
        # because the all of the contributing specs to RenamedChild have been
        # deleted. The contents of Instance2 and Instance3 are now empty because
        # there are no more class arcs (direct nor implied).
        composedImpliedLocalClassContents = {
            '.' : ['classAttr', 'nestedClassAttr', 'localClassAttr',
                   'impliedClassAttr', 'impliedNestedClassAttr', 
                   'impliedLocalClassAttr'],
        }

        self._VerifyStageContents(stage2, {
            'Class': {
                '.' : ['impliedClassAttr'],
            },
            'NestedClass': {
                '.' : ['impliedNestedClassAttr'],
            },
            'Char' : {
                'LocalClass': composedImpliedLocalClassContents,
                'Instance1' : composedImpliedLocalClassContents,
                'Instance2' : {},
                'Instance3' : {}
            }
        })

    def test_TestNestedClassInherits(self):
        """Test downstream dependency namespace edits across inherits and
        their implied classes when inherits are nested inside other inherits."""

        self._RunTestNestedClassClassArcs("inherits")

    def test_TestNestedClassSpecializes(self):
        """Test downstream dependency namespace edits across specializes and
        their implied classes when specializes are nested inside other
        specializes."""

        self._RunTestNestedClassClassArcs("specializes")

    def test_TestMixedInheritAndSpecializesClassHierarchies(self):
        """Tests downstream dependency namespace edits across a mix of inherits
        and specializes arcs in a single nested class hierarchy."""

        # Layer1 has a nested class hierarchy interleaving inherits and 
        # specializes arcs and ending in a reference. I.e. /ClassA inherits 
        # /ClassB specializes /ClassC inherits /ClassD references /Ref. They all
        # define a Child and GrandChild hierarchy.
        # Then we have three instance prims that specialize /ClassA, 
        # /ClassA/Child, and /ClassA/Child/GrandChild respectively.
        # The direct namespace edits will be performed at the referenced prim 
        # /Ref to demonstrate how the edits are propagated through the mixed 
        # class arcs.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Instance1" (
                specializes = </ClassA>
            ) {}

            def "Instance2" (
                specializes = </ClassA/Child>
            ) {}

            def "Instance3" (
                specializes = </ClassA/Child/GrandChild>
            ) {}

            class "ClassA" (
                inherits = </ClassB>
            ) {
                int classAAttr
            
                def "Child" {
                    int childAAttr

                    def "GrandChild" {
                        int grandChildAAttr
                    }
                }    
            }

            class "ClassB" (
                specializes = </ClassC>
            ) {
                int classBAttr
            
                def "Child" {
                    int childBAttr

                    def "GrandChild" {
                        int grandChildBAttr
                    }
                }    
            }

            class "ClassC" (
                inherits = </ClassD>
            ) {
                int classCAttr
            
                def "Child" {
                    int childCAttr

                    def "GrandChild" {
                        int grandChildCAttr
                    }
                }    
            }

            class "ClassD" (
                references = </Ref>
            ) {
                int classDAttr
            
                def "Child" {
                    int childDAttr

                    def "GrandChild" {
                        int grandChildDAttr
                    }
                }    
            }

            class "Ref" {
                int refAttr
            
                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }    
            }
        ''')

        # Layer2 has three instance prims that directly reference the instance 
        # prims from layer1. It also provides implied class opinions for each of
        # the class arcs that will implied from the class arcs across the 
        # references.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            def "Instance1" (
                references = @''' + layer1.identifier + '''@</Instance1>
            ) {}

            def "Instance2" (
                references = @''' + layer1.identifier + '''@</Instance2>
            ) {}

            def "Instance3" (
                references = @''' + layer1.identifier + '''@</Instance3>
            ) {}

            over "ClassA" {
                int implied2ClassAAttr
            
                over "Child" {
                    int implied2ChildAAttr

                    over "GrandChild" {
                        int implied2GrandChildAAttr
                    }
                }    
            }

            over "ClassB" {
                int implied2ClassBAttr
            
                over "Child" {
                    int implied2ChildBAttr

                    over "GrandChild" {
                        int implied2GrandChildBAttr
                    }
                }    
            }

            over "ClassC" {
                int implied2ClassCAttr
            
                over "Child" {
                    int implied2ChildCAttr

                    over "GrandChild" {
                        int implied2GrandChildCAttr
                    }
                }    
            }

            over "ClassD" {
                int implied2ClassDAttr
            
                over "Child" {
                    int implied2ChildDAttr

                    over "GrandChild" {
                        int implied2GrandChildDAttr
                    }
                }    
            }
        ''')


        # Layer3 is similar to layer2 and has three instance prims that directly
        # reference the instance prims from layer2 (that in turn reference 
        # layer1). It also provides implied class opinions for each of the class
        # arcs that will implied from the class arcs across the references.
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3.ImportFromString('''#usda 1.0
            def "Instance1" (
                references = @''' + layer2.identifier + '''@</Instance1>
            ) {}

            def "Instance2" (
                references = @''' + layer2.identifier + '''@</Instance2>
            ) {}

            def "Instance3" (
                references = @''' + layer2.identifier + '''@</Instance3>
            ) {}

            over "ClassA" {
                int implied3ClassAAttr
            
                over "Child" {
                    int implied3ChildAAttr

                    over "GrandChild" {
                        int implied3GrandChildAAttr
                    }
                }    
            }

            over "ClassB" {
                int implied3ClassBAttr
            
                over "Child" {
                    int implied3ChildBAttr

                    over "GrandChild" {
                        int implied3GrandChildBAttr
                    }
                }    
            }

            over "ClassC" {
                int implied3ClassCAttr
            
                over "Child" {
                    int implied3ChildCAttr

                    over "GrandChild" {
                        int implied3GrandChildCAttr
                    }
                }    
            }

            over "ClassD" {
                int implied3ClassDAttr
            
                over "Child" {
                    int implied3ChildDAttr

                    over "GrandChild" {
                        int implied3GrandChildDAttr
                    }
                }    
            }
        ''')

        # Open stages for each layer.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)
        stage3 = Usd.Stage.Open(layer3, Usd.Stage.LoadAll)

        # Create an editor for stage1 so we can edit the base class.
        editor = Usd.NamespaceEditor(stage1)

        # Only stage3 is added as a dependent stage. Stage2 will be affected by
        # changes to layer2 that are caused by dependencies on stage3.
        editor.AddDependentStage(stage3)

        # Verify the composition fields in layer one where the class arcs for 
        # the class hierarchy are defined in addition to the specializes on the
        # instance prims.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), 
            {
                '/ClassD' : {
                    'references' : (Sdf.Reference(primPath = '/Ref'),)
                },
                '/ClassC' : {
                    'inherits' : ('/ClassD',)
                },
                '/ClassB' : {
                    'specializes' : ('/ClassC',)
                },
                '/ClassA' : {
                    'inherits' : ('/ClassB',)
                },
                '/Instance1' : {
                    'specializes' : ('/ClassA',)
                },
                '/Instance2' : {
                    'specializes' : ('/ClassA/Child',)
                },
                '/Instance3' : {
                    'specializes' : ('/ClassA/Child/GrandChild',)
                },
            })

        # Verify the composition fields in layer2 and layer3 where we just have 
        # the references to the instance prims. These will not be be changed
        # across the edits we plan to perform in this test.
        def _VerifyUnchangedLayer2And3CompositionFields():
            self.assertEqual(self._GetCompositionFieldsInLayer(layer2), 
                {
                    '/Instance1' : {
                        'references' : (
                            Sdf.Reference(layer1.identifier, '/Instance1'),)
                    },
                    '/Instance2' : {
                        'references' : (
                            Sdf.Reference(layer1.identifier, '/Instance2'),)
                    },
                    '/Instance3' : {
                        'references' : (
                            Sdf.Reference(layer1.identifier, '/Instance3'),)
                    },
                })
            self.assertEqual(self._GetCompositionFieldsInLayer(layer3), 
                {
                    '/Instance1' : {
                        'references' : (
                            Sdf.Reference(layer2.identifier, '/Instance1'),)
                    },
                    '/Instance2' : {
                        'references' : (
                            Sdf.Reference(layer2.identifier, '/Instance2'),)
                    },
                    '/Instance3' : {
                        'references' : (
                            Sdf.Reference(layer2.identifier, '/Instance3'),)
                    },
                })
        
        _VerifyUnchangedLayer2And3CompositionFields()

        # Verify the initial contents of stage1 which reflects how /ClassA
        # composes in /ClassB which composes in /ClassC which composes in 
        # /ClassD which composes in /Ref.
        refAttrs = ['refAttr']
        refChildAttrs =  ['childAttr']
        refGrandChildAttrs = ['grandChildAttr']
        refChildContents = {
            '.' : refChildAttrs,
            'GrandChild' : {
                '.' : refGrandChildAttrs
            }
        }    

        classDAttrs = refAttrs + ['classDAttr']
        classDChildAttrs =  refChildAttrs + ['childDAttr']
        classDGrandChildAttrs = refGrandChildAttrs + ['grandChildDAttr']
        classDChildContents = {
            '.' : classDChildAttrs,
            'GrandChild' : {
                '.' : classDGrandChildAttrs
            }
        }    

        classCAttrs = classDAttrs + ['classCAttr']
        classCChildAttrs =  classDChildAttrs + ['childCAttr']
        classCGrandChildAttrs = classDGrandChildAttrs + ['grandChildCAttr']
        classCChildContents = {
            '.' : classCChildAttrs,
            'GrandChild' : {
                '.' : classCGrandChildAttrs
            }
        }    

        classBAttrs = classCAttrs + ['classBAttr']
        classBChildAttrs =  classCChildAttrs + ['childBAttr']
        classBGrandChildAttrs = classCGrandChildAttrs + ['grandChildBAttr']
        classBChildContents = {
            '.' : classBChildAttrs,
            'GrandChild' : {
                '.' : classBGrandChildAttrs
            }
        }    

        classAAttrs = classBAttrs + ['classAAttr']
        classAChildAttrs =  classBChildAttrs + ['childAAttr']
        classAGrandChildAttrs = classBGrandChildAttrs + ['grandChildAAttr']
        classAChildContents = {
            '.' : classAChildAttrs,
            'GrandChild' : {
                '.' : classAGrandChildAttrs
            }
        }    

        self._VerifyStageContents(stage1, {
            'Ref' : {
                '.' : refAttrs,
                'Child' : refChildContents
            },
            'ClassD' : {
                '.' : classDAttrs,
                'Child' : classDChildContents
            },
            'ClassC' : {
                '.' : classCAttrs,
                'Child' : classCChildContents
            },
            'ClassB' : {
                '.' : classBAttrs,
                'Child' : classBChildContents
            },
            'ClassA' : {
                '.' : classAAttrs,
                'Child' : classAChildContents
            },
            # /Instance1 composes /ClassA
            'Instance1' : {
                '.' : classAAttrs,
                'Child' : classAChildContents
            },
            # /Instance2 composes /ClassA/Child
            'Instance2' : classAChildContents,
            # /Instance3 composes /ClassA/Child/GrandChild
            'Instance3' : {
                '.' : classAGrandChildAttrs
            },
        })

        # Verify the initial contents of stage2 which reflects how each Instance
        # composes the corresponding instance in layer1 along with the implied
        # class opinions in layer2.
        stage2RefComposedAttrs = classAAttrs + \
            ['implied2ClassDAttr', 'implied2ClassCAttr', 
             'implied2ClassBAttr', 'implied2ClassAAttr']
        stage2ChildComposedAttrs =  classAChildAttrs + \
            ['implied2ChildDAttr', 'implied2ChildCAttr', 
             'implied2ChildBAttr', 'implied2ChildAAttr']
        stage2GrandChildComposedAttrs = classAGrandChildAttrs + \
            ['implied2GrandChildDAttr', 'implied2GrandChildCAttr', 
             'implied2GrandChildBAttr', 'implied2GrandChildAAttr']
        stage2ChildComposedContents = {
            '.' : stage2ChildComposedAttrs,
            'GrandChild' : {
                '.' : stage2GrandChildComposedAttrs
            }
        }    

        self._VerifyStageContents(stage2, {
            'ClassD' : {
                '.' : ['implied2ClassDAttr'],
                'Child' : {
                    '.' : ['implied2ChildDAttr'],
                    'GrandChild' : {
                        '.' : ['implied2GrandChildDAttr']
                    }
                }    
            },
            'ClassC' : {
                '.' : ['implied2ClassCAttr'],
                'Child' : {
                    '.' : ['implied2ChildCAttr'],
                    'GrandChild' : {
                        '.' : ['implied2GrandChildCAttr']
                    }
                }    
            },
            'ClassB' : {
                '.' : ['implied2ClassBAttr'],
                'Child' : {
                    '.' : ['implied2ChildBAttr'],
                    'GrandChild' : {
                        '.' : ['implied2GrandChildBAttr']
                    }
                }    
            },
            'ClassA' : {
                '.' : ['implied2ClassAAttr'],
                'Child' : {
                    '.' : ['implied2ChildAAttr'],
                    'GrandChild' : {
                        '.' : ['implied2GrandChildAAttr']
                    }
                }    
            },
            'Instance1' : {
                '.' : stage2RefComposedAttrs, 
                'Child' : stage2ChildComposedContents,
            },
            'Instance2' : stage2ChildComposedContents,
            'Instance3' : {
                '.' : stage2GrandChildComposedAttrs
            },
        })

        # Verify the initial contents of stage3 which reflects how each Instance
        # composes the corresponding instance in layer2 along with the implied
        # class opinions in layer3.
        stage3RefComposedAttrs = stage2RefComposedAttrs + \
            ['implied3ClassDAttr', 'implied3ClassCAttr', 
             'implied3ClassBAttr', 'implied3ClassAAttr']
        stage3ChildComposedAttrs =  stage2ChildComposedAttrs + \
            ['implied3ChildDAttr', 'implied3ChildCAttr', 
             'implied3ChildBAttr', 'implied3ChildAAttr']
        stage3GrandChildComposedAttrs = stage2GrandChildComposedAttrs + \
            ['implied3GrandChildDAttr', 'implied3GrandChildCAttr', 
             'implied3GrandChildBAttr', 'implied3GrandChildAAttr']
        stage3ChildComposedContents = {
            '.' : stage3ChildComposedAttrs,
            'GrandChild' : {
                '.' : stage3GrandChildComposedAttrs
            }
        }    

        self._VerifyStageContents(stage3, {
            'ClassD' : {
                '.' : ['implied3ClassDAttr'],
                'Child' : {
                    '.' : ['implied3ChildDAttr'],
                    'GrandChild' : {
                        '.' : ['implied3GrandChildDAttr']
                    }
                }    
            },
            'ClassC' : {
                '.' : ['implied3ClassCAttr'],
                'Child' : {
                    '.' : ['implied3ChildCAttr'],
                    'GrandChild' : {
                        '.' : ['implied3GrandChildCAttr']
                    }
                }    
            },
            'ClassB' : {
                '.' : ['implied3ClassBAttr'],
                'Child' : {
                    '.' : ['implied3ChildBAttr'],
                    'GrandChild' : {
                        '.' : ['implied3GrandChildBAttr']
                    }
                }    
            },
            'ClassA' : {
                '.' : ['implied3ClassAAttr'],
                'Child' : {
                    '.' : ['implied3ChildAAttr'],
                    'GrandChild' : {
                        '.' : ['implied3GrandChildAAttr']
                    }
                }    
            },
            'Instance1' : {
                '.' : stage3RefComposedAttrs,
                'Child' : stage3ChildComposedContents
            },
            'Instance2' : stage3ChildComposedContents,
            'Instance3' : {
                '.' : stage3GrandChildComposedAttrs
            },
        })

        # Edit: Rename /Ref/Child to to RenamedChild on stage1
        with self.ApplyEdits(editor, "Rename /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(
                editor.MovePrimAtPath("/Ref/Child", "/Ref/RenamedChild"))

        # Verify the changed composition fields on layer1. Only the specializes 
        # in /Instance1 and /Instance2 on layer1 have been updated to reflect
        # that /ClassA/Child has been moved to /ClassA/RenamedChild in response
        # to the dependencies on /Ref/Child across all the class arcs.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), 
            {
                '/ClassD' : {
                    'references' : (Sdf.Reference(primPath = '/Ref'),)
                },
                '/ClassC' : {
                    'inherits' : ('/ClassD',)
                },
                '/ClassB' : {
                    'specializes' : ('/ClassC',)
                },
                '/ClassA' : {
                    'inherits' : ('/ClassB',)
                },
                '/Instance1' : {
                    'specializes' : ('/ClassA',)
                },
                '/Instance2' : {
                    'specializes' : ('/ClassA/RenamedChild',)
                },
                '/Instance3' : {
                    'specializes' : ('/ClassA/RenamedChild/GrandChild',)
                },
            })

        # Verify that composition fiels in layer2 and layer3 remain unchanged
        _VerifyUnchangedLayer2And3CompositionFields()

        # Verify the changed contents of stage1 where /Ref/Child is renamed to
        # /Ref/RenamedChild and this is propagated up through each class arc so
        # that ClassD through ClassA and Instance1 have all had their "Child"
        # prims fully renamed to "RenamedChild". The contents of /Instance2 and
        # /Instance3 are completely unchanged as their specializes arcs were 
        # updated to refer to the renamed class paths.
        self._VerifyStageContents(stage1, {
            'Ref' : {
                '.' : refAttrs,
                'RenamedChild' : refChildContents
            },
            'ClassD' : {
                '.' : classDAttrs,
                'RenamedChild' : classDChildContents
            },
            'ClassC' : {
                '.' : classCAttrs,
                'RenamedChild' : classCChildContents
            },
            'ClassB' : {
                '.' : classBAttrs,
                'RenamedChild' : classBChildContents
            },
            'ClassA' : {
                '.' : classAAttrs,
                'RenamedChild' : classAChildContents
            },
            'Instance1' : {
                '.' : classAAttrs,
                'RenamedChild' : classAChildContents
            },
            'Instance2' : classAChildContents,
            'Instance3' : {
                '.' : classAGrandChildAttrs
            },
        })

        # XXX: The verifications of stage2 and stage3 contents are commnented
        # out do to a bug/limitation with the prim index graph that prevents us
        # from being able to find all the implied class spec dependencies when
        # inherits arcs are nested under specializes arcs.

        # Verify the changed contents of stage2 where "Child" is renamed to
        # "RenamedChild" under every implied class spec to match the renames in
        # classes they were implied from. Instance1 has its child prim "Child"
        # fully renamed to "RenamedChild" because of the rename across its
        # reference. The contents of /Instance2 and /Instance3 are completely 
        # unchanged as the composed prims they reference were unchanged due the
        # the changes in layer1.
        # 
        # Note that the changes reflected in stage2 are only due to the 
        # dependencies in stage3 on all the specs in layer2 as stage2 was not 
        # added as a dependent stage of the namespace editor. If stage3 didn't 
        # have prims that depend on these layer2 specs, the layer2 specs would
        # not have updated to reflect the rename.
        #
        # self._VerifyStageContents(stage2, {
        #     'ClassD' : {
        #         '.' : ['implied2ClassDAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied2ChildDAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied2GrandChildDAttr']
        #             }
        #         }    
        #     },
        #     'ClassC' : {
        #         '.' : ['implied2ClassCAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied2ChildCAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied2GrandChildCAttr']
        #             }
        #         }    
        #     },
        #     'ClassB' : {
        #         '.' : ['implied2ClassBAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied2ChildBAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied2GrandChildBAttr']
        #             }
        #         }    
        #     },
        #     'ClassA' : {
        #         '.' : ['implied2ClassAAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied2ChildAAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied2GrandChildAAttr']
        #             }
        #         }    
        #     },
        #     'Instance1' : {
        #         '.' : stage2RefComposedAttrs, 
        #         'RenamedChild' : stage2ChildComposedContents,
        #     },
        #     'Instance2' : stage2ChildComposedContents,
        #     'Instance3' : {
        #         '.' : stage2GrandChildComposedAttrs
        #     },
        # })

        # Verify the changed contents of stage3 where "Child" is renamed to
        # "RenamedChild" under every implied class spec to match the renames in
        # classes they were implied from. Instance1 has its child prim "Child"
        # fully renamed to "RenamedChild" because of the rename across its
        # reference. The contents of /Instance2 and /Instance3 are completely 
        # unchanged as the composed prims they reference in layer2 were 
        # unchanged due the the changes in layer1.
        #
        # self._VerifyStageContents(stage3, {
        #     'ClassD' : {
        #         '.' : ['implied3ClassDAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied3ChildDAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied3GrandChildDAttr']
        #             }
        #         }    
        #     },
        #     'ClassC' : {
        #         '.' : ['implied3ClassCAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied3ChildCAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied3GrandChildCAttr']
        #             }
        #         }    
        #     },
        #     'ClassB' : {
        #         '.' : ['implied3ClassBAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied3ChildBAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied3GrandChildBAttr']
        #             }
        #         }    
        #     },
        #     'ClassA' : {
        #         '.' : ['implied3ClassAAttr'],
        #         'RenamedChild' : {
        #             '.' : ['implied3ChildAAttr'],
        #             'GrandChild' : {
        #                 '.' : ['implied3GrandChildAAttr']
        #             }
        #         }    
        #     },
        #     'Instance1' : {
        #         '.' : stage3RefComposedAttrs,
        #         'RenamedChild' : stage3ChildComposedContents
        #     },
        #     'Instance2' : stage3ChildComposedContents,
        #     'Instance3' : {
        #         '.' : stage3GrandChildComposedAttrs
        #     },
        # })

if __name__ == '__main__':
    unittest.main()
