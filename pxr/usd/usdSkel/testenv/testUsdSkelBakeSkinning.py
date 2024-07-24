#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Gf, Usd, UsdSkel
import unittest


class TestUsdSkelBakeSkinning(unittest.TestCase):

    def test_DQSkinning(self):
        testFile = "dqs.usda"
        stage = Usd.Stage.Open(testFile)

        self.assertTrue(UsdSkel.BakeSkinning(stage.Traverse()))

        stage.GetRootLayer().Export("dqs.baked.usda")


    def test_DQSkinningWithInterval(self):
        testFile = "dqs.usda"
        stage = Usd.Stage.Open(testFile)

        self.assertTrue(UsdSkel.BakeSkinning(
            stage.Traverse(), Gf.Interval(1, 10)))

        stage.GetRootLayer().Export("dqs.bakedInterval.usda")


    def test_LinearBlendSkinning(self):
        testFile = "lbs.usda"
        stage = Usd.Stage.Open(testFile)

        self.assertTrue(UsdSkel.BakeSkinning(stage.Traverse()))

        stage.GetRootLayer().Export("lbs.baked.usda")


    def test_LinearBlendSkinningWithInterval(self):
        testFile = "lbs.usda"
        stage = Usd.Stage.Open(testFile)

        self.assertTrue(UsdSkel.BakeSkinning(
            stage.Traverse(), Gf.Interval(1, 10)))

        stage.GetRootLayer().Export("lbs.bakedInterval.usda")


    def test_BlendShapes(self):
        testFile = "blendshapes.usda"
        stage = Usd.Stage.Open(testFile)

        self.assertTrue(UsdSkel.BakeSkinning(stage.Traverse()))
        
        stage.GetRootLayer().Export("blendshapes.baked.usda")


    def test_BlendShapesWithNormals(self):
        testFile = "blendshapesWithNormals.usda"
        stage = Usd.Stage.Open(testFile)

        self.assertTrue(UsdSkel.BakeSkinning(stage.Traverse()))
        
        stage.GetRootLayer().Export("blendshapesWithNormals.baked.usda")


if __name__ == "__main__":
    unittest.main()
