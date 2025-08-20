//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/timeCode.h"

#include <iostream>
#include <functional>
#include <initializer_list>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (computeXf)
    (scale)
    (xf)
    );

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdRequestInvalidationComputedTransform)
{
    self.PrimComputation(_tokens->computeXf)
        .Callback(+[](const VdfContext &ctx) {
            const double fallbackScale = 1.0;
            const double scale = *ctx.GetInputValuePtr<double>(
                _tokens->scale, &fallbackScale);

            const GfMatrix4d id(1);
            const GfMatrix4d xf = *ctx.GetInputValuePtr<GfMatrix4d>(
                _tokens->xf, &id) * scale;
            const GfMatrix4d &parentXf = *ctx.GetInputValuePtr<GfMatrix4d>(
                _tokens->computeXf, &id);
            return xf * parentXf;
        })
        .Inputs(
            AttributeValue<GfMatrix4d>(_tokens->xf),
            AttributeValue<double>(_tokens->scale),
            NamespaceAncestor<GfMatrix4d>(_tokens->computeXf)
        );
}

static void
ConfigureTestPlugin()
{
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));

    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdRequestInvalidation");
}

static UsdStageRefPtr
CreateTestStage()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    const bool importedLayer = layer->ImportFromString(
        R"usda(#usda 1.0
        (
            defaultPrim = "Root"
        )
        def ComputedTransform "Root" (
            kind = "component"
        )
        {
            def ComputedTransform "A1"
            {
                matrix4d xf = ( (2, 0, 0, 0), (0, 2, 0, 0), (0, 0, 2, 0), (0, 0, 0, 1) )
                double scale = 1
                double scale.spline = {
                    1: 1,
                    2: 2,
                }
                def ComputedTransform "B"
                {
                    matrix4d xf = ( (3, 0, 0, 0), (0, 3, 0, 0), (0, 0, 3, 0), (0, 0, 0, 1) )
                }
            }
            def ComputedTransform "A2"
            {
                matrix4d xf = ( (5, 0, 0, 0), (0, 5, 0, 0), (0, 0, 5, 0), (0, 0, 0, 1) )
            }
        }
        )usda");
    TF_AXIOM(importedLayer);

    UsdStageRefPtr stage = UsdStage::Open(layer);
    TF_AXIOM(stage);
    return stage;
}

namespace {

// Structure for keeping track of invalidation state as received from request
// callback invocations.
// 
struct _InvalidationState {
    // Map from invalid index to number of times invalidated.
    std::unordered_map<int, int> indices;

    // The combined invalid time interval.
    EfTimeInterval interval;

    // Number of times the callback has been invoked.
    int numInvoked = 0;

    // Reset the invalidation state.
    void Reset() {
        indices.clear();
        interval.Clear();
        numInvoked = 0;
    }

    // The value invalidation callback invoked by the request.
    void ValueCalback(
        const ExecRequestIndexSet &invalidIndices,
        const EfTimeInterval &invalidInterval)
    {
        // Add all invalid indices to the map and increment the invalidation
        // count for each entry.
        for (const int i : invalidIndices) {
            auto [it, emplaced] = indices.emplace(i, 0);
            ++it->second;
        }

        // Combine the invalid interval.
        interval |= invalidInterval;

        // Increment the number of times the callback has been invoked.
        ++numInvoked;
    }

    // The time invalidation callback invoked by the request
    void TimeCallback(
        const ExecRequestIndexSet &invalidIndices)
    {
        // Add all invalid indices to the map and increment the invalidation
        // count for each entry.
        for (const int i : invalidIndices) {
            auto [it, emplaced] = indices.emplace(i, 0);
            ++it->second;
        }

        // Increment the number of times the callback has been invoked.
        ++numInvoked;
    }
};

}

// Validate the invalid indices map in a human-readable way.
// 
// Iterate over an array of invalid indices, where each entry in the array
// represents the number of times invalidation was expected for the index at
// the given entry.
// 
static bool
_ValidateSet(
    const std::unordered_map<int, int> indexMap,
    std::initializer_list<int> indexArray)
{
    int index = 0;
    for (const int expected : indexArray) {
        const auto it = indexMap.find(index);
        const int recorded = it == indexMap.end() ? 0 : it->second;
        if (expected != recorded) {
            std::cerr
                << "Index " << index 
                << ": expected " << expected << ", recorded " << recorded
                << std::endl;
            return false;
        }
        ++index;
    }
    return true;
}

