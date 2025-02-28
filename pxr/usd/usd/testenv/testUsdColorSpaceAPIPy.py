#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Gf, Sdf, Tf, Usd, UsdShade

class TestCache(Usd.ColorSpaceHashCache):
    def IsCached(self, path, name):
        return self.Find(path) == name

# Create a new stage
class TestUsdColorSpaceAPI(unittest.TestCase):
    def test_ColorSpaceAPI(self):
        stage = Usd.Stage.CreateInMemory()
        rec2020 = Gf.ColorSpaceNames.LinearRec2020
        rec709 = Gf.ColorSpaceNames.LinearRec709
        cache = TestCache()

        # Create a fallback color space
        rootPrim = stage.OverridePrim("/rec2020")
        rootCsAPI = Usd.ColorSpaceAPI.Apply(rootPrim)
        assert rootPrim.HasAPI(Usd.ColorSpaceAPI)
        rootCsAttr = rootCsAPI.CreateColorSpaceNameAttr(rec2020)
        assert rootCsAttr

        # Fetch the color space in a variety of ways
        assert Gf.ColorSpace.IsValid(rec2020)
        assert rootCsAttr.Get() == rec2020
        assert rootCsAPI.GetColorSpaceNameAttr().Get() == rec2020

        # Compute the color space
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(rootPrim, cache) == rec2020
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(rootCsAttr, cache) == rec2020

        assert cache.IsCached(Sdf.Path("/rec2020"), rec2020)

        # Create a color attribute on rootPrim and verify it inherits the color space from rootPrim
        colorAttr = rootPrim.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(colorAttr, cache) == rec2020

        # Create a child prim with a different color space, and verify it overrides the parent
        childPrim = stage.OverridePrim("/rec2020/linSRGB")
        childCsAPI = Usd.ColorSpaceAPI.Apply(childPrim)
        childCsAttr = childCsAPI.CreateColorSpaceNameAttr(rec709)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childPrim, cache) == rec709

        assert cache.IsCached(Sdf.Path("/rec2020/linSRGB"), rec709)

        # Create a color attribute on childPrim, and verify it inherits the color space from childPrim
        childColorAttr = childPrim.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childColorAttr, cache) == rec709

        # Create a grandchild prim with no color space, and verify it inherits the parent's color space
        grandchildPrim = stage.OverridePrim("/rec2020/linSRGB/noColorSpace")
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(grandchildPrim, cache) == rec709

        assert cache.IsCached(Sdf.Path("/rec2020/linSRGB/noColorSpace"), rec709)

        # Create a color attribute on grandchildPrim, and verify it inherits the color space from childPrim
        grandchildColorAttr = grandchildPrim.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(grandchildColorAttr, cache) == rec709

        # Create a root prim without assigning color space, and verify the result is empty
        rootPrim2 = stage.OverridePrim("/noColorSpace")
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(rootPrim2, cache) == ""

        assert cache.IsCached(Sdf.Path("/noColorSpace"), "")

        # Repeat all of the preceding rootPrim tests on rootPrim2
        colorAttr2 = rootPrim2.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(colorAttr2, cache) == ""

        # Create a child prim with a different color space, and verify it overrides the parent
        childPrim2 = stage.OverridePrim("/noColorSpace/linSRGB")
        child2CsAPI = Usd.ColorSpaceAPI.Apply(childPrim2)
        child2CsAttr = child2CsAPI.CreateColorSpaceNameAttr(rec709)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childPrim2, cache) == rec709

        assert cache.IsCached(Sdf.Path("/noColorSpace/linSRGB"), rec709)

        # Create a color attribute on childPrim2, and verify it inherits the color space from childPrim2
        childColorAttr2 = childPrim2.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childColorAttr2, cache) == rec709

        # Create a grandchild prim with no color space, and verify it inherits the parent's color space
        grandchildPrim2 = stage.OverridePrim("/noColorSpace/linSRGB/noColorSpace")
        grandchild2CsAPI = Usd.ColorSpaceAPI.Apply(grandchildPrim2)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(grandchildPrim2, cache) == rec709

        assert cache.IsCached(Sdf.Path("/noColorSpace/linSRGB/noColorSpace"), rec709)

        # Create a color attribute on grandchildPrim2, and verify it inherits the color space from childPrim2
        grandchildColorAttr2 = grandchildPrim2.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(grandchildColorAttr2, cache) == rec709

        # Test the CanApply method
        assert Usd.ColorSpaceAPI.CanApply(rootPrim)
        assert Usd.ColorSpaceAPI.CanApply(childPrim)
        assert Usd.ColorSpaceAPI.CanApply(grandchildPrim)

        # Test the Get method
        assert Usd.ColorSpaceAPI.Get(stage, Sdf.Path("/rec2020")).GetPrim() == rootPrim


        # Test the UsdColorSpaceDefinitionAPI. It's multipleApply so create two instances.
        # use the primaries from GfColorSpaceNames->LinearRec2020 and GfColorSpaceNames->ACEScg
        # but name them "testRec2020" and "testACEScg" respectively.
        colorSpaceDefPrim = stage.OverridePrim(Sdf.Path("/colorSpaceDef"))
        testRec2020 = "testRec2020"
        testACEScg = "testACESCg"
        testRec2020DefAPI = Usd.ColorSpaceDefinitionAPI.Apply(colorSpaceDefPrim, testRec2020)
        testACEScgDefAPI = Usd.ColorSpaceDefinitionAPI.Apply(colorSpaceDefPrim, testACEScg)
        colorSpaceDefAttr1 = testRec2020DefAPI.CreateNameAttr(testRec2020)
        colorSpaceDefAttr2 = testACEScgDefAPI.CreateNameAttr(testACEScg)

        # Create a hierarchy of prims referencing either of the defined color spaces,
        # and verify they inherit the color space from the definition.
        defPrim = stage.OverridePrim(Sdf.Path("/colorSpaceDef/noCS/defPrim"))
        defCsAPI = Usd.ColorSpaceAPI.Apply(defPrim)
        defColorAttr = defPrim.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)

        # set the testRec2020 color space on the attribute, then verify that compute succeeds
        defColorAttr.SetColorSpace(testRec2020)

        # set the color space on the prim itself to be testACEScg
        defColorSpaceAttr = defCsAPI.CreateColorSpaceNameAttr(testACEScg)

        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(defColorAttr, cache) == testRec2020

        # verify that a prim at /colorSpaceDef/noCS has no color space
        noCSPrim = stage.OverridePrim(Sdf.Path("/colorSpaceDef/noCS"))
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(noCSPrim, cache) == ""

        # create a child of defPrim, and verify it inherits the color space from defPrim
        childDefPrim = stage.OverridePrim(Sdf.Path("/colorSpaceDef/noCS/defPrim/childDefPrim"))

        # first verify that the prim's color space is inherited from the parent
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childDefPrim, cache) == testACEScg

        # create a color attribute on childDefPrim, and verify it inherits the color space from defPrim
        childDefColorAttr = childDefPrim.CreateAttribute("color", Sdf.ValueTypeNames.Color3f)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childDefColorAttr, cache) == testACEScg

        # set the attribute color space to testACEScg, and verify that it overrides the parent
        childDefColorAttr.SetColorSpace(testRec2020)
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childDefColorAttr, cache) == testRec2020

        # reverify that the child prim still inherits the color space from the parent
        assert Usd.ColorSpaceAPI.ComputeColorSpaceName(childDefPrim, cache) == testACEScg

        # since we haven't filled any data in for the test cases, if we compute the
        # GfColorSpace, it should be equal to a default constructed GfColorSpace.
        cs = Usd.ColorSpaceAPI.ComputeColorSpace(childDefPrim, cache)
        assert cs == Gf.ColorSpace(testACEScg)

        # Create a linear aces cg color space
        acesCG = Gf.ColorSpace(Gf.ColorSpaceNames.LinearAP1)

        # use the primaries and whitepoint to populate the color space definition
        # on testACEScgDefAPI; then verify that the color space is computed correctly.
        redChroma, greenChroma, blueChroma, whitePoint = acesCG.GetPrimariesAndWhitePoint()
        acesCGRedChromaAttr = testACEScgDefAPI.CreateRedChromaAttr(redChroma)
        acesCGGreenChromaAttr = testACEScgDefAPI.CreateGreenChromaAttr(greenChroma)
        acesCGBlueChromaAttr = testACEScgDefAPI.CreateBlueChromaAttr(blueChroma)
        acesCGWhitePointAttr = testACEScgDefAPI.CreateWhitePointAttr(whitePoint)

        # verify that the color space is computed correctly
        assert testACEScgDefAPI.ComputeColorSpaceFromDefinitionAttributes() == acesCG

        # verify it again, vs childDefPrim.
        assert Usd.ColorSpaceAPI.ComputeColorSpace(childDefPrim, cache) == acesCG

        print("UsdColorSpaceAPI and UsdColorSpaceDefinitionAPI tests passed")

if __name__ == "__main__":
    unittest.main()
