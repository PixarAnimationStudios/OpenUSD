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
        pale.GetPrim().GetInherits().AddInherit(classPalePath)
        shaderClass = UsdShade.Shader(classPale)
        # it's not valid because it's not defined, but we can still author using it
        self.assertTrue(not shaderClass)

        return stage

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

        # test set/get documentation.
        doc = "My shade input"
        usdShadeInput.SetDocumentation(doc)
        self.assertEqual(usdShadeInput.GetDocumentation(), doc)
        
        # test set/get dislayGroup
        displayGroup = "floats"
        usdShadeInput.SetDisplayGroup(displayGroup)
        self.assertEqual(usdShadeInput.GetDisplayGroup(), displayGroup)

        self.assertEqual(usdShadeInput.GetBaseName(), 'myFloatInput')
        self.assertEqual(usdShadeInput.GetTypeName(), Sdf.ValueTypeNames.Float)
        usdShadeInput.Set(1.0)
        self.assertTrue(not usdShadeInput.HasConnectedSource())
        
        usdShadeInput.ConnectToSource(whiterPale, 'Fout')
        self.assertTrue(usdShadeInput.HasConnectedSource())

        self.assertEqual(usdShadeInput.GetRawConnectedSourcePaths(),
                [whiterPale.GetPath().AppendProperty("outputs:Fout")])

        self.assertTrue(self._ConnectionsEqual(
                usdShadeInput.GetConnectedSource(),
                (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        usdShadeInput.ClearSource()
        self.assertTrue(not usdShadeInput.HasConnectedSource())
        self.assertEqual(usdShadeInput.GetConnectedSource(), None)

        # Now make the connection in the class
        inheritedInput = shaderClass.CreateInput('myFloatInput', 
                                                 Sdf.ValueTypeNames.Float)
        inheritedInput.ConnectToSource(whiterPale, 'Fout')
        # note we're now testing the inheritING prim's parameter
        self.assertTrue(usdShadeInput.HasConnectedSource())
        self.assertTrue(self._ConnectionsEqual(usdShadeInput.GetConnectedSource(),
                                      (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        # clearing no longer changes anything
        usdShadeInput.ClearSource()
        self.assertTrue(usdShadeInput.HasConnectedSource())
        self.assertTrue(self._ConnectionsEqual(usdShadeInput.GetConnectedSource(),
                                      (whiterPale, 'Fout', UsdShade.AttributeType.Output)))
        # but disconnecting should
        usdShadeInput.DisconnectSource()
        self.assertTrue(not usdShadeInput.HasConnectedSource())
        self.assertEqual(usdShadeInput.GetConnectedSource(), None)


        ################################
        print('Test asset id')
        ################################
        self.assertEqual(pale.GetImplementationSource(), UsdShade.Tokens.id)
        self.assertEqual(whiterPale.GetImplementationSource(), 
                         UsdShade.Tokens.id)

        self.assertTrue(pale.SetShaderId('SharedFloat_1'))
        self.assertEqual(pale.GetShaderId(), 'SharedFloat_1')

        self.assertTrue(whiterPale.CreateIdAttr('SharedColor_1'))
        self.assertEqual(whiterPale.GetIdAttr().Get(), 'SharedColor_1')

        self.assertTrue(pale.GetSourceAsset() is None)
        self.assertTrue(whiterPale.GetSourceCode() is None)

        # Test boundaries of parameter type-testing when connecting
        print "Test Typed Input Connections"

        colInput = pale.CreateInput("col1", Sdf.ValueTypeNames.Color3f)
        self.assertTrue(colInput)
        self.assertTrue(colInput.ConnectToSource(whiterPale, "colorOut"))
        outputAttr = whiterPale.GetPrim().GetAttribute("outputs:colorOut")
        self.assertTrue(outputAttr)
        self.assertEqual(outputAttr.GetTypeName(), Sdf.ValueTypeNames.Color3f)

        v3fInput = pale.CreateInput("v3f1", Sdf.ValueTypeNames.Float3)
        self.assertTrue(v3fInput)
        self.assertTrue(v3fInput.ConnectToSource(whiterPale, "colorOut"))

        pointInput = pale.CreateInput("point1", Sdf.ValueTypeNames.Point3f)
        self.assertTrue(pointInput)
        self.assertTrue(pointInput.ConnectToSource(whiterPale, "colorOut"))

        floatInput = pale.CreateInput("float1", Sdf.ValueTypeNames.Float)
        self.assertTrue(floatInput)
        # XXX The following test must be disabled until we re-enable strict
        # type-checking for input / output connections.  See bug/113600
        # can't connect float to color!
        #with RequiredException(Tf.ErrorException):
        #    floatInput.ConnectToSource(whiterPale, "colorOut")

        self.assertTrue(floatInput.ConnectToSource(whiterPale, "floatInput",
            sourceType=UsdShade.AttributeType.Input))
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

    def test_SdrMetadata(self):
        stage = self._SetupStage()

        ################################
        print ('Testing Shader Metadata API')
        ################################

        pale = UsdShade.Shader.Get(stage, palePath)
        self.assertTrue(pale)

        self.assertEqual(pale.GetSdrMetadata(), {})

        # Pale inherits from ClassPale.
        classPale = UsdShade.Shader.Get(stage, classPalePath)

        from pxr import Sdr
        baseSdrMetadata = {Sdr.NodeMetadata.Primvars : 
                                "primvarA|primvarB|primvarC"}
        classPale.SetSdrMetadata(baseSdrMetadata)

        self.assertEqual(pale.GetSdrMetadata(), baseSdrMetadata)
        paleSdrMetadata = {Sdr.NodeMetadata.Departments : "anim|layout",
                              Sdr.NodeMetadata.Category : "preview"}
        for i,j in paleSdrMetadata.iteritems():
            pale.SetSdrMetadataByKey(i, j)

        self.assertEqual(pale.GetSdrMetadata(), 
            {'category': 'preview', 
             'primvars': 'primvarA|primvarB|primvarC', 
             'departments': 'anim|layout'})

        pale.ClearSdrMetadataByKey(Sdr.NodeMetadata.Primvars)
        self.assertEqual(pale.GetSdrMetadata(), 
            {'category': 'preview', 
             'primvars': 'primvarA|primvarB|primvarC', 
             'departments': 'anim|layout'})

        classPale.ClearSdrMetadataByKey(Sdr.NodeMetadata.Primvars)
        self.assertEqual(pale.GetSdrMetadata(), paleSdrMetadata)

        pale.ClearSdrMetadata()
        self.assertEqual(pale.GetSdrMetadata(), {})

    def test_ImplementationSource(self):
        stage = self._SetupStage()

        ################################
        print ('Testing Implementation Source API')
        ################################

        pale = UsdShade.Shader.Get(stage, palePath)
        self.assertTrue(pale)

        whiterPale = UsdShade.Shader.Get(stage, whiterPalePath)
        self.assertTrue(whiterPale)
        
        self.assertEqual(pale.GetImplementationSource(), UsdShade.Tokens.id)
        self.assertEqual(whiterPale.GetImplementationSource(), 
                         UsdShade.Tokens.id)

        self.assertTrue(pale.SetShaderId('SharedFloat_1'))
        self.assertEqual(pale.GetShaderId(), 'SharedFloat_1')

        self.assertTrue(whiterPale.SetShaderId('SharedColor_1'))
        self.assertEqual(whiterPale.GetShaderId(), 'SharedColor_1')

        pale.GetImplementationSourceAttr().Set(UsdShade.Tokens.sourceAsset)
        self.assertTrue(pale.GetShaderId() is None)

        whiterPale.GetImplementationSourceAttr().Set(UsdShade.Tokens.sourceCode)
        self.assertTrue(whiterPale.GetShaderId() is None)
    
        glslfxSource = "This is the shader source"
        self.assertTrue(pale.SetSourceCode(sourceCode=glslfxSource, 
                                           sourceType="glslfx"))

        # Calling SetSourceCode() updates the implementationSource to 'code'.
        self.assertEqual(pale.GetImplementationSource(), 
                         UsdShade.Tokens.sourceCode)

        self.assertTrue(pale.GetShaderId() is None)

        self.assertTrue(pale.GetSourceAsset() is None)
        self.assertTrue(pale.GetSourceAsset(sourceType="glslfx") is None)

        self.assertTrue(pale.GetSourceCode(sourceType="osl") is None)
        self.assertTrue(pale.GetSourceCode() is None)

        self.assertEqual(pale.GetSourceCode(sourceType="glslfx"), glslfxSource)

        oslAssetPath = Sdf.AssetPath("/source/asset.osl")
        self.assertTrue(whiterPale.SetSourceAsset(
                sourceAsset=oslAssetPath, 
                sourceType=UsdShade.Tokens.universalSourceType))

        # Calling SetSourceAsset() updates the implementationSource to 'asset'.
        self.assertEqual(whiterPale.GetImplementationSource(), 
                         UsdShade.Tokens.sourceAsset)

        self.assertTrue(whiterPale.GetShaderId() is None)

        # Since the sourceAsset was set with universal sourceType, we can fetch 
        # it successfully irrespective of the sourceType that's passed in.
        self.assertEqual(whiterPale.GetSourceAsset(sourceType="osl"), 
                         oslAssetPath)
        self.assertTrue(whiterPale.GetSourceAsset(sourceType="glslfx"), 
                        oslAssetPath)
        self.assertEqual(whiterPale.GetSourceAsset(), oslAssetPath)

        self.assertTrue(whiterPale.GetSourceCode() is None)
        self.assertTrue(whiterPale.GetSourceCode(sourceType="osl") is None)

        # Set another sourceAsset corresponding to a specific sourceType.
        glslfxAssetPath = Sdf.AssetPath("/source/asset.glslfx")
        self.assertTrue(whiterPale.SetSourceAsset(
                sourceAsset=glslfxAssetPath, 
                sourceType="glslfx"))
        self.assertEqual(whiterPale.GetSourceAsset(sourceType="osl"), 
                         oslAssetPath)
        self.assertTrue(whiterPale.GetSourceAsset(sourceType="glslfx"), 
                        glslfxAssetPath)
        self.assertEqual(whiterPale.GetSourceAsset(), oslAssetPath)

if __name__ == '__main__':
    unittest.main()
