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

from pxr import Usd, UsdSkel, UsdGeom, Vt, Sdf
import unittest


class TestUsdSkelSkinningQuery(unittest.TestCase):

    def test_JointInfluences(self):
        """Tests interpretation of joint influences."""

        testFile = "jointInfluences.usda"
        stage = Usd.Stage.Open(testFile)

        rootPath = "/JointInfluences"

        cache = UsdSkel.Cache()

        root = UsdSkel.Root(stage.GetPrimAtPath(rootPath))
        self.assertTrue(cache.Populate(root))

        def _GetSkinningQuery(path):
            return cache.GetSkinningQuery(stage.GetPrimAtPath(path))

        self.assertFalse(_GetSkinningQuery(
            rootPath+"/ErrorCases/MismatchedInterpolation")
                         .HasJointInfluences())

        self.assertFalse(_GetSkinningQuery(
            rootPath+"/ErrorCases/InvalidInterpolation1")
                         .HasJointInfluences())

        self.assertFalse(_GetSkinningQuery(
            rootPath+"/ErrorCases/InvalidInterpolation2")
                         .HasJointInfluences())

        self.assertFalse(_GetSkinningQuery(
            rootPath+"/ErrorCases/MismatchedElementSize")
                         .HasJointInfluences())

        self.assertFalse(_GetSkinningQuery(
            rootPath+"/ErrorCases/InvalidElementSize")
                         .HasJointInfluences())

        #
        # Validate error cases during ComputeJointInfluences()
        #
        query = _GetSkinningQuery(
            rootPath+"/ErrorCases/InvalidArraySize1")
        assert query.GetPrim()
        assert query
        assert not query.ComputeJointInfluences()
        assert not query.ComputeVaryingJointInfluences(10)

        query = _GetSkinningQuery(
            rootPath+"/ErrorCases/InvalidArraySize2")
        assert query
        assert not query.ComputeJointInfluences()
        assert not query.ComputeVaryingJointInfluences(10)

        #
        # The remaining cases should all be valid.
        #

        query = _GetSkinningQuery(rootPath+"/RigidWeights")
        assert query
        assert query.IsRigidlyDeformed()
        assert query.GetNumInfluencesPerComponent() == 3
        influences = query.ComputeJointInfluences()
        assert influences
        indices,weights = influences
        assert indices == Vt.IntArray([1,2,3])
        assert weights == Vt.FloatArray([5,6,7])

        influences = query.ComputeVaryingJointInfluences(3)
        assert influences
        indices, weights = influences
        assert indices == Vt.IntArray([1,2,3,1,2,3,1,2,3])
        assert weights == Vt.FloatArray([5,6,7,5,6,7,5,6,7])
        
        query = _GetSkinningQuery(rootPath+"/NonRigidWeights")
        assert query
        assert not query.IsRigidlyDeformed()
        assert query.GetNumInfluencesPerComponent() == 2
        influences = query.ComputeJointInfluences()
        assert influences
        indices,weights = influences
        assert indices == Vt.IntArray([1,2,3,4])
        assert weights == Vt.FloatArray([5,6,7,8])

        varyingInfluences = query.ComputeVaryingJointInfluences(2)
        assert influences == varyingInfluences


if __name__ == "__main__":
    unittest.main()
