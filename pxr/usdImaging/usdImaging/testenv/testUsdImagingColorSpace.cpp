//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdShade/material.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (Surface)
    (diffuseColor)
    (specularColor)
    // primvars
    (displayColor)
    (customColor)
);

// NOTE: This class is copied from testUsdImagingDataSourceAttribute.cpp
class TestStageGlobals : public UsdImagingDataSourceStageGlobals {
public:
    TestStageGlobals() : _time(0) {}
    ~TestStageGlobals() override = default;

    UsdTimeCode GetTime() const override { return UsdTimeCode(_time); }

    void FlagAsTimeVarying(
        const SdfPath &hydraPath,
        const HdDataSourceLocator &locator) const override {
        _timeVarying[hydraPath].insert(locator);
    }

    void FlagAsAssetPathDependent(const SdfPath &usdPath) const override {
        _assetPathDependent.insert(usdPath);
    }

    HdDataSourceLocatorSet const &
    GetTimeVaryingLocators(SdfPath const &hydraPath) const {
        return _timeVarying[hydraPath];
    }

    std::set<SdfPath> const &GetAssetPathDependents() const {
        return _assetPathDependent;
    }

private:
    double _time;
    mutable std::map<SdfPath, HdDataSourceLocatorSet> _timeVarying;
    mutable std::set<SdfPath> _assetPathDependent;
};


UsdImagingStageSceneIndexRefPtr
OpenStage(std::string const& usdFileName)
{
    UsdStageRefPtr stage = UsdStage::Open(usdFileName);
    UsdTimeCode frame(0);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));

    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;

    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);
    UsdImagingStageSceneIndexRefPtr stageSceneIndex =
        sceneIndices.stageSceneIndex;
    stageSceneIndex->SetTime(frame);

    return stageSceneIndex;
}

bool
CheckParamColorSpace(
    UsdImagingStageSceneIndexRefPtr const& stageSceneIndex,
    SdfPath const& materialPath,
    TfToken const& nodeNameToken,
    TfToken const& paramNameToken,
    VtValue const& colorSpaceValue)
{
    HdSceneIndexPrim prim = stageSceneIndex->GetPrim(materialPath);
    if (prim.primType.IsEmpty()) {
        std::cout << " - no material prim <" << materialPath << ">.\n";
        return false;
    }

    HdMaterialSchema matSchema = HdMaterialSchema::GetFromParent(prim.dataSource);
    HdMaterialNetworkSchema netSchema = matSchema.GetMaterialNetwork(TfToken());
    HdMaterialNodeSchema nodeSchema = netSchema.GetNodes().Get(nodeNameToken);
    if (!nodeSchema.IsDefined()) {
        std::cout << " - no node schema for '" << nodeNameToken << "' node\n";
        return false;
    }
    HdMaterialNodeParameterSchema paramSchema =
        nodeSchema.GetParameters().Get(paramNameToken);
    if (!paramSchema) {
        std::cout << " - no param schema for '" << paramNameToken << "' param.\n";
        return false;
    }

    const VtValue paramColorSpace = paramSchema.GetColorSpace()->GetValue(0);
    if (paramColorSpace != colorSpaceValue) {
        std::cout << " - color space does not match expected value (" 
                  << colorSpaceValue << ") " << materialPath << "/" 
                  << nodeNameToken << "." << paramNameToken 
                  << "color space = " << paramColorSpace << std::endl;
        return false;
    }
    return true;
}

bool
CheckPrimvarColorSpace(
    UsdImagingStageSceneIndexRefPtr const& stageSceneIndex,
    SdfPath const& primPath,
    TfToken const& primvarNameToken,
    VtValue const& colorSpaceValue)
{
    HdSceneIndexPrim prim = stageSceneIndex->GetPrim(primPath);
    if (prim.primType.IsEmpty()) {
        std::cout << " - no prim <" << primPath << ">.\n";
        return false;
    }

    HdPrimvarsSchema primvarsSchema =
        HdPrimvarsSchema::GetFromParent(prim.dataSource);
    HdPrimvarSchema primvarSchema = primvarsSchema.GetPrimvar(primvarNameToken);
    if (!primvarSchema) {
        std::cout << " - no primvar schema for '" << primvarNameToken << "'.\n";
        return false;
    }

    const VtValue paramColorSpace = primvarSchema.GetColorSpace()->GetValue(0);
    if (paramColorSpace != colorSpaceValue) {
        std::cout << " - color space does not match expected value (" 
                  << colorSpaceValue << ") " << primPath << "/" 
                  << "." << primvarNameToken 
                  << "color space = " << paramColorSpace << std::endl;
                  return false;
    }
    return true;
}

