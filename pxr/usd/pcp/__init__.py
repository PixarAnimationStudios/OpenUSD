#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
