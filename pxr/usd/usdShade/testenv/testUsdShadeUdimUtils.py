#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import UsdShade, Sdf
import os
import unittest


class TestUsdShadeUdimUtils(unittest.TestCase):
    def test_IsUdimIdentifier(self):
        """Basic test for UsdShade.UdimUtils.IsUdimIdentifier"""
        def _test(layer, propPath, value):
            attr = layer.GetPropertyAtPath(propPath)
            self.assertEqual(
                UsdShade.UdimUtils.IsUdimIdentifier(attr.default.path),
                value,
            )

        layer = Sdf.Layer.FindOrOpen("./layer.usda")
        _test(layer, "/foo.udim_style_a", True)
        _test(layer, "/foo.udim_style_b", True)
        _test(layer, "/foo.udim_missing", True)
        _test(layer, "/foo.not_udim", False)

    def test_ResolveUdimTilePaths(self):
        """Tests resolution of Udim filename, tile"""
        def _test(layer, propPath, expected):
            attr = layer.GetPropertyAtPath(propPath)
            actual = UsdShade.UdimUtils.ResolveUdimTilePaths(
                attr.default.path, layer)

            self.assertEqual(len(actual), len(expected))

            for actual, expected in zip(actual, expected):
                actualPath = os.path.normcase(actual[0])
                expectedPath = os.path.normcase(os.path.abspath(expected[0]))

                print(actualPath)
                print(expectedPath)

                self.assertEqual(actualPath, expectedPath)
                self.assertEqual(actual[1], expected[1])

        
        layer = Sdf.Layer.FindOrOpen("./layer.usda")
        _test(layer, "/foo.udim_style_a",
              [("style_a.1001.exr", "1001"), ("style_a.1011.exr", "1011")])
        
        _test(layer, "/foo.udim_style_b",
              [("style_b_1002.exr", "1002"), ("style_b_1021.exr", "1021")])
        
        _test(layer, "/foo.udim_package",
            [
                ("udims.usdz[udim_tex_1001_.foo.bar.baz]", "1001"), 
                ("udims.usdz[udim_tex_1002_.foo.bar.baz]", "1002"), 
                ("udims.usdz[udim_tex_1003_.foo.bar.baz]", "1003"), 
            ])

        _test(layer, "/foo.udim_missing", [])

    def test_ReplaceUdimPattern(self):
        self.assertEqual(
            UsdShade.UdimUtils.ReplaceUdimPattern("style_a.<UDIM>.exr", "1011"),
            "style_a.1011.exr"
        )

        self.assertEqual(
            UsdShade.UdimUtils.ReplaceUdimPattern("style_b.<UDIM>.exr", "1021"),
            "style_b.1021.exr"
        )

        self.assertEqual(
            UsdShade.UdimUtils.ReplaceUdimPattern("style_z.exr", "1021"),
            "style_z.exr"
        )

    def test_ResolveUdimPath(self):
        """Basic test for UsdShade.UdimUtils.ResolveUdimPath"""
        def _test(layer, propPath, value):
            attr = layer.GetPropertyAtPath(propPath)
            self.assertEqual(
                os.path.normcase(
                    UsdShade.UdimUtils.ResolveUdimPath(attr.default.path, layer)),
                os.path.normcase(
                    os.path.abspath(value)) if value != "" else value,
            )

        layer = Sdf.Layer.FindOrOpen("./layer.usda")
        _test(layer, "/foo.udim_style_a", "style_a.<UDIM>.exr")
        _test(layer, "/foo.udim_style_b", "style_b_<UDIM>.exr")
        _test(layer, "/foo.udim_missing", "")
        _test(layer, "/foo.not_udim", "")

if __name__=="__main__":
    unittest.main()
