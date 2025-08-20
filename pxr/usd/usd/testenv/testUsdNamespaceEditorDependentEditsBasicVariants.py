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

class TestUsdNamespaceEditorDependentEditsBasicVariants(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency namespace edits across variants.
    '''

    def test_VariantsAndActiveVariantSelections(self):
        '''Tests downstream dependency namespace edits across a reference 
        contained within a variant both when the variant is selected and when
        it is not.
        '''

        # Setup: 
        # Layer1 has a simple Ref Child GrandChild hierarchy where the primary
        # edits will occur.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref" {
                int refAttr
                
                def "Child" {
                    int childAttr
            
                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer2 sets up some variants. 
        # The prim /PrimVariants has a variant set "primVariant" with three 
        # variants:
        #   "one" - references layer1's /Ref and provides overs for it and its 
        #      namespace descendants.
        #   "two" - references layer1's /Ref/Child and provides overs for it 
        #      and GrandChild
        #   "three" - references layer1's /Ref/Child/GrandChild and provides 
        #      overs for it.
        # Then we have prims /Prim1, /Prim2, and /Prim3 which all reference 
        # /PrimVariants and provide local opinions for the contents of variants
        # "one" "two" and "three" respectively. But these prims do NOT provide a
        # variant selection here so these would not normally compose with those
        # variants when layer2 is opened as a stage itself.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            def "PrimVariants" (
                variantSets = ["primVariant"]
            ) {
                variantSet "primVariant" = {
                    "one" (
                        references = @''' + layer1.identifier + '''@</Ref>
                    ) {
                        over "Child" {
                            int variantOneChildAttr
                            over "GrandChild" {
                                int variantOneGrandChildAttr
                            }
                        }
                        def "VariantOneChild" {}
                        int variantOneAttr       
                    }
                    "two" (
                        references = @''' + layer1.identifier + '''@</Ref/Child>
                    ) {
                        over "GrandChild" {
                            int variantTwoGrandChildAttr
                        }
                        def "VariantTwoChild" {}
                        int variantTwoAttr
                    }
                    "three" (
                        references = @''' + layer1.identifier + \
                            '''@</Ref/Child/GrandChild>
                    ) {
                        def "VariantThreeChild" {}
                        int variantThreeAttr       
                    }
                }
            }

            def "Prim1" (
                references = </PrimVariants>
            ) {
                over "Child" {
                    int localChildAttr
                    over "GrandChild" {
                        int localGrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim2" (
                references = </PrimVariants>
            ) {
                over "GrandChild" {
                    int localGrandChildAttr
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim3" (
                references = </PrimVariants>
            ) {
                def "LocalChild" {}
                int localAttr
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # The session layer defines overs for /Prim1, /Prim2, and /Prim3 that
        # set the "primVariant" variant selection to "one", "two", and "three" 
        # respectively.
        sessionLayer  = Sdf.Layer.CreateAnonymous("session.usda")
        sessionLayer.ImportFromString('''#usda 1.0
            over "Prim1" (
                variants = {
                    string primVariant = "one"
                }
            ) {
            }

            over "Prim2" (
                variants = {
                    string primVariant = "two"
                }
            ) {
            }

            over "Prim3" (
                variants = {
                    string primVariant = "three"
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

        # Verify the initial composition fields. Layer2 has references defined 
        # in each of the variant specs under /PrimVariants and the three /Prim#
        # root prims have a reference directly to /PrimVariants.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{primVariant=one}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/PrimVariants{primVariant=two}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/PrimVariants{primVariant=three}' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                                '/Ref/Child/GrandChild'),)
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the initial contents of stage1 which is just the simple Ref 
        # Child GrandChild hierarchy.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })

        # Verify the initial contents of stage2. 
        # /PrimVariants is empty because it has no variant selection so none of
        # its primVariant variants are composed. 
        # /Prim1 references /PrimVariants and the session layer sets its 
        # primVariant to "one" so its contents are composed opinions from the 
        # local /Prim1 specs, the /PrimVariants{primVariant=one} variant, and 
        # the referenced opinions for /Ref in in layer1 (brought in by the 
        # variant).
        # /Prim2 references /PrimVariants and the session layer sets its 
        # primVariant to "two" so its contents are composed opinions from the 
        # local /Prim2 specs, the /PrimVariants{primVariant=two} variant, and 
        # the referenced opinions for /Ref/Child in in layer1 (brought in by the
        # variant).
        # /Prim3 references /PrimVariants and the session layer sets its 
        # primVariant to "three" so its contents are composed opinions from the
        # local /Prim3 specs, the /PrimVariants{primVariant=three} variant, and
        # the referenced opinions for /Ref/Child/GrandChild in in layer1 
        # (brought in by the variant).
        prim1Contents = {
            '.' : ['refAttr', 'variantOneAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr', 'variantOneChildAttr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'variantOneGrandChildAttr', 
                           'localGrandChildAttr'],
                }           
            },
            'VariantOneChild' : {},
            'LocalChild' : {}       
        }
        prim2Contents = {
            '.' : ['childAttr', 'variantTwoAttr', 'localAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'variantTwoGrandChildAttr', 
                       'localGrandChildAttr'],
            },
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        prim3Contents = {
            '.' : ['grandChildAttr', 'variantThreeAttr', 'localAttr'],
            'VariantThreeChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim1': prim1Contents,
            'Prim2': prim2Contents,
            'Prim3': prim3Contents,
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Rename /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify changed composition fields. The references in the variants 
        # "two" and "three" on /PrimVariants are updated to refer to the renamed
        # /Ref/RenamedChild paths because these variant specs that are composed
        # into the other stage2 prims.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{primVariant=one}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/PrimVariants{primVariant=two}' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                                '/Ref/RenamedChild'),)
            },
            '/PrimVariants{primVariant=three}' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                                '/Ref/RenamedChild/GrandChild'),)
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are updated to reflect the simple rename
        # of Child to RenamedChild
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Ref/Child" : self.PrimResyncType.RenameSource,
            "/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # On stage2 the contents of Prim1 have changed to reflect the full 
        # rename /Prim1/Child to /Prim1/RenamedChild as all specs that 
        # originally contributed to Child have been moved to RenamedChild. 
        # /Prim2 and /Prim3 contents have not changed at all because the
        # references in their included variants were updated to the new path.
        prim1Contents = {
            '.' : ['refAttr', 'variantOneAttr', 'localAttr'],
            'RenamedChild' : {
                '.' : ['childAttr', 'variantOneChildAttr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'variantOneGrandChildAttr', 
                           'localGrandChildAttr'],
                }           
            },
            'VariantOneChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim1': prim1Contents,
            'Prim2': prim2Contents,
            'Prim3': prim3Contents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim1/Child" : self.PrimResyncType.RenameSource,
            "/Prim1/RenamedChild" : self.PrimResyncType.RenameDestination,
            "/Prim2" : self.PrimResyncType.UnchangedPrimStack,
            "/Prim3" : self.PrimResyncType.UnchangedPrimStack,
        })

        # Edit: Delete /Ref/RenamedChild
        with self.ApplyEdits(editor, "Delete /Ref/RenamedChild"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/RenamedChild'))

        # Verify changed composition fields. The references in the variants 
        # "two" and "three" on /PrimVariants that are composed into stage2 prims
        # are updated to remomve the now deleted /Ref/RenamedChild and 
        # /Ref/RenamedChild/GrandChild paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{primVariant=one}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/PrimVariants{primVariant=two}' : {
                'references' : ()
            },
            '/PrimVariants{primVariant=three}' : {
                'references' : ()
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are updated to reflect the deletion of 
        # RenamedChild
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Ref/RenamedChild" : self.PrimResyncType.Delete,
        })

        # On stage2 the contents of Prim1 have changed to reflect the full that
        # /Prim1/RenamedChild has been fully fully deleted as all specs that 
        # originally contributed to it have bene deleted. 
        #
        # /Prim2's contents have changed to reflect that it no longer has 
        # opinions from layer1's /Ref/Child since the reference was deleted from
        # the "two" variant. Both the local opinions and the direct variant 
        # opinions are still composed (they are not deleted), but note that the
        # variant's opinions for GrandChild as well as /Prim2's local opinions
        # for GrandChild were deleted so that the composed GrandChild prim would
        # be deleted as a result of removing the reference that introduced this
        # prim child. This consistent with how we handle these kinds of deletes
        # across reference even when there are no variants.
        #
        # /Prim3's contents have changed to reflect that it no longer has 
        # opinions from layer1's /Ref/Child/GrandChild since the reference was 
        # deleted from the "three" variant. Both the local opinions and the
        # direct variant opinions are still composed (they are not deleted).
        prim1Contents = {
            '.' : ['refAttr', 'variantOneAttr', 'localAttr'],
            'VariantOneChild' : {},
            'LocalChild' : {}       
        }
        prim2Contents = {
            '.' : ['variantTwoAttr', 'localAttr'],
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        prim3Contents = {
            '.' : ['variantThreeAttr', 'localAttr'],
            'VariantThreeChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim1': prim1Contents,
            'Prim2': prim2Contents,
            'Prim3': prim3Contents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim1/RenamedChild" : self.PrimResyncType.Delete,
            "/Prim2" : self.PrimResyncType.Other,
            "/Prim3" : self.PrimResyncType.Other,
        })

        # Reset both layer1 and layer2 contents for the next test case and then
        # mute stage2's session layer. This will remove the variant selections
        # for stage2's prims so that we can test the effects of the same
        # namespace edits on variants that are not selected.
        with Sdf.ChangeBlock():
            layer2.ImportFromString(layer2ImportString)
            layer1.ImportFromString(layer1ImportString)
        stage2.MuteLayer(sessionLayer.identifier)

        # Verify the restored composition fields which have returned to their
        # state at the start of this test.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{primVariant=one}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/PrimVariants{primVariant=two}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/PrimVariants{primVariant=three}' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                                '/Ref/Child/GrandChild'),)
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the returned initial contents of stage1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })

        # Verify the contents of stage2. This time, because the session layer is
        # muted, there are no variant selections on any of the prims. So none of
        # the prims compose any opinions from any variants in /PrimVariants and
        # therefore cannot compose any opinions from layer1 as the references to
        # layer1 are are defined in the variants.
        prim1Contents = {
            '.' : ['localAttr'],
            'Child' : {
                '.' : ['localChildAttr'],
                'GrandChild' : {
                    '.' : ['localGrandChildAttr'],
                }           
            },
            'LocalChild' : {}       
        }
        prim2Contents = {
            '.' : ['localAttr'],
            'GrandChild' : {
                '.' : ['localGrandChildAttr'],
            },
            'LocalChild' : {}       
        }
        prim3Contents = {
            '.' : ['localAttr'],
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim1': prim1Contents,
            'Prim2': prim2Contents,
            'Prim3': prim3Contents,
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Rename /Ref/Child to /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify that the composition fields in layer2 have NOT changed. This is
        # because without the variant selections, there are no composed prims 
        # with dependencies on the variants in /PrimVariants so we won't find
        # these variant specs to update. Variant selection paths are never 
        # composed as USD stage prims themselves.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{primVariant=one}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/PrimVariants{primVariant=two}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/PrimVariants{primVariant=three}' : {
                'references' : (Sdf.Reference(layer1.identifier, 
                                                '/Ref/Child/GrandChild'),)
            },
            '/Prim1' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are updated to reflect the simple
        # rename of Child to RenamedChild, same as before.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Ref/Child" : self.PrimResyncType.RenameSource,
            "/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # Verify the contents of stage2 are completely unchanged as there are no
        # composed prim dependencies on the specs in layer1 without the variant
        # selections.
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim1': prim1Contents,
            'Prim2': prim2Contents,
            'Prim3': prim3Contents,
        })
        self._VerifyStageResyncNotices(stage2, None)

        # Unmute the session layer on stage2 which reapplies the variant
        # selections in composition for the three main prims again.
        stage2.UnmuteLayer(sessionLayer.identifier)

        # Verify the contents of post-edit stage2 with the variant selections
        # applied again. The results are different from the results from when
        # the same edit was performed with the variant selections active. 
        # Specifically: for /Prim1, the specs from across the reference to 
        # layer1's /Ref bring in the renamed RenamedChild prim, but the local
        # spec and variant specs still bring in Child as a different prim since
        # those specs could not be updated without the active variant selection.
        #
        # /Prim2 and /Prim3 do not compose any specs at all from layer1 as the
        # references in their variants could not be updated to use the renamed
        # path when the variant selection wasn't active.
        # 
        # This all demonstrates how we do not attempt to fix namespace edited
        # paths in variants that aren't currently selected.
        prim1Contents = {
            '.' : ['refAttr', 'variantOneAttr', 'localAttr'],
            'RenamedChild' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr'],
                }           
            },
            'Child' : {
                '.' : ['variantOneChildAttr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['variantOneGrandChildAttr', 'localGrandChildAttr'],
                }           
            },
            'VariantOneChild' : {},
            'LocalChild' : {}       
        }
        prim2Contents = {
            '.' : ['variantTwoAttr', 'localAttr'],
            'GrandChild' : {
                '.' : ['variantTwoGrandChildAttr', 'localGrandChildAttr'],
            },
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        prim3Contents = {
            '.' : ['variantThreeAttr', 'localAttr'],
            'VariantThreeChild' : {},
            'LocalChild' : {}       
        }

        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim1': prim1Contents,
            'Prim2': prim2Contents,
            'Prim3': prim3Contents,
        })

    def test_BasicNestedVariants(self):
        '''Tests downstream dependency namespace edits across a reference 
        to a prim with nested selected variants.
        '''

        # Setup: 
        # Layer1 has a simple Ref, Child, GrandChild hierarchy
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref" {
                int refAttr
                
                def "Child" {
                    int childAttr
            
                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer2 sets up some nested variant sets. /PrimVariants has variantSet
        # "one" whose "default" variant contains the variant set "two", whose
        # "default" variant has the variant set "three" whose default variant 
        # finally has reference to layer1's /Ref prim. Thus, the reference field
        # lives on the spec at the path 
        # /PrimVariants{one=default}{two=default}{three=default}
        # Then the prim /Prim references /PrimVariants using the default variant
        # selection for all three variants. Opinions over Child and GrandChild
        # exist locally on /Prim and in the "two" variant in /PrimVariants.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0

        def "PrimVariants" (
            variantSets = ["one"]
        ) {
            variantSet "one" = {
                "default" (
                    variantSets = ["two"]
                ) {
                    variantSet "two" = {
                        "default" (
                            variantSets = ["three"]
                        ) {
                            variantSet "three" = {
                                "default" (
                                    references = @''' + layer1.identifier + '''@</Ref>
                                ) {
                                }
                            }
                            over "Child" {
                                int variantTwoChildAttr
                                over "GrandChild" {
                                    int variantTwoGrandChildAttr
                                }
                            }
                            def "VariantTwoChild" {}
                            int variantTwoAttr
                        }
                    }
                }
            }
        }

        def "Prim" (
            references = </PrimVariants>
            variants = {
                string one = "default"
                string two = "default"
                string three = "default"
            }
        ) {
            over "Child" {
                int localChildAttr
                over "GrandChild" {
                    int localGrandChildAttr
                }
            }
            def "LocalChild" {}
            int localAttr
        }

        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages. 
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2)

        # Create a namespace editor for the first stage with stage2 as a 
        # dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields. Layer2 has a reference 
        # at the final nested variant spec path to layer1 and a reference from 
        # /Prim to /PrimVariants.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{one=default}{two=default}{three=default}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the initial contents of stage1 which is just the simple Ref,
        # Child, GrandChild hierarchy.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })

        # Verify the initial contents of stage2. 
        # /PrimVariants is empty because it has no variant selection for the
        # variant "one" so the variant (and thus its nested variants) is not
        # composed.
        # /Prim has the "default" variant selected for each of the nested 
        # variants "one", "two", and "three" in the referenced /PrimVariants so
        # it composes all available opinions from those variants. Specifically,
        # /Prim has composed opinions from 1) its local specs, 2) the opinions
        # in the "two" variant's "default" spec, and 3) layer1's /Ref as 
        # referenced in the "three" variant's "default" spec.
        composedPrimRootAttrs = ['refAttr', 'variantTwoAttr', 'localAttr']
        composedChildContents = {
            '.' : ['childAttr', 'variantTwoChildAttr', 'localChildAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'variantTwoGrandChildAttr', 
                        'localGrandChildAttr'],
            }           
        }
        primContents = {
            '.' : composedPrimRootAttrs,
            'Child' : composedChildContents,
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim': primContents,
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Rename /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath('/Ref/Child', '/Ref/RenamedChild'))

        # Verify that no composition fields have changed as none of them 
        # targeted /Ref/RenamedChild (or any of its descendants) directly.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{one=default}{two=default}{three=default}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are updated to reflect the simple rename
        # of Child to RenamedChild
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Ref/Child" : self.PrimResyncType.RenameSource,
            "/Ref/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # On stage2 the contents of Prim have changed to reflect the full rename
        # /Prim/Child to /Prim/RenamedChild as all specs that originally 
        # contributed to Child have been moved to RenamedChild.
        primContents = {
            '.' : composedPrimRootAttrs,
            'RenamedChild' : composedChildContents,
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim': primContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim/Child" : self.PrimResyncType.RenameSource,
            "/Prim/RenamedChild" : self.PrimResyncType.RenameDestination,
        })

        # Edit: Rename /Ref to /RenamedRef
        with self.ApplyEdits(editor, "Rename /Ref -> /RenamedRef"):
            self.assertTrue(editor.MovePrimAtPath('/Ref', '/RenamedRef'))

        # Verify that the reference to /Ref in layer2's nested variant spec
        # has been updated to refer to /RenamedRef.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{one=default}{two=default}{three=default}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/RenamedRef'),)
            },
            '/Prim' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are updated to reflect the simple rename
        # Ref to RenamedRef
        self._VerifyStageContents(stage1, {
            'RenamedRef': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }           
                }
            },
        })
        self._VerifyStageResyncNotices(stage1, {
            "/Ref" : self.PrimResyncType.RenameSource,
            "/RenamedRef" : self.PrimResyncType.RenameDestination,
        })

        # Verify that the contents of stage2 are completely unchanged as the 
        # update of the reference path in layer2 maintains the exact same 
        # composition.
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim': primContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim" : self.PrimResyncType.UnchangedPrimStack,
        })

        # Edit: Delete /RenamedRef/RenamedChild
        with self.ApplyEdits(editor, "Delete /RenamedRef/RenamedChild"):
            self.assertTrue(editor.DeletePrimAtPath('/RenamedRef/RenamedChild'))

        # Verify that no composition fields have changed as none of them 
        # targeted /RenamedRef/RenamedChild (or any of its descendants) 
        # directly.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{one=default}{two=default}{three=default}' : {
                'references' : (Sdf.Reference(layer1.identifier, '/RenamedRef'),)
            },
            '/Prim' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are updated to reflect that RenamedChild
        # has been deleted.
        self._VerifyStageContents(stage1, {
            'RenamedRef': {
                '.' : ['refAttr'],
            },
        })
        self._VerifyStageResyncNotices(stage1, {
            "/RenamedRef/RenamedChild" : self.PrimResyncType.Delete,
        })

        # On stage2 the contents of /Prim have changed to reflect that there are
        # no opinions coming across the reference to layer1 for ./RenamedChild 
        # and ./RenamedChild/GrandChild. And because these namespace descendant
        # opinions across the reference were deleted, we delete the downstream
        # overs for these opinions (local specs and the specs in variant "two")
        # resulting in the full deletion of RenamedChild and GrandChild from 
        # /Prim.
        primContents = {
            '.' : composedPrimRootAttrs,
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim': primContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim/RenamedChild" : self.PrimResyncType.Delete,
        })

        # Edit: Delete /RenamedRef/RenamedChild
        with self.ApplyEdits(editor, "Delete /RenamedRef"):
            self.assertTrue(editor.DeletePrimAtPath('/RenamedRef'))

        # Verify that the reference to /RenamedRef in layer2's nested variant 
        # spec has been deleted from the references field.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), { 
            '/PrimVariants{one=default}{two=default}{three=default}' : {
                'references' : ()
            },
            '/Prim' : {
                'references' : (Sdf.Reference(primPath = '/PrimVariants'),)
            },
        })

        # Verify the contents of stage1 are now empty given its only prim has
        # now been deleted.
        self._VerifyStageContents(stage1, {})
        self._VerifyStageResyncNotices(stage1, {
            "/RenamedRef" : self.PrimResyncType.Delete,
        })

        # On stage2 the contents of /Prim have changed to reflect that there are
        # no opinions from layer1 at all after the deleted reference. However, 
        # this only results in the removal of "refAttr" from /Prim's properties
        # as we do not delete the local specs and the specs in variant "two"
        # just because a direct reference was removed for a weaker node as that
        # would result in /Prim being completely deleted.
        composedPrimRootAttrs = ['variantTwoAttr', 'localAttr']
        primContents = {
            '.' : composedPrimRootAttrs,
            'VariantTwoChild' : {},
            'LocalChild' : {}       
        }
        self._VerifyStageContents(stage2, {
            'PrimVariants' : {},
            'Prim': primContents,
        })
        self._VerifyStageResyncNotices(stage2, {
            "/Prim" : self.PrimResyncType.Other,
        })

if __name__ == '__main__':
    unittest.main()
