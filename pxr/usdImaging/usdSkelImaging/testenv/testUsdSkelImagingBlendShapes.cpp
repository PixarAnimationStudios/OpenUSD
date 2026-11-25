//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdSkelImaging/animationSchema.h"
#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/resolvedSkeletonSchema.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonResolvingSceneIndex.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/skinningSettings.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/tokens.h"

#include "pxr/base/tf/errorMark.h"

#include <algorithm>
#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

class _BlendShapeObserver : public HdSceneIndexObserver
{
public:
    void PrimsAdded(
        const HdSceneIndexBase &,
        const AddedPrimEntries &) override {}

    void PrimsRemoved(
        const HdSceneIndexBase &,
        const RemovedPrimEntries &) override {}

    void PrimsRenamed(
        const HdSceneIndexBase &,
        const RenamedPrimEntries &) override {}

    void PrimsDirtied(
        const HdSceneIndexBase &,
        const DirtiedPrimEntries &entries) override
    {
        for (const auto &entry : entries) {
            if (entry.primPath == SdfPath("/Skeleton")) {
                _locators.insert(entry.dirtyLocators);
            }
        }
    }

    void CheckAndClear()
    {
        TF_AXIOM(_locators.Contains(
            UsdSkelImagingResolvedSkeletonSchema::GetBlendShapesLocator()));
        TF_AXIOM(_locators.Contains(
            UsdSkelImagingResolvedSkeletonSchema::
                GetBlendShapeWeightsLocator()));
        TF_AXIOM(_locators.Contains(
            UsdSkelImagingResolvedSkeletonSchema::
                GetBlendShapeRangesLocator()));
        _locators = HdDataSourceLocatorSet();
    }

private:
    HdDataSourceLocatorSet _locators;
};

static void
_CheckBlendShapes(
    const HdSceneIndexBaseRefPtr &sceneIndex,
    const VtTokenArray &expectedShapes,
    const VtFloatArray &expectedWeights,
    const float shutterOffset = 0.0f)
{
    const UsdSkelImagingResolvedSkeletonSchema resolved =
        UsdSkelImagingResolvedSkeletonSchema::GetFromParent(
            sceneIndex->GetPrim(SdfPath("/Skeleton")).dataSource);
    TF_AXIOM(resolved);

    TF_AXIOM(resolved.GetBlendShapes()->GetTypedValue(shutterOffset) ==
             expectedShapes);
    TF_AXIOM(resolved.GetBlendShapeWeights()->GetTypedValue(shutterOffset) ==
             expectedWeights);
    const VtVec2iArray expectedRanges{
        GfVec2i(0, static_cast<int>(expectedShapes.size()))};
    TF_AXIOM(resolved.GetBlendShapeRanges()->GetTypedValue(shutterOffset) ==
             expectedRanges);
}

template <typename T>
static typename HdRetainedTypedSampledDataSource<T>::Handle
_ToDataSource(const T &value)
{
    return HdRetainedTypedSampledDataSource<T>::New(value);
}

static void
TestInstanceAnimationRanges()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "TestInstanceAnimationRanges\n";
    std::cout << "-------------------------------------------------------\n";

    if (!HdSkinningSettings::IsSkinningDeferred()) {
        std::cout << "Skipped: deferred skinning is disabled.\n";
        return;
    }

    const auto input = HdRetainedSceneIndex::New();
    const VtArray<SdfPath> paths = {
        SdfPath("/Anim0"), SdfPath("/Anim1"),
        SdfPath("/Anim2"), SdfPath("/Anim3")
    };
    const std::vector<VtTokenArray> shapes = {
        {TfToken("a"), TfToken("b")},
        {},
        {TfToken("c")},
        {TfToken("d"), TfToken("e")}
    };
    const std::vector<VtFloatArray> weights = {
        {1.0f, 2.0f}, {3.0f}, {4.0f, 5.0f}, {6.0f}
    };
    for (size_t i = 0; i < paths.size(); ++i) {
        input->AddPrims({{paths[i],
            UsdSkelImagingPrimTypeTokens->skelAnimation,
            HdRetainedContainerDataSource::New(
                UsdSkelImagingAnimationSchema::GetSchemaToken(),
                UsdSkelImagingAnimationSchema::Builder()
                    .SetBlendShapes(i == 1 ? nullptr : _ToDataSource(shapes[i]))
                    .SetBlendShapeWeights(_ToDataSource(weights[i]))
                    .Build())}});
    }

    input->AddPrims({
        {SdfPath("/Prototype"), TfToken(),
            HdRetainedContainerDataSource::New(
                UsdSkelImagingBindingSchema::GetSchemaToken(),
                UsdSkelImagingBindingSchema::Builder()
                    .SetHasSkelRoot(_ToDataSource(true)).Build())},
        {SdfPath("/Instancer"), HdPrimTypeTokens->instancer,
            HdRetainedContainerDataSource::New(
                HdPrimvarsSchema::GetSchemaToken(),
                HdRetainedContainerDataSource::New(
                    UsdSkelTokens->skelAnimationSource,
                    HdPrimvarSchema::Builder()
                        .SetPrimvarValue(_ToDataSource(paths)).Build()))},
        {SdfPath("/Skeleton"), UsdSkelImagingPrimTypeTokens->skeleton,
            HdRetainedContainerDataSource::New(
                HdInstancedBySchema::GetSchemaToken(),
                HdInstancedBySchema::Builder()
                    .SetPaths(
                        _ToDataSource(VtArray<SdfPath>{SdfPath("/Instancer")}))
                    .SetPrototypeRoots(
                        _ToDataSource(VtArray<SdfPath>{SdfPath("/Prototype")}))
                    .Build())}});

    const auto sceneIndex =
        UsdSkelImagingSkeletonResolvingSceneIndex::New(input);
    const UsdSkelImagingResolvedSkeletonSchema resolved =
        UsdSkelImagingResolvedSkeletonSchema::GetFromParent(
            sceneIndex->GetPrim(SdfPath("/Skeleton")).dataSource);
    TF_AXIOM(resolved);

    const VtTokenArray expectedShapes = {
        TfToken("a"), TfToken("b"), TfToken("c"), TfToken("d")
    };
    const VtFloatArray expectedWeights = {1.0f, 2.0f, 4.0f, 6.0f};
    const VtVec2iArray expectedRanges = {
        GfVec2i(0, 2), GfVec2i(2, 0), GfVec2i(2, 1), GfVec2i(3, 1)
    };
    for (const float time : {0.0f, 1.0f}) {
        TF_AXIOM(resolved.GetBlendShapeRanges()->GetTypedValue(time) ==
                 expectedRanges);
        TF_AXIOM(resolved.GetBlendShapes()->GetTypedValue(time) ==
                 expectedShapes);
        TF_AXIOM(resolved.GetBlendShapeWeights()->GetTypedValue(time) ==
                 expectedWeights);
    }
}

