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

from pxr import Pcp, Sdf, Tf

def LoadPcpCache(rootLayer):
    return Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

class TestPcpPrimIndex(unittest.TestCase):
    def AssertRestrictedDepth(self, pcpCache, path, expected):
        pi, err = pcpCache.ComputePrimIndex(path)
        self.assertFalse(err, "Unexpected composition errors: {}".format(
            ",".join(str(e) for e in err)))

        for node, e in Pcp._TestPrimIndex(pi, expected):
            expectedArcType, expectedPath, expectedDepth = e

            self.assertEqual(
                node.arcType, expectedArcType,
                "Error at {} {}".format(node.arcType, node.path))
            self.assertEqual(
                node.path, expectedPath,
                "Error at {} {}".format(node.arcType, node.path))
            self.assertEqual(
                node.GetSpecContributionRestrictedDepth(), expectedDepth,
                "Error at {} {}".format(node.arcType, node.path))

    def AssertPrimIndex(self, pcpCache, path, expected):
        pi, err = pcpCache.ComputePrimIndex(path)

        # Uncomment to generate dot graphs for prim index
        # being tested.
        # pi.DumpToDotGraph('{}.dot'.format(Sdf.Path(path).name))

        self.assertFalse(err, "Unexpected composition errors: {}".format(
            ",".join(str(e) for e in err)))

        for node, e in Pcp._TestPrimIndex(pi, expected):
            expectedArcType, expectedLayerStack, expectedPath = e
            
            self.assertEqual(
                node.arcType, expectedArcType,
                "Error at {} {} {}".format(node.arcType, node.layerStack, node.path))
            self.assertEqual(
                node.layerStack.identifier.rootLayer, expectedLayerStack,
                "Error at {} {} {}".format(node.arcType, node.layerStack, node.path))
            self.assertEqual(
                node.isCulled, False,
                "Error at {} {} {}".format(node.arcType, node.layerStack, node.path))
            self.assertEqual(
                node.path, expectedPath,
                "Error at {} {}{}".format(node.arcType, node.layerStack, node.path))

    def test_TestPrimIndex(self):
        """Test _TestPrimIndex generator"""
        
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Class"
        {
        }

        def "Ref" (
            inherits = </Class>
            variantSets = "x"
            variants = {
                string "x" = "a"
            }
        )
        {
            variantSet "x" = {
                "a" {
                }
            }
        }

        def "Root"
        {
            def "Ref"  (
                references = </Ref>
            )
            {
            }
        }
        '''.strip())

        pcp = LoadPcpCache(layer)

        pi, err = pcp.ComputePrimIndex("/Root/Ref")
        self.assertFalse(err)

        # Collect all nodes from the prim index in strong-to-weak order.
        nodes = []
        def _CollectNodes(node):
            nodes.append(node)
            for child in node.children:
                _CollectNodes(child)
        _CollectNodes(pi.rootNode)

        # Use the test generator to iterate over the prim index and
        # verify that it matches the structure given by the "expected"
        # variable.
        testedNodes = []
        for node, e in Pcp._TestPrimIndex(
            pi, 
            expected = [
                (Pcp.ArcTypeRoot, "/Root/Ref"), [
                    (Pcp.ArcTypeInherit, "/Class"), [
                    ],
                    (Pcp.ArcTypeReference, "/Ref"), [
                        (Pcp.ArcTypeInherit, "/Class"), [
                        ],
                        (Pcp.ArcTypeVariant, "/Ref{x=a}"), [
                        ]
                    ],
                ]
            ]):
            self.assertEqual((node.arcType, node.path), e)
            testedNodes.append(node)

        # Also verify that the generator actually visited every node
        # in the prim index.
        self.assertEqual(nodes, testedNodes)

    def test_ContributionRestrictedDepth_Unrestricted(self):
        """Verify contribution restriction depth when no
        restrictions are in place."""

        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Ref"
        {
        }

        def "Root"
        {
            def "Ref"  (
                references = </Ref>
            )
            {
            }
        }

        '''.strip())

        pcp = LoadPcpCache(layer)

        # There are no restrictions on any prims so we expect the restriction
        # depth on all nodes in all prim indexes to be 0.
        self.AssertRestrictedDepth(
            pcp, "/Root",
            [
                (Pcp.ArcTypeRoot, "/Root", 0), [
                ]
            ])

        self.AssertRestrictedDepth(
            pcp, "/Root/Ref",
            [
                (Pcp.ArcTypeRoot, "/Root/Ref", 0), [
                    (Pcp.ArcTypeReference, "/Ref", 0), [
                    ]
                ]
            ])

    def test_ContributionRestrictedDepth_Specializes(self):
        """Verify contribution restriction depth with specializes and
        namespace-nested composition arcs. This is tricky because of
        the way we copy nodes around to achieve the desired strength
        ordering."""

        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Ref" (
            specializes = </Specialize>
        )
        {
        }

        def "ChildRef"
        {
            def "Child2" (
                references = </ChildRef2>
            )
            {
            }
        }

        def "ChildRef2"
        {
        }

        def "Specialize"
        {
            def "Child" (
                references = </ChildRef>
            )
            {
            }
        }

        def "Root" (
            references = </Ref>
        )
        {
        }

        '''.strip())

        pcpCache = LoadPcpCache(layer)

        self.AssertRestrictedDepth(
            pcpCache, "/Root",
            [
                (Pcp.ArcTypeRoot, "/Root", 0), [
                    (Pcp.ArcTypeReference, "/Ref", 0), [
                        (Pcp.ArcTypeSpecialize, "/Specialize", 1), [
                        ]
                    ],
                    (Pcp.ArcTypeSpecialize, "/Specialize", 0), [
                    ]
                ]
            ])

        self.AssertRestrictedDepth(
            pcpCache, "/Root/Child",
            [
                (Pcp.ArcTypeRoot, "/Root/Child", 0), [
                    (Pcp.ArcTypeReference, "/Ref/Child", 0), [
                        (Pcp.ArcTypeSpecialize, "/Specialize/Child", 1), [
                            (Pcp.ArcTypeReference, "/ChildRef", 1), [
                            ]
                        ]
                    ],
                    (Pcp.ArcTypeSpecialize, "/Specialize/Child", 0), [
                        (Pcp.ArcTypeReference, "/ChildRef", 0), [
                        ]
                    ]
                ]
            ])

        self.AssertRestrictedDepth(
            pcpCache, "/Root/Child/Child2",
            [
                (Pcp.ArcTypeRoot, "/Root/Child/Child2", 0), [
                    (Pcp.ArcTypeReference, "/Ref/Child/Child2", 0), [
                        (Pcp.ArcTypeSpecialize, "/Specialize/Child/Child2", 1), [
                            (Pcp.ArcTypeReference, "/ChildRef/Child2", 1), [
                                (Pcp.ArcTypeReference, "/ChildRef2", 1), [
                                ]
                            ]
                        ]
                    ],
                    (Pcp.ArcTypeSpecialize, "/Specialize/Child/Child2", 0), [
                        (Pcp.ArcTypeReference, "/ChildRef/Child2", 0), [
                            (Pcp.ArcTypeReference, "/ChildRef2", 0), [
                            ]
                        ]
                    ]
                ]
            ])

    @unittest.skip("currently fails due to bug with specializes")
    def test_ContributionRestrictedDepth_SpecializesAndPermissions(self):
        """Verify contribution restriction depth with specializes and
        permission restrictions."""
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Ref" (
            specializes = </Specialize>
        )
        {
        }

        def "ChildRef"
        {
        }

        def "Specialize"
        {
            def "Child" (
                permission = private
                references = </ChildRef>
            )
            {
            }
        }

        def "Root" (
            references = </Ref>
        )
        {
        }

        '''.strip())

        pcpCache = LoadPcpCache(layer)

        # Since /Specialize/Child is marked as private, all stronger nodes
        # are restricted from contributing opinions. So, /Root/Child and
        # /Ref/Child should have a restriction depth of 2.
        self.AssertRestrictedDepth(
            pcpCache, "/Root/Child",
            [
                (Pcp.ArcTypeRoot, "/Root/Child", 2), [
                    (Pcp.ArcTypeReference, "/Ref/Child", 2), [
                        (Pcp.ArcTypeSpecialize, "/Specialize/Child", 0), [
                            (Pcp.ArcTypeReference, "/ChildRef", 0), [
                            ]
                        ]
                    ],
                    (Pcp.ArcTypeSpecialize, "/Specialize/Child", 0), [
                        (Pcp.ArcTypeReference, "/ChildRef", 0), [
                        ]
                    ]
                ]
            ])

    def test_ContributionRestrictedDepth_SpecializesAndPermissions2(self):
        """Verify contribution restriction depth with specializes and
        permission restrictions."""
        
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Ref" (
            specializes = </Specialize>
        )
        {
        }

        def "ChildRef"
        {
            def "Child2" (
                permission = private
                references = </ChildRef2>
            )
            {
            }
        }

        def "ChildRef2"
        {
        }

        def "Specialize"
        {
            def "Child" (
                references = </ChildRef>
            )
            {
            }
        }

        def "Root" (
            references = </Ref>
        )
        {
        }

        '''.strip())

        pcpCache = LoadPcpCache(layer)

        # Since /ChildRef/Child2 is marked as private, all stronger nodes
        # are restricted from contributing opinions. So, /Root/Child/Child2,
        # /Ref/Child/Child2 and /Specialize/Child/Child2 should all have a
        # restriction depth of 3.
        self.AssertRestrictedDepth(
            pcpCache, "/Root/Child/Child2",
            [
                (Pcp.ArcTypeRoot, "/Root/Child/Child2", 3), [
                    (Pcp.ArcTypeReference, "/Ref/Child/Child2", 3), [
                        (Pcp.ArcTypeSpecialize, "/Specialize/Child/Child2", 1), [
                            (Pcp.ArcTypeReference, "/ChildRef/Child2", 1), [
                                (Pcp.ArcTypeReference, "/ChildRef2", 1), [
                                ]
                            ]
                        ]
                    ],
                    (Pcp.ArcTypeSpecialize, "/Specialize/Child/Child2", 3), [
                        (Pcp.ArcTypeReference, "/ChildRef/Child2", 0), [
                            (Pcp.ArcTypeReference, "/ChildRef2", 0), [
                            ]
                        ]
                    ]
                ]
            ])

    def test_ContributionRestrictedDepth_Relocates(self):
        """Verify contribution restriction depth with relocates."""

        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Ref"
        {
            def "Child"
            {
                def "A"
                {
                }
            }
        }

        def "Root" (
            references = </Ref>
            relocates = {
                <Child> : <Child_Relocated>
            }
        )
        {
        }
        '''.strip())

        pcpCache = LoadPcpCache(layer)

        # Composition disallows opinions at the source of a relocation,
        # so relocate nodes are always marked inert when introduced to
        # restrict contributions. So, we expect the restriction depth
        # for relocate nodes to be equal to the namespace depth when
        # that node is introduced.
        self.AssertRestrictedDepth(
            pcpCache, "/Root/Child_Relocated",
            [
                (Pcp.ArcTypeRoot, "/Root/Child_Relocated", 0), [
                    (Pcp.ArcTypeRelocate, "/Root/Child", 2), [
                        (Pcp.ArcTypeReference, "/Ref/Child", 0), [
                        ]
                    ]
                ]
            ])

        self.AssertRestrictedDepth(
            pcpCache, "/Root/Child_Relocated/A",
            [
                (Pcp.ArcTypeRoot, "/Root/Child_Relocated/A", 0), [
                    (Pcp.ArcTypeRelocate, "/Root/Child/A", 2), [
                        (Pcp.ArcTypeReference, "/Ref/Child/A", 0), [
                        ]
                    ]
                ]
            ])

    def test_ContributionRestrictedDepth_Permissions(self):
        """Verify contribution restriction depth with private permissions."""

        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''
        #sdf 1.4.32

        def "Ref3"
        {
            def "Child" (
                permission = private
            )
            {
            }
        }

        def "Ref2"
        {
            def "Restricted" (
                permission = private
                references = </Ref3>
            )
            {
            }
        }

        def "Ref"
        {
            def "Ref2" (
                references = </Ref2>
            )
            {
            }
        }

        def "Root"
        {
            def "Ref"  (
                references = </Ref>
            )
            {
            }
        }

        '''.strip())

        pcp = LoadPcpCache(layer)

        # /Root/Ref/Ref2/Restricted is marked private a few levels deep in
        # the chain of references, on /Ref2/Restricted. The node for this
        # reference and all weaker nodes should be marked as unrestricted.
        #
        # All stronger nodes should have a restriction depth equal to the
        # number of path components, since this is the exact level at
        # namespace where the restriction was introduced.
        self.AssertRestrictedDepth(
            pcp, "/Root/Ref/Ref2/Restricted",
            [
                (Pcp.ArcTypeRoot, "/Root/Ref/Ref2/Restricted", 4), [
                    (Pcp.ArcTypeReference, "/Ref/Ref2/Restricted", 3), [
                        (Pcp.ArcTypeReference, "/Ref2/Restricted", 0), [
                            (Pcp.ArcTypeReference, "/Ref3", 0), [
                            ]
                        ]
                    ]
                ]
            ])

        # /Root/Ref/Ref2/Restricted/Child is marked private one level
        # deeper in the referencing chain, in /Ref3/Child. We expect
        # the restriction depth on previously-restricted nodes to remain
        # the same, since that's the depth where they were first restricted.
        #
        # /Ref2/Restricted/Child, which is a newly-restricted node, should
        # now have a restriction depth equal to the number of components in
        # its path.
        self.AssertRestrictedDepth(
            pcp, "/Root/Ref/Ref2/Restricted/Child",
            [
                (Pcp.ArcTypeRoot, "/Root/Ref/Ref2/Restricted/Child", 4), [
                    (Pcp.ArcTypeReference, "/Ref/Ref2/Restricted/Child", 3), [
                        (Pcp.ArcTypeReference, "/Ref2/Restricted/Child", 3), [
                            (Pcp.ArcTypeReference, "/Ref3/Child", 0), [
                            ]
                        ]
                    ]
                ]
            ])

    @unittest.skipIf(not Tf.GetEnvSetting("PCP_CULLING"), "Culling is disabled")
    def test_PrimIndexCulling_Specializes(self):
        """Tests node culling optimization with specializes arcs"""
        refLayer = Sdf.Layer.CreateAnonymous("ref")
        refLayer.ImportFromString('''
        #sdf 1.4.32

        def "SpecRefA"
        {
            def "ChildA"
            {
            }
        }

        def "SpecRefB"
        {
        }

        def "Spec" (
            references = [</SpecRefA>, </SpecRefB>]
        )
        {
        }

        def "Ref" (
            specializes = </Spec>
        )
        {
        }
        '''.strip())

        rootLayer = Sdf.Layer.CreateAnonymous("root")
        rootLayer.ImportFromString(f'''
        #sdf 1.4.32

        def "Root" (
            references = @{refLayer.identifier}@</Ref>
        )
        {{
            def "ChildB"
            {{
            }}
        }}
        '''.strip())
        
        pcp = LoadPcpCache(rootLayer)

        # Compute /Root. No culling occurs here since there are opinions
        # at all sites.
        self.AssertPrimIndex(
            pcp, "/Root",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root"), [
                    # Reference from /Root -> /Ref in @root@
                    (Pcp.ArcTypeReference, refLayer, "/Ref"), [
                        (Pcp.ArcTypeSpecialize, refLayer, "/Spec"), [
                            (Pcp.ArcTypeReference, refLayer, "/SpecRefA"), [],
                            (Pcp.ArcTypeReference, refLayer, "/SpecRefB"), []
                        ]
                    ],

                    # Implied specialize due to /Ref -> /Spec in @ref@
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Spec"), [],

                    # Propagated specialize due to /Ref -> /Spec in @ref@
                    (Pcp.ArcTypeSpecialize, refLayer, "/Spec"), [
                        (Pcp.ArcTypeReference, refLayer, "/SpecRefA"), [],
                        (Pcp.ArcTypeReference, refLayer, "/SpecRefB"), []
                    ]
                ]
            ])

        # Compute /Root/ChildA. The reference arcs pointing to /SpecRefB
        # beneath the specializes arcs are culled since there are no opinions
        # at /SpecRefB/ChildA.
        self.AssertPrimIndex(
            pcp, "/Root/ChildA",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/ChildA"), [
                    # Reference from /Root -> /Ref in @root@
                    (Pcp.ArcTypeReference, refLayer, "/Ref/ChildA"), [
                        (Pcp.ArcTypeSpecialize, refLayer, "/Spec/ChildA"), [
                            (Pcp.ArcTypeReference, refLayer, "/SpecRefA/ChildA"), []
                        ]
                    ],

                    # Propagated specialize due to /Ref -> /Spec in @ref@
                    (Pcp.ArcTypeSpecialize, refLayer, "/Spec/ChildA"), [
                        (Pcp.ArcTypeReference, refLayer, "/SpecRefA/ChildA"), []
                    ]
                ]
            ])

        # Compute /Root/ChildB. All nodes should be culled since there are
        # no opinions for ChildB anywhere except at /Root/ChildB.
        self.AssertPrimIndex(
            pcp, "/Root/ChildB",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/ChildB"), []
            ])

    @unittest.skipIf(not Tf.GetEnvSetting("PCP_CULLING"), "Culling is disabled")
    def test_PrimIndexCulling_SubrootSpecializes(self):
        """Tests node culling optimization with specializes arcs pointing
        to subroot prims"""
        refLayer = Sdf.Layer.CreateAnonymous("ref")
        refLayer.ImportFromString('''
        #sdf 1.4.32

        def "SpecRefA"
        {
            def "ChildA"
            {
            }
        }

        def "SpecRefB"
        {
        }

        def "Ref"
        {
            def "Instance" (
                specializes = </Ref/Spec>
            )
            {
            }

            def "Spec" (
                references = [</SpecRefA>, </SpecRefB>]
            )
            {
            }
        }
        '''.strip())

        rootLayer = Sdf.Layer.CreateAnonymous("root")
        rootLayer.ImportFromString(f'''
        #sdf 1.4.32

        def "Root" (
            references = @{refLayer.identifier}@</Ref>
        )
        {{
            def "Spec"
            {{
            }}

            def "Instance"
            {{
                def "ChildB"
                {{
                }}
            }}
        }}
        '''.strip())
        
        pcp = LoadPcpCache(rootLayer)

        # Compute /Root/Instance. No culling occurs here since there are
        # opinions at all sites.
        self.AssertPrimIndex(
            pcp, "/Root/Instance",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/Instance"), [
                    # Reference from /Root -> /Ref in @root@
                    (Pcp.ArcTypeReference, refLayer, "/Ref/Instance"), [
                        (Pcp.ArcTypeSpecialize, refLayer, "/Ref/Spec"), [
                            (Pcp.ArcTypeReference, refLayer, "/SpecRefA"), [],
                            (Pcp.ArcTypeReference, refLayer, "/SpecRefB"), []
                        ]
                    ],

                    # Implied specialize due to /Ref/Instance -> /Ref/Spec in @ref@
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Root/Spec"), [],

                    # Propagated specialize due to /Ref/Instance -> /Ref/Spec in @ref@
                    (Pcp.ArcTypeSpecialize, refLayer, "/Ref/Spec"), [
                        (Pcp.ArcTypeReference, refLayer, "/SpecRefA"), [],
                        (Pcp.ArcTypeReference, refLayer, "/SpecRefB"), []
                    ]
                ]
            ])

        # Compute /Root/Instance/ChildA. The reference arcs pointing to
        # /SpecRefB beneath the specializes arcs are culled since there are no
        # opinions at /SpecRefB/ChildA.
        self.AssertPrimIndex(
            pcp, "/Root/Instance/ChildA",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/Instance/ChildA"), [
                    # Reference from /Root -> /Ref in @root@
                    (Pcp.ArcTypeReference, refLayer, "/Ref/Instance/ChildA"), [
                        (Pcp.ArcTypeSpecialize, refLayer, "/Ref/Spec/ChildA"), [
                            (Pcp.ArcTypeReference, refLayer, "/SpecRefA/ChildA"), []
                        ]
                    ],

                    # Propagated specialize due to /Ref/Instance -> /Ref/Spec in @ref@
                    (Pcp.ArcTypeSpecialize, refLayer, "/Ref/Spec/ChildA"), [
                        (Pcp.ArcTypeReference, refLayer, "/SpecRefA/ChildA"), []
                    ]
                ]
            ])

        # Compute /Root/Instance/ChildB. All nodes should be culled since there
        # are no opinions for ChildB anywhere except at /Root/Instance/ChildB.
        self.AssertPrimIndex(
            pcp, "/Root/Instance/ChildB",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/Instance/ChildB"), []
            ])

    @unittest.skipIf(not Tf.GetEnvSetting("PCP_CULLING"), "Culling is disabled")
    def test_PrimIndexCulling_SpecializesHierarchy(self):
        """Tests node culling optimization with hierarchy of specializes arcs"""

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString('''
        #sdf 1.4.32

        def "Ref"
        {
            def "Instance" (
                specializes = </Ref/SpecA>
            )
            {
            }

            def "SpecA" (
                specializes = </Ref/SpecB>
            )
            {
                def "Child"
                {
                }
            }

            def "SpecB"
            {
                def "Child"
                {
                }
            }
        }

        def "RefA" (
            references = </Ref>
        )
        {
        }

        def "Root" (
            references = </RefA>
        )
        {
        }        
        '''.strip())
        
        pcp = LoadPcpCache(rootLayer)

        # Compute /Root/Instance to show initial prim index structure.
        # No culling occurs here.
        self.AssertPrimIndex(
            pcp, "/Root/Instance",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/Instance"), [
                    # Reference from /Root -> /RefA
                    (Pcp.ArcTypeReference, rootLayer, "/RefA/Instance"), [
                        # Reference from /RefA -> /Ref
                        (Pcp.ArcTypeReference, rootLayer, "/Ref/Instance"), [
                            # Specializes from /Ref/Instance -> /Ref/SpecA
                            (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecA"), [
                                # Specializes from /Ref/SpecA -> /Ref/SpecB
                                (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecB"), []
                            ]
                        ],

                        # Implied specializes due to /Ref/Instance -> /Ref/SpecA
                        (Pcp.ArcTypeSpecialize, rootLayer, "/RefA/SpecA"), [
                            (Pcp.ArcTypeSpecialize, rootLayer, "/RefA/SpecB"), [
                            ]
                        ]
                    ],

                    # Implied specializes due to /Ref/Instance -> /Ref/SpecA
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Root/SpecA"), [
                        (Pcp.ArcTypeSpecialize, rootLayer, "/Root/SpecB"), []
                    ],

                    # Propagated specializes due to implied /RefA/SpecA
                    (Pcp.ArcTypeSpecialize, rootLayer, "/RefA/SpecA"), [],

                    # Propagated specializes due to /Ref/Instance -> /Ref/SpecA
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecA"), [],

                    # Propagated specializes due to implied /Root/SpecB
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Root/SpecB"), [],

                    # Propagated specializes due to implied /RefA/SpecB
                    (Pcp.ArcTypeSpecialize, rootLayer, "/RefA/SpecB"), [],

                    # Propagated specializes due to /Ref/SpecA -> /Ref/SpecB.
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecB"), []
                ]
            ])

        # Compute /Root/Instance/Child. Most of the nodes are culled and
        # removed because they provide no opinions on the Child prim, but
        # the origin specializes hierarchy noted below cannot be culled.
        self.AssertPrimIndex(
            pcp, "/Root/Instance/Child",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/Instance/Child"), [
                    # Reference from /Root -> /RefA
                    (Pcp.ArcTypeReference, rootLayer, "/RefA/Instance/Child"), [
                        # Reference from /RefA -> /Ref
                        (Pcp.ArcTypeReference, rootLayer, "/Ref/Instance/Child"), [
                            # The propagated specializes subtree for
                            # /Ref/SpecA/Child is culled, but the propagated subtree
                            # for /Ref/SpecB/Child is not. That prevents the origin
                            # subtree for /Ref/SpecA/Child from being culled.
                            (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecA/Child"), [
                                (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecB/Child"), []
                            ]
                        ],
                    ],

                    # Propagated specializes due to /Ref/SpecA -> /Ref/SpecB.
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecA/Child"), [],

                    # Propagated specializes due to /Ref/SpecA -> /Ref/SpecB.
                    (Pcp.ArcTypeSpecialize, rootLayer, "/Ref/SpecB/Child"), []
                ]
            ])

    @unittest.skipIf(not Tf.GetEnvSetting("PCP_CULLING"), "Culling is disabled")
    def test_PrimIndexCulling_SpecializesAncestralCulling(self):
        """Tests node culling optimization where the prim index for
        the parent prim contains a node that is marked as culled
        but has not been removed from the prim index."""
        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString('''
        #sdf 1.4.32

        def "RefB"
        {
            def "Instance" (
                specializes = </RefB/Spec>
            )
            {
                def "Child"
                {
                }
            }

            def "Spec"
            {
                def "Child"
                {
                }
            }
        }

        def "Inh"
        {
        }

        def "Ref" (
            references = </RefB>
            inherits = </Inh>
        )
        {
        }

        def "Root" (
            references = </Ref>
        )
        {
        }
        '''.strip())

        pcp = LoadPcpCache(rootLayer)

        # Compute /Root/Instance/Child. The important part here is that the
        # subtrees for the implied specializes nodes at /Ref/Spec/Child and
        # /Root/Spec/Child have been culled and removed from the prim index.
        self.AssertPrimIndex(
            pcp, "/Root/Instance/Child",
            [
                (Pcp.ArcTypeRoot, rootLayer, "/Root/Instance/Child"), [
                    (Pcp.ArcTypeReference, rootLayer, "/Ref/Instance/Child"), [
                        (Pcp.ArcTypeReference, rootLayer, "/RefB/Instance/Child"), [
                            (Pcp.ArcTypeSpecialize, rootLayer, "/RefB/Spec/Child"), [],
                        ]
                    ],

                    (Pcp.ArcTypeSpecialize, rootLayer, "/RefB/Spec/Child")
                ]
            ])

    def test_RecursivePrimIndexComputationLocalErrors(self):
        """Test to make sure recursive primIndex computation correctly stores 
        composition errors"""
        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString('''
        #sdf 1.4.32

        def "Main" (
        )
        {
            def "First" (
                prepend references = </Main/Second>
            )
            {
            }

            def "Second" (
                prepend references = </Main/First>
            )
            {
            }
        }
        '''.strip())
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))
        pi, errs = pcp.ComputePrimIndex('/Main/First')
        self.assertEqual(len(errs), 1)
        self.assertEqual(len(errs), len(pi.localErrors))
        self.assertEqual(str(errs[0]), str(pi.localErrors[0]))

    def test_TestInvalidPcpNodeRef(self):
        """Test to ensure that a invalid PcpNodeRef will return false
            when cast to a bool"""

        nullPcpNodeRef =  Pcp._GetInvalidPcpNode()
        self.assertFalse(bool(nullPcpNodeRef))


if __name__ == "__main__":
    unittest.main()
