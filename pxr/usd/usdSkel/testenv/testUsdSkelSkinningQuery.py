#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
        self.assertTrue(cache.Populate(
            root, Usd.PrimDefaultPredicate))

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
        skinningMethodAttr = query.GetSkinningMethodAttr()
        assert not skinningMethodAttr

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
        skinningMethodAttr = query.GetSkinningMethodAttr()
        assert skinningMethodAttr
        assert skinningMethodAttr.Get() == 'classicLinear'

        varyingInfluences = query.ComputeVaryingJointInfluences(2)
        assert influences == varyingInfluences

        query = _GetSkinningQuery(rootPath+"/NonRigidWeightsDQ")
        assert query
        assert not query.IsRigidlyDeformed()
        assert query.GetNumInfluencesPerComponent() == 2
        influences = query.ComputeJointInfluences()
        assert influences
        indices,weights = influences
        assert indices == Vt.IntArray([1,2,3,4])
        assert weights == Vt.FloatArray([5,6,7,8])
        skinningMethodAttr = query.GetSkinningMethodAttr()
        assert skinningMethodAttr
        assert skinningMethodAttr.Get() == 'dualQuaternion'

if __name__ == "__main__":
    unittest.main()
