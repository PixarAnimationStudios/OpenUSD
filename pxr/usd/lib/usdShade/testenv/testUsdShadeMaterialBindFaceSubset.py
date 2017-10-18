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

from pxr import Usd, UsdGeom, UsdShade, Vt
import unittest

class TestUsdShadeMaterialBindFaceSubset(unittest.TestCase):
    def test_Basic(self):
        usdFilePath = "Sphere.usda"

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        sphere = stage.GetPrimAtPath('/Sphere/Mesh')
        mat1 = stage.GetPrimAtPath('/Sphere/Materials/initialShadingGroup')
        mat2= stage.GetPrimAtPath('/Sphere/Materials/lambert2SG')
        mat3= stage.GetPrimAtPath('/Sphere/Materials/lambert3SG')

        self.assertTrue(sphere and mat1 and mat2 and mat3)

        # Verify that the sphere mesh does not have an existing material face-set.
        geomSphere = UsdGeom.Imageable(sphere)
        materialBindSubsets = UsdShade.Material.GetMaterialBindSubsets(
            geomSphere)
        self.assertEqual(len(materialBindSubsets), 0)

        faceIndices1 = Vt.IntArray((0, 1, 2, 3))
        faceIndices2 = Vt.IntArray((4, 5, 6, 7, 8, 9, 10, 11))
        faceIndices3 = Vt.IntArray((12, 13, 14, 15))

        # Create a new family of subsets with familyName="materialBind" .
        subset1 = UsdShade.Material.CreateMaterialBindSubset(geomSphere, 
            'subset1', faceIndices1, UsdGeom.Tokens.face)
        # Default elementType is 'face'
        subset2 = UsdShade.Material.CreateMaterialBindSubset(geomSphere, 
            'subset2', faceIndices2)
        self.assertEqual(subset2.GetElementTypeAttr().Get(), UsdGeom.Tokens.face)

        (valid, reason) = UsdGeom.Subset.ValidateFamily(geomSphere, 
                            UsdGeom.Tokens.face, 
                            UsdShade.Tokens.materialBind)
        self.assertTrue(valid)

        (valid, reason) = UsdGeom.Subset.ValidateSubsets(
                            [subset1, subset2], 
                            elementCount=16,
                            familyType=UsdGeom.Tokens.nonOverlapping)
        self.assertTrue(valid)

        # Not quite a partition yet.
        (valid, reason) = UsdGeom.Subset.ValidateSubsets(
                            [subset1, subset2], 
                            elementCount=16,
                            familyType=UsdGeom.Tokens.partition)
        self.assertFalse(valid)

        # Add a subset that makes the family a partition.
        subset3 = UsdShade.Material.CreateMaterialBindSubset(geomSphere, 
            'subset3', faceIndices3)
        (valid, reason) = UsdGeom.Subset.ValidateSubsets(
                            [subset1, subset2, subset3], 
                            elementCount=16,
                            familyType=UsdGeom.Tokens.partition)
        self.assertTrue(valid)

        self.assertEqual(
            UsdShade.Material.GetMaterialBindSubsetsFamilyType(geomSphere),
            UsdGeom.Tokens.nonOverlapping)

        UsdShade.Material.SetMaterialBindSubsetsFamilyType(geomSphere, 
            UsdGeom.Tokens.partition)

        (valid, reason) = UsdGeom.Subset.ValidateFamily(geomSphere, 
                            UsdGeom.Tokens.face, 
                            UsdShade.Tokens.materialBind)
        self.assertTrue(valid)
        
        UsdShade.Material(mat1).Bind(subset1.GetPrim())
        UsdShade.Material(mat2).Bind(subset2.GetPrim())
        UsdShade.Material(mat3).Bind(subset3.GetPrim())

        # Don't save the modified source stage. Export it into a 
        # new layer for baseline diffing.
        stage.Export("SphereWithMaterialBind.usda", addSourceFileComment=False)

if __name__ == "__main__":
    unittest.main()
