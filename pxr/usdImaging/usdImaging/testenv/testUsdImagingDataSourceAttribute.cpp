//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourceRelationship.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class TestStageGlobals : public UsdImagingDataSourceStageGlobals
{
public:
    TestStageGlobals() : _time(0) {}
    ~TestStageGlobals() override = default;

    UsdTimeCode GetTime() const override { return UsdTimeCode(_time); }

    void FlagAsTimeVarying(
        const SdfPath &hydraPath,
        const HdDataSourceLocator &locator) const override {
        _timeVarying[hydraPath].insert(locator);
    }

    void FlagAsAssetPathDependent(
        const SdfPath &usdPath) const override {
        _assetPathDependent.insert(usdPath);
    }

    HdDataSourceLocatorSet const&
    GetTimeVaryingLocators(SdfPath const& hydraPath) const {
        return _timeVarying[hydraPath];
    }

    std::set<SdfPath> const& GetAssetPathDependents() const {
        return _assetPathDependent;
    }

private:
    double _time;
    mutable std::map<SdfPath, HdDataSourceLocatorSet> _timeVarying;
    mutable std::set<SdfPath> _assetPathDependent;
};

void RelationshipTest()
{
    SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous(".usda");
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    UsdStageRefPtr stage = UsdStage::Open(rootLayer, sessionLayer);

    UsdPrim world = stage->DefinePrim(SdfPath("/World"));
    TF_VERIFY(world);
    stage->DefinePrim(SdfPath("/World/foo"));
    stage->DefinePrim(SdfPath("/World/bar"));
    UsdRelationship rel = world.CreateRelationship(TfToken("taco"));
    TF_VERIFY(rel);

    TestStageGlobals stageGlobals;

    UsdImagingDataSourceRelationshipHandle relDs =
        UsdImagingDataSourceRelationship::New(rel, stageGlobals);

    // API tests.
    TF_VERIFY(relDs->GetValue(0).IsHolding<VtArray<SdfPath>>());
    std::vector<HdSampledDataSource::Time> sampleTimes;
    TF_VERIFY(relDs->GetContributingSampleTimesForInterval(-1, 1, &sampleTimes) == false);
    TF_VERIFY(sampleTimes.size() == 0);

    // Variability tracking.
    TF_VERIFY(stageGlobals.GetTimeVaryingLocators(SdfPath("/World")).IsEmpty());

    // Empty relationship.
    TF_VERIFY(relDs->GetTypedValue(0).size() == 0);

    // 1 target.
    rel.AddTarget(SdfPath("/World/foo"));
    VtArray<SdfPath> targets = relDs->GetTypedValue(0);
    TF_VERIFY(targets.size() == 1);
    TF_VERIFY(targets[0] == SdfPath("/World/foo"));

    // 2 targets.
    rel.AddTarget(SdfPath("/World/bar"));
    targets = relDs->GetTypedValue(0);
    TF_VERIFY(targets.size() == 2);
    TF_VERIFY(targets[0] == SdfPath("/World/foo"));
    TF_VERIFY(targets[1] == SdfPath("/World/bar"));
}

