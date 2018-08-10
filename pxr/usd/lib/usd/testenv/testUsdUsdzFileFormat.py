#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

import unittest

from pxr import Tf, Usd, Sdf

class TestUsdzFileFormat(unittest.TestCase):
    def test_CannotCreateNewUsdzStage(self):
        """Test that a new .usdz layer cannot be created via the
        Usd.Stage API"""
        with self.assertRaises(Tf.ErrorException):
            stage = Usd.Stage.CreateNew("test.usdz")

        with self.assertRaises(Tf.ErrorException):
            stage = Usd.Stage.CreateInMemory("test.usdz")

    def test_CannotSaveToUsdzFile(self):
        """Test that changes cannot be saved to a .usdz layer"""
        # Users can make scene description changes to .usdz layers
        # and other layers within the .usdz file, but cannot save them.
        stage = Usd.Stage.Open("single_usd.usdz")
        self.assertTrue(stage.GetPrimAtPath("/Root_USD"))
        self.assertTrue(stage.DefinePrim("/Root_New"))
        with self.assertRaises(Tf.ErrorException):
            stage.Save()

    def test_FirstFileNotUsd(self):
        """Test that opening a .usdz file where the first file is not
        a loadable layer is an error."""
        with self.assertRaises(Tf.ErrorException):
            stage = Usd.Stage.Open("first_file_not_usd.usdz")

    def test_UsdzSublayers(self):
        """Test using .usdz files as sublayers"""
        stage = Usd.Stage.Open("sublayers.usda")
        self.assertEqual(
            stage.GetLayerStack(includeSessionLayers=False),
            [Sdf.Layer.Find(l) for l in [
                "sublayers.usda", 
                "single_usd.usdz", 
                "single_usda.usdz", 
                "single_usdc.usdz"]])

    def test_UsdzReferences(self):
        """Test using .usdz files in references"""
        stage = Usd.Stage.Open("refs.usda")
        self.assertEqual(
            stage.GetPrimAtPath("/Refs").GetPrimStack(),
            [Sdf.Layer.Find(l).GetPrimAtPath(p) for (l, p) in [
                ("refs.usda", "/Refs"),
                ("single_usd.usdz", "/Root_USD") ,
                ("single_usda.usdz", "/Root_USDA"), 
                ("single_usdc.usdz", "/Root_USDC")]])

    def test_RelativeReferences(self):
        """Test referencing behavior in layers in .usdz files."""
        def _TestExpectedLayer(stage, layerId):
            layer = Sdf.Layer.Find(layerId)
            self.assertTrue(layer)
            self.assertTrue(layer in stage.GetUsedLayers())

        # Verify that all expected layers are opened and used on the stage.

        # Test anchored relative references of the form "./<dir>/<layer>"
        stage = Usd.Stage.Open("anchored_refs.usdz")
        _TestExpectedLayer(stage, "anchored_refs.usdz")
        _TestExpectedLayer(stage, "anchored_refs.usdz[ref.usd]")
        _TestExpectedLayer(stage, "anchored_refs.usdz[sub/ref.usda]")
        _TestExpectedLayer(stage, "anchored_refs.usdz[sub/ref.usdc]")

        # Repeat the test, but for the case where the first layer in the
        # .usdz file is in a subdirectory to ensure relative paths are
        # anchored to the first layer in the .usdz file and not just the
        # root of the .usdz file.
        stage = Usd.Stage.Open("anchored_refs_sub.usdz")
        _TestExpectedLayer(stage, "anchored_refs_sub.usdz")
        _TestExpectedLayer(stage, "anchored_refs_sub.usdz[anchored_refs/ref.usd]")
        _TestExpectedLayer(stage, "anchored_refs_sub.usdz[anchored_refs/sub/ref.usda]")
        _TestExpectedLayer(stage, "anchored_refs_sub.usdz[anchored_refs/sub/ref.usdc]")

        # Repeat these tests, but with the .usdz file nested within another 
        # .usdz file.
        stage = Usd.Stage.Open("nested_anchored_refs.usdz")
        _TestExpectedLayer(stage, "nested_anchored_refs.usdz")
        _TestExpectedLayer(stage, "nested_anchored_refs.usdz[anchored_refs.usdz[ref.usd]]")
        _TestExpectedLayer(stage, "nested_anchored_refs.usdz[anchored_refs.usdz[sub/ref.usda]]")
        _TestExpectedLayer(stage, "nested_anchored_refs.usdz[anchored_refs.usdz[sub/ref.usdc]]")

        stage = Usd.Stage.Open("nested_anchored_refs_sub.usdz")
        _TestExpectedLayer(stage, "nested_anchored_refs_sub.usdz")
        _TestExpectedLayer(stage, "nested_anchored_refs_sub.usdz[anchored_refs_sub.usdz[anchored_refs/ref.usd]]")
        _TestExpectedLayer(stage, "nested_anchored_refs_sub.usdz[anchored_refs_sub.usdz[anchored_refs/sub/ref.usda]]")
        _TestExpectedLayer(stage, "nested_anchored_refs_sub.usdz[anchored_refs_sub.usdz[anchored_refs/sub/ref.usdc]]")

        # Test search relative references of the form "<dir>/<layer>".
        # These are expected to resolve relative to the layer they
        # are authored in, then relative to the root layer of the .usdz
        # file if not found.
        stage = Usd.Stage.Open("search_refs.usdz")
        _TestExpectedLayer(stage, "search_refs.usdz")
        _TestExpectedLayer(stage, "search_refs.usdz[refs/ref.usd]")
        _TestExpectedLayer(stage, "search_refs.usdz[refs/sub/ref_in_subdir.usd]")
        _TestExpectedLayer(stage, "search_refs.usdz[refs/sub/ref_in_both.usd]")
        _TestExpectedLayer(stage, "search_refs.usdz[sub/ref_in_root.usd]")

        # Repeat the test, but for the case where the first layer in the
        # .usdz file is in a subdirectory.
        stage = Usd.Stage.Open("search_refs_sub.usdz")
        _TestExpectedLayer(stage, "search_refs_sub.usdz")
        _TestExpectedLayer(stage, "search_refs_sub.usdz[search_refs/refs/ref.usd]")
        _TestExpectedLayer(stage, "search_refs_sub.usdz[search_refs/refs/sub/ref_in_subdir.usd]")
        _TestExpectedLayer(stage, "search_refs_sub.usdz[search_refs/refs/sub/ref_in_both.usd]")
        _TestExpectedLayer(stage, "search_refs_sub.usdz[search_refs/sub/ref_in_root.usd]")

        # Repeat these tests, but with the .usdz file nested within another 
        # .usdz file.
        stage = Usd.Stage.Open("nested_search_refs.usdz")
        _TestExpectedLayer(stage, "nested_search_refs.usdz")
        _TestExpectedLayer(stage, "nested_search_refs.usdz[search_refs.usdz[refs/ref.usd]]")
        _TestExpectedLayer(stage, "nested_search_refs.usdz[search_refs.usdz[refs/sub/ref_in_subdir.usd]]")
        _TestExpectedLayer(stage, "nested_search_refs.usdz[search_refs.usdz[refs/sub/ref_in_both.usd]]")
        _TestExpectedLayer(stage, "nested_search_refs.usdz[search_refs.usdz[sub/ref_in_root.usd]]")

        stage = Usd.Stage.Open("nested_search_refs_sub.usdz")
        _TestExpectedLayer(stage, "nested_search_refs_sub.usdz")
        _TestExpectedLayer(stage, "nested_search_refs_sub.usdz[search_refs_sub.usdz[search_refs/refs/ref.usd]]")
        _TestExpectedLayer(stage, "nested_search_refs_sub.usdz[search_refs_sub.usdz[search_refs/refs/sub/ref_in_subdir.usd]]")
        _TestExpectedLayer(stage, "nested_search_refs_sub.usdz[search_refs_sub.usdz[search_refs/refs/sub/ref_in_both.usd]]")
        _TestExpectedLayer(stage, "nested_search_refs_sub.usdz[search_refs_sub.usdz[search_refs/sub/ref_in_root.usd]]")
        
    def test_ComposedStage(self):
        """Test composing stages from .usdz files"""
        def _TestComposedStage(usdzFile, srcRootLayer):
            usdzStage = Usd.Stage.Open(usdzFile)
            srcStage = Usd.Stage.Open(srcRootLayer)
            self.assertEqual(
                srcStage.ExportToString(addSourceFileComment=False), 
                usdzStage.ExportToString(addSourceFileComment=False))
        
        # Test results of opening .usdz files on Usd.Stage by
        # comparing flattened stage to the result of opening and
        # flattening the original source layer(s).
        _TestComposedStage("single_usd.usdz", "single/test.usd")
        _TestComposedStage("single_usda.usdz", "single/test.usda")
        _TestComposedStage("single_usdc.usdz", "single/test.usdc")
        _TestComposedStage("anchored_refs.usdz", "anchored_refs/root.usd")
        _TestComposedStage("anchored_refs_sub.usdz", "anchored_refs/root.usd")
        _TestComposedStage("nested_anchored_refs.usdz", "anchored_refs/root.usd")
        _TestComposedStage("nested_anchored_refs_sub.usdz", "anchored_refs/root.usd")
        _TestComposedStage("search_refs.usdz", "search_refs/root.usd")
        _TestComposedStage("search_refs_sub.usdz", "search_refs/root.usd")
        _TestComposedStage("nested_search_refs.usdz", "search_refs/root.usd")
        _TestComposedStage("nested_search_refs_sub.usdz", "search_refs/root.usd")

        # Test ability to open specific layers in .usdz files.
        _TestComposedStage("anchored_refs.usdz[root.usd]", 
                           "anchored_refs/root.usd")
        _TestComposedStage("anchored_refs.usdz[sub/ref.usda]", 
                           "anchored_refs/sub/ref.usda")
        _TestComposedStage("nested_anchored_refs.usdz[anchored_refs.usdz]", 
                           "anchored_refs/root.usd")
        _TestComposedStage("nested_anchored_refs.usdz[anchored_refs.usdz[root.usd]]", 
                           "anchored_refs/root.usd")

if __name__ == "__main__":
    unittest.main()
