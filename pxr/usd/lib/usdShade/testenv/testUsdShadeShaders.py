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

from pxr import Tf, Sdf, Usd, UsdGeom, UsdShade
import unittest

palePath = Sdf.Path("/Model/Materials/MaterialSharp/Pale")
whiterPalePath = Sdf.Path("/Model/Materials/MaterialSharp/WhiterPale")
classPalePath = Sdf.Path("/classPale")

class TestUsdShadeShaders(unittest.TestCase):
    def _ConnectionsEqual(self, a, b):
        return a[0].GetPrim() == b[0].GetPrim() and a[1] == b[1] and a[2] == b[2]

    def _SetupStage(self):
        stage = Usd.Stage.CreateInMemory()

        UsdGeom.Scope.Define(stage, "/Model")
        UsdGeom.Scope.Define(stage, "/Model/Materials")
        UsdShade.Material.Define(stage, "/Model/Materials/MaterialSharp")

        pale = UsdShade.Shader.Define(stage, palePath)
        self.assertTrue(pale)
        whiterPale = UsdShade.Shader.Define(stage, whiterPalePath)
        self.assertTrue(whiterPale)

        # Make a class for pale so we can test that disconnecting/blocking works
        classPale = stage.CreateClassPrim(classPalePath)
        self.assertTrue(classPale)
        pale.GetPrim().GetInherits().AppendInherit(classPalePath)
        shaderClass = UsdShade.Shader(classPale)
        # it's not valid because it's not defined, but we can still author using it
        self.assertTrue(not shaderClass)

        return stage

    def test_ParamConnections(self):
        stage = self._SetupStage()
        ################################
        print ('Test parameter connections')
        ################################

        pale = UsdShade.Shader.Get(stage, palePath)
        self.assertTrue(pale)

        whiterPale = UsdShade.Shader.Get(stage, whiterPalePath)
        self.assertTrue(whiterPale)
        
        shaderClass = UsdShade.Shader.Get(stage, classPalePath)

        print ('Test RenderType')
        chords = pale.CreateParameter("chords", Sdf.ValueTypeNames.String)
        self.assertTrue(chords)
        self.assertTrue(not chords.HasRenderType())
        self.assertEqual(chords.GetRenderType(), "")
        chords.SetRenderType("notes")
        self.assertTrue(chords.HasRenderType())
        self.assertEqual(chords.GetRenderType(), "notes")

        ################################
        print ('Test scalar connections')
        ################################

        usdShadeParam = pale.CreateParameter('myFloatParameter', 
                                            Sdf.ValueTypeNames.Float)
        self.assertEqual(usdShadeParam.GetName(), 'myFloatParameter')
        self.assertEqual(usdShadeParam.GetTypeName(), Sdf.ValueTypeNames.Float)
        usdShadeParam.Set(1.0)
        self.assertTrue(not usdShadeParam.IsConnected())
        usdShadeParam.ConnectToSource(whiterPale, 'Fout')
        self.assertTrue(usdShadeParam.IsConnected())
        self.assertTrue(self._ConnectionsEqual(usdShadeParam.GetConnectedSource(),
                        (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        usdShadeParam.ClearSource()
        self.assertFalse(usdShadeParam.IsConnected())
        self.assertEqual(usdShadeParam.GetConnectedSource(), None)

        # Now make the connection in the class
        inheritedParam = shaderClass.CreateParameter('myFloatParameter', 
                                                    Sdf.ValueTypeNames.Float)
        inheritedParam.ConnectToSource(whiterPale, 'Fout')
        # note we're now testing the inheritING prim's parameter
        self.assertTrue(usdShadeParam.IsConnected())
        self.assertTrue(self._ConnectionsEqual(usdShadeParam.GetConnectedSource(),
                          (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        # clearing no longer changes anything
        usdShadeParam.ClearSource()
        self.assertTrue(usdShadeParam.IsConnected())
        self.assertTrue(self._ConnectionsEqual(usdShadeParam.GetConnectedSource(),
                         (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        # but disconnecting should
        usdShadeParam.DisconnectSource()
        self.assertTrue(not usdShadeParam.IsConnected())
        self.assertEqual(usdShadeParam.GetConnectedSource(), None)


        ################################
        print('Test asset id')
        ################################
        pale.CreateIdAttr('SharedFloat_1')
        whiterPale.CreateIdAttr('SharedColor_1')
        self.assertEqual(pale.GetIdAttr().Get(), 'SharedFloat_1')
        self.assertEqual(whiterPale.GetIdAttr().Get(), 'SharedColor_1')

        # Test boundaries of parameter type-testing when connecting
        print "Test Typing Parameter Connections"

        colParam = pale.CreateParameter("col1", Sdf.ValueTypeNames.Color3f);
        self.assertTrue(colParam)
        self.assertTrue(colParam.ConnectToSource(whiterPale, "colorOut"))
        outputAttr = whiterPale.GetPrim().GetAttribute("outputs:colorOut")
        self.assertTrue(outputAttr)
        self.assertEqual(outputAttr.GetTypeName(), Sdf.ValueTypeNames.Color3f)

        v3fParam = pale.CreateParameter("v3f1", Sdf.ValueTypeNames.Float3)
        self.assertTrue(v3fParam)
        self.assertTrue(v3fParam.ConnectToSource(whiterPale, "colorOut"))

        pointParam = pale.CreateParameter("point1", Sdf.ValueTypeNames.Point3f)
        self.assertTrue(pointParam)
        self.assertTrue(pointParam.ConnectToSource(whiterPale, "colorOut"))

        floatParam = pale.CreateParameter("float1", Sdf.ValueTypeNames.Float)
        self.assertTrue(floatParam)
        # XXX The following test must be disabled until we re-enable strict
        # type-checking for parameter connections.  See bug/113600
        # can't connect float to color!
        #with RequiredException(Tf.ErrorException):
        #    floatParam.ConnectToSource(whiterPale, "colorOut")

        self.assertTrue(floatParam.ConnectToSource(whiterPale, "floatParam", 
            sourceType=UsdShade.AttributeType.Parameter))
        outputAttr = whiterPale.GetPrim().GetAttribute("outputs:floatParam")
        self.assertTrue(not outputAttr)
        outputAttr = whiterPale.GetPrim().GetAttribute("floatParam")
        self.assertTrue(outputAttr)

        print "Test Parameter Fetching"
        # test against single param fetches
        vecParam = pale.CreateParameter('vec', Sdf.ValueTypeNames.Color3f)
        self.assertTrue(vecParam)
        self.assertTrue(pale.GetParameter('vec'))
        self.assertEqual(pale.GetParameter('vec').GetName(), 'vec')
        self.assertEqual(pale.GetParameter('vec').GetTypeName(), Sdf.ValueTypeNames.Color3f)
        self.assertTrue(pale.GetParameter('vec').SetRenderType('foo'))
        self.assertEqual(pale.GetParameter('vec').GetRenderType(), 'foo')

        # test against multiple params
        params = pale.GetParameters()

        # assure new item in collection
        self.assertEqual(len([pr for pr in params if pr.GetRenderType() == 'foo']), 1)

        # ensure the param count increments properly
        oldlen = len(pale.GetParameters())
        newparam = pale.CreateParameter('struct', Sdf.ValueTypeNames.Color3f)
        
        # assure new item in collection
        self.assertEqual(len(pale.GetParameters()), (oldlen+1))

        # ensure by-value capture in 'params'
        self.assertNotEqual(len(pale.GetParameters()), len(params))

        # Ensure that backwards compatibility code works properly.
        self.assertEqual(len(pale.GetInputs()), (oldlen+1))

    def test_InputOutputConnections(self):
        stage = self._SetupStage()

        ################################
        print ('Test Input/Output connections')
        ################################

        pale = UsdShade.Shader.Get(stage, palePath)
        self.assertTrue(pale)

        whiterPale = UsdShade.Shader.Get(stage, whiterPalePath)
        self.assertTrue(whiterPale)
        
        shaderClass = UsdShade.Shader.Get(stage, classPalePath)

        print ('Test RenderType')
        chords = pale.CreateInput("chords", Sdf.ValueTypeNames.String)
        self.assertTrue(chords)
        self.assertTrue(not chords.HasRenderType())
        self.assertEqual(chords.GetRenderType(), "")
        chords.SetRenderType("notes")
        self.assertTrue(chords.HasRenderType())
        self.assertEqual(chords.GetRenderType(), "notes")

        ################################
        print ('Test scalar connections')
        ################################

        usdShadeInput = pale.CreateInput('myFloatInput', Sdf.ValueTypeNames.Float)
        self.assertEqual(usdShadeInput.GetBaseName(), 'myFloatInput')
        self.assertEqual(usdShadeInput.GetTypeName(), Sdf.ValueTypeNames.Float)
        usdShadeInput.Set(1.0)
        self.assertTrue(not UsdShade.ConnectableAPI.HasConnectedSource(usdShadeInput))
        
        UsdShade.ConnectableAPI.ConnectToSource(usdShadeInput, whiterPale, 'Fout')
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectedSource(usdShadeInput))

        self.assertEqual(
                UsdShade.ConnectableAPI.GetRawConnectedSourcePaths(usdShadeInput),
                [whiterPale.GetPath().AppendProperty("outputs:Fout")])

        self.assertTrue(self._ConnectionsEqual(
                UsdShade.ConnectableAPI.GetConnectedSource(usdShadeInput),
                (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        UsdShade.ConnectableAPI.ClearSource(usdShadeInput)
        self.assertTrue(not UsdShade.ConnectableAPI.HasConnectedSource(usdShadeInput))
        self.assertEqual(UsdShade.ConnectableAPI.GetConnectedSource(usdShadeInput), None)

        # Now make the connection in the class
        inheritedInput = shaderClass.CreateInput('myFloatInput', 
                                                 Sdf.ValueTypeNames.Float)
        print inheritedInput.GetAttr()
        UsdShade.ConnectableAPI.ConnectToSource(inheritedInput, whiterPale, 'Fout')
        # note we're now testing the inheritING prim's parameter
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectedSource(usdShadeInput))
        self.assertTrue(self._ConnectionsEqual(UsdShade.ConnectableAPI.GetConnectedSource(
                                       usdShadeInput),
                                      (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        # clearing no longer changes anything
        UsdShade.ConnectableAPI.ClearSource(usdShadeInput)
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectedSource(usdShadeInput))
        self.assertTrue(self._ConnectionsEqual(UsdShade.ConnectableAPI.GetConnectedSource(
                                      usdShadeInput),
                                      (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        # but disconnecting should
        UsdShade.ConnectableAPI.DisconnectSource(usdShadeInput)
        self.assertTrue(not UsdShade.ConnectableAPI.HasConnectedSource(usdShadeInput))
        self.assertEqual(UsdShade.ConnectableAPI.GetConnectedSource(usdShadeInput), None)


        ################################
        print('Test asset id')
        ################################
        pale.CreateIdAttr('SharedFloat_1')
        whiterPale.CreateIdAttr('SharedColor_1')
        self.assertEqual(pale.GetIdAttr().Get(), 'SharedFloat_1')
        self.assertEqual(whiterPale.GetIdAttr().Get(), 'SharedColor_1')

        # Test boundaries of parameter type-testing when connecting
        print "Test Typed Input Connections"

        colInput = pale.CreateInput("col1", Sdf.ValueTypeNames.Color3f);
        self.assertTrue(colInput)
        self.assertTrue(UsdShade.ConnectableAPI.ConnectToSource(colInput, whiterPale, 
                                                       "colorOut"))
        outputAttr = whiterPale.GetPrim().GetAttribute("outputs:colorOut")
        self.assertTrue(outputAttr)
        self.assertEqual(outputAttr.GetTypeName(), Sdf.ValueTypeNames.Color3f)

        v3fInput = pale.CreateInput("v3f1", Sdf.ValueTypeNames.Float3)
        self.assertTrue(v3fInput)
        self.assertTrue(UsdShade.ConnectableAPI.ConnectToSource(v3fInput, whiterPale, 
                                                       "colorOut"))

        pointInput = pale.CreateInput("point1", Sdf.ValueTypeNames.Point3f)
        self.assertTrue(pointInput)
        self.assertTrue(UsdShade.ConnectableAPI.ConnectToSource(pointInput, whiterPale, 
                                                       "colorOut"))

        floatInput = pale.CreateInput("float1", Sdf.ValueTypeNames.Float)
        self.assertTrue(floatInput)
        # XXX The following test must be disabled until we re-enable strict
        # type-checking for input / output connections.  See bug/113600
        # can't connect float to color!
        #with RequiredException(Tf.ErrorException):
        #    floatInput.ConnectToSource(whiterPale, "colorOut")

        self.assertTrue(UsdShade.ConnectableAPI.ConnectToSource(floatInput, whiterPale, 
            "floatInput", sourceType=UsdShade.AttributeType.Input))
        outputAttr = whiterPale.GetPrim().GetAttribute("outputs:floatInput")
        self.assertFalse(outputAttr)
        outputAttr = whiterPale.GetPrim().GetAttribute("inputs:floatInput")
        self.assertTrue(outputAttr)

        print "Test Input Fetching"
        # test against single input fetches
        vecInput = pale.CreateInput('vec', Sdf.ValueTypeNames.Color3f)
        self.assertTrue(vecInput)
        self.assertTrue(pale.GetInput('vec'))
        self.assertEqual(pale.GetInput('vec').GetBaseName(), 'vec')
        self.assertEqual(pale.GetInput('vec').GetTypeName(), Sdf.ValueTypeNames.Color3f)
        self.assertTrue(pale.GetInput('vec').SetRenderType('foo'))
        self.assertEqual(pale.GetInput('vec').GetRenderType(), 'foo')

        # test against multiple input parameters.
        inputs = pale.GetInputs()

        # assure new item in collection
        self.assertEqual(len([pr for pr in inputs if pr.GetRenderType() == 'foo']),  1)

        # ensure the input count increments properly
        oldlen = len(pale.GetInputs())
        newparam = pale.CreateInput('struct', Sdf.ValueTypeNames.Color3f)

        # assure new item in collection
        self.assertEqual(len(pale.GetInputs()), (oldlen+1))

        # ensure by-value capture in 'inputs'
        self.assertNotEqual(len(pale.GetInputs()), len(inputs))

if __name__ == '__main__':
    unittest.main()
