#!/pxrpythonsubst
#                                                                              
# Copyright 2017 Pixar                                                         
#                                                                              
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf, Usd, UsdShade, UsdGeom
import os, unittest

class TestUsdShadeMaterialBaseMaterial(unittest.TestCase):
    def _SetupShading(self, stage, rootPrimPath):
        # Create this prim first, since it's the "entrypoint" to the layer, and
        # we want it to appear at the top
        rootPrim = stage.DefinePrim(rootPrimPath)

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

        print(stage.GetRootLayer().ExportToString())
        print(ConnAPI.GetConnectedSources(floatShaderInput))
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

            # verify that inheritance is found and resolved
            self.assertEqual(bool(childShaderPrim), True)
            childShader = UsdShade.Shader(childShaderPrim)
            self.assertTrue(childShader)
            childShaderInput = childShader.GetInput('floatInput')
            self.assertEqual(childShaderInput.GetAttr().Get(), 1.0)
            self.assertTrue(ConnAPI.IsSourceConnectionFromBaseMaterial(
                    childShaderInput))

    def test_Basic(self):
        fileName = "test_base_material_specializes.usda"
        stage = Usd.Stage.CreateNew(fileName)
        rootPrimPath = Sdf.Path("/ModelShading")
        UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
        UsdGeom.SetStageMetersPerUnit(stage, UsdGeom.LinearUnits.centimeters)
        materialsPath = self._SetupShading(stage, rootPrimPath)
        stage.SetDefaultPrim(stage.GetPrimAtPath(rootPrimPath))
        
        self._TestShading(stage, materialsPath)
        stage.Save()

if __name__ == "__main__":
    unittest.main()
