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

from __future__ import print_function

from pxr import Gf, Sdf, Sdr, Tf, Usd, UsdGeom, UsdLux, UsdShade, Plug
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

    def test_BasicConnectableLights(self):
        # Try checking connectableAPI on core lux types first before going
        # through the prim.
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            UsdLux.RectLight))
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            UsdLux.PluginLightFilter))
        stage = Usd.Stage.CreateInMemory()
        rectLight = UsdLux.RectLight.Define(stage, '/RectLight')
        self.assertTrue(rectLight)
        lightAPI = rectLight.LightAPI()
        self.assertTrue(lightAPI)
        self.assertTrue(lightAPI.ConnectableAPI())

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
        self.assertEqual(lightAPI.GetInputs(), [])

        # GetInputs(false) is a super-set of all the built-ins.
        # There could be other inputs coming from any auto applied APISchemas.
        allInputs = [inputName.GetBaseName() for inputName in
                lightAPI.GetInputs(onlyAuthored=False)]
        self.assertTrue(set(inputNames).issubset(set(allInputs)))

        # Verify each input's attribute is prefixed.
        for name in inputNames:
            self.assertEqual(lightAPI.GetInput(name).GetAttr().GetName(),
                             "inputs:" + name)
        # Verify input attributes match the getter API attributes.
        self.assertEqual(lightAPI.GetInput('color').GetAttr(), 
                         rectLight.GetColorAttr())
        self.assertEqual(lightAPI.GetInput('texture:file').GetAttr(), 
                         rectLight.GetTextureFileAttr())

        # Create a new input, and verify that the input interface conforming
        # attribute is created.
        lightInput = lightAPI.CreateInput('newInput', Sdf.ValueTypeNames.Float)
        self.assertIn(lightInput, lightAPI.GetInputs())
        # By default GetInputs() returns onlyAuthored inputs, of which
        # there is now 1.
        self.assertEqual(len(lightAPI.GetInputs()), 1)
        self.assertEqual(lightAPI.GetInput('newInput'), lightInput)
        self.assertEqual(lightInput.GetAttr(), 
                         lightAPI.GetPrim().GetAttribute("inputs:newInput"))

        # Rect light has no authored outputs.
        self.assertEqual(lightAPI.GetOutputs(), [])
        # Rect light has no built-in outputs, either.
        self.assertEqual(lightAPI.GetOutputs(onlyAuthored=False), [])

        # Create a new output, and verify that the output interface conforming
        # attribute is created.
        lightOutput = lightAPI.CreateOutput('newOutput', Sdf.ValueTypeNames.Float)
        self.assertEqual(lightAPI.GetOutputs(), [lightOutput])
        self.assertEqual(lightAPI.GetOutputs(onlyAuthored=False), [lightOutput])
        self.assertEqual(lightAPI.GetOutput('newOutput'), lightOutput)
        self.assertEqual(lightOutput.GetAttr(), 
                         lightAPI.GetPrim().GetAttribute("outputs:newOutput"))

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

        # Light outputs can be connected.
        self.assertTrue(lightOutput.CanConnect(lightGraphOutput))
        self.assertTrue(lightOutput.CanConnect(filterGraphOutput))

        # Light inputs diverge from the default behavior and should be 
        # connectable across its own scope (encapsulation is not required)
        self.assertTrue(lightInput.CanConnect(lightOutput))
        self.assertTrue(lightInput.CanConnect(lightGraphOutput))
        self.assertTrue(lightInput.CanConnect(filterGraphOutput))

        # From the default behavior light filter outputs cannot be connected.
        self.assertFalse(filterOutput.CanConnect(lightGraphOutput))
        self.assertFalse(filterOutput.CanConnect(filterGraphOutput))

        # Light filters inputs diverge from the default behavior and should be 
        # connectable across its own scope (encapsulation is not required)
        self.assertTrue(filterInput.CanConnect(filterOutput))
        self.assertTrue(filterInput.CanConnect(filterGraphOutput))
        self.assertTrue(filterInput.CanConnect(lightGraphOutput))

        # The shaping API can add more connectable attributes to the light 
        # and implements the same connectable interface functions. We test 
        # those here.
        shapingAPI = UsdLux.ShapingAPI.Apply(lightAPI.GetPrim())
        self.assertTrue(shapingAPI)
        self.assertTrue(shapingAPI.ConnectableAPI())
        # Verify input attributes match the getter API attributes.
        self.assertEqual(shapingAPI.GetInput('shaping:cone:angle').GetAttr(), 
                         shapingAPI.GetShapingConeAngleAttr())
        self.assertEqual(shapingAPI.GetInput('shaping:focus').GetAttr(), 
                         shapingAPI.GetShapingFocusAttr())
        # These inputs have the same connectable behaviors as all light inputs,
        # i.e. they should also diverge from the default behavior of only be 
        # connected to sources from immediate descendant (encapsultated) prims 
        # of the light.
        shapingInput = shapingAPI.GetInput('shaping:focus')
        self.assertTrue(shapingInput.CanConnect(lightOutput))
        self.assertTrue(shapingInput.CanConnect(lightGraphOutput))
        self.assertTrue(shapingInput.CanConnect(filterGraphOutput))

        # The shadow API can add more connectable attributes to the light 
        # and implements the same connectable interface functions. We test 
        # those here.
        shadowAPI = UsdLux.ShadowAPI.Apply(lightAPI.GetPrim())
        self.assertTrue(shadowAPI)
        self.assertTrue(shadowAPI.ConnectableAPI())
        # Verify input attributes match the getter API attributes.
        self.assertEqual(shadowAPI.GetInput('shadow:color').GetAttr(), 
                         shadowAPI.GetShadowColorAttr())
        self.assertEqual(shadowAPI.GetInput('shadow:distance').GetAttr(), 
                         shadowAPI.GetShadowDistanceAttr())
        # These inputs have the same connectable behaviors as all light inputs,
        # i.e. they should also diverge from the default behavior of only be 
        # connected to sources from immediate descendant (encapsultated) prims 
        # of the light.
        shadowInput = shadowAPI.GetInput('shadow:color')
        self.assertTrue(shadowInput.CanConnect(lightOutput))
        self.assertTrue(shadowInput.CanConnect(lightGraphOutput))
        self.assertTrue(shadowInput.CanConnect(filterGraphOutput))

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
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            UsdLux.LightAPI))
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            UsdLux.LightFilter))

    def test_GetShaderId(self):
        # Test the LightAPI shader ID API 

        # UsdLuxLightAPI and UsdLuxLightFilter implement the same API for 
        # their shaderId attributes so we can test them using the same function.
        def _TestShaderIDs(lightOrFilter, shaderIdAttrName):
            # The default render context's shaderId attribute does exist in the 
            # API. These attributes do not yet exist for other contexts.
            self.assertEqual(
                lightOrFilter.GetShaderIdAttrForRenderContext("").GetName(),
                shaderIdAttrName)
            self.assertFalse(
                lightOrFilter.GetShaderIdAttrForRenderContext("ri"))
            self.assertFalse(
                lightOrFilter.GetShaderIdAttrForRenderContext("other"))

            # By default LightAPI shader IDs are empty for all render contexts.
            self.assertEqual(lightOrFilter.GetShaderId([]), "")
            self.assertEqual(lightOrFilter.GetShaderId(["other", "ri"]), "")

            # Set a value in the default shaderID attr.
            lightOrFilter.GetShaderIdAttr().Set("DefaultLight")
            # No new attributes were created.
            self.assertEqual(
                lightOrFilter.GetShaderIdAttrForRenderContext("").GetName(),
                shaderIdAttrName)
            self.assertFalse(
                lightOrFilter.GetShaderIdAttrForRenderContext("ri"))
            self.assertFalse(
                lightOrFilter.GetShaderIdAttrForRenderContext("other"))

            # The default value is now the shaderID returned for all render 
            # contexts since no render contexts define their own shader ID
            self.assertEqual(
                lightOrFilter.GetShaderId([]), "DefaultLight")
            self.assertEqual(
                lightOrFilter.GetShaderId(["other", "ri"]), "DefaultLight")

            # Create a shaderID attr for the "ri" render context with a new ID 
            # value.
            lightOrFilter.CreateShaderIdAttrForRenderContext("ri", "SphereLight")
            # The shaderId attr for "ri" now exists
            self.assertEqual(
                lightOrFilter.GetShaderIdAttrForRenderContext("").GetName(),
                shaderIdAttrName)
            self.assertEqual(
                lightOrFilter.GetShaderIdAttrForRenderContext("ri").GetName(),
                "ri:" + shaderIdAttrName)
            self.assertFalse(
                lightOrFilter.GetShaderIdAttrForRenderContext("other"))

            # When passed no render contexts we still return the default 
            # shader ID.
            self.assertEqual(lightOrFilter.GetShaderId([]), "DefaultLight")
            # Since we defined a shader ID for "ri" but not "other", the "ri" 
            # shader ID is returned when queryring for both. Querying for just 
            # "other" falls back to the default shaderID
            self.assertEqual(
                lightOrFilter.GetShaderId(["other", "ri"]), "SphereLight")
            self.assertEqual(
                lightOrFilter.GetShaderId(["ri"]), "SphereLight")
            self.assertEqual(
                lightOrFilter.GetShaderId(["other"]), "DefaultLight")

        # Create an untyped prim with a LightAPI applied and test the ShaderId
        # functions of UsdLux.LightAPI
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/PrimLight")
        light = UsdLux.LightAPI.Apply(prim)
        self.assertTrue(light)
        _TestShaderIDs(light, "light:shaderId")

        # Create a LightFilter prim  and test the ShaderId functions of 
        # UsdLux.LightFilter
        lightFilter = UsdLux.LightFilter.Define(stage, "/PrimLightFilter")
        self.assertTrue(lightFilter)
        _TestShaderIDs(lightFilter, "lightFilter:shaderId")

    def test_LightExtentAndBBox(self):
        # Test extent and bbox computations for the boundable lights.

        time = Usd.TimeCode.Default()

        # Helper for computing the extent and bounding boxes for a light and
        # comparing against an expect extent pair.
        def _VerifyExtentAndBBox(light, expectedExtent):
            self.assertEqual(
                UsdGeom.Boundable.ComputeExtentFromPlugins(light, time),
                expectedExtent)
            self.assertEqual(
                light.ComputeLocalBound(time, "default"),
                Gf.BBox3d(
                    Gf.Range3d(
                        Gf.Vec3d(expectedExtent[0]), 
                        Gf.Vec3d(expectedExtent[1])), 
                    Gf.Matrix4d(1.0)))

        # Create a prim of each boundable light type.
        stage = Usd.Stage.CreateInMemory()
        rectLight = UsdLux.RectLight.Define(stage, "/RectLight")
        self.assertTrue(rectLight)
        diskLight = UsdLux.DiskLight.Define(stage, "/DiskLight")
        self.assertTrue(diskLight)
        cylLight = UsdLux.CylinderLight.Define(stage, "/CylLight")
        self.assertTrue(cylLight)
        sphereLight = UsdLux.SphereLight.Define(stage, "/SphereLight")
        self.assertTrue(sphereLight)
        portalLight = UsdLux.PortalLight.Define(stage, "/PortalLight")
        self.assertTrue(portalLight)

        # Verify the extent and bbox computations for each light given its
        # fallback attribute values.
        _VerifyExtentAndBBox(rectLight, [(-0.5, -0.5, 0.0), (0.5, 0.5, 0.0)])
        _VerifyExtentAndBBox(diskLight, [(-0.5, -0.5, 0.0), (0.5, 0.5, 0.0)])
        _VerifyExtentAndBBox(cylLight, [(-0.5, -0.5, -0.5), (0.5, 0.5, 0.5)])
        _VerifyExtentAndBBox(sphereLight, [(-0.5, -0.5, -0.5), (0.5, 0.5, 0.5)])
        _VerifyExtentAndBBox(portalLight, [(-0.5, -0.5, 0.0), (0.5, 0.5, 0.0)])

        # Change the size related attribute of each light and verify the extents
        # and bounding boxes are updated.
        rectLight.CreateWidthAttr(4.0)
        rectLight.CreateHeightAttr(6.0)
        _VerifyExtentAndBBox(rectLight, [(-2.0, -3.0, 0.0), (2.0, 3.0, 0.0)])

        diskLight.CreateRadiusAttr(5.0)
        _VerifyExtentAndBBox(diskLight, [(-5.0, -5.0, 0.0), (5.0, 5.0, 0.0)])

        cylLight.CreateRadiusAttr(4.0)
        cylLight.CreateLengthAttr(10.0)
        _VerifyExtentAndBBox(cylLight, [(-4.0, -4.0, -5.0), (4.0, 4.0, 5.0)])

        sphereLight.CreateRadiusAttr(3.0)
        _VerifyExtentAndBBox(sphereLight, [(-3.0, -3.0, -3.0), (3.0, 3.0, 3.0)])

        portalLight.CreateWidthAttr(4.0)
        portalLight.CreateHeightAttr(6.0)
        _VerifyExtentAndBBox(portalLight, [(-2.0, -3.0, 0.0), (2.0, 3.0, 0.0)])

        # For completeness verify that distant and dome lights are not 
        # boundable.
        domeLight = UsdLux.DomeLight.Define(stage, "/DomeLight")
        self.assertTrue(domeLight)
        self.assertFalse(UsdGeom.Boundable(domeLight))
        domeLight_1 = UsdLux.DomeLight.Define(stage, "/DomeLight_1")
        self.assertTrue(domeLight_1)
        self.assertFalse(UsdGeom.Boundable(domeLight_1))
        distLight = UsdLux.DistantLight.Define(stage, "/DistLight")
        self.assertTrue(distLight)
        self.assertFalse(UsdGeom.Boundable(distLight))

    def test_SdrShaderNodesForLights(self):
        """
        Test the automatic registration of SdrShaderNodes for all the UsdLux
        light types.
        """

        # The expected shader node inputs that should be found for all of our
        # UsdLux light types.
        expectedLightInputNames = [
            # LightAPI
            'color', 
            'colorTemperature', 
            'diffuse', 
            'enableColorTemperature', 
            'exposure', 
            'intensity', 
            'normalize', 
            'specular',

            # ShadowAPI
            'shadow:color',
            'shadow:distance',
            'shadow:enable',
            'shadow:falloff',
            'shadow:falloffGamma',

            # ShapingAPI
            'shaping:cone:angle',
            'shaping:cone:softness',
            'shaping:focus',
            'shaping:focusTint',
            'shaping:ies:angleScale',
            'shaping:ies:file',
            'shaping:ies:normalize'
            ]

        # Map of the names of the expected light nodes to the additional inputs
        # we expect for those types.
        expectedLightNodes = {
            'CylinderLight' : ['length', 'radius'],
            'DiskLight' : ['radius'],
            'DistantLight' : ['angle'],
            'DomeLight' : ['texture:file', 'texture:format'],
            'GeometryLight' : [],
            'PortalLight' : ['width', 'height'],
            'RectLight' : ['width', 'height', 'texture:file'],
            'SphereLight' : ['radius'],
            'MeshLight' : [],
            'VolumeLight' : []
            }

        expectedLightTypes = [
            'CylinderLight',
            'DiskLight',
            'DistantLight',
            'DomeLight',
            'DomeLight_1',
            'GeometryLight',
            'PortalLight',
            'RectLight',
            'SphereLight',
            'MeshLight',
            'VolumeLight'
            ]

        # Get all the derived types of UsdLuxBoundableLightBase and 
        # UsdLuxNonboundableLightBase that are defined in UsdLux
        lightTypes = list(filter(
            Plug.Registry().GetPluginWithName("usdLux").DeclaresType,
            Tf.Type(UsdLux.BoundableLightBase).GetAllDerivedTypes() +
            Tf.Type(UsdLux.NonboundableLightBase).GetAllDerivedTypes()))
        self.assertTrue(lightTypes)

        # Augment lightTypes to include MeshLightAPI and VolumeLightAPI
        lightTypes.append(
            Tf.Type.FindByName('UsdLuxMeshLightAPI'))
        lightTypes.append(
            Tf.Type.FindByName('UsdLuxVolumeLightAPI'))

        # Verify that at least one known light type is in our list to guard
        # against this giving false positives if no light types are available.
        self.assertIn(UsdLux.RectLight, lightTypes)
        self.assertEqual(len(lightTypes), len(expectedLightTypes))

        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/Prim")

        usdSchemaReg = Usd.SchemaRegistry()
        for lightType in lightTypes:

            print("Test SdrNode for schema type " + str(lightType))
            
            if usdSchemaReg.IsAppliedAPISchema(lightType):
                prim.ApplyAPI(lightType)
            else:
                typeName = usdSchemaReg.GetConcreteSchemaTypeName(lightType)
                if not typeName:
                    continue
                prim.SetTypeName(typeName)
            light = UsdLux.LightAPI(prim)
            self.assertTrue(light)
            sdrIdentifier = light.GetShaderId([])
            self.assertTrue(sdrIdentifier)
            prim.ApplyAPI(UsdLux.ShadowAPI)
            prim.ApplyAPI(UsdLux.ShapingAPI)

            # Every concrete light type and some API schemas (with appropriate
            # shaderId as sdr Identifier) in usdLux domain will have an 
            # SdrShaderNode with source type 'USD' registered for it under its 
            # USD schema type name. 
            node = Sdr.Registry().GetNodeByIdentifier(sdrIdentifier, ['USD'])
            self.assertTrue(node is not None)
            self.assertIn(sdrIdentifier, expectedLightNodes)

            # Names, identifier, and role for the node all match the USD schema
            # type name
            self.assertEqual(node.GetIdentifier(), sdrIdentifier)
            self.assertEqual(node.GetName(), sdrIdentifier)
            self.assertEqual(node.GetImplementationName(), sdrIdentifier)
            self.assertEqual(node.GetRole(), sdrIdentifier)
            self.assertTrue(node.GetInfoString().startswith(sdrIdentifier))

            # The context is always 'light' for lights. 
            # Source type is 'USD'
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

            # The node will be valid for our light types.
            self.assertTrue(node.IsValid())

            # Helper for comparing an SdrShaderProperty from node to the 
            # corresponding UsdShadeInput/UsdShadeOutput from a UsdLux light
            def _CompareLightPropToNodeProp(nodeInput, primInput):
                # Input names and default values match.
                primDefaultValue = primInput.GetAttr().Get()
                self.assertEqual(nodeInput.GetName(), primInput.GetBaseName())
                self.assertEqual(nodeInput.GetDefaultValue(), primDefaultValue)

                # Some USD property types don't match exactly one to one and are
                # converted to different types. In particular relevance to 
                # lights and Token becomes String.
                expectedTypeName = primInput.GetTypeName()
                # Array valued attributes have their array size determined from
                # the default value and will be converted to scalar in the 
                # SdrProperty if the array size is zero.
                if expectedTypeName.isArray:
                    if not primDefaultValue or len(primDefaultValue) == 0:
                        expectedTypeName = expectedTypeName.scalarType
                elif expectedTypeName == Sdf.ValueTypeNames.Token:
                    expectedTypeName = Sdf.ValueTypeNames.String 
                # Bool SdfTypes should Have Int SdrTypes, but still return as
                # Bool when queried for GetTypeAsSdfType
                if expectedTypeName == Sdf.ValueTypeNames.Bool:
                    self.assertEqual(nodeInput.GetType(),
                            Sdf.ValueTypeNames.Int)
                # Verify the node's input type maps back to USD property's type
                # (with the noted above exceptions).
                self.assertEqual(
                    nodeInput.GetTypeAsSdfType()[0], expectedTypeName,
                    msg="{}.{} Type {} != {}".format(
                        str(node.GetName()),
                        str(nodeInput.GetName()),
                        str(nodeInput.GetTypeAsSdfType()[0]),
                        str(expectedTypeName)))
                # If the USD property type is an Asset, it will be listed in 
                # the node's asset identifier inputs.
                if expectedTypeName == Sdf.ValueTypeNames.Asset:
                    self.assertIn(nodeInput.GetName(), 
                                  node.GetAssetIdentifierInputNames())

            # There will be a one to one correspondence between node inputs
            # and prim inputs. Note that the prim may have additional inputs
            # because of auto applied API schemas, but we only need to verify
            # that the node has ONLY the expected inputs and the prim at least
            # has those input proerties.
            expectedInputNames = \
                expectedLightInputNames + expectedLightNodes[sdrIdentifier]
            # Verify node has exactly the expected inputs.
            self.assertEqual(sorted(expectedInputNames),
                             sorted(node.GetInputNames()))
            # Verify each node input matches a prim input.
            for inputName in expectedInputNames:
                nodeInput = node.GetInput(inputName)
                primInput = light.GetInput(inputName)
                self.assertFalse(nodeInput.IsOutput())
                _CompareLightPropToNodeProp(nodeInput, primInput)

            # None of the UsdLux base lights have outputs
            self.assertEqual(node.GetOutputNames(), [])
            self.assertEqual(light.GetOutputs(onlyAuthored=False), [])

            # The reverse is tested just above, but for all asset identifier
            # inputs listed for the node there is a corresponding asset value
            # input property on the prim.
            for inputName in node.GetAssetIdentifierInputNames():
                self.assertEqual(light.GetInput(inputName).GetTypeName(),
                                 Sdf.ValueTypeNames.Asset)

            # These primvars come from sdrMetadata on the prim itself which
            # isn't supported for light schemas so it will always be empty.
            self.assertFalse(node.GetPrimvars())
            # sdrMetadata on input properties is supported so additional 
            # primvar properties will correspond to prim inputs with that 
            # metadata set.
            for propName in node.GetAdditionalPrimvarProperties():
                self.assertTrue(light.GetInput(propName).GetSdrMetadataByKey(
                    'primvarProperty'))

            # Default input can also be specified in the property's sdrMetadata.
            if node.GetDefaultInput():
                defaultInput = light.GetInput(
                    node.GetDefaultInput().GetName())
                self.assertTrue(defaultInput.GetSdrMetadataByKey('defaultInput'))


if __name__ == '__main__':
    unittest.main()
