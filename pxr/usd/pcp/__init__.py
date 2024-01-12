#
# Copyright 2016 Pixar
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
#
from pxr import Tf
Tf.PreparePythonModule()
del Tf

# Utilities for unit testing

def _TestPrimIndex(primIndex, expected):
    '''Generator for verifying the expected structure and
    values throughout the given prim index.

    The "expected" parameter is a list that mirrors the tree
    structure of the prim index:

    expected  : [nodeEntry]
    nodeEntry : (tuple of arbitrary data), [nodeEntry, ...]

    The first item in a nodeEntry is a tuple of arbitrary data
    associated with a node in the prim index. The second item is
    a list of nodeEntries corresponding to the children of that
    node.

    This generator will visit each node in the prim index and
    yield the node and the associated data tuple.

    For example:

    expected = [
        (Pcp.ArcTypeRoot, "/Root"), [
            (Pcp.ArcTypeReference, "/Ref1"), [],
            (Pcp.ArcTypeReference, "/Ref2"), []
        ]
    ]
          
    for node, entry in Pcp._TestPrimIndex(primIndex, expected):
        assert node.arcType == entry.arcType
        assert node.path == entry.path

    '''
    def _recurse(node, expected):
        try:
            yield node, expected[0]
        except IndexError as e:
            raise RuntimeError(
                "No entry in expected corresponding to node {}"
                .format(node.site)) from e

        for idx, n in enumerate(node.children):
            try:
                expectedSubtree = expected[1][(idx*2):(idx*2)+2]
            except IndexError as e:
                raise RuntimeError(
                    "No entry in expected corresponding to node {}"
                    .format(n.site)) from e
            yield from _recurse(n, expectedSubtree)

    yield from _recurse(primIndex.rootNode, expected)
