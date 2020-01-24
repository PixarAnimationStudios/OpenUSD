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
