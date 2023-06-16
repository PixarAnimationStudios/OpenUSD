#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

import unittest

from pxr import Pcp, Sdf

def LoadPcpCache(rootLayer, sessionLayer = None):
    l = Sdf.Layer.FindOrOpen(rootLayer)
    sl = None
    if sessionLayer is not None:
        sl = Sdf.Layer.FindOrOpen(sessionLayer)

    return Pcp.Cache(Pcp.LayerStackIdentifier(l, sl))

class TestPcpExpressionComposition(unittest.TestCase):
    def AssertVariables(self, pcpCache, path, expected):
        '''Helper function for verifying the expected value of 
        expression variables in the layer stacks throughout the prim index
        for the prim at the given path.

        The "expected" parameter is a list that mirrors the tree
        structure of the prim index:

        expected  : [nodeEntry]
        nodeEntry : (root layer id, expressionVars), [nodeEntry, ...]

        The first entry in "expected" corresponds to the expected expression
        variables dictionary in the root node, followed by a list of
        entries corresponding to the children of the root node, etc.
        '''
        def _recurse(node, expected):
            self.assertEqual(
                node.layerStack.identifier.rootLayer,
                Sdf.Layer.Find(expected[0][0]))

            self.assertEqual(
                node.layerStack.expressionVariables.GetVariables(),
                expected[0][1],
                "Unexpected expression variables for layer stack {}"
                .format(node.layerStack.identifier))

            for idx, n in enumerate(node.children):
                _recurse(n, expected[1][(idx*2):(idx*2)+2])

        pi, err = pcpCache.ComputePrimIndex(path)
        self.assertFalse(err, "Unexpected composition errors: {}".format(
            ",".join(str(e) for e in err)))
        _recurse(pi.rootNode, expected)

        return pi

    def test_ExpressionVarChanges_MultipleReferences(self):
        """Test expression variable changes involving multiple references on
        a single prim."""
        pcpCache = LoadPcpCache('multi_ref/root.sdf')
        rootLayer = Sdf.Layer.FindOrOpen('multi_ref/root.sdf')
        ref1Layer = Sdf.Layer.FindOrOpen('multi_ref/ref1.sdf')
        ref2Layer = Sdf.Layer.FindOrOpen('multi_ref/ref2.sdf')

        self.AssertVariables(
            pcpCache, '/MultiRef',
            expected = [
                ('multi_ref/root.sdf', {}), [
                    ('multi_ref/ref1.sdf', {'SOURCE':'ref1'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'ref1'}), [
                        ]
                    ],
                    ('multi_ref/ref2.sdf', {'SOURCE':'ref2'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'ref2'}), [
                        ]
                    ],
                ]
            ])

        with Pcp._TestChangeProcessor(pcpCache) as changes:
            rootLayer.expressionVariables = {'SOURCE':'root'}
            self.assertEqual(changes.GetSignificantChanges(), ['/MultiRef'])

        multiRef = self.AssertVariables(
            pcpCache, '/MultiRef',
            expected = [
                ('multi_ref/root.sdf', {'SOURCE':'root'}), [
                    ('multi_ref/ref1.sdf', {'SOURCE':'root'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'root'}), [
                        ]
                    ],
                    ('multi_ref/ref2.sdf', {'SOURCE':'root'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'root'}), [
                        ]
                    ],
                ]
            ])

        self.assertEqual(
            multiRef.rootNode.children[0].children[0].layerStack,
            multiRef.rootNode.children[1].children[0].layerStack)

        with Pcp._TestChangeProcessor(pcpCache) as changes:
            with Sdf.ChangeBlock():
                ref1Layer.expressionVariables = {'A':'B'}
                ref2Layer.expressionVariables = {'A':'C'}

            self.assertEqual(changes.GetSignificantChanges(), ['/MultiRef'])

        multiRef = self.AssertVariables(
            pcpCache, '/MultiRef',
            expected = [
                ('multi_ref/root.sdf', {'SOURCE':'root'}), [
                    ('multi_ref/ref1.sdf', {'SOURCE':'root', 'A':'B'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'root', 'A':'B'}), [
                        ]
                    ],
                    ('multi_ref/ref2.sdf', {'SOURCE':'root', 'A':'C'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'root', 'A':'C'}), [
                        ]
                    ],
                ]
            ])
        
        self.assertNotEqual(
            multiRef.rootNode.children[0].children[0].layerStack,
            multiRef.rootNode.children[1].children[0].layerStack)
        
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            rootLayer.expressionVariables = {'SOURCE':'root', 'A':'D'}
            self.assertEqual(changes.GetSignificantChanges(), ['/MultiRef'])

        multiRef = self.AssertVariables(
            pcpCache, '/MultiRef',
            expected = [
                ('multi_ref/root.sdf', {'SOURCE':'root', 'A':'D'}), [
                    ('multi_ref/ref1.sdf', {'SOURCE':'root', 'A':'D'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'root', 'A':'D'}), [
                        ]
                    ],
                    ('multi_ref/ref2.sdf', {'SOURCE':'root', 'A':'D'}), [
                        ('multi_ref/base_ref.sdf', {'SOURCE':'root', 'A':'D'}), [
                        ]
                    ],
                ]
            ])

        self.assertEqual(
            multiRef.rootNode.children[0].children[0].layerStack,
            multiRef.rootNode.children[1].children[0].layerStack)

    def test_ExpressionVarChanges_CommonReference(self):
        """Test that changes to authored expression variables invalidate the
        appropriate layer stacks and prim pcpCache."""
        pcpCache = LoadPcpCache('common_ref/root.sdf')
        ref1Layer = Sdf.Layer.FindOrOpen('common_ref/ref1.sdf')
        ref2Layer = Sdf.Layer.FindOrOpen('common_ref/ref2.sdf')
        
        ref1 = self.AssertVariables(
            pcpCache, '/A',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref1.sdf', {'A':'B'}), [
                        ('common_ref/base_ref.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        ref2 = self.AssertVariables(
            pcpCache, '/B',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref2.sdf', {'A':'B'}), [
                        ('common_ref/base_ref.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        # We expect the root.sdf and base_ref.sdf layer stacks to be shared
        # across the two prim indexes, even though base_ref.sdf is referenced
        # from different layer stacks.
        self.assertEqual(ref1.rootNode.layerStack, ref2.rootNode.layerStack)
        self.assertNotEqual(ref1.rootNode.children[0].layerStack, 
                            ref2.rootNode.children[0].layerStack)
        self.assertEqual(ref1.rootNode.children[0].children[0].layerStack, 
                         ref2.rootNode.children[0].children[0].layerStack)

        # Author new expression variables in ref1.sdf. This should affect only
        # /A and not /B, since /B does not reference ref1.sdf. The new
        # expression variables should show up in the ref1.sdf and base_ref.sdf
        # layer stacks.
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            ref1Layer.expressionVariables = {'X':'Y'}
            self.assertEqual(changes.GetSignificantChanges(), ['/A'])

        ref1 = self.AssertVariables(
            pcpCache, '/A',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref1.sdf', {'A':'B', 'X':'Y'}), [
                        ('common_ref/base_ref.sdf', {'A':'B', 'X':'Y'}), [
                        ]
                    ]
                ]
            ])

        ref2 = self.AssertVariables(
            pcpCache, '/B',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref2.sdf', {'A':'B'}), [
                        ('common_ref/base_ref.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        # At this point, the base_ref.sdf layer stacks must differ between
        # the two prim indexes since they have different composed expression
        # variables.
        self.assertEqual(ref1.rootNode.layerStack, ref2.rootNode.layerStack)
        self.assertNotEqual(ref1.rootNode.children[0].layerStack, 
                            ref2.rootNode.children[0].layerStack)
        self.assertNotEqual(ref1.rootNode.children[0].children[0].layerStack, 
                            ref2.rootNode.children[0].children[0].layerStack)

        # Remove the authored opinion and verify everything goes back to
        # how it was.
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            ref1Layer.ClearExpressionVariables()
            self.assertEqual(changes.GetSignificantChanges(), ['/A'])

        ref1 = self.AssertVariables(
            pcpCache, '/A',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref1.sdf', {'A':'B'}), [
                        ('common_ref/base_ref.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        ref2 = self.AssertVariables(
            pcpCache, '/B',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref2.sdf', {'A':'B'}), [
                        ('common_ref/base_ref.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        self.assertEqual(ref1.rootNode.layerStack, ref2.rootNode.layerStack)
        self.assertNotEqual(ref1.rootNode.children[0].layerStack, 
                            ref2.rootNode.children[0].layerStack)
        self.assertEqual(ref1.rootNode.children[0].children[0].layerStack, 
                         ref2.rootNode.children[0].children[0].layerStack)

        # Batch changes to expression variables in both ref1.sdf and ref2.sdf
        # and verify that the base_ref.sdf layer stack in /A and /B have
        # different composed expression variables.
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            with Sdf.ChangeBlock():
                ref1Layer.expressionVariables = {'X':'Y'}
                ref2Layer.expressionVariables = {'X':'Z'}

            self.assertEqual(changes.GetSignificantChanges(), ['/A', '/B'])

        ref1 = self.AssertVariables(
            pcpCache, '/A',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref1.sdf', {'A':'B', 'X':'Y'}), [
                        ('common_ref/base_ref.sdf', {'A':'B', 'X':'Y'}), [
                        ]
                    ]
                ]
            ])

        ref2 = self.AssertVariables(
            pcpCache, '/B',
            expected = [
                ('common_ref/root.sdf', {'A':'B'}), [
                    ('common_ref/ref2.sdf', {'A':'B', 'X':'Z'}), [
                        ('common_ref/base_ref.sdf', {'A':'B', 'X':'Z'}), [
                        ]
                    ]
                ]
            ])

        # Again, we expect the base_ref.sdf layer stack to differ between the
        # two prim indexes because of the different composed expression
        # variables.
        self.assertEqual(ref1.rootNode.layerStack, ref2.rootNode.layerStack)
        self.assertNotEqual(ref1.rootNode.children[0].layerStack, 
                            ref2.rootNode.children[0].layerStack)
        self.assertNotEqual(ref1.rootNode.children[0].children[0].layerStack, 
                            ref2.rootNode.children[0].children[0].layerStack)

    def test_ExpressionVarChanges_ChainedReferences(self):
        """Test that changes to expression variables propagates to downstream
        layer stacks in cases where multiple references have been chained
        together"""
        pcpCache = LoadPcpCache('chained_ref/root.sdf')
        ref1Layer = Sdf.Layer.FindOrOpen('chained_ref/ref1.sdf')
        ref2Layer = Sdf.Layer.FindOrOpen('chained_ref/ref2.sdf')

        self.AssertVariables(
            pcpCache, '/Root1',
            expected = [
                ('chained_ref/root.sdf', {'A':'B'}), [
                    ('chained_ref/ref1.sdf', {'A':'B'}), [
                        ('chained_ref/ref2.sdf', {'A':'B'}), [
                            ('chained_ref/ref3.sdf', {'A':'B'}), [
                            ]
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root2',
            expected = [
                ('chained_ref/root.sdf', {'A':'B'}), [
                    ('chained_ref/ref2.sdf', {'A':'B'}), [
                        ('chained_ref/ref3.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root3',
            expected = [
                ('chained_ref/root.sdf', {'A':'B'}), [
                    ('chained_ref/ref3.sdf', {'A':'B'}), [
                    ]
                ]
            ])
        
        # Batch together changes to the expression variables in ref1.sdf and
        # ref2.sdf and verify they propagate to downstream layer stacks
        # in all prim indexes.
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            with Sdf.ChangeBlock():
                ref1Layer.expressionVariables = {'I':'J'}
                ref2Layer.expressionVariables = {'X':'Y'}

            self.assertEqual(changes.GetSignificantChanges(),
                             ['/Root1', '/Root2'])

        self.AssertVariables(
            pcpCache, '/Root1',
            expected = [
                ('chained_ref/root.sdf', {'A':'B'}), [
                    ('chained_ref/ref1.sdf', {'A':'B', 'I':'J'}), [
                        ('chained_ref/ref2.sdf', {'A':'B', 'I':'J', 'X':'Y'}), [
                            ('chained_ref/ref3.sdf', {'A':'B', 'I':'J', 'X':'Y'}), [
                            ]
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root2',
            expected = [
                ('chained_ref/root.sdf', {'A':'B'}), [
                    ('chained_ref/ref2.sdf', {'A':'B', 'X':'Y'}), [
                        ('chained_ref/ref3.sdf', {'A':'B', 'X':'Y'}), [
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root3',
            expected = [
                ('chained_ref/root.sdf', {'A':'B'}), [
                    ('chained_ref/ref3.sdf', {'A':'B'}), [
                    ]
                ]
            ])

    def test_ExpressionVarChanges_SignificantLayerStackChanges(self):
        """Test scenarios involving significant changes to layer stacks
        combined with expression variable changes"""
        pcpCache = LoadPcpCache('sig_changes/root.sdf')
        rootLayer = Sdf.Layer.FindOrOpen('sig_changes/root.sdf')
        ref1Layer = Sdf.Layer.FindOrOpen('sig_changes/ref1.sdf')

        self.AssertVariables(
            pcpCache, '/Root1',
            expected = [
                ('sig_changes/root.sdf', {'A':'B'}), [
                    ('sig_changes/ref1.sdf', {'A':'B'}), [
                        ('sig_changes/ref2.sdf', {'A':'B'}), [
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root2',
            expected = [
                ('sig_changes/root.sdf', {'A':'B'}), [
                    ('sig_changes/ref2.sdf', {'A':'B'}), [
                    ]
                ]
            ])

        # Batch together a change to the expression variables in the root layer
        # stack and a significant layer stack change. The variable change
        # should still propagate to all downstream layer stacks.
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            with Sdf.ChangeBlock():
                rootLayer.expressionVariables = {'A':'C'}
                rootLayer.subLayerPaths.append('sig_changes/sub.sdf')

            self.assertEqual(changes.GetSignificantChanges(), ['/'])

        self.AssertVariables(
            pcpCache, '/Root1',
            expected = [
                ('sig_changes/root.sdf', {'A':'C'}), [
                    ('sig_changes/ref1.sdf', {'A':'C'}), [
                        ('sig_changes/ref2.sdf', {'A':'C'}), [
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root2',
            expected = [
                ('sig_changes/root.sdf', {'A':'C'}), [
                    ('sig_changes/ref2.sdf', {'A':'C'}), [
                    ]
                ]
            ])

        # Batch together a change to the expression variables in root.sdf and
        # ref1.sdf, along with a significant layer stack change. We expect
        # only /Root1 to be resynced since its the only index that references
        # ref1.sdf, and variable changes should be propagated appropriately.
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            with Sdf.ChangeBlock():
                rootLayer.expressionVariables = {'A':'D'}
                ref1Layer.expressionVariables = {'X':'Y'}
                ref1Layer.subLayerPaths.append('sig_changes/sub1.sdf')

            self.assertEqual(changes.GetSignificantChanges(), ['/Root1'])
        
        self.AssertVariables(
            pcpCache, '/Root1',
            expected = [
                ('sig_changes/root.sdf', {'A':'D'}), [
                    ('sig_changes/ref1.sdf', {'A':'D', 'X':'Y'}), [
                        ('sig_changes/ref2.sdf', {'A':'D', 'X':'Y'}), [
                        ]
                    ]
                ]
            ])

        self.AssertVariables(
            pcpCache, '/Root2',
            expected = [
                ('sig_changes/root.sdf', {'A':'D'}), [
                    ('sig_changes/ref2.sdf', {'A':'D'}), [
                    ]
                ]
            ])

    def test_NoChanges(self):
        """Test scenarios where no recomputations are expected."""
        pcpCache = LoadPcpCache('no_changes/root.sdf')
        rootLayer = pcpCache.GetLayerStackIdentifier().rootLayer

        # Authoring expression variables in the root layer stack should
        # not incur any significant changes if nothing depends on those
        # expression variables.
        pi, err = pcpCache.ComputePrimIndex('/A')
        with Pcp._TestChangeProcessor(pcpCache) as changes:
            rootLayer.pseudoRoot.SetInfo('expressionVariables', {'X':'A'})

            self.assertEqual(changes.GetSignificantChanges(), [])
            self.assertEqual(changes.GetSpecChanges(), [])
            self.assertEqual(changes.GetPrimChanges(), [])

    def test_ExpressionVariableCompositionInLayerSTack(self):
        """Test expression variable composition in layer stacks"""
        sessionSublayer = Sdf.Layer.CreateAnonymous('session-sublayer')
        sessionSublayer.ImportFromString('''
        #sdf 1.4.32
        (
            expressionVariables = {
                string SESSION_SUBLAYER_ONLY = "session-sublayer"
            }
        )
        '''.strip())
        
        sessionLayer = Sdf.Layer.CreateAnonymous('session')
        sessionLayer.ImportFromString('''
        #sdf 1.4.32
        (
            expressionVariables = {{
                string SESSION_ONLY = "session"
                string SESSION_OVERRIDE = "session"
            }}
            subLayers = [
                @{sub}@
            ]
        )
        '''.format(sub=sessionSublayer.identifier).strip())

        subLayer = Sdf.Layer.CreateAnonymous('sublayer')
        subLayer.ImportFromString('''
        #sdf 1.4.32
        (
            expressionVariables = {
                string SUBLAYER_ONLY = "sublayer"
            }
        )
        '''.strip())
        
        rootLayer = Sdf.Layer.CreateAnonymous('root')
        rootLayer.ImportFromString('''
        #sdf 1.4.32
        (
            expressionVariables = {{
                string ROOT_ONLY = "root"
                string SESSION_OVERRIDE = "root"
            }}
            subLayers = [
                @{sub}@
            ]
        )
        '''.format(sub=subLayer.identifier).strip())

        rootId = Pcp.LayerStackIdentifier(rootLayer, sessionLayer)
        pcpCache = Pcp.Cache(rootId)

        rootLayerStack, _ = pcpCache.ComputeLayerStack(rootId)
        self.assertEqual(rootLayerStack.expressionVariables.GetVariables(), 
                         {'SESSION_ONLY':'session', 
                          'SESSION_OVERRIDE':'session',
                          'ROOT_ONLY':'root'})

if __name__ == "__main__":
    unittest.main()
