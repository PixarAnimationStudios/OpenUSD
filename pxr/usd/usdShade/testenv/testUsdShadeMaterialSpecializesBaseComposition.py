#!/pxrpythonsubst                                                                   
#                                                                                   
# Copyright 2017 Pixar                                                              
#                                                                                   
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
