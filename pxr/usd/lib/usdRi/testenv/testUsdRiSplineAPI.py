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
#

from pxr import Sdf, Usd, UsdLux, UsdRi
import unittest

def SetEaseOut(spline):
    spline.CreateInterpolationAttr().Set(UsdRi.Tokens.catmullRom)
    spline.CreatePositionsAttr().Set([0.0, 0.3, 0.7, 1.0])
    spline.CreateValuesAttr().Set([1.0, 0.8, 0.2, 0.0])

def IsValid(spline):
    return spline.Validate()[0]

class TestUsdRiSplineAPI(unittest.TestCase):
    def test_Basic(self):
        stage = Usd.Stage.CreateInMemory()

        # can't use these if not properly initialized
        bogusSpline = UsdRi.SplineAPI()
        assert not bogusSpline.Validate()[0]

        light = UsdLux.SphereLight.Define(stage, '/Light')
        rod = UsdRi.PxrRodLightFilter.Define(stage, '/Light/Rod')
        light.GetFiltersRel().SetTargets([rod.GetPath()])

        falloffRamp = rod.GetFalloffRampAPI()
        colorRamp = rod.GetColorRampAPI()

        # initially invalid since no spline exists
        assert not IsValid(falloffRamp)
        assert not IsValid(colorRamp)

        # set a simple ease-out falloff spline
        SetEaseOut(falloffRamp)
        assert IsValid(falloffRamp)

        # try a mismatch of values & positions
        falloffRamp.CreateValuesAttr().Set([1.0, 0.8, 0.2])
        assert not IsValid(falloffRamp)

        # try a bogus interpolation value
        SetEaseOut(falloffRamp)
        assert IsValid(falloffRamp)
        falloffRamp.CreateInterpolationAttr().Set('bogus')
        assert not IsValid(falloffRamp)

        # check all known interp types
        for interp in ['linear', 'constant', 'catmullRom', 'bspline']:
            falloffRamp.CreateInterpolationAttr().Set(interp)
            assert IsValid(falloffRamp)

        # try non-sorted positions
        falloffRamp.CreatePositionsAttr().Set([1.0, 0.7, 0.3, 0.0])
        assert not IsValid(falloffRamp)

        # set a red->green->blue color ramp
        colorRamp.CreateInterpolationAttr().Set(UsdRi.Tokens.linear)
        colorRamp.CreatePositionsAttr().Set([0.0, 0.5, 1.0])
        colorRamp.CreateValuesAttr().Set([
            (1.0, 0.0, 0.0),
            (0.0, 1.0, 0.0),
            (0.0, 0.0, 1.0)])
        assert IsValid(colorRamp)

if __name__ == "__main__":
    unittest.main()