void AttributeTest()
{
    SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous(".usda");
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    UsdStageRefPtr stage = UsdStage::Open(rootLayer, sessionLayer);

    UsdPrim world = stage->DefinePrim(SdfPath("/World"));
    TF_VERIFY(world);
    UsdAttribute attrStatic = world.CreateAttribute(TfToken("taco"),
            SdfValueTypeNames->Bool, SdfVariabilityUniform);
    TF_VERIFY(attrStatic);
    attrStatic.Set<bool>(true);

    UsdAttribute attrSampled = world.CreateAttribute(TfToken("burrito"),
            SdfValueTypeNames->Bool, SdfVariabilityVarying);
    TF_VERIFY(attrSampled);
    attrSampled.Set<bool>(true, UsdTimeCode(-0.5));

    UsdAttribute attrAssetPath = world.CreateAttribute(TfToken("quesadilla"),
            SdfValueTypeNames->Asset, SdfVariabilityUniform);
    TF_VERIFY(attrAssetPath);
    attrAssetPath.Set<SdfAssetPath>(SdfAssetPath("`${ASSET_PATH}`"));

    TestStageGlobals stageGlobals;

    HdSampledDataSourceHandle attrStaticDs =
        UsdImagingDataSourceAttributeNew(attrStatic, stageGlobals,
            SdfPath("/World"), HdDataSourceLocator(TfToken("taco")));
    HdSampledDataSourceHandle attrSampledDs =
        UsdImagingDataSourceAttributeNew(attrSampled, stageGlobals,
            SdfPath("/World"), HdDataSourceLocator(TfToken("burrito")));
    HdSampledDataSourceHandle attrAssetPathDs =
        UsdImagingDataSourceAttributeNew(attrAssetPath, stageGlobals,
            SdfPath("/World"), HdDataSourceLocator(TfToken("quesadilla")));

    // API tests.
    std::vector<HdSampledDataSource::Time> sampleTimes;

    TF_VERIFY(attrStaticDs->GetValue(0).IsHolding<bool>());
    TF_VERIFY(attrStaticDs->GetContributingSampleTimesForInterval(-1, 1, &sampleTimes) == false);
    TF_VERIFY(sampleTimes.size() == 0);

    TF_VERIFY(attrSampledDs->GetValue(0).IsHolding<bool>());
    TF_VERIFY(attrSampledDs->GetContributingSampleTimesForInterval(-1, 1, &sampleTimes) == false);
    TF_VERIFY(sampleTimes.size() == 0);

    TF_VERIFY(attrAssetPathDs->GetValue(0).IsHolding<SdfAssetPath>());
    TF_VERIFY(attrAssetPathDs->GetContributingSampleTimesForInterval(-1, 1, &sampleTimes) == false);
    TF_VERIFY(sampleTimes.size() == 0);

    // Variability tracking.
    TF_VERIFY(stageGlobals.GetTimeVaryingLocators(SdfPath("/World")).IsEmpty());

    // Asset path tracking.
    TF_VERIFY(stageGlobals.GetAssetPathDependents() == 
        std::set<SdfPath>{SdfPath("/World.quesadilla")});

    // Add a second timesample on "burrito"
    attrSampled.Set<bool>(false, UsdTimeCode(0.5));
    attrSampledDs =
        UsdImagingDataSourceAttributeNew(attrSampled, stageGlobals,
            SdfPath("/World"), HdDataSourceLocator(TfToken("burrito")));

    TF_VERIFY(attrSampledDs->GetValue(0).IsHolding<bool>());
    TF_VERIFY(attrSampledDs->GetContributingSampleTimesForInterval(-1, 1, &sampleTimes) == true);
    TF_VERIFY(sampleTimes.size() == 4 && sampleTimes[0] == -1 && sampleTimes[1] == -0.5 && sampleTimes[2] == 0.5 && sampleTimes[3] == 1);
    HdDataSourceLocatorSet const& locators =
        stageGlobals.GetTimeVaryingLocators(SdfPath("/World"));
    HdDataSourceLocatorSet baseline = {
        HdDataSourceLocator(TfToken("burrito"))
    };
    TF_VERIFY(locators == baseline);

    // Value resolution
    TF_VERIFY(HdBoolDataSource::Cast(attrStaticDs)->GetTypedValue(0) == true);
    TF_VERIFY(HdBoolDataSource::Cast(attrSampledDs)->GetTypedValue(-0.7) == true);
    TF_VERIFY(HdBoolDataSource::Cast(attrSampledDs)->GetTypedValue(0) == true);
    TF_VERIFY(HdBoolDataSource::Cast(attrSampledDs)->GetTypedValue(0.7) == false);
}

int main()
{
    TfErrorMark mark;

    RelationshipTest();
    AttributeTest();

    if (TF_VERIFY(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
}
