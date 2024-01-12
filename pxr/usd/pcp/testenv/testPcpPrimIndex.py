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

if __name__ == "__main__":
    unittest.main()
