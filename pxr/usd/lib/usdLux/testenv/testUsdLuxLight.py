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

from pxr import Gf, Usd, UsdLux
import unittest, math

class TestUsdLuxLight(unittest.TestCase):

    def test_BlackbodySpectrum(self):
        warm_color = UsdLux.BlackbodyTemperatureAsRgb(1000)
        whitepoint = UsdLux.BlackbodyTemperatureAsRgb(6500)
        cool_color = UsdLux.BlackbodyTemperatureAsRgb(10000)
        # Whitepoint is ~= (1,1,1)
        assert Gf.IsClose(whitepoint, Gf.Vec3f(1.0), 0.1)
        # Warm has more red than green or blue
        assert warm_color[0] > warm_color[1]
        assert warm_color[0] > warm_color[2]
        # Cool has more blue than red or green
        assert cool_color[2] > cool_color[0]
        assert cool_color[2] > cool_color[1]

    def test_BasicLights(self):
        stage = Usd.Stage.CreateInMemory()
        light = UsdLux.SphereLight.Define(stage, '/light')

        # Intensity is linear
        self.assertEqual( light.ComputeBaseEmission(), Gf.Vec3f(1.0) )
        light.CreateIntensityAttr().Set(123.0)
        self.assertEqual( light.ComputeBaseEmission(), Gf.Vec3f(123.0) )

        # Exposure is power-of-two and multiplies against intensity
        light.CreateExposureAttr().Set(1.0)
        self.assertEqual( light.ComputeBaseEmission(), Gf.Vec3f(246.0) )
        light.CreateExposureAttr().Set(-1.0)
        self.assertEqual( light.ComputeBaseEmission(), Gf.Vec3f(61.5) )

        # Color multiplies the result
        light.CreateColorAttr().Set( Gf.Vec3f(1.0, 2.0, 0.0) )
        self.assertEqual( light.ComputeBaseEmission(),
                          Gf.Vec3f(61.5, 123.0, 0.0))

        # Color temperature further multiplies the result,
        # but only once enabled
        e0 = light.ComputeBaseEmission()
        light.CreateColorTemperatureAttr().Set( 1000 )
        e1 = light.ComputeBaseEmission()
        self.assertEqual(e0, e1)
        light.CreateEnableColorTemperatureAttr().Set(True)
        e2 = light.ComputeBaseEmission()
        self.assertNotEqual(e0, e2)
        # Default temperature is whitepoint and approximately (1,1,1)
        light.CreateEnableColorTemperatureAttr().Clear()
        e3 = light.ComputeBaseEmission()
        self.assertTrue( Gf.IsClose(e0, e3, 0.1))

if __name__ == '__main__':
    unittest.main()
