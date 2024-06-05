#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, UsdGeom, UsdMedia
import unittest

class TestUsdMediaAssetPreviews(unittest.TestCase):

    def test_DefaultThumbnails(self):
        # BUild up a layer on a stage, testing negative cases as we go
        # until we get something good
        layer = Sdf.Layer.CreateAnonymous()
        stage = Usd.Stage.Open(layer)
        xform = UsdGeom.Xform.Define(stage, '/Model')

        thumbnails = UsdMedia.AssetPreviewsAPI.\
                     Thumbnails(defaultImage = Sdf.AssetPath("foo.jpg"))

        apAPI = UsdMedia.AssetPreviewsAPI(xform)
        self.assertFalse(apAPI.GetDefaultThumbnails())

        apAPI.SetDefaultThumbnails(thumbnails)
        # Still should fail because schema has not been applied
        self.assertFalse(apAPI)
        self.assertFalse(apAPI.GetDefaultThumbnails())

        apAPI = UsdMedia.AssetPreviewsAPI.Apply(xform.GetPrim())
        self.assertTrue(apAPI)
        retrievedThumbnails = apAPI.GetDefaultThumbnails()
        self.assertIsNotNone(retrievedThumbnails)
        self.assertEqual(thumbnails.defaultImage, 
                         retrievedThumbnails.defaultImage)
        
        # no defaultPrim metadata, so this should fail
        self.assertFalse(UsdMedia.AssetPreviewsAPI.GetAssetDefaultPreviews(
            layer))

        stage.SetDefaultPrim(xform.GetPrim())
        apAPI1 = UsdMedia.AssetPreviewsAPI.GetAssetDefaultPreviews(layer)
        self.assertTrue(apAPI1)
        retrievedThumbnails1 = apAPI1.GetDefaultThumbnails()
        self.assertIsNotNone(retrievedThumbnails1)
        self.assertEqual(thumbnails.defaultImage, 
                         retrievedThumbnails1.defaultImage)
        
        # Finally, write out the layer, and repeat the last part from
        # string asset-path rather than layer
        fn = "assetPreviews.usda"
        layer.Export(fn)
        apAPI2 = UsdMedia.AssetPreviewsAPI.GetAssetDefaultPreviews(fn)
        self.assertTrue(apAPI2)
        retrievedThumbnails2 = apAPI2.GetDefaultThumbnails()
        self.assertIsNotNone(retrievedThumbnails2)
        self.assertEqual(thumbnails.defaultImage, 
                         retrievedThumbnails2.defaultImage)
        # test clearing, and that when there is no data, we should get None
        apAPI2.ClearDefaultThumbnails()
        self.assertIsNone(apAPI2.GetDefaultThumbnails())
        


if __name__ == '__main__':
    unittest.main()
