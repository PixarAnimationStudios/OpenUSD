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

from pxr import Gf, Sdf, Sdr, Tf, Usd, UsdLux, UsdShade
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
        self.assertTrue(light.ConnectableAPI())

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
        # GetInputs returns only authored inputs by default
        self.assertEqual(light.GetInputs(), [])
        # GetInputs(false) returns the inputs for all the built-ins.
        self.assertEqual(light.GetInputs(onlyAuthored=False), 
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
        # By default GetInputs() returns onlyAuthored inputs, of which
        # there is now 1.
        self.assertEqual(len(light.GetInputs()), 1)
        # Passing onlyAuthored=False will return the authored input
        # in addition to the builtins.
        self.assertEqual(len(light.GetInputs(onlyAuthored=False)),
            len(inputNames)+1)
        self.assertEqual(light.GetInput('newInput'), lightInput)
        self.assertEqual(lightInput.GetAttr(), 
                         light.GetPrim().GetAttribute("inputs:newInput"))

        # Rect light has no authored outputs.
        self.assertEqual(light.GetOutputs(), [])
        # Rect light has no built-in outputs, either.
        self.assertEqual(light.GetOutputs(onlyAuthored=False), [])

        # Create a new output, and verify that the output interface conforming
        # attribute is created.
        lightOutput = light.CreateOutput('newOutput', Sdf.ValueTypeNames.Float)
        self.assertEqual(light.GetOutputs(), [lightOutput])
        self.assertEqual(light.GetOutputs(onlyAuthored=False), [lightOutput])
        self.assertEqual(light.GetOutput('newOutput'), lightOutput)
        self.assertEqual(lightOutput.GetAttr(), 
                         light.GetPrim().GetAttribute("outputs:newOutput"))

        # Do the same with a light filter
        lightFilter = UsdLux.LightFilter.Define(stage, '/LightFilter')
        self.assertTrue(lightFilter)
        self.assertTrue(lightFilter.ConnectableAPI())

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
        self.assertEqual(lightFilter.GetOutputs(onlyAuthored=False), [])

        # Create a new output, and verify that the output interface conforming
        # attribute is created.
        filterOutput = lightFilter.CreateOutput('newOutput', 
                                                Sdf.ValueTypeNames.Float)
        self.assertEqual(lightFilter.GetOutputs(), [filterOutput])
        self.assertEqual(lightFilter.GetOutputs(onlyAuthored=False),
            [filterOutput])
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
        # sources from immediate descendants (encapsultated) prims of the light.
        self.assertFalse(lightInput.CanConnect(lightOutput))
        self.assertTrue(lightInput.CanConnect(lightGraphOutput))
        self.assertFalse(lightInput.CanConnect(filterGraphOutput))

        # From the default behavior light filter outputs cannot be connected.
        self.assertFalse(filterOutput.CanConnect(lightGraphOutput))
        self.assertFalse(filterOutput.CanConnect(filterGraphOutput))

        # From the custom behavior, light filter inputs can only be connected to 
        # sources from immediate descendants (encapsultated) prims of the light 
        # filter.
        self.assertFalse(filterInput.CanConnect(filterOutput))
        self.assertTrue(filterInput.CanConnect(filterGraphOutput))
        self.assertFalse(filterInput.CanConnect(lightGraphOutput))

        # The shaping API can add more connectable attributes to the light 
        # and implements the same connectable interface functions. We test 
        # those here.
        shapingAPI = UsdLux.ShapingAPI.Apply(light.GetPrim())
        self.assertTrue(shapingAPI)
        self.assertTrue(shapingAPI.ConnectableAPI())
        # Verify input attributes match the getter API attributes.
        self.assertEqual(shapingAPI.GetInput('shaping:cone:angle').GetAttr(), 
                         shapingAPI.GetShapingConeAngleAttr())
        self.assertEqual(shapingAPI.GetInput('shaping:focus').GetAttr(), 
                         shapingAPI.GetShapingFocusAttr())
        # These inputs have the same connectable behaviors as all light inputs,
        # i.e. they can only be connected to sources from immediate 
        # descendant (encapsultated) prims of the light.
        shapingInput = shapingAPI.GetInput('shaping:focus')
        self.assertFalse(shapingInput.CanConnect(lightOutput))
        self.assertTrue(shapingInput.CanConnect(lightGraphOutput))
        self.assertFalse(shapingInput.CanConnect(filterGraphOutput))

        # The shadow API can add more connectable attributes to the light 
        # and implements the same connectable interface functions. We test 
        # those here.
        shadowAPI = UsdLux.ShadowAPI.Apply(light.GetPrim())
        self.assertTrue(shadowAPI)
        self.assertTrue(shadowAPI.ConnectableAPI())
        # Verify input attributes match the getter API attributes.
        self.assertEqual(shadowAPI.GetInput('shadow:color').GetAttr(), 
                         shadowAPI.GetShadowColorAttr())
        self.assertEqual(shadowAPI.GetInput('shadow:distance').GetAttr(), 
                         shadowAPI.GetShadowDistanceAttr())
        # These inputs have the same connectable behaviors as all light inputs,
        # i.e. they can only be connected to sources from immediate 
        # descendant (encapsultated) prims of the light.
        shadowInput = shadowAPI.GetInput('shadow:color')
        self.assertFalse(shadowInput.CanConnect(lightOutput))
        self.assertTrue(shadowInput.CanConnect(lightGraphOutput))
        self.assertFalse(shadowInput.CanConnect(filterGraphOutput))

        # Even though the shadow and shaping API schemas provide connectable
        # attributes and an interface for the ConnectableAPI, the typed schema
        # of the prim is still what provides its connectable behavior. Here
        # we verify that applying these APIs to a prim whose type is not 
        # connectable does NOT cause the prim to conform to the Connectable API.
        nonConnectablePrim = stage.DefinePrim("/Sphere", "Sphere")
        shadowAPI = UsdLux.ShadowAPI.Apply(nonConnectablePrim)
        self.assertTrue(shadowAPI)
        self.assertFalse(shadowAPI.ConnectableAPI())
        shapingAPI = UsdLux.ShapingAPI.Apply(nonConnectablePrim)
        self.assertTrue(shapingAPI)
        self.assertFalse(shapingAPI.ConnectableAPI())

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

    def test_SdrShaderNodesForLights(self):
        """
        Test the automatic registration of SdrShaderNodes for all the UsdLux
        light types.
        """
        # Get all the derived types of UsdLuxLight
        lightTypes = Tf.Type(UsdLux.Light).GetAllDerivedTypes()
        self.assertTrue(lightTypes)
        # Verify that at least one known light type is in our list to guard
        # against this giving false positives if no light types are available.
        self.assertIn(UsdLux.RectLight, lightTypes)

        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/Prim")

        for lightType in lightTypes:
            # Every concrete light type will have an SdrShaderNode with source
            # type 'USD' registered for it under its USD schema type name.
            typeName = Usd.SchemaRegistry.GetConcreteSchemaTypeName(lightType)
            node = Sdr.Registry().GetNodeByName(typeName, ['USD'])
            self.assertTrue(node.IsValid())

            # Set the prim to the light type so we can cross check node inputs
            # with the light prim built-in properties.
            prim.SetTypeName(typeName)
            light = UsdLux.Light(prim)
            self.assertTrue(light)

            # Names, identifier, and role for the node all match the USD schema
            # type name
            self.assertEqual(node.GetIdentifier(), typeName)
            self.assertEqual(node.GetName(), typeName)
            self.assertEqual(node.GetImplementationName(), typeName)
            self.assertEqual(node.GetRole(), typeName)
            self.assertTrue(node.GetInfoString().startswith(typeName))

            # The context is always 'light'. Source type is 'USD'
            self.assertEqual(node.GetContext(), 'light')
            self.assertEqual(node.GetSourceType(), 'USD')

            # Help string is generated and encoded in the node's metadata (no
            # need to verify the specific wording).
            self.assertTrue(set(node.GetMetadata().keys()), {'primvars', 'help'})
            self.assertEqual(node.GetMetadata()["help"], node.GetHelp())

            # Source code and URIs are all empty.
            self.assertFalse(node.GetSourceCode())
            self.assertFalse(node.GetResolvedDefinitionURI())
            self.assertFalse(node.GetResolvedImplementationURI())

            # Other classifications are left empty.
            self.assertFalse(node.GetCategory())
            self.assertFalse(node.GetDepartments())
            self.assertFalse(node.GetFamily())
            self.assertFalse(node.GetLabel())
            self.assertFalse(node.GetVersion())
            self.assertFalse(node.GetAllVstructNames())
            self.assertEqual(node.GetPages(), [''])

            # Helper for comparing an SdrShaderProperty from node to the 
            # corresponding UsdShadeInput/UsdShadeOutput from a UsdLuxLight
            def _CompareLightPropToNodeProp(nodeInput, lightInput):
                # Input names and default values match.
                self.assertEqual(nodeInput.GetName(), lightInput.GetBaseName())
                self.assertEqual(nodeInput.GetDefaultValue(),
                                 lightInput.GetAttr().Get())

                # Some USD property types don't match exactly one to one and are
                # converted to different types. In particular relevance to 
                # lights, Bool becomes Int and Token becomes String.
                expectedTypeName = lightInput.GetTypeName()
                if expectedTypeName == Sdf.ValueTypeNames.Bool:
                    expectedTypeName = Sdf.ValueTypeNames.Int 
                elif expectedTypeName == Sdf.ValueTypeNames.Token:
                    expectedTypeName = Sdf.ValueTypeNames.String 
                # Verify the node's input type maps back to USD property's type
                # (with the noted above exceptions).
                self.assertEqual(
                    nodeInput.GetTypeAsSdfType()[0], expectedTypeName,
                    msg="Type {} != {}".format(
                        str(nodeInput.GetTypeAsSdfType()[0]),
                        str(expectedTypeName)))
                # If the USD property type is an Asset, it will be listed in 
                # the node's asset indentifier inputs.
                if expectedTypeName == Sdf.ValueTypeNames.Asset:
                    self.assertIn(nodeInput.GetName(), 
                                  node.GetAssetIdentifierInputNames())

            # There will be a one to one correspondence between node inputs
            # and light prim inputs.
            nodeInputs = [node.GetInput(i) for i in node.GetInputNames()]
            lightInputs = light.GetInputs(onlyAuthored=False)
            self.assertEqual(len(nodeInputs), len(lightInputs))
            for nodeInput, lightInput in zip(nodeInputs, lightInputs):
                self.assertFalse(nodeInput.IsOutput())
                _CompareLightPropToNodeProp(nodeInput, lightInput)

            # There will also be a one to one correspondence between node 
            # outputs and light prim outputs.
            nodeOutputs = [node.GetOutput(i) for i in node.GetOutputNames()]
            lightOutputs = light.GetOutputs(onlyAuthored=False)
            self.assertEqual(len(nodeOutputs), len(lightOutputs))
            for nodeOutput, lightOutput in zip(nodeOutputs, lightOutputs):
                self.assertTrue(nodeOutput.IsOutput())
                _CompareLightPropToNodeProp(nodeOutput, lightOutput)

            # The reverse is tested just above, but for all asset identifier
            # inputs listed for the node there is a corresponding asset value
            # input property on the light prim.
            for inputName in node.GetAssetIdentifierInputNames():
                self.assertEqual(light.GetInput(inputName).GetTypeName(),
                                 Sdf.ValueTypeNames.Asset)

            # These primvars come from sdrMetadata on the prim itself which
            # isn't supported for light schemas so it will alwasy be empty.
            self.assertFalse(node.GetPrimvars())
            # sdrMetadata on input properties is supported so additional 
            # primvar properties will correspond to light inputs with that 
            # metadata set.
            for propName in node.GetAdditionalPrimvarProperties():
                self.assertTrue(light.GetInput(propName).GetSdrMetadataByKey(
                    'primvarProperty'))

            # Default input can also be specified in the property's sdrMetadata.
            if node.GetDefaultInput():
                defaultLightInput = light.GetInput(
                    node.GetDefaultInput().GetName())
                self.assertTrue(lightInput.GetSdrMetadataByKey('defaultInput'))


if __name__ == '__main__':
    unittest.main()
