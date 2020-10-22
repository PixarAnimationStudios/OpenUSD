#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
from __future__ import print_function

from pxr import Sdf, Pcp, Tf
import unittest, os

# Composition capacity limits.  See PcpPrimIndex_Graph::_Node.
# Rather than expose these limits publically in Pcp we keep them as
# private implementation details, but repeat them here as experimental
# assumptions to validate in this test.
MAX_SIBLING_ARCS = 1<<10
MAX_ARC_NAMESPACE_DEPTH = 1<<10
MAX_ARC_CAPACITY = 1<<15

class TestPcpCapacity(unittest.TestCase):

    def test_PcpSiblingCapacity(self):
        """
        Test MAX_SIBLING_ARCS.
        """
        layer = Sdf.Layer.CreateAnonymous()
        rootPrimSpec = Sdf.PrimSpec(layer, 'root', Sdf.SpecifierDef)

        # create 1<<10 arcs out from a single prim:
        #
        # /root -> { /p1, /p2, ... }
        #
        for i in range(MAX_SIBLING_ARCS+1):
            rootPrimSpec.referenceList.Add(
                Sdf.Reference(layer.identifier, '/p'+str(i)))

        # Create the Pcp structures
        lsi = Pcp.LayerStackIdentifier(layer)
        self.assertTrue(lsi)
        pcpCache = Pcp.Cache(lsi, usd=True)
        self.assertTrue(pcpCache)
        pcpCache.ComputeLayerStack(lsi)
        (pi, errors) = pcpCache.ComputePrimIndex(rootPrimSpec.path)

        # Validate that we get the expected error
        assert Pcp.ErrorType_ArcCapacityExceeded in \
            [err.errorType for err in errors]

    def test_PcpArcNamespaceDepthCapacity(self):
        """
        Test MAX_ARC_NAMESPACE_DEPTH.
        """
        layer = Sdf.Layer.CreateAnonymous()
        rootPrimSpec = Sdf.PrimSpec(layer, 'root', Sdf.SpecifierDef)

        # create a deep namespace of prims:
        #
        # /root/c/c/.../c
        #
        primSpec = rootPrimSpec
        for i in range(MAX_ARC_NAMESPACE_DEPTH-1):
            primSpec = Sdf.PrimSpec(primSpec, 'c', Sdf.SpecifierDef)

        # add an inherit arc at the leaf:
        #
        # /root/c/c/.../c -> /class
        #
        primSpec.inheritPathList.Add('/class')

        # Create the Pcp structures
        lsi = Pcp.LayerStackIdentifier(layer)
        self.assertTrue(lsi)
        pcpCache = Pcp.Cache(lsi, usd=True)
        self.assertTrue(pcpCache)
        pcpCache.ComputeLayerStack(lsi)
        (pi, errors) = pcpCache.ComputePrimIndex(primSpec.path)

        # Validate that we get the expected error
        assert Pcp.ErrorType_ArcNamespaceDepthCapacityExceeded in \
            [err.errorType for err in errors]

    def test_PcpCapacity(self):
        """
        Test MAX_ARC_CAPACITY.
        """
        layer = Sdf.Layer.CreateAnonymous()

        # create a linked chain of prim references:
        #
        # /p0 -> /p1 -> /p2 -> ... /p_MAX
        #
        lastPrimSpec = None
        for i in range(MAX_ARC_CAPACITY):
            primSpec = Sdf.PrimSpec(layer, 'p'+str(i), Sdf.SpecifierDef)
            if lastPrimSpec:
                primSpec.referenceList.Add(
                    Sdf.Reference(layer.identifier, lastPrimSpec.path))
            lastPrimSpec = primSpec

        # Create the Pcp structures
        lsi = Pcp.LayerStackIdentifier(layer)
        self.assertTrue(lsi)
        pcpCache = Pcp.Cache(lsi, usd=True)
        self.assertTrue(pcpCache)
        pcpCache.ComputeLayerStack(lsi)
        (pi, errors) = pcpCache.ComputePrimIndex(lastPrimSpec.path)

        # Validate that we get the expected error
        assert Pcp.ErrorType_IndexCapacityExceeded in \
            [err.errorType for err in errors]

# XXX(blevin) The below test is intended to hit the
# capacity limit in InsertChildSubgraph, by using sub-root references.
# Currently it hits a stack overflow in recursive _AddArc() processing.
# If we want to make this work, we'd need to re-visit how we do
# ancestral node processing, which is a non-trivial project.
# Leaving this here in case it is useful in the future.
if 0:
    def test_PcpCapacity(self):
        """
        Test MAX_ARC_CAPACITY with sub-root references.
        """
        layer = Sdf.Layer.CreateAnonymous()
        rootPrimSpec = Sdf.PrimSpec(layer, 'root', Sdf.SpecifierDef)

        # create a linked chain of prim references:
        #
        # /root/p0 -> /root/p1 -> /root/p2 -> ... /root/p_MAX
        #
        lastPrimSpec = None
        for i in range(MAX_ARC_CAPACITY):
            primSpec = Sdf.PrimSpec(rootPrimSpec, 'p'+str(i), Sdf.SpecifierDef)
            if lastPrimSpec:
                primSpec.referenceList.Add(
                    Sdf.Reference(layer.identifier, lastPrimSpec.path))
            lastPrimSpec = primSpec

        print(lastPrimSpec.path)

        # Create the Pcp structures
        lsi = Pcp.LayerStackIdentifier(layer)
        self.assertTrue(lsi)
        pcpCache = Pcp.Cache(lsi, usd=True)
        self.assertTrue(pcpCache)
        pcpCache.ComputeLayerStack(lsi)
        (pi, errors) = pcpCache.ComputePrimIndex(lastPrimSpec.path)

        # Validate that we get the expected error
        assert Pcp.ErrorType_IndexCapacityExceeded in \
            [err.errorType for err in errors]

if __name__ == "__main__":
    unittest.main()
