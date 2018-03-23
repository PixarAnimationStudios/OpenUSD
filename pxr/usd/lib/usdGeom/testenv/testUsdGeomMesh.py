#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Usd, UsdGeom, Vt
import unittest


class TestUsdGeomMesh(unittest.TestCase):


    def test_ValidateTopology(self):
        """Tests helpers for validating mesh topology."""
        
        # sum(vertexCounts) != len(vertexIndices)
        faceVertexIndices = Vt.IntArray([0,1,2])
        faceVertexCounts = Vt.IntArray([2,2])

        valid,why = UsdGeom.Mesh.ValidateTopology(faceVertexIndices,
                                                  faceVertexCounts,
                                                  numPoints=3)

        self.assertFalse(valid)
        # Make sure we have a reason.
        self.assertTrue(why)

        # Negative vertex indices.
        faceVertexIndices = Vt.IntArray([0,-1,1])
        faceVertexCounts = Vt.IntArray([3])

        valid,why = UsdGeom.Mesh.ValidateTopology(faceVertexIndices,
                                                  faceVertexCounts,
                                                  numPoints=3)
        self.assertFalse(valid)
        # Make sure we have a reason.
        self.assertTrue(why)

        # Out of range vertex indices.
        faceVertexIndices = Vt.IntArray([1,2,3])
        faceVertexCounts = Vt.IntArray([3])

        valid,why = UsdGeom.Mesh.ValidateTopology(faceVertexIndices,
                                                  faceVertexCounts,
                                                  numPoints=3)

        self.assertFalse(valid)
        # Make sure we have a reason.
        self.assertTrue(why)

        # Valid topology.
        faceVertexIndices = Vt.IntArray([0,1,2,3,4,5])
        faceVertexCounts = Vt.IntArray([3,3])

        valid,why = UsdGeom.Mesh.ValidateTopology(faceVertexIndices,
                                                  faceVertexCounts,
                                                  numPoints=6)
        self.assertTrue(valid)
        # Shoult not have set a reason.
        self.assertFalse(why)


if __name__ == '__main__':
    unittest.main()
