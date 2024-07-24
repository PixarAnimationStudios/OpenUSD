#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdSkel, UsdGeom, Vt, Sdf
import unittest


class TestUsdSkelBindingAPI(unittest.TestCase):


    def test_JointInfluences(self):
        """Tests for helpers for getting/setting joint influences."""
        
        stage = Usd.Stage.CreateInMemory()
        gprim = stage.DefinePrim('/Model/Gprim')

        binding = UsdSkel.BindingAPI(gprim)

        indices = binding.CreateJointIndicesPrimvar(constant=False,
                                                    elementSize=3)
        assert indices
        assert indices.GetInterpolation() == UsdGeom.Tokens.vertex
        assert indices.GetElementSize() == 3

        weights = binding.CreateJointWeightsPrimvar(constant=True)
        assert weights
        assert weights.GetInterpolation() == UsdGeom.Tokens.constant

        # Should be able to re-create bindings with an alternate
        # interpolation and/or element size.
        weights = binding.CreateJointWeightsPrimvar(constant=False,
                                                    elementSize=3)
        assert weights
        assert weights.GetInterpolation() == UsdGeom.Tokens.vertex
        assert weights.GetElementSize() == 3

        assert binding.SetRigidJointInfluence(10, 0.5)
        indices = binding.GetJointIndicesPrimvar()
        weights = binding.GetJointWeightsPrimvar()
        assert indices.Get() == Vt.IntArray([10])
        assert weights.Get() == Vt.FloatArray([0.5])


if __name__ == "__main__":
    unittest.main()
