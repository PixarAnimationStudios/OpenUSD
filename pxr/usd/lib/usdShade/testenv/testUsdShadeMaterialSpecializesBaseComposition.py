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

class TestUsdShadeMaterialSpecializesBaseComposition(unittest.TestCase):

    def _GetMaterial(self, stage, path):
        material = UsdShade.Material(stage.GetPrimAtPath(path))
        self.assertTrue(material)
        return material

    def test_BasicSetup(self):
        '''
        Test that we can get base materials at all.
        '''
        stage = Usd.Stage.Open('library.usda')
        self.assertTrue(stage)

        child = self._GetMaterial(stage, '/ChildMaterial')
        base = self._GetMaterial(stage, '/BaseMaterial')
        self.assertFalse(base.GetBaseMaterial())
        self.assertEqual(child.GetBaseMaterial().GetPath(), base.GetPath())

    def test_SpecializesNotPresentIsIgnored(self):
        '''
        Test that we don't think a material is a child if its parent doesn't
        exist on the stage.
        '''
        stage = Usd.Stage.Open('asset.usda')
        self.assertTrue(stage)

        mat = self._GetMaterial(stage, '/Asset/Looks/NotAChildMaterial')
        self.assertFalse(mat.GetBaseMaterial())
        self.assertFalse(mat.HasBaseMaterial())

    def test_ParentIsAcrossReferenceArc(self):
        '''
        Test that we can get base materials when the parent and child are
        across a reference arc.
        '''
        stage = Usd.Stage.Open('set_with_specialized_materials.usda')
        self.assertTrue(stage)

        looksScope = '/Shot/Set/SetChild/ReferencedAsset/Looks/'
        child = self._GetMaterial(stage, looksScope + 'ChildMaterial')
        base = self._GetMaterial(stage, looksScope + 'BaseMaterial')
        self.assertFalse(base.GetBaseMaterial())
        self.assertEqual(child.GetBaseMaterial().GetPath(), base.GetPath())

    def test_MultipleReferencedParents(self):
        '''
        Make sure we get a parent with multiple references in our parents.
        '''
        stage = Usd.Stage.Open('multiple_referenced_parents.usda')
        self.assertTrue(stage)

        basePath = '/Shot/Set/SetChild/ReferencedAsset/Looks/'
        child = self._GetMaterial(stage, basePath + 'ChildMaterial')
        base = self._GetMaterial(stage, basePath + 'BaseMaterial')
        self.assertFalse(base.GetBaseMaterial())
        self.assertEqual(child.GetBaseMaterial().GetPath(), base.GetPath())

    def test_ParentMaterialConcealedBehindNonMaterial(self):
        '''
        Test that we find the right specializes arc when some are not
        materials.
        '''
        stage = Usd.Stage.Open('concealed_parent_material.usda')
        self.assertTrue(stage)

        child = self._GetMaterial(stage, '/Shot/Looks/InterestingChild')
        base = self._GetMaterial(stage, '/Shot/Looks/Base')
        self.assertFalse(base.GetBaseMaterial())
        self.assertEqual(child.GetBaseMaterial().GetPath(), base.GetPath())


if __name__ == "__main__":
    unittest.main()