bool
TestAttributes()
{
    bool clean = true;
    UsdImagingStageSceneIndexRefPtr stageSceneIndex = OpenStage("basic.usda");

    // Check Color Space on Red Surface Material 
    // diffuseColor should be lin_ap0_scene - color space API defined on the 
    // 'Materials' Xform 
    clean &= CheckParamColorSpace(
        stageSceneIndex,
        SdfPath("/Materials/Red"),
        _tokens->Surface,
        _tokens->diffuseColor,
        VtValue(TfToken("lin_ap0_scene")));

    // Check Color space on Green Material Attributes
    // diffuseColor Should be sRGB - attribute metadata 
    // specularColor lin_rec2020_scene - color space API defined on the 
    // parent 'Green' material node
    const SdfPath greenMaterialPath("/Materials/Green");
    clean &= CheckParamColorSpace(
        stageSceneIndex,
        greenMaterialPath,
        _tokens->Surface,
        _tokens->diffuseColor,
        VtValue(TfToken("sRGB")));
    clean &= CheckParamColorSpace(
        stageSceneIndex,
        greenMaterialPath,
        _tokens->Surface,
        _tokens->specularColor,
        VtValue(TfToken("lin_rec2020_scene")));

    // Check Color Space on Blue Material Attributes
    // diffuseColor should be sRGB - attribute metadata
    // specularColor should be srgb_rec709_scene - color space API defined on 
    // the 'Surface' shader node
    const SdfPath blueMaterialPath("/Materials/Blue");
    clean &= CheckParamColorSpace(
        stageSceneIndex,
        blueMaterialPath,
        _tokens->Surface,
        _tokens->diffuseColor,
        VtValue(TfToken("sRGB")));
    clean &= CheckParamColorSpace(
        stageSceneIndex,
        blueMaterialPath,
        _tokens->Surface,
        _tokens->specularColor,
        VtValue(TfToken("srgb_rec709_scene")));
    return clean;
}

bool
TestPrimvars()
{
    bool clean = true;
    UsdImagingStageSceneIndexRefPtr stageSceneIndex = OpenStage("primvars.usda");

    // Check Color Space on Sphere1
    // displayColor should be g22_rec709_scene - color space API defined on the 
    // 'World' Xform 
    clean &= CheckPrimvarColorSpace(
        stageSceneIndex,
        SdfPath("/World/Spheres/Sphere1"),
        _tokens->displayColor,
        VtValue(TfToken("g22_rec709_scene")));

    // Check Color Space on Sphere2
    // displayColor should be srgb_p3d65_scene - color space API defined on
    // Sphere2 itself
    clean &= CheckPrimvarColorSpace(
        stageSceneIndex,
        SdfPath("/World/Spheres/Sphere2"),
        _tokens->displayColor,
        VtValue(TfToken("srgb_p3d65_scene")));

    // Check Color Space on Sphere3
    // displayColor should be lin_rec2020_scene - color space API defined on the 
    // Sphere's parent transform 'SingleSphere'
    // customColor should be in sRGB - defined as attribute metadata 
    const SdfPath sphere3Path("/World/SingleSphere/Sphere3");
    clean &= CheckPrimvarColorSpace(
        stageSceneIndex,
        sphere3Path,
        _tokens->displayColor,
        VtValue(TfToken("lin_rec2020_scene")));
    clean &= CheckPrimvarColorSpace(
        stageSceneIndex,
        sphere3Path,
        _tokens->customColor,
        VtValue(TfToken("sRGB")));

    return clean;
}


int main(int argc, char *argv[])
{
    TfErrorMark mark;

    bool success = TestAttributes();
    success &= TestPrimvars();

    if (success && TF_VERIFY(mark.IsClean())) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }
    else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
