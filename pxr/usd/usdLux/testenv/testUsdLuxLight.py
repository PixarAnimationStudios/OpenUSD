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

from pxr import Gf, Sdf, Usd, UsdLux, UsdShade
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

    def test_BasicConnectableLights(self):
        stage = Usd.Stage.CreateInMemory()
        light = UsdLux.RectLight.Define(stage, '/RectLight')
        self.assertTrue(light)

        # Rect light has the following built-in inputs attributes.
        inputNames = ['color', 
                      'colorTemperature', 
                      'diffuse', 
                      'enableColorTemperature', 
                      'exposure', 
                      'height', 
                      'intensity', 
                      'normalize', 
                      'specular', 
                      'texture:file', 
                      'width']
        # GetInputs returns the inputs for all the built-ins.
        self.assertEqual(light.GetInputs(), 
                         [light.GetInput(name) for name in inputNames])
        # Verify each input's attribute is prefixed.
        for name in inputNames:
            self.assertEqual(light.GetInput(name).GetAttr().GetName(),
                             "inputs:" + name)
        # Verify input attributes match the getter API attributes.
        self.assertEqual(light.GetInput('color').GetAttr(), 
                         light.GetColorAttr())
        self.assertEqual(light.GetInput('texture:file').GetAttr(), 
                         light.GetTextureFileAttr())

        # Create a new input, and verify that the input interface conforming
        # attribute is created.
        lightInput = light.CreateInput('newInput', Sdf.ValueTypeNames.Float)
        self.assertIn(lightInput, light.GetInputs())
        self.assertEqual(light.GetInput('newInput'), lightInput)
        self.assertEqual(lightInput.GetAttr(), 
                         light.GetPrim().GetAttribute("inputs:newInput"))

        # Rect light has no built-in outputs.
        self.assertEqual(light.GetOutputs(), [])

        # Create a new output, and verify that the output interface conforming
        # attribute is created.
        lightOutput = light.CreateOutput('newOutput', Sdf.ValueTypeNames.Float)
        self.assertEqual(light.GetOutputs(), [lightOutput])
        self.assertEqual(light.GetOutput('newOutput'), lightOutput)
        self.assertEqual(lightOutput.GetAttr(), 
                         light.GetPrim().GetAttribute("outputs:newOutput"))

        # Do the same with a light filter
        lightFilter = UsdLux.LightFilter.Define(stage, '/LightFilter')
        self.assertTrue(lightFilter)

        # Light filter has no built-in inputs.
        self.assertEqual(lightFilter.GetInputs(), [])

        # Create a new input, and verify that the input interface conforming
        # attribute is created.
        filterInput = lightFilter.CreateInput('newInput', 
                                              Sdf.ValueTypeNames.Float)
        self.assertEqual(lightFilter.GetInputs(), [filterInput])
        self.assertEqual(lightFilter.GetInput('newInput'), filterInput)
        self.assertEqual(filterInput.GetAttr(), 
                         lightFilter.GetPrim().GetAttribute("inputs:newInput"))

        # Light filter has no built-in outputs.
        self.assertEqual(lightFilter.GetOutputs(), [])

        # Create a new output, and verify that the output interface conforming
        # attribute is created.
        filterOutput = lightFilter.CreateOutput('newOutput', 
                                                Sdf.ValueTypeNames.Float)
        self.assertEqual(lightFilter.GetOutputs(), [filterOutput])
        self.assertEqual(lightFilter.GetOutput('newOutput'), filterOutput)
        self.assertEqual(filterOutput.GetAttr(), 
                         lightFilter.GetPrim().GetAttribute("outputs:newOutput"))

        # Test the connection behavior customization.
        # Create a connectable prim with an output under the light.
        lightGraph = UsdShade.NodeGraph.Define(stage, '/RectLight/Prim')
        self.assertTrue(lightGraph)
        lightGraphOutput = lightGraph.CreateOutput(
            'graphOut', Sdf.ValueTypeNames.Float)
        self.assertTrue(lightGraphOutput)

        # Create a connectable prim with an output under the light filter.
        filterGraph = UsdShade.NodeGraph.Define(stage, '/LightFilter/Prim')
        self.assertTrue(filterGraph)
        filterGraphOutput = filterGraph.CreateOutput(
            'graphOut', Sdf.ValueTypeNames.Float)
        self.assertTrue(filterGraphOutput)

        # From the default behavior light outputs cannot be connected.
        self.assertFalse(lightOutput.CanConnect(lightGraphOutput))
        self.assertFalse(lightOutput.CanConnect(filterGraphOutput))

        # From the custom behavior, light inputs can only be connected to 
        # sources from the light or its descendant (encapsultated) prims.
        self.assertTrue(lightInput.CanConnect(lightOutput))
        self.assertTrue(lightInput.CanConnect(lightGraphOutput))
        self.assertFalse(lightInput.CanConnect(filterGraphOutput))

        # From the default behavior light filter outputs cannot be connected.
        self.assertFalse(filterOutput.CanConnect(lightGraphOutput))
        self.assertFalse(filterOutput.CanConnect(filterGraphOutput))

        # From the custom behavior, light filter inputs can only be connected to 
        # sources from the light filter or its descendant (encapsultated) prims.
        self.assertTrue(filterInput.CanConnect(filterOutput))
        self.assertTrue(filterInput.CanConnect(filterGraphOutput))
        self.assertFalse(filterInput.CanConnect(lightGraphOutput))

    def test_DomeLight_OrientToStageUpAxis(self):
        from pxr import UsdGeom
        stage = Usd.Stage.CreateInMemory()
        # Try Y-up first.  Explicitly set this to override any site-level
        # override.
        UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
        # Create a dome.
        light = UsdLux.DomeLight.Define(stage, '/dome')
        # No Xform ops to begin with.
        self.assertEqual(light.GetOrderedXformOps(), [])
        # Align to up axis.
        light.OrientToStageUpAxis()
        # Since the stage is already Y-up, no additional xform op was required.
        self.assertEqual(light.GetOrderedXformOps(), [])
        # Now change the stage to Z-up and re-align the dome.
        UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.z)
        light.OrientToStageUpAxis()
        # That should require a +90 deg rotate on X.
        ops = light.GetOrderedXformOps()
        self.assertEqual(len(ops), 1)
        self.assertEqual(ops[0].GetBaseName(),
            UsdLux.Tokens.orientToStageUpAxis)
        self.assertEqual(ops[0].GetOpType(), UsdGeom.XformOp.TypeRotateX)
        self.assertEqual(ops[0].GetAttr().Get(), 90.0)

    def test_UsdLux_HasConnectableAPI(self):
        from pxr import Tf
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(UsdLux.Light))
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            UsdLux.LightFilter))

if __name__ == '__main__':
    unittest.main()
