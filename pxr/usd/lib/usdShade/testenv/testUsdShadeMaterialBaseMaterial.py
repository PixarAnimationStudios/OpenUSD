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

from pxr import Sdf, Usd, UsdShade
import os, unittest

USE_SPECIALIZES = os.getenv('USD_USE_LEGACY_BASE_MATERIAL', '1') == '0'

class TestUsdShadeMaterialBaseMaterial(unittest.TestCase):
    def _SetupShading(self, stage):
        # Create this prim first, since it's the "entrypoint" to the layer, and
        # we want it to appear at the top
        rootPrim = stage.DefinePrim("/ModelShading")

        materialsPath = rootPrim.GetPath().AppendChild('Materials')
        return materialsPath


    def _TestShading(self, stage, materialsPath):

        ConnAPI = UsdShade.ConnectableAPI
        # Create parent material
        parentMaterialPath = materialsPath.AppendChild('ParentMaterial')
        parentMaterial = UsdShade.Material.Define(stage, parentMaterialPath)
        floatInterfaceInput = parentMaterial.CreateInput('floatVal', 
                Sdf.ValueTypeNames.Float)
        parentShader1 = UsdShade.Shader.Define(
            stage, parentMaterialPath.AppendChild("Shader_1"))
        floatShaderInput = parentShader1.CreateInput(
            'floatInput', Sdf.ValueTypeNames.Float)
        floatShaderInput.Set(1.0)

        parentShader2 = UsdShade.Shader.Define(
            stage, parentMaterialPath.AppendChild("Shader_2"))
        floatShaderOutput = parentShader2.CreateOutput('floatOutput', 
            Sdf.ValueTypeNames.Float)
        
        self.assertTrue(ConnAPI.ConnectToSource(floatShaderInput, 
                                                floatShaderOutput))

        print stage.GetRootLayer().ExportToString()
        print ConnAPI.GetConnectedSource(floatShaderInput)
        self.assertTrue(ConnAPI.HasConnectedSource(floatShaderInput))
        self.assertFalse(ConnAPI.IsSourceConnectionFromBaseMaterial(
                floatShaderInput))
        self.assertTrue(not parentMaterial.HasBaseMaterial())

        # Create child materials
        # one with SetBaseMaterial
        childMaterials = []
        childMaterialPath = materialsPath.AppendChild('ChildMaterial_1')
        childMaterial = UsdShade.Material.Define(stage, childMaterialPath)
        childMaterial.SetBaseMaterial(parentMaterial)
        childMaterials.append(childMaterial)

        # one with SetBaseMaterialPath
        childMaterialPath = materialsPath.AppendChild('ChildMaterial_2')
        childMaterial = UsdShade.Material.Define(stage, childMaterialPath)
        childMaterial.SetBaseMaterialPath(parentMaterialPath)
        childMaterials.append(childMaterial)

        # verify that material is found
        for childMaterial in childMaterials:
            self.assertTrue(childMaterial.HasBaseMaterial())
            self.assertEqual(childMaterial.GetBaseMaterialPath(), parentMaterialPath)
            childShaderPath = childMaterial.GetPrim().GetPath().AppendChild(
                "Shader_1")
            childShaderPrim = stage.GetPrimAtPath(childShaderPath)

            if USE_SPECIALIZES:
                # NEW encoding
                # verify that inheritance is found and resolved
                self.assertEqual(bool(childShaderPrim), True)
                childShader = UsdShade.Shader(childShaderPrim)
                self.assertTrue(childShader)
                childShaderInput = childShader.GetInput('floatInput')
                self.assertEqual(childShaderInput.GetAttr().Get(), 1.0)
                self.assertTrue(ConnAPI.IsSourceConnectionFromBaseMaterial(
                        childShaderInput))
            else:
                # OLD encoding
                # verify that child shader is not found by default 
                # (that's for the new encoding)
                self.assertEqual(bool(childShaderPrim), False)

    def test_Basic(self):
        if USE_SPECIALIZES:
            fileName = "test_base_material_specializes.usda"
        else:
            fileName = "test_base_material.usda"
        stage = Usd.Stage.CreateNew(fileName)
        materialsPath = self._SetupShading(stage)
        self._TestShading(stage, materialsPath)
        stage.GetRootLayer().Save()

if __name__ == "__main__":
    unittest.main()
