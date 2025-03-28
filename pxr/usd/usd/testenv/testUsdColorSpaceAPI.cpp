//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/color.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/colorSpaceAPI.h"
#include "pxr/usd/usd/colorSpaceDefinitionAPI.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/sdf/path.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE
using std::string;


class TestCache : public UsdColorSpaceAPI::ColorSpaceHashCache {
public:
    bool IsCached(const SdfPath& path, const TfToken& name) const {
        auto it = _cache.find(path);
        return it != _cache.end() && it->second == name;
    }
};

int main() {
    // Create a new stage
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    
    // Create a fallback color space
    UsdPrim rootPrim = stage->OverridePrim(SdfPath("/rec2020"));
    UsdColorSpaceAPI rootCsAPI = UsdColorSpaceAPI::Apply(rootPrim);
    TF_VERIFY(rootPrim.HasAPI<UsdColorSpaceAPI>());
    TF_VERIFY(rootPrim.HasAPI(TfToken("ColorSpaceAPI")));
    TF_VERIFY(rootPrim.HasAPI(TfToken("ColorSpaceAPI"), /*schemaVersion*/ 0));
    auto rootCsAttr = rootCsAPI.CreateColorSpaceNameAttr(VtValue(GfColorSpaceNames->LinearRec2020));
    TF_VERIFY(rootCsAttr);

    // Fetch the color space in a variety of ways
    TfToken colorSpace;
    TF_VERIFY(rootCsAttr.Get(&colorSpace));
    TF_VERIFY(colorSpace == GfColorSpaceNames->LinearRec2020);
    TF_VERIFY(GfColorSpace::IsValid(colorSpace));

    TF_VERIFY(rootCsAPI.GetColorSpaceNameAttr().Get(&colorSpace));
    TF_VERIFY(colorSpace == GfColorSpaceNames->LinearRec2020);

    TestCache cache;

    // Compute the color space.
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(rootPrim, &cache) == GfColorSpaceNames->LinearRec2020);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(rootCsAttr, &cache) == GfColorSpaceNames->LinearRec2020);

    TF_VERIFY(cache.IsCached(SdfPath("/rec2020"), GfColorSpaceNames->LinearRec2020));

    // Create a color attribute on rootPrim, and verify it inherits the color space from rootPrim
    UsdAttribute colorAttr = rootPrim.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(colorAttr, &cache) == GfColorSpaceNames->LinearRec2020);

    // Create a child prim with a different color space, and verify it overrides the parent
    UsdPrim childPrim = stage->OverridePrim(SdfPath("/rec2020/linSRGB"));
    auto childCsAPI = UsdColorSpaceAPI::Apply(childPrim);
    auto childCsAttr = childCsAPI.CreateColorSpaceNameAttr(VtValue(GfColorSpaceNames->LinearRec709));
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childPrim, &cache) == GfColorSpaceNames->LinearRec709);

    TF_VERIFY(cache.IsCached(SdfPath("/rec2020/linSRGB"), GfColorSpaceNames->LinearRec709));

    // Create a color attribute on childPrim, and verify it inherits the color space from childPrim
    UsdAttribute childColorAttr = childPrim.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childColorAttr, &cache) == GfColorSpaceNames->LinearRec709);

    // Create a grandchild prim with no color space, and verify it inherits the parent's color space
    UsdPrim grandchildPrim = stage->OverridePrim(SdfPath("/rec2020/linSRGB/noColorSpace"));
    auto grandchildCsAPI = UsdColorSpaceAPI::Apply(grandchildPrim);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(grandchildPrim, &cache) == GfColorSpaceNames->LinearRec709);

    TF_VERIFY(cache.IsCached(SdfPath("/rec2020/linSRGB/noColorSpace"), GfColorSpaceNames->LinearRec709));

    // Create a color attribute on grandchildPrim, and verify it inherits the color space from childPrim
    UsdAttribute grandchildColorAttr = grandchildPrim.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(grandchildColorAttr, &cache) == GfColorSpaceNames->LinearRec709);

    // Create a root prim without assigning color space, and verify the fallback is empty.
    UsdPrim rootPrim2 = stage->OverridePrim(SdfPath("/noColorSpace"));
    auto root2CsAPI = UsdColorSpaceAPI::Apply(rootPrim2);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(rootPrim2, &cache).IsEmpty());

    TF_VERIFY(cache.IsCached(SdfPath("/noColorSpace"), TfToken()));

    // Repeat all of the preceding rootPrim tests on rootPrim2.
    UsdAttribute colorAttr2 = rootPrim2.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(colorAttr2, &cache).IsEmpty());

    // Create a child prim with a different color space, and verify it overrides the parent
    UsdPrim childPrim2 = stage->OverridePrim(SdfPath("/noColorSpace/linSRGB"));
    auto child2CsAPI = UsdColorSpaceAPI::Apply(childPrim2);
    auto child2CsAttr = child2CsAPI.CreateColorSpaceNameAttr(VtValue(GfColorSpaceNames->LinearRec709));
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childPrim2, &cache) == GfColorSpaceNames->LinearRec709);

    TF_VERIFY(cache.IsCached(SdfPath("/noColorSpace/linSRGB"), GfColorSpaceNames->LinearRec709));

    // Create a color attribute on childPrim2, and verify it inherits the color space from childPrim2
    UsdAttribute childColorAttr2 = childPrim2.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childColorAttr2, &cache) == GfColorSpaceNames->LinearRec709);

    // Create a grandchild prim with no color space, and verify it inherits the parent's color space
    UsdPrim grandchildPrim2 = stage->OverridePrim(SdfPath("/noColorSpace/linSRGB/noColorSpace"));
    auto grandchild2CsAPI = UsdColorSpaceAPI::Apply(grandchildPrim2);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(grandchildPrim2, &cache) == GfColorSpaceNames->LinearRec709);

    TF_VERIFY(cache.IsCached(SdfPath("/noColorSpace/linSRGB/noColorSpace"), GfColorSpaceNames->LinearRec709));

    // Create a color attribute on grandchildPrim2, and verify it inherits the color space from childPrim2
    UsdAttribute grandchildColorAttr2 = grandchildPrim2.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(grandchildColorAttr2, &cache) == GfColorSpaceNames->LinearRec709);

    // Test the CanApply method
    TF_VERIFY(UsdColorSpaceAPI::CanApply(rootPrim));
    TF_VERIFY(UsdColorSpaceAPI::CanApply(childPrim));
    TF_VERIFY(UsdColorSpaceAPI::CanApply(grandchildPrim));

    // Test the Get method
    TF_VERIFY(UsdColorSpaceAPI::Get(stage, SdfPath("/rec2020")).GetPrim() == rootPrim);
 
    // Test the UsdColorSpaceDefinitionAPI. It's multipleApply so create two instances.
    // use the primaries from GfColorSpaceNames->LinearRec2020 and GfColorSpaceNames->ACEScg
    // but name them "testRec2020" and "testACEScg" respectively.
    UsdPrim colorSpaceDefPrim = stage->OverridePrim(SdfPath("/colorSpaceDef"));
    TfToken testRec2020("testRec2020");
    TfToken testACEScg("testACEScg");
    auto testRec2020DefAPI = UsdColorSpaceDefinitionAPI::Apply(colorSpaceDefPrim, testRec2020);
    auto testACEScgDefAPI = UsdColorSpaceDefinitionAPI::Apply(colorSpaceDefPrim, testACEScg);
    auto colorSpaceDefAttr1 = testRec2020DefAPI.CreateNameAttr(VtValue(testRec2020));
    auto colorSpaceDefAttr2 = testACEScgDefAPI.CreateNameAttr(VtValue(testACEScg));

    // Create a hierarchy of prims referencing either of the defined color spaces,
    // and verify they inherit the color space from the definition.

    UsdPrim defPrim = stage->OverridePrim(SdfPath("/colorSpaceDef/noCS/defPrim"));
    auto defCsAPI = UsdColorSpaceAPI::Apply(defPrim);
    auto defColorAttr = defPrim.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);

    // set the testRec2020 color space on the attribute, then verify that compute succeeds
    defColorAttr.SetColorSpace(testRec2020);

    // set the color space on the prim itself to be testACEScg
    auto defColorSpaceAttr = defCsAPI.CreateColorSpaceNameAttr(VtValue(testACEScg));

    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(defColorAttr, &cache) == testRec2020);

    // verify that a prim at /colorSpaceDef/noCS has no color space
    UsdPrim noCSPrim = stage->OverridePrim(SdfPath("/colorSpaceDef/noCS"));
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(noCSPrim, &cache).IsEmpty());

    // create a child of defPrim, and verify it inherits the color space from defPrim
    UsdPrim childDefPrim = stage->OverridePrim(SdfPath("/colorSpaceDef/noCS/defPrim/childDefPrim"));
    auto childDefCsAPI = UsdColorSpaceAPI::Apply(childDefPrim);

    // first verify that the prim's color space is inherited from the parent
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childDefPrim, &cache) == testACEScg);

    // create a color attribute on childDefPrim, and verify it inherits the color space from defPrim
    auto childDefColorAttr = childDefPrim.CreateAttribute(TfToken("color"), SdfValueTypeNames->Color3f);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childDefColorAttr, &cache) == testACEScg);

    // set the attribute color space to testACEScg, and verify that it overrides the parent
    childDefColorAttr.SetColorSpace(testRec2020);
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childDefColorAttr, &cache) == testRec2020);

    // reverify that the child prim still inherits the color space from the parent
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpaceName(childDefPrim, &cache) == testACEScg);

    // since we haven't filled any data in for the test cases, if we compute the
    // GfColorSpace, it should be equal to a default constructed GfColorSpace.
    GfColorSpace cs = UsdColorSpaceAPI::ComputeColorSpace(childDefPrim, &cache);
    TF_VERIFY(cs == GfColorSpace(testACEScg));

    // Create a linear aces cg color space
    GfColorSpace acesCG(GfColorSpaceNames->LinearAP1);

    // use the primaries and whitepoint to populate the color space definition
    // on testACEScgDefAPI; then verify that the color space is computed correctly.
    auto [redChroma, greenChroma, blueChroma, whitePoint] = 
                                            acesCG.GetPrimariesAndWhitePoint();
    auto acesCGRedChromaAttr = testACEScgDefAPI.CreateRedChromaAttr(VtValue(redChroma));
    auto acesCGGreenChromaAttr = testACEScgDefAPI.CreateGreenChromaAttr(VtValue(greenChroma));
    auto acesCGBlueChromaAttr = testACEScgDefAPI.CreateBlueChromaAttr(VtValue(blueChroma));
    auto acesCGWhitePointAttr = testACEScgDefAPI.CreateWhitePointAttr(VtValue(whitePoint));

    // verify that the color space is computed correctly
    TF_VERIFY(testACEScgDefAPI.ComputeColorSpaceFromDefinitionAttributes() == acesCG);

    // verify it again, vs childDefPrim.
    TF_VERIFY(UsdColorSpaceAPI::ComputeColorSpace(childDefPrim, &cache) == acesCG);

    printf("UsdColorSpaceAPI and UsdColorSpaceDefinitionAPI tests passed\n");
    return EXIT_SUCCESS;
}