static void
TestRequestCallbacks()
{
    UsdStageRefPtr stage = CreateTestStage();

    ExecUsdSystem system(stage);

    _InvalidationState invalidation;

    ExecUsdRequest request =
    system.BuildRequest({
            {stage->GetPrimAtPath(SdfPath("/Root")), _tokens->computeXf},
            {stage->GetPrimAtPath(SdfPath("/Root/A1")), _tokens->computeXf},
            {stage->GetPrimAtPath(SdfPath("/Root/A1/B")), _tokens->computeXf},
            {stage->GetPrimAtPath(SdfPath("/Root/A2")), _tokens->computeXf},
        },
        std::bind(&_InvalidationState::ValueCalback, &invalidation,
            std::placeholders::_1,
            std::placeholders::_2),
        std::bind(&_InvalidationState::TimeCallback, &invalidation,
            std::placeholders::_1));
    TF_AXIOM(request.IsValid());

    system.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    system.Compute(request);
    ASSERT_EQ(invalidation.numInvoked, 0);

    // Change the value of an attribute directly connected to a leaf node and
    // validate the resulting invalidation.
    UsdAttribute Bxf = stage->GetAttributeAtPath(SdfPath("/Root/A1/B.xf"));
    TF_AXIOM(Bxf);
    Bxf.Set(GfMatrix4d(1.0));
    ASSERT_EQ(invalidation.numInvoked, 1);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,0,1,0}));
    TF_AXIOM(invalidation.interval.IsFullInterval());

    // Change the value of an attribute transitively connected to a leaf node
    // and validate the resulting invalidation.
    UsdAttribute A1xf = stage->GetAttributeAtPath(SdfPath("/Root/A1.xf"));
    TF_AXIOM(A1xf);
    A1xf.Set(GfMatrix4d(1.0));
    ASSERT_EQ(invalidation.numInvoked, 2);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,1,1,0}));
    TF_AXIOM(invalidation.interval.IsFullInterval());

    // Invalidate B.xf again, which should not send out additional notification.
    Bxf.Set(GfMatrix4d(3.0));
    ASSERT_EQ(invalidation.numInvoked, 2);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,1,1,0}));
    TF_AXIOM(invalidation.interval.IsFullInterval());

    // Cache values again to renew interest in invalidation notification.
    invalidation.Reset();
    system.Compute(request);
    ASSERT_EQ(invalidation.numInvoked, 0);

    // Change the value of a previously changed attribute again.
    Bxf.Set(GfMatrix4d(2.0));
    ASSERT_EQ(invalidation.numInvoked, 1);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,0,1,0}));
    TF_AXIOM(invalidation.interval.IsFullInterval());

    // Change the value of a never before changed attribute.
    UsdAttribute A2xf = stage->GetAttributeAtPath(SdfPath("/Root/A2.xf"));
    TF_AXIOM(A2xf);
    A2xf.Set(GfMatrix4d(4.0));
    ASSERT_EQ(invalidation.numInvoked, 2);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,0,1,1}));
    TF_AXIOM(invalidation.interval.IsFullInterval());

    // Cache values again to renew interest in invalidation notification.
    invalidation.Reset();
    system.Compute(request);
    ASSERT_EQ(invalidation.numInvoked, 0);

    // Change the value of an irrelevant field
    A1xf.SetMetadata(SdfFieldKeys->Documentation, "test doc");
    ASSERT_EQ(invalidation.numInvoked, 0);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,0,0,0}));
    TF_AXIOM(invalidation.interval.IsEmpty());

    // Test changing multiple default values at the same time.
    SdfLayerHandle rootLayer = stage->GetRootLayer();
    {
        SdfChangeBlock block;
        rootLayer->GetAttributeAtPath(SdfPath("/Root/A1.xf"))->SetDefaultValue(
            VtValue(GfMatrix4d(5.0)));
        rootLayer->GetAttributeAtPath(SdfPath("/Root/A2.xf"))->SetDefaultValue(
            VtValue(GfMatrix4d(5.0)));
    }
    ASSERT_EQ(invalidation.numInvoked, 1);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,1,1,1}));
    TF_AXIOM(invalidation.interval.IsFullInterval());

    // The exec system should be initialized with the default time, so there
    // should be no time invalidation here.
    invalidation.Reset();
    system.ChangeTime(UsdTimeCode::Default());
    ASSERT_EQ(invalidation.numInvoked, 0);

    // /Root/A1.scale is not varying between the default time and frame 1, so 
    // there should not be invalidation.
    invalidation.Reset();
    system.ChangeTime(UsdTimeCode(1.0));
    ASSERT_EQ(invalidation.numInvoked, 0);

    // /Root/A1.scale 's spline value is different on frame 2, so we should be
    // able to observe invalidation.
    invalidation.Reset();
    system.ChangeTime(UsdTimeCode(2.0));
    ASSERT_EQ(invalidation.numInvoked, 1);
    TF_AXIOM(_ValidateSet(invalidation.indices, {0,1,1,0}));
    TF_AXIOM(invalidation.interval.IsEmpty());

    // The knot value on frame 2 should be held over the following frames.
    invalidation.Reset();
    system.ChangeTime(UsdTimeCode(3.0));
    ASSERT_EQ(invalidation.numInvoked, 0);
}

int
main(int argc, char* argv[])
{
    ConfigureTestPlugin();

    TestRequestCallbacks();

    return 0;
}
