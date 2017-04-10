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

class TestUsdShadeMaterialFaceSet(unittest.TestCase):
    def test_Basic(self):
        usdFilePath = "Sphere.usda"

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        sphere = stage.GetPrimAtPath('/Sphere/Mesh')
        look1 = stage.GetPrimAtPath('/Sphere/Materials/initialShadingGroup')
        look2 = stage.GetPrimAtPath('/Sphere/Materials/lambert2SG')
        look3 = stage.GetPrimAtPath('/Sphere/Materials/lambert3SG')

        self.assertTrue(sphere and look1 and look2 and look3)

        # Verify that the sphere mesh does not have an existing material face-set.
        self.assertFalse(UsdShade.Material.HasMaterialFaceSet(sphere))

        # GetMaterialFaceSet should return an invalid FaceSetAPI object.
        faceSet = UsdShade.Material.GetMaterialFaceSet(sphere)
        self.assertFalse(faceSet)

        # Create a new "material" face-set.
        faceSet = UsdShade.Material.CreateMaterialFaceSet(sphere)
        self.assertEqual(faceSet.GetFaceSetName(), "material")
        self.assertTrue(faceSet)
        self.assertTrue(faceSet.GetPrim() and faceSet.GetIsPartition())

        faceIndices1 = Vt.IntArray((0, 1, 2, 3))
        faceIndices2 = Vt.IntArray((4, 5, 6, 7, 8, 9, 10, 11))
        faceIndices3 = Vt.IntArray((12, 13, 14, 15))

        faceSet.AppendFaceGroup(faceIndices1, look1.GetPath())
        faceSet.AppendFaceGroup(faceIndices2, look2.GetPath())
        faceSet.AppendFaceGroup(faceIndices3, look3.GetPath())

        # Don't save the modified source stage. Export it into a 
        # new layer for baseline diffing.
        stage.Export("SphereWithFaceSets.usda", addSourceFileComment=False)

if __name__ == "__main__":
    unittest.main()
