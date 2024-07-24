#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
        with self.assertRaises(RuntimeError):
            bogusSpline.Validate()[0]

        light = UsdLux.SphereLight.Define(stage, '/Light')
        rod = stage.DefinePrim("/Light/Rod", "PxrRodLightFilter")
        light.GetFiltersRel().SetTargets([rod.GetPath()])

        # Create SplineAPI for "fallOffRamp" spline for the Rod prim
        falloffRamp = UsdRi.SplineAPI(rod, "falloffRamp", 
                Sdf.ValueTypeNames.FloatArray, True)
        # Create SplineAPI for "colorRamp" spline for the Rod prim
        colorRamp = UsdRi.SplineAPI(rod, "colorRamp", 
                Sdf.ValueTypeNames.Color3fArray, True)

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
        for interp in ['linear', 'constant', 'catmull-rom', 'bspline']:
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