static void
TestBlendShapeStageEdits()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "TestBlendShapeStageEdits\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "blendShapes.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);

    const UsdSkelAnimation animation =
        UsdSkelAnimation::Get(stage, SdfPath("/Animation"));
    TF_AXIOM(animation);

    const VtTokenArray shapes{TfToken("smile"), TfToken("blink")};
    const VtFloatArray weights{0.25f, 0.75f};
    const UsdAttribute shapesAttr = animation.GetBlendShapesAttr();
    const UsdAttribute weightsAttr = animation.GetBlendShapeWeightsAttr();

    const auto input = UsdImagingStageSceneIndex::New();
    input->SetStage(stage);
    input->SetTime(UsdTimeCode(1.0));
    const auto resolved = UsdSkelImagingSkeletonResolvingSceneIndex::New(input);
    _BlendShapeObserver observer;
    resolved->AddObserver(HdSceneIndexObserverPtr(&observer));
    _CheckBlendShapes(resolved, shapes, weights);

    // This used to crash, see GitHub issue #4296.
    std::cout << "Blocking blendShapes" << std::endl;
    shapesAttr.Block();
    input->ApplyPendingUpdates();
    _CheckBlendShapes(resolved, {}, {});
    observer.CheckAndClear();

    TF_AXIOM(shapesAttr.Set(shapes));
    input->ApplyPendingUpdates();
    _CheckBlendShapes(resolved, shapes, weights);
    observer.CheckAndClear();
    TF_AXIOM(shapesAttr.Set(VtTokenArray{shapes[0]}));
    input->ApplyPendingUpdates();
    _CheckBlendShapes(resolved, {shapes[0]}, {weights[0]});
    observer.CheckAndClear();
    TF_AXIOM(shapesAttr.Set(shapes));
    TF_AXIOM(weightsAttr.Set(VtFloatArray{weights[0]}));
    input->ApplyPendingUpdates();
    _CheckBlendShapes(resolved, {shapes[0]}, {weights[0]});
    observer.CheckAndClear();

    weightsAttr.Block();
    input->ApplyPendingUpdates();
    _CheckBlendShapes(resolved, {}, {});
    observer.CheckAndClear();

    // Animate the weights so that their length changes over time.
    TF_AXIOM(weightsAttr.Clear());
    TF_AXIOM(weightsAttr.Set(weights, UsdTimeCode(1.0)));
    TF_AXIOM(weightsAttr.Set(VtFloatArray{0.5f}, UsdTimeCode(2.0)));
    input->ApplyPendingUpdates();
    _CheckBlendShapes(resolved, shapes, weights);
    _CheckBlendShapes(resolved, {shapes[0]}, {0.5f}, 1.0f);
    const UsdSkelImagingResolvedSkeletonSchema schema =
        UsdSkelImagingResolvedSkeletonSchema::GetFromParent(
            resolved->GetPrim(SdfPath("/Skeleton")).dataSource);
    const HdSampledDataSourceHandle sources[] = {
        schema.GetBlendShapes(), schema.GetBlendShapeWeights(),
        schema.GetBlendShapeRanges()};
    for (const auto &source : sources) {
        std::vector<float> times;
        // 1.0 is outside the interval, so only the weights can add it.
        TF_AXIOM(source->GetContributingSampleTimesForInterval(
            -0.5f, 0.5f, &times));
        TF_AXIOM(std::find(times.begin(), times.end(), 1.0f) != times.end());
    }
    input->SetTime(UsdTimeCode(2.0));
    _CheckBlendShapes(resolved, {shapes[0]}, {0.5f});

    resolved->RemoveObserver(HdSceneIndexObserverPtr(&observer));
}

int main()
{
    TfErrorMark mark;

    TestBlendShapeStageEdits();
    TestInstanceAnimationRanges();

    if (TF_AXIOM(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
}
