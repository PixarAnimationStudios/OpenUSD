#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Sdf, Usd
from testUsdNamespaceEditorDependentEditsBase \
      import TestUsdNamespaceEditorDependentEditsBase

class TestUsdNamespaceEditorDependentEditsBasicReferencesAndPayloads(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests for how we handle dependent namespace edits across composition arcs
    when the edits would cause namespace conflicts with sibling composition arcs
    that we will not also make edits to.
    '''

    def test_SiblingReferenceWithSameHierarchy(self):
        """Test downstream dependency name space edits across basic references,
        when there is a namespace conflict with a sibling composition arc."""

        # Layer1 has two prims, /Ref1 and /Ref2, with the same basic namespace
        # descendant hierarchies of a "Child" and "GrandChild". The property 
        # names are different between the two hierarchies so we can tell which 
        # specs are contributing when both prims are referenced. The two 
        # hierarchies each have a uniquely named additional grandchild prim
        # under their "Child" prims for this example.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref1" {
                int ref1Attr

                def "Child" {
                    int child1Attr

                    def "GrandChild" {
                        int grandChild1Attr
                    }

                    def "Ref1OnlyGrandChild" {
                        int ref1OnlyGrandChildAttr
                    }
                }

                def "ChildSibling" {
                }
            }

            def "Ref2" {
                int ref2Attr

                def "Child" {
                    int child2Attr

                    def "GrandChild" {
                        int grandChild2Attr
                    }

                    def "Ref2OnlyGrandChild" {
                        int ref2OnlyGrandChildAttr
                    }
                }

                def "ChildSibling" {
                }
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer2 has two prims.
        # Prim1 references both /Ref1 and /Ref2 and provides local opinions
        #   for all the referenced namespace descendants
        # Prim2 references /Ref1/Child and /Ref2/Child and provide local 
        #   opinions for all the referenced namespace descendants.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0

            def "Prim1" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1>,
                    @''' + layer1.identifier + '''@</Ref2>
                ]
            ) {
                int localRefAttr

                over "Child" {
                    int localChildAttr

                    over "GrandChild" {
                        int localGrandChildAttr
                    }

                    over "Ref1OnlyGrandChild" {
                        int localRef1OnlyGrandChildAttr
                    }

                    over "Ref2OnlyGrandChild" {
                        int localRef2OnlyGrandChildAttr
                    }
                }
            }

            def "Prim2" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1/Child>,
                    @''' + layer1.identifier + '''@</Ref2/Child>
                ]
            ) {
                int localChildAttr

                over "GrandChild" {
                    int localGrandChildAttr
                }

                over "Ref1OnlyGrandChild" {
                    int localRef1OnlyGrandChildAttr
                }

                over "Ref2OnlyGrandChild" {
                    int localRef2OnlyGrandChildAttr
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

        # Verify initial composition fields. Layer2 has the references to 
        # both Ref prims as expected from the layer setup.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/Child'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Child')
                )
            },
        })

        # Verify initial contents of stage 1. Storing small subcontents
        # dictionaries will be useful for later comparisons.
        ref1ChildContents = {
            '.' : ['child1Attr'],
            'GrandChild' : {
                '.' : ['grandChild1Attr'],
            },
            'Ref1OnlyGrandChild' : {
                '.' : ['ref1OnlyGrandChildAttr'],
            },
        }

        ref2ChildContents = {
            '.' : ['child2Attr'],
            'GrandChild' : {
                '.' : ['grandChild2Attr'],
            },
            'Ref2OnlyGrandChild' : {
                '.' : ['ref2OnlyGrandChildAttr'],
            },
        }

        ref1Contents = {
            '.' : ['ref1Attr'],
            'Child' : ref1ChildContents,
            'ChildSibling' : {}
        }

        ref2Contents = {
            '.' : ['ref2Attr'],
            'Child' : ref2ChildContents,
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # Verify the initial contents of stage2. 
        # Prim1 is composed from specs under /Ref1, /Ref2, and local specs from 
        # layer2.
        # Prim2 is composed from specs under /Ref1/Child, /Ref2/Child, and local
        # specs from layer2.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'Child' : {
                '.' : ['child1Attr', 'child2Attr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChild1Attr', 'grandChild2Attr',
                           'localGrandChildAttr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['ref1OnlyGrandChildAttr', 
                           'localRef1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['ref2OnlyGrandChildAttr', 
                           'localRef2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {}
        }

        prim2Contents = {
            '.' : ['child1Attr', 'child2Attr', 'localChildAttr'],
            'GrandChild' : {
                '.' : ['grandChild1Attr', 'grandChild2Attr',
                       'localGrandChildAttr'],
            },
            'Ref1OnlyGrandChild' : {
                '.' : ['ref1OnlyGrandChildAttr', 'localRef1OnlyGrandChildAttr'],
            },
            'Ref2OnlyGrandChild' : {
                '.' : ['ref2OnlyGrandChildAttr', 'localRef2OnlyGrandChildAttr'],
            },
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Rename /Ref1/Child to be /Ref1/RenamedChild. This operation
        # is expected to produce warnings.
        with self.ApplyEdits(editor,
                "Move /Ref1/Child -> /Ref1/RenamedChild",
                expectWarnings = True):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref1/Child', '/Ref1/RenamedChild'))

        # The only composition arc that is updated is the one reference to 
        # /Ref1/Child in /Prim2
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/RenamedChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Child')
                )
            },
        })

        # Verify on stage1 that just /Ref1/Child is renamed.
        ref1Contents = {
            '.' : ['ref1Attr'],
            'RenamedChild' : ref1ChildContents,
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect that the reference
        # to /Ref1 brings in the name child RenamedChild instead of Child.
        # However, the reference to /Ref2 still brings in the namespace child,
        # Child. We do not update sibling arcs and do not author relocates to
        # move sibling arcs when processing downstream dependencies. 
        # Additionally, we do NOT move the local specs for child in layer2
        # because we've chosen to leave them attached to the remaining Child
        # prim. Thus we have the split here in stage2 where /Prim1/Child will
        # compose opinions from /Ref2 and the local layer stack, while
        # /Prim1/RenamedChild will only have opinions from /Ref1.
        #
        # It's important to specifically call out the behavior with regards to
        # the local specs for /Ref/Child/Ref1OnlyGrandChild which were local
        # overs for prim specs that only came from /Ref1/Child and do not exist
        # in /Ref2/Child. One could expect that these local specs would be moved
        # to /Prim1/RenamedChild/Ref1OnlyGrandChild so that they would continue
        # composing with the specs from under /Ref1. But we only look at the 
        # fact that these specs are considered to be local specs under 
        # /Prim/Child which we can't move because of the continued existense of
        # the sibling reference's "Child" prim. I.e. we don't traverse the 
        # namespace hierarchy at a more granular level after determining that
        # a local spec can't be moved due to an existing uneditable composed
        # spec.
        #
        # In contrast, /Prim2's contents do not change because the necessary
        # reference arc has been updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'Child' : {
                '.' : ['child2Attr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChild2Attr', 'localGrandChildAttr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['localRef1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['ref2OnlyGrandChildAttr', 
                           'localRef2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {},
            'RenamedChild' : {
                '.' : ['child1Attr'],
                'GrandChild' : {
                    '.' : ['grandChild1Attr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['ref1OnlyGrandChildAttr'],
                },
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Now rename /Ref2/Child to be /Ref2/RenamedChild to match the
        # namespace hierarchy of /Ref1 again. Unlike the previous edit, we do
        # not expect any warnings.
        with self.ApplyEdits(editor, "Move /Ref2/Child -> /Ref2/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/Child', '/Ref2/RenamedChild'))

        # Now the reference to /Ref2/Child in /Prim2 has been updated to use
        # the renamed path.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/RenamedChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/RenamedChild')
                )
            },
        })

        # Verify on stage1 that just /Ref2/Child has been renamed.
        ref2Contents = {
            '.' : ['ref2Attr'],
            'RenamedChild' : ref2ChildContents,
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect that the reference
        # to /Ref2 brings in the name child RenamedChild instead of Child. This
        # now "remerges" the contents of Child and RenamedChild into the single
        # prim RenamedChild since both references now provide specs for
        # RenamedChild. Note that this time we DO move the local specs for
        # /Prim1/Child to /Prim1/RenamedChild because /Prim1/Child has no more
        # ancestral opinions from other arcs and has been officially moved.
        # Thus, in two steps, we have moved the entire original contents of
        # /Prim1/Child to /Prim1/RenamedChild.
        # 
        # And since it was noted previously, we'll call out the local specs
        # for /Prim1/Child/Ref1OnlyGrandChild which are moved to 
        # /Prim1/RenamedChild/Ref1OnlyGrandChild because all local specs under
        # /Prim1/Child were moved along with it.
        #
        # In contrast, /Prim2's contents do not change because the necessary
        # reference arc has been updated again.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'RenamedChild' : {
                '.' : ['child1Attr', 'child2Attr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChild1Attr', 'grandChild2Attr',
                           'localGrandChildAttr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['ref1OnlyGrandChildAttr', 
                           'localRef1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['ref2OnlyGrandChildAttr', 
                           'localRef2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # These next few cases demonstrate reparenting RenamedChild in both
        # reference prims and how order can matter in this particular setup.

        # Edit: First reparent (and rename) /Ref1/RenamedChild to /MovedChild_1.
        # This moves the child prim out from being a descendant of the
        # referenced /Ref1
        with self.ApplyEdits(editor,
                "Move /Ref1/RenamedChild -> /MovedChild_1",
                expectWarnings = True):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref1/RenamedChild', '/MovedChild_1'))

        # The direct reference to /Ref1/RenamedChild in /Prim2 has been updated
        # to reference the new prim path of /MovedChild_1.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/MovedChild_1'),
                    Sdf.Reference(layer1.identifier, '/Ref2/RenamedChild')
                )
            },
        })

        # Verify on stage1 that just /Ref1/RenamedChild has been moved out of
        # /Ref1 and is now /MovedChild_1.
        ref1Contents = {
            '.' : ['ref1Attr'],
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'MovedChild_1' : ref1ChildContents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of /Prim1 reflect the fact the /Ref1 no longer
        # has RenamedChild as a namespace child and therefore RenamedChild has
        # no opinions from across that reference. However, the reference to
        # /Ref2 still provides opinions for RenamedChild so it still exists with
        # opinions from across /Ref2. And because the opinions remain, we keep
        # all the local opinions in layer2 for /Prim1/RenamedChild and its
        # descendants (which includes Ref1OnlyGrandChild for the same reason
        # explained in prior rename edits.)
        #
        # /Prim2's contents do not change because the reference arc has been
        # updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'RenamedChild' : {
                '.' : ['child2Attr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChild2Attr', 'localGrandChildAttr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['localRef1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['ref2OnlyGrandChildAttr', 
                           'localRef2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Next reparent (and rename) /Ref2/RenamedChild to
        # /Ref2/ChildSibling/MovedChild_2. This moves the
        # child prim so that it is still a descendant of the referenced /Ref2
        with self.ApplyEdits(editor, 
                "Move /Ref2/RenamedChild -> /Ref2/ChildSibling/MovedChild_2"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/RenamedChild', '/Ref2/ChildSibling/MovedChild_2'))

        # The direct reference to /Ref2/RenamedChild in /Prim2 has been updated
        # for the path change.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/MovedChild_1'),
                    Sdf.Reference(layer1.identifier,
                                  '/Ref2/ChildSibling/MovedChild_2')
                )
            },
        })

        # Verify on stage1 that just /Ref2/RenamedChild has been moved into
        # ChildSibling and renamed.
        ref2Contents = {
            '.' : ['ref2Attr'],
            'ChildSibling' : {
                'MovedChild_2' : ref2ChildContents,
            }
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'MovedChild_1' : ref1ChildContents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect the moving of
        # RenamedChild to be under ChildSibling as MovedChild_2. This time again
        # we DO move the local specs for /Prim1/RenamedChild to
        # /Prim1/ChildSibling/MovedChild_2 because /Prim1/RenamedChild has
        # no more ancestral opinions from other arcs and has been officially
        # moved.
        #
        # /Prim2's contents do not change because the reference arc has been
        # updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'ChildSibling' : {
                'MovedChild_2' : {
                    '.' : ['child2Attr', 'localChildAttr'],
                    'GrandChild' : {
                        '.' : ['grandChild2Attr', 'localGrandChildAttr'],
                    },
                    'Ref1OnlyGrandChild' : {
                        '.' : ['localRef1OnlyGrandChildAttr'],
                    },
                    'Ref2OnlyGrandChild' : {
                        '.' : ['ref2OnlyGrandChildAttr', 
                               'localRef2OnlyGrandChildAttr'],
                    },
                },
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Undo the the last two edits by moving both moved prims back to
        # /Ref2 and /Ref1 as RenamedChild in the reverse order we originally
        # moved them.
        with self.ApplyEdits(editor, 
                "Move /Ref2/ChildSibling/MovedChild_2 -> /Ref2/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/ChildSibling/MovedChild_2', '/Ref2/RenamedChild'))
        with self.ApplyEdits(editor, 
                "Move /MovedChild_1 -> /Ref1/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/MovedChild_1', '/Ref1/RenamedChild'))

        # Both reference arcs for /Prim2 have been updated.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/RenamedChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/RenamedChild')
                )
            },
        })

        # Verify on stage1 that /Ref1 and /Ref2 contents have returned to the
        # "RenamedChild" state.
        ref1Contents = {
            '.' : ['ref1Attr'],
            'RenamedChild' : ref1ChildContents,
            'ChildSibling' : {}
        }

        ref2Contents = {
            '.' : ['ref2Attr'],
            'RenamedChild' : ref2ChildContents,
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # On stage2, /Prim1's contents have returned to be the exact same state
        # as before the two reparent edits (a true undo). This is notable
        # because we did NOT delete the local specs for RenamedChild and
        # GrandChild at any point. This was directly the result of the order in
        # which we performed the reparent operations.
        #
        # /Prim2's contents do not change because the reference arc has been
        # updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'RenamedChild' : {
                '.' : ['child1Attr', 'child2Attr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChild1Attr', 'grandChild2Attr',
                           'localGrandChildAttr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['ref1OnlyGrandChildAttr', 
                           'localRef1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['ref2OnlyGrandChildAttr', 
                           'localRef2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # This time we're going to perform the prior reparent (and rename) in
        # reverse order.

        # Edit: First move /Ref2/RenamedChild to /Ref2/ChildSibling/MovedChild_2.
        # This moves the child prim so that it is still a descendant of the
        # referenced /Ref2
        with self.ApplyEdits(editor,
                "Move /Ref2/RenamedChild -> /Ref2/ChildSibling/MovedChild_2",
                expectWarnings = True):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/RenamedChild', '/Ref2/ChildSibling/MovedChild_2'))

        # The direct reference to /Ref2/RenamedChild in /Prim2 has been updated
        # for its path change.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/RenamedChild'),
                    Sdf.Reference(layer1.identifier,
                                  '/Ref2/ChildSibling/MovedChild_2')
                )
            },
        })

        # Verify on stage1 that just /Ref2/RenamedChild has been moved into
        # ChildSibling and renamed.
        ref2Contents = {
            '.' : ['ref2Attr'],
            'ChildSibling' : {
                'MovedChild_2' : ref2ChildContents,
            }
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect the moving of
        # RenamedChild to be under ChildSibling as MovedChild_2. But unlike the
        # last time, /Ref1/RenamedChild still exists so RenamedChild is still a
        # prim child of /Prim1. And because of that, we don't move the local
        # specs for /Prim1/RenamedChild to MovedChild_2 like we did last time.
        #
        # /Prim2's contents do not change because the reference arc has been
        # updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'RenamedChild' : {
                '.' : ['child1Attr', 'localChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChild1Attr', 'localGrandChildAttr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['ref1OnlyGrandChildAttr', 
                           'localRef1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['localRef2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {
                'MovedChild_2' : {
                    '.' : ['child2Attr'],
                    'GrandChild' : {
                        '.' : ['grandChild2Attr'],
                    },
                    'Ref2OnlyGrandChild' : {
                        '.' : ['ref2OnlyGrandChildAttr'],
                    },
                },
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Now move /Ref1/RenamedChild to /MovedChild_1 as the second
        # operation. This moves the child prim out from being a descendant of
        # the referenced /Ref1
        with self.ApplyEdits(editor, 
                "Move /Ref1/RenamedChild -> /MovedChild_1"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref1/RenamedChild', '/MovedChild_1'))

        # The direct reference to /Ref1/RenamedChild in /Prim2 has been updated
        # for the path change.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/MovedChild_1'),
                    Sdf.Reference(layer1.identifier,
                                  '/Ref2/ChildSibling/MovedChild_2')
                )
            },
        })

        # Verify on stage1 that just /Ref1/RenamedChild has been moved out of
        # /Ref1 and is now /MovedChild_1.
        ref1Contents = {
            '.' : ['ref1Attr'],
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'MovedChild_1' : ref1ChildContents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of /Prim1 reflect the fact the /Ref1 no longer
        # has RenamedChild as a namespace child and therefore RenamedChild has
        # no opinions from across that reference. And this time, because there
        # are no ancestral arc opinions for /Prim1/RenamedChild, we delete all
        # local opinions for /Prim1/RenamedChild (which is unlike last time 
        # where we kept these because of still existing ancestral opinions from
        # /Ref2). Note that we have no "post edit state" that would inform us 
        # that we could've possibly moved these opinions to MovedChild_2. Thus,
        # /Prim1's contents are in a different state than when we performed 
        # these two edits in the opposite order.
        #
        # /Prim2's contents do not change because the reference arc has been
        # updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'ChildSibling' : {
                'MovedChild_2' : {
                    '.' : ['child2Attr'],
                    'GrandChild' : {
                        '.' : ['grandChild2Attr'],
                    },
                    'Ref2OnlyGrandChild' : {
                        '.' : ['ref2OnlyGrandChildAttr'],
                    },
                },
            }
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Undo the last two edits again by moving both moved prims back
        # to /Ref2 and /Ref1 as RenamedChild in reverse order.
        with self.ApplyEdits(editor, "Move /MovedChild_1 -> /Ref1/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/MovedChild_1', '/Ref1/RenamedChild'))
        with self.ApplyEdits(editor,
                "Move /Ref2/ChildSibling/MovedChild_2 -> /Ref2/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref2/ChildSibling/MovedChild_2', '/Ref2/RenamedChild'))

        # Both reference arcs for /Prim2 have been updated.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/RenamedChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/RenamedChild')
                )
            },
        })

        # Verify on stage1 that /Ref1 and /Ref2 contents have returned to the
        # "RenamedChild" state.
        ref1Contents = {
            '.' : ['ref1Attr'],
            'RenamedChild' : ref1ChildContents,
            'ChildSibling' : {}
        }

        ref2Contents = {
            '.' : ['ref2Attr'],
            'RenamedChild' : ref2ChildContents,
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # This time, on stage2, /Prim1's contents have mostly returned to be the
        # same state as before the two reparents with exception that we no
        # longer have the local opinions for RenamedChild nor any of its 
        # namespace descendants that we had before these edits. This is because
        # the order we performed the edits caused us to have to delete these
        # local specs during the second edit and we have no way of restoring 
        # these deleted opinions.
        #
        # /Prim2's contents do not change because the reference arc has been
        # updated.
        prim1Contents = {
            '.' : ['ref1Attr', 'ref2Attr', 'localRefAttr'],
            'RenamedChild' : {
                '.' : ['child1Attr', 'child2Attr'],
                'GrandChild' : {
                    '.' : ['grandChild1Attr', 'grandChild2Attr'],
                },
                'Ref1OnlyGrandChild' : {
                    '.' : ['ref1OnlyGrandChildAttr'],
                },
                'Ref2OnlyGrandChild' : {
                    '.' : ['ref2OnlyGrandChildAttr'],
                },
            },
            'ChildSibling' : {}
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

    def test_DeletionForSiblingReferenceWithSameHierarchy(self):
        """Test downstream dependency name space deletion across references,
        when there is a namespace conflict with a sibling composition arc."""

        # Setup: Layer1 has two root prims, /Ref1 and /Ref2. /Ref1 and /Ref2
        # each have child prims with same name both, SharedChild_A and 
        # SharedChild_B. Under SharedChild_A and SharedChild_B, both have 
        # defined prims SharedGrandChild_A and SharedGrandChild_B for the 
        # respective SharedChild prims. Additionally each of /Ref1 and /Ref2
        # define their own uniquely named Ref#OnlyGrandChild prims under the
        # SharedChild prims. They also each root prim defines its own uniquely
        # named Ref#OnlyChild prim with a grand child prim. We'll be deleting
        # prims from a stage opened for this layer.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref1" {
                int ref1_Attr

                def "SharedChild_A" {
                    int ref1_sharedChild_A_Attr

                    def "SharedGrandChild_A" {
                        int ref1_sharedGrandChild_A_Attr
                    }

                    def "Ref1OnlyGrandChild_A" {
                        int ref1Only_GrandChild_A_Attr
                    }
                }
                def "SharedChild_B" {
                    int ref1_sharedChild_B_Attr

                    def "SharedGrandChild_B" {
                        int ref1_sharedGrandChild_B_Attr
                    }

                    def "Ref1OnlyGrandChild_B" {
                        int ref1Only_GrandChild_B_Attr
                    }
                }
                def "Ref1OnlyChild" {
                    int ref1OnlyChild_Attr

                    def "Ref1OnlyGrandChild" {
                        int ref1OnlyGrandChild_Attr
                    }
                }
            }

            def "Ref2" {
                int ref2_Attr

                def "SharedChild_A" {
                    int ref2_sharedChild_A_Attr

                    def "SharedGrandChild_A" {
                        int ref2_sharedGrandChild_A_Attr
                    }

                    def "Ref2OnlyGrandChild_A" {
                        int ref2Only_GrandChild_A_Attr
                    }
                }
                def "SharedChild_B" {
                    int ref2_sharedChild_B_Attr

                    def "SharedGrandChild_B" {
                        int ref2_sharedGrandChild_B_Attr
                    }

                    def "Ref2OnlyGrandChild_B" {
                        int ref2Only_GrandChild_B_Attr
                    }
                }
                def "Ref2OnlyChild" {
                    int ref2OnlyChild_Attr

                    def "Ref2OnlyGrandChild" {
                        int ref2OnlyGrandChild_Attr
                    }
                }
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer2 has two prims.
        # Prim1 references both /Ref1 and /Ref2 from layer1.
        # Prim2 references all of the child prims under /Ref1 and /Ref2 in 
        # layer1 directly (a total of six referenced prims).
        # Local spec opinions adding properties are defined for every prim path
        # that will be composed in from these references on both prims.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0

            def "Prim1" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1>,
                    @''' + layer1.identifier + '''@</Ref2>
                ]
            ) {
                int localRefAttr

                over "SharedChild_A" {
                    int localSharedChild_A_Attr

                    over "SharedGrandChild_A" {
                        int localSharedGrandChild_A_Attr
                    }

                    over "Ref1OnlyGrandChild_A" {
                        int localRef1OnlyGrandChild_A_Attr
                    }

                    over "Ref2OnlyGrandChild_A" {
                        int localRef2OnlyGrandChild_A_Attr
                    }
                }
                over "SharedChild_B" {
                    int localSharedChild_B_Attr

                    over "SharedGrandChild_B" {
                        int localSharedGrandChild_B_Attr
                    }

                    over "Ref1OnlyGrandChild_B" {
                        int localRef1OnlyGrandChild_B_Attr
                    }

                    over "Ref2OnlyGrandChild_B" {
                        int localRef2OnlyGrandChild_B_Attr
                    }
                }
                over "Ref1OnlyChild" {
                    int localRef1OnlyChild_Attr

                    over "Ref1OnlyGrandChild" {
                        int localRef1OnlyGrandChild_Attr
                    }
                }
                def "Ref2OnlyChild" {
                    int localRef2OnlyChild_Attr

                    over "Ref2OnlyGrandChild" {
                        int localRef2OnlyGrandChild_Attr
                    }
                }
            }

            def "Prim2" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1/SharedChild_A>,
                    @''' + layer1.identifier + '''@</Ref1/SharedChild_B>,
                    @''' + layer1.identifier + '''@</Ref1/Ref1OnlyChild>,
                    @''' + layer1.identifier + '''@</Ref2/SharedChild_A>,
                    @''' + layer1.identifier + '''@</Ref2/SharedChild_B>,
                    @''' + layer1.identifier + '''@</Ref2/Ref2OnlyChild>,
                ]
            ) {
                int localChildAttr

                over "SharedGrandChild_A" {
                    int localSharedGrandChild_A_Attr
                }
                over "Ref1OnlyGrandChild_A" {
                    int localRef1OnlyGrandChild_A_Attr
                }
                over "Ref2OnlyGrandChild_A" {
                    int localRef2OnlyGrandChild_A_Attr
                }

                over "SharedGrandChild_B" {
                    int localSharedGrandChild_B_Attr
                }
                over "Ref1OnlyGrandChild_B" {
                    int localRef1OnlyGrandChild_B_Attr
                }
                over "Ref2OnlyGrandChild_B" {
                    int localRef2OnlyGrandChild_B_Attr
                }

                over "Ref1OnlyGrandChild" {
                    int localRef1OnlyGrandChild_Attr
                }
                over "Ref2OnlyGrandChild" {
                    int localRef2OnlyGrandChild_Attr
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
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/SharedChild_A'),
                    Sdf.Reference(layer1.identifier, '/Ref1/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref1/Ref1OnlyChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/SharedChild_A'),
                    Sdf.Reference(layer1.identifier, '/Ref2/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Ref2OnlyChild'),
                )
            },
        })

        # Verify initial contents of stage 1. 
        # Since there is no composition happen across multiple layers in stage1
        # this is a basic iteration of the contents of layer1. Storing small 
        # subcontents dictionaries will be useful for use in later post-edit 
        # comparisons.
        ref1ChildAContents = {
            '.' : ['ref1_sharedChild_A_Attr'],
            'SharedGrandChild_A' : {
                '.' : ['ref1_sharedGrandChild_A_Attr'],
            },
            'Ref1OnlyGrandChild_A' : {
                '.' : ['ref1Only_GrandChild_A_Attr'],
            }
        }

        ref1ChildBContents = {
            '.' : ['ref1_sharedChild_B_Attr'],
            'SharedGrandChild_B' : {
                '.' : ['ref1_sharedGrandChild_B_Attr'],
            },
            'Ref1OnlyGrandChild_B' : {
                '.' : ['ref1Only_GrandChild_B_Attr'],
            },
        }

        ref1OnlyChildContents = {
            '.' : ['ref1OnlyChild_Attr'],
            'Ref1OnlyGrandChild' : {
                '.' : ['ref1OnlyGrandChild_Attr'],
            },
        }

        ref1Contents = {
            '.' : ['ref1_Attr'],
            'SharedChild_A' : ref1ChildAContents,
            'SharedChild_B' : ref1ChildBContents,
            'Ref1OnlyChild' : ref1OnlyChildContents
        }

        ref2ChildAContents = {
            '.' : ['ref2_sharedChild_A_Attr'],
            'SharedGrandChild_A' : {
                '.' : ['ref2_sharedGrandChild_A_Attr'],
            },
            'Ref2OnlyGrandChild_A' : {
                '.' : ['ref2Only_GrandChild_A_Attr'],
            },
        }

        ref2ChildBContents = {
            '.' : ['ref2_sharedChild_B_Attr'],
            'SharedGrandChild_B' : {
                '.' : ['ref2_sharedGrandChild_B_Attr'],
            },
            'Ref2OnlyGrandChild_B' : {
                '.' : ['ref2Only_GrandChild_B_Attr'],
            },
        }

        ref2OnlyChildContents = {
            '.' : ['ref2OnlyChild_Attr'],
            'Ref2OnlyGrandChild' : {
                '.' : ['ref2OnlyGrandChild_Attr'],
            },
        }

        ref2Contents = {
            '.' : ['ref2_Attr'],
            'SharedChild_A' : ref2ChildAContents,
            'SharedChild_B' : ref2ChildBContents,
            'Ref2OnlyChild' : ref2OnlyChildContents
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # Verify the initial contents of stage2. We organize this by multiple
        # contents dictionaries for organization and easier post-edit 
        # comparisons.
        #
        # /Prim1/SharedChild_A contents...
        # Properties composed from Ref1, Ref2, and local opinions
        composedSharedChild_A_Properties = [
            'ref1_sharedChild_A_Attr', 
            'ref2_sharedChild_A_Attr', 
            'localSharedChild_A_Attr']
        # SharedGrandChild_A composed from Ref1, Ref2, and local opinions
        composedSharedGrandChild_A_Contents = {
            '.' : ['ref1_sharedGrandChild_A_Attr', 
                   'ref2_sharedGrandChild_A_Attr', 
                   'localSharedGrandChild_A_Attr'],
        }
        # Ref1OnlyGrandChild_A composed from Ref1 and local opinions only
        composedRef1OnlyGrandChild_A_Contents = {
            '.' : ['ref1Only_GrandChild_A_Attr', 
                   'localRef1OnlyGrandChild_A_Attr'],
        }
        # Ref2OnlyGrandChild_A composed from Ref2 and local opinions only
        composedRef2OnlyGrandChild_A_Contents = {
            '.' : ['ref2Only_GrandChild_A_Attr', 
                   'localRef2OnlyGrandChild_A_Attr'],
        }
        # SharedChild_A all contents
        composedSharedChild_A_Contents = {
            '.' : composedSharedChild_A_Properties,
            'SharedGrandChild_A' : composedSharedGrandChild_A_Contents,
            'Ref1OnlyGrandChild_A' : composedRef1OnlyGrandChild_A_Contents,
            'Ref2OnlyGrandChild_A' : composedRef2OnlyGrandChild_A_Contents,
        }

        # /Prim1/SharedChild_B contents...
        # Properties composed from Ref1, Ref2, and local opinions
        composedSharedChild_B_Properties = [
            'ref1_sharedChild_B_Attr', 
            'ref2_sharedChild_B_Attr', 
            'localSharedChild_B_Attr']
        # SharedGrandChild_B composed from Ref1, Ref2, and local opinions
        composedSharedGrandChild_B_Contents = {
            '.' : ['ref1_sharedGrandChild_B_Attr', 
                   'ref2_sharedGrandChild_B_Attr', 
                   'localSharedGrandChild_B_Attr'],
        }
        # Ref1OnlyGrandChild_B composed from Ref1 and local opinions only
        composedRef1OnlyGrandChild_B_Contents = {
            '.' : ['ref1Only_GrandChild_B_Attr', 
                   'localRef1OnlyGrandChild_B_Attr'],
        }
        # Ref2OnlyGrandChild_B composed from Ref2 and local opinions only
        composedRef2OnlyGrandChild_B_Contents = {
            '.' : ['ref2Only_GrandChild_B_Attr', 
                   'localRef2OnlyGrandChild_B_Attr'],
        }
        # SharedChild_B all contents
        composedSharedChild_B_Contents = {
            '.' : composedSharedChild_B_Properties,
            'SharedGrandChild_B' : composedSharedGrandChild_B_Contents,
            'Ref1OnlyGrandChild_B' : composedRef1OnlyGrandChild_B_Contents,
            'Ref2OnlyGrandChild_B' : composedRef2OnlyGrandChild_B_Contents,
        }

        # /Prim1/Ref1OnlyChild contents...
        # Child and GrandChild contents composed from Ref1 and local only.
        composedRef1OnlyGrandChildContents = {
            '.' : ['ref1OnlyGrandChild_Attr', 'localRef1OnlyGrandChild_Attr'],
        }
        composedRef1OnlyChildContents =  {
            '.' : ['ref1OnlyChild_Attr', 'localRef1OnlyChild_Attr'],
            'Ref1OnlyGrandChild' : composedRef1OnlyGrandChildContents,
        }

        # /Prim1/Ref2OnlyChild contents...
        # Child and GrandChild contents composed from Ref2 and local only.
        composedRef2OnlyGrandChildContents = {
            '.' : ['ref2OnlyGrandChild_Attr', 'localRef2OnlyGrandChild_Attr'],
        }
        composedRef2OnlyChildContents = {
            '.' : ['ref2OnlyChild_Attr', 'localRef2OnlyChild_Attr'],
            'Ref2OnlyGrandChild' : composedRef2OnlyGrandChildContents,
        }

        # /Prim1 combined contents
        prim1Contents = {
            '.' : ['ref1_Attr', 'ref2_Attr', 'localRefAttr'],
            'SharedChild_A' : composedSharedChild_A_Contents,
            'SharedChild_B' : composedSharedChild_B_Contents,
            'Ref1OnlyChild' : composedRef1OnlyChildContents,
            'Ref2OnlyChild' : composedRef2OnlyChildContents
        }

        # /Prim2 properties are composed from each of the directly referenced
        # child prims of Ref1 and Ref2 plus the locally defined attribute.
        composedPrim2Properties = [
            'ref1_sharedChild_A_Attr', 'ref2_sharedChild_A_Attr', 
            'ref1_sharedChild_B_Attr', 'ref2_sharedChild_B_Attr',
            'ref1OnlyChild_Attr', 'ref2OnlyChild_Attr', 'localChildAttr']
        # /Prim2's combined contents include all the composed GrandChild prims
        # of the referenced child prims. Note that these content match the 
        # contents of the GrandChild prims under Prim1 purely because we defined
        # identical local property specs to the ones defined for prim1 for these
        # grandchild paths.
        prim2Contents = {
            '.' : composedPrim2Properties,
            'SharedGrandChild_A' : composedSharedGrandChild_A_Contents,
            'Ref1OnlyGrandChild_A' : composedRef1OnlyGrandChild_A_Contents,
            'Ref2OnlyGrandChild_A' : composedRef2OnlyGrandChild_A_Contents,
            'SharedGrandChild_B' : composedSharedGrandChild_B_Contents,
            'Ref1OnlyGrandChild_B' : composedRef1OnlyGrandChild_B_Contents,
            'Ref2OnlyGrandChild_B' : composedRef2OnlyGrandChild_B_Contents,
            'Ref1OnlyGrandChild' : composedRef1OnlyGrandChildContents,
            'Ref2OnlyGrandChild' : composedRef2OnlyGrandChildContents,
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Delete /Ref1/SharedChild_A
        with self.ApplyEdits(editor, "Delete /Ref1/SharedChild_A",
                expectWarnings = True):
            self.assertTrue(editor.DeletePrimAtPath('/Ref1/SharedChild_A'))

        # The only composition arc to update is the one reference to 
        # /Ref1/SharedChild_A in /Prim2 which is now deleted from its 
        # references.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref1/Ref1OnlyChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/SharedChild_A'),
                    Sdf.Reference(layer1.identifier, '/Ref2/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Ref2OnlyChild'),
                )
            },
        })

        # Verify on stage1 that just only ShareChild_A has been deleted from 
        # /Ref1; no other contensts have changed.
        ref1Contents = {
            '.' : ['ref1_Attr'],
            'SharedChild_B' : ref1ChildBContents,
            'Ref1OnlyChild' : ref1OnlyChildContents
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect that the reference
        # to /Ref1 no longer brings in the child prim SharedChild_A. However, we
        # cannot remove any local opinions in layer2 for /Prim1/SharedChild_A 
        # because the reference to /Ref2 still brings in opinions for 
        # SharedChild_A that we cannot delete. So we end up with an incomplete
        # delete on stage2 where /Prim1/SharedChild_A and its descendants still 
        # exist with only local opinions and opinions for /Ref2.
        # 
        # Similarly, on /Prim2, the child prim SharedGrandChild_A was composed
        # from local opinions and referenced opinions from 
        # /Ref1/SharedChild_A/SharedGrandChild_A and 
        # /Ref2/SharedChild_A/SharedGrandChild_A. But with the reference to
        # /Ref1/SharedChild_A deleted, we still can't delete the opinions for
        # SharedGrandChild_A from sibling /Ref2/ShareChild_A so the composed
        # SharedGrandChild_A still exists with local and /Ref2 opinions, 
        # matching the corresponding prim under /Prim1. Ref1OnlyGrandChild_A,
        # however, only composed local opinions and opinions from 
        # /Ref1/SharedChild_A so those local opinions, and therefore prim, are
        # fully deleted.
        composedSharedChild_A_Properties = [
            'ref2_sharedChild_A_Attr', 'localSharedChild_A_Attr']
        composedSharedGrandChild_A_Contents = {
            '.' : ['ref2_sharedGrandChild_A_Attr', 'localSharedGrandChild_A_Attr'],
        }
        composedRef1OnlyGrandChild_A_Contents = {
            '.' : ['localRef1OnlyGrandChild_A_Attr'],
        }
        composedSharedChild_A_Contents = {
            '.' : composedSharedChild_A_Properties,
            'SharedGrandChild_A' : composedSharedGrandChild_A_Contents,
            'Ref1OnlyGrandChild_A' : composedRef1OnlyGrandChild_A_Contents,
            'Ref2OnlyGrandChild_A' : composedRef2OnlyGrandChild_A_Contents,
        }
        prim1Contents = {
            '.' : ['ref1_Attr', 'ref2_Attr', 'localRefAttr'],
            'SharedChild_A' : composedSharedChild_A_Contents,
            'SharedChild_B' : composedSharedChild_B_Contents,
            'Ref1OnlyChild' : composedRef1OnlyChildContents,
            'Ref2OnlyChild' : composedRef2OnlyChildContents
        }

        composedPrim2Properties = [
            'ref2_sharedChild_A_Attr', 'ref1_sharedChild_B_Attr', 
            'ref2_sharedChild_B_Attr',
            'ref1OnlyChild_Attr', 'ref2OnlyChild_Attr', 'localChildAttr']
        prim2Contents = {
            '.' : composedPrim2Properties,
            'SharedGrandChild_A' : composedSharedGrandChild_A_Contents,
            'Ref2OnlyGrandChild_A' : composedRef2OnlyGrandChild_A_Contents,   
            'SharedGrandChild_B' : composedSharedGrandChild_B_Contents,
            'Ref1OnlyGrandChild_B' : composedRef1OnlyGrandChild_B_Contents,
            'Ref2OnlyGrandChild_B' : composedRef2OnlyGrandChild_B_Contents,
            'Ref1OnlyGrandChild' : composedRef1OnlyGrandChildContents,
            'Ref2OnlyGrandChild' : composedRef2OnlyGrandChildContents,
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Delete /Ref2/SharedChild_A
        with self.ApplyEdits(editor, "Delete /Ref2/SharedChild_A"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref2/SharedChild_A'))

        # The only composition arc to update is the one reference to 
        # /Ref2/SharedChild_A in /Prim2 which is now deleted from its 
        # references.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref1/Ref1OnlyChild'),
                    Sdf.Reference(layer1.identifier, '/Ref2/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Ref2OnlyChild'),
                )
            },
        })

        # Verify on stage1 that just only ShareChild_A has been deleted from 
        # /Ref2 now; no other contensts have changed.
        ref2Contents = {
            '.' : ['ref2_Attr'],
            'SharedChild_B' : ref2ChildBContents,
            'Ref2OnlyChild' : ref2OnlyChildContents
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect that the reference
        # to /Ref2 no longer brings in the child prim SharedChild_A. However, 
        # this time, we don't have any opinions from other sibling arcs for 
        # SharedChild_A (they were deleted in the prior edit) so we do remove
        # all local opinions in layer2 for /Prim1/SharedChild_A. So we end up 
        # with a full deletion of the composed /Prim1/SharedChild_A on stage2.
        # 
        # Similarly, on /Prim2, the child prim SharedGrandChild_A was composed
        # from the remaining local opinions and the referenced opinions from 
        # /Ref2/SharedChild_A/SharedGrandChild_A. But with the remaining 
        # reference deleted, we can delete the local opinions for
        # SharedGrandChild_A and SharedGrandChild_A no longer exists as a child
        # of /Prim2.
        prim1Contents = {
            '.' : ['ref1_Attr', 'ref2_Attr', 'localRefAttr'],
            'SharedChild_B' : composedSharedChild_B_Contents,
            'Ref1OnlyChild' : composedRef1OnlyChildContents,
            'Ref2OnlyChild' : composedRef2OnlyChildContents
        }

        composedPrim2Properties = [
            'ref1_sharedChild_B_Attr', 'ref2_sharedChild_B_Attr',
            'ref1OnlyChild_Attr', 'ref2OnlyChild_Attr', 'localChildAttr']
        prim2Contents = {
            '.' : composedPrim2Properties,
            'SharedGrandChild_B' : composedSharedGrandChild_B_Contents,
            'Ref1OnlyGrandChild_B' : composedRef1OnlyGrandChild_B_Contents,
            'Ref2OnlyGrandChild_B' : composedRef2OnlyGrandChild_B_Contents,
            'Ref1OnlyGrandChild' : composedRef1OnlyGrandChildContents,
            'Ref2OnlyGrandChild' : composedRef2OnlyGrandChildContents,
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Delete /Ref1
        with self.ApplyEdits(editor, "Delete /Ref1",
                expectWarnings = True):
            self.assertTrue(editor.DeletePrimAtPath('/Ref1'))

        # This time /Prim1's reference to /Ref1 is removed as well as the 
        # references in /Prim2 to /Ref1/SharedChild_B and /Ref1/Ref1OnlyChild.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref2'),
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref2/SharedChild_B'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Ref2OnlyChild'),
                )
            },
        })

        # Verify on stage1 that /Ref1 is gone. /Ref2 is unchanged.
        self._VerifyStageContents(stage1, {
            'Ref2' : ref2Contents,
        })

        # On stage2, the contents of Prim1 change to reflect that the reference
        # to /Ref1 no longer exists. This means in no longer brings in any 
        # properties, any opinions for the child prim SharedChild_B, or any 
        # opinions for the child prim Ref1OnlyChild. For Ref1OnlyChild, this 
        # prim is now fully deleted along with any local opinions since /Ref2
        # has no opinions for this prim. However, we cannot remove any local 
        # opinions in layer2 for /Prim1/SharedChild_B because the reference to
        # /Ref2 still brings in opinions for SharedChild_B that we cannot 
        # delete. So we still end up with an incomplete delete on stage2 where
        # /Prim1/SharedChild_B and its descendants still exist with only local
        # opinions and opinions for /Ref2.
        # 
        # Similarly, on /Prim2, the child prim SharedGrandChild_B was composed
        # from local opinions and referenced opinions from 
        # /Ref1/SharedChild_B/SharedGrandChild_B and 
        # /Ref2/SharedChild_B/SharedGrandChild_B. But with the reference to
        # /Ref1/SharedChild_B deleted, we still can't delete the opinions for
        # SharedGrandChild_B from sibling /Ref2/ShareChild_B so the composed
        # SharedGrandChild_B still exists with local and /Ref2 opinions, 
        # matching the corresponding prim under /Prim1. Ref1OnlyGrandChild_B,
        # however, only composed local opinions and opinions from 
        # /Ref1/SharedChild_B so those local opinions, and therefore prim, are
        # fully deleted. Additionally, this delete also removed the reference
        # to /Ref1/Ref1OnlyChild which brought in the only non-local opinions 
        # that defined Ref1OnlyGrandChild, so that whole prim (including its 
        # local opinions) has been deleted from /Prim2
        composedSharedChild_B_Properties = [
            'ref2_sharedChild_B_Attr', 'localSharedChild_B_Attr']
        composedSharedGrandChild_B_Contents = {
            '.' : ['ref2_sharedGrandChild_B_Attr', 'localSharedGrandChild_B_Attr'],
        }
        composedRef1OnlyGrandChild_B_Contents = {
            '.' : ['localRef1OnlyGrandChild_B_Attr'],
        }
        composedSharedChild_B_Contents = {
            '.' : composedSharedChild_B_Properties,
            'SharedGrandChild_B' : composedSharedGrandChild_B_Contents,
            'Ref1OnlyGrandChild_B' : composedRef1OnlyGrandChild_B_Contents,
            'Ref2OnlyGrandChild_B' : composedRef2OnlyGrandChild_B_Contents,
        }
        prim1Contents = {
            '.' : ['ref2_Attr', 'localRefAttr'],
            'SharedChild_B' : composedSharedChild_B_Contents,
            'Ref2OnlyChild' : composedRef2OnlyChildContents
        }

        composedPrim2Properties = [
            'ref2_sharedChild_B_Attr','ref2OnlyChild_Attr', 'localChildAttr']
        prim2Contents = {
            '.' : composedPrim2Properties,
            'SharedGrandChild_B' : composedSharedGrandChild_B_Contents,
            'Ref2OnlyGrandChild_B' : composedRef2OnlyGrandChild_B_Contents,
            'Ref2OnlyGrandChild' : composedRef2OnlyGrandChildContents,
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Delete /Ref2
        with self.ApplyEdits(editor, "Delete /Ref2"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref2'))

        # The only references left referred to /Ref2 or its descendants so all
        # reference from both /Prim1 and /Prim2 have been removed.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : ()
            },
            '/Prim2' : {
                'references' : ()
            },
        })

        # Now, with all references having been removed, there are no sibling 
        # arcs to prevent us from deleting local opinions for child prims of 
        # either of our composed prims so all child prims have been fully
        # deleted. The only remaining contents are the local properties defined
        # for /Prim1 and /Prim2 themselves on layer2.
        prim1Contents = {
            '.' : ['localRefAttr'],
        }
        prim2Contents = {
            '.' : ['localChildAttr'],
        }
        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

    def test_DeletionForSiblingReferenceWithSameHierarchyWithRelocates(self):
        """Test downstream dependency namespace edits across basic references,
        when there is a namespace conflict with a sibling composition arc and 
        the conflicting sibling specs involve relocates. This test should 
        behave similarly to the test case without relocates, but this explicitly
        tests that relocates do not affect the expected behavior."""

        # Setup:
        # First layer is a source layer that will be referenced in the next
        # layer. The /Root prim will be referenced and its two children will
        # be relocated into a different hieararchy that is explained in the next
        # layer.
        reloSourceLayer = Sdf.Layer.CreateAnonymous("reloSourceLayer.usda")
        reloSourceLayer.ImportFromString('''#usda 1.0
            def "Root" {
                int reloSourceAttr

                def "UnrelocatedChild" {
                    int reloSourceChildAttr       
                }

                def "UnrelocatedGrandChild" {
                    int reloSourceGrandChildAttr
                }
            }
        ''')

        # Layer1 has two prims, /Ref1 and /Ref2, which have the same Child and 
        # GrandChild namespace descendant hierarchies when composed. /Ref1's 
        # is explicit specs defining the hierarchy. /Ref2, on the other hand,
        # references the /Root from the above reloSourceLayer and then uses 
        # relocates to rename UnrelocatedChild to be Child and to move and 
        # rename UnrelocatedGrandChild to be GrandChild under Child. These 
        # "effectively identical" prim hierarchies are referenced in the next
        # layer.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            (
                relocates = {
                    </Ref2/UnrelocatedChild> : </Ref2/Child>,
                    </Ref2/UnrelocatedGrandChild> : </Ref2/Child/GrandChild>
                }
            )

            def "Ref1" {
                int ref1Attr
                
                def "Child" {
                    int child1Attr
                    
                    def "GrandChild" {
                        int grandChild1Attr
                    }
                }
            }

            def "Ref2" (
                references = @''' + reloSourceLayer.identifier + '''@</Root>
            ) {
            }
        ''')

        # Layer2 has two prims. 
        # Prim1 references both /Ref1 and /Ref2 and provides local opinions for
        #   their descendant prims.
        # Prim2 references /Ref1/Child and /Ref2/Child and provides local 
        #   opinions for their descendant prims.
        # As stated, Ref1 and Ref2 both have the same composed prim descendants.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            def "Prim1" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1>,
                    @''' + layer1.identifier + '''@</Ref2>
                ]
            ) {
                int localRefAttr

                over "Child" {
                    int localChildAttr

                    over "GrandChild" {
                        int localGrandChildAttr
                    }
                }
            }

            def "Prim2" (
                references = [
                    @''' + layer1.identifier + '''@</Ref1/Child>,
                    @''' + layer1.identifier + '''@</Ref2/Child>
                ]
            ) {
                int localChildAttr

                over "GrandChild" {
                    int localGrandChildAttr
                }
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # We'll edit the first stage with stage2 as a dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage1)
        editor.AddDependentStage(stage2)

        # Verify initial composition fields on layer1 which has the reference
        # and relocates for Ref2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/' : {
                'relocates' : [
                    ('/Ref2/UnrelocatedChild', '/Ref2/Child'),
                    ('/Ref2/UnrelocatedGrandChild', '/Ref2/Child/GrandChild')
                ]
            },
            '/Ref2' : {
                'references' : (
                    Sdf.Reference(reloSourceLayer.identifier, '/Root'),
                )
            },

        })

        # Verify initial composition fields on layer2 which has the references
        # to both Ref1 and Ref2 for each prim.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1/Child'),
                    Sdf.Reference(layer1.identifier, '/Ref2/Child'),
                )
            },
        })

        # Verify initial contents of stage 1. Storing small subcontents
        # dictionaries will be useful for later comparisons. As expected, the 
        # /Ref1 and /Ref2 have the same composed prim name hierarchy but just
        # have different composed attributes as they compose different specs.
        ref1ChildContents = {
            '.' : ['child1Attr'],
            'GrandChild' : {
                '.' : ['grandChild1Attr'],
            },
        }

        ref2ChildContents = {
            '.' : ['reloSourceChildAttr'],
            'GrandChild' : {
                '.' : ['reloSourceGrandChildAttr'],
            },
        }

        ref1Contents = {
            '.' : ['ref1Attr'],
            'Child' : ref1ChildContents,
        }

        ref2Contents = {
            '.' : ['reloSourceAttr'],
            'Child' : ref2ChildContents,
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # Verify the initial contents of stage2. Each prim is composed from
        # specs under /Ref1, the relocation source specs (brought in via /Ref2),
        # and local specs from layer2.
        composedRootProperties = [
            'ref1Attr', 'reloSourceAttr', 'localRefAttr']
        composedChildProperties = [
            'child1Attr', 'reloSourceChildAttr', 'localChildAttr']
        composedGrandChildProperties = [
            'grandChild1Attr', 'reloSourceGrandChildAttr', 'localGrandChildAttr']
        
        prim1Contents = {
            '.' : composedRootProperties,
            'Child' : {
                '.' : composedChildProperties,
                'GrandChild' : {
                    '.' : composedGrandChildProperties
                },
            },
        }

        prim2Contents = {
            '.' : composedChildProperties,
            'GrandChild' : {
                '.' : composedGrandChildProperties,
            },
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Delete /Ref1/Child
        with self.ApplyEdits(editor, "Delete /Ref1/Child",
                expectWarnings = True):
            self.assertTrue(editor.DeletePrimAtPath("/Ref1/Child"))

        # Verify that layer1's composition fields haven't changed as all its
        # composition fields relate to /Ref2, not /Ref1
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/' : {
                'relocates' : [
                    ('/Ref2/UnrelocatedChild', '/Ref2/Child'),
                    ('/Ref2/UnrelocatedGrandChild', '/Ref2/Child/GrandChild')
                ]
            },
            '/Ref2' : {
                'references' : (
                    Sdf.Reference(reloSourceLayer.identifier, '/Root'),
                )
            },

        })

        # Verify in layer2 that /Prim2's reference to /Ref1/Child has been 
        # removed now that that prim spec has been deleted. The rest of the 
        # composition fields haven't changed.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref2/Child'),
                )
            },
        })

        # Verify the new contents of stage1. The contents of /Ref1 no longer 
        # has the deleted Child prim.
        ref1Contents = {
            '.' : ['ref1Attr'],
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # Verify the new contents of stage2. /Prim1/Child, 
        # /Prim1/Child/GrandChild, and /Prim2/Child all no longer have opinions
        # that come from /Ref1/Child or /Ref1/Child/GrandChild. But these prims
        # could not be deleted because of the relocation source specs that still
        # compose from the sibling reference to /Ref2. We also don't delete the 
        # local specs for these prims until the prims themselves are deleted.
        composedChildProperties = [
            'reloSourceChildAttr', 'localChildAttr']
        composedGrandChildProperties = [
            'reloSourceGrandChildAttr', 'localGrandChildAttr']

        prim1Contents = {
            '.' : composedRootProperties,
            'Child' : {
                '.' : composedChildProperties,
                'GrandChild' : {
                    '.' : composedGrandChildProperties,
                },
            },
        }

        prim2Contents = {
            '.' : composedChildProperties,
            'GrandChild' : {
                '.' : composedGrandChildProperties,
            },
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

        # Edit: Delete /Ref2/Child
        with self.ApplyEdits(editor, "Delete /Ref2/Child"):
            self.assertTrue(editor.DeletePrimAtPath("/Ref2/Child"))

        # Verify composition field changes on layer1. Because /Ref2/Child and 
        # /Ref2/Child/GrandChild were relocation target paths, the deletion 
        # causes the two relocates to be updated so their targets are the 
        # empty path, which results in the /Ref2/Child and 
        # /Ref2/Child/GrandChild being deleted in composition without the 
        # relocation source prims all of a sudden existing.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/' : {
                'relocates' : [
                    ('/Ref2/UnrelocatedChild', Sdf.Path.emptyPath),
                    ('/Ref2/UnrelocatedGrandChild', Sdf.Path.emptyPath),
                ]
            },
            '/Ref2' : {
                'references' : (
                    Sdf.Reference(reloSourceLayer.identifier, '/Root'),
                )
            },

        })

        # Verify composition field changes on layer2. Because /Ref2/Child is
        # effectively deleted in layer1, /Prim2's reference to /Ref2/Child is
        # removed.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (
                    Sdf.Reference(layer1.identifier, '/Ref1'),
                    Sdf.Reference(layer1.identifier, '/Ref2')
                )
            },
            '/Prim2' : {
                'references' : ()
            },
        })

        # Verify the new contents of stage1. The contents of /Ref2 no longer 
        # has the deleted Child prim.
        ref2Contents = {
            '.' : ['reloSourceAttr'],
        }

        self._VerifyStageContents(stage1, {
            'Ref1' : ref1Contents,
            'Ref2' : ref2Contents,
        })

        # Verify the new contents of stage2. This time all descendants of 
        # /Prim1 and /Prim2 have been deleted as there no other sibling nodes
        # with specs that would prevent us from deleting those descendant prims
        # and their local specs.
        # 
        # XXX: Note that /Prim2's local specs are not deleted even though its 
        # references were deleted as that would cause /Prim2 to not exist at 
        # all. It is still an open question as to whether that would be a more 
        # desirable behavior.
        prim1Contents = {
            '.' : composedRootProperties,
        }

        prim2Contents = {
            '.' : ['localChildAttr'],
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
        })

if __name__ == '__main__':
    unittest.main()
