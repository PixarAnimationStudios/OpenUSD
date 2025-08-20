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

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{

// Accumulates indices and intervals sent by request invalidation.
class InvalidationAccumulator
{
public:
    void operator()(
        const ExecRequestIndexSet &indices,
        const EfTimeInterval &interval) {
        _indices.insert(indices.begin(), indices.end());
        _interval |= interval;
    }

    const ExecRequestIndexSet &GetIndices() const {
        return _indices;
    }

    const EfTimeInterval &GetInterval() const {
        return _interval;
    }

private:
    ExecRequestIndexSet _indices;
    EfTimeInterval _interval;
};

}

static UsdStageRefPtr
CreateTestStage()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    const bool importedLayer = layer->ImportFromString(
        R"usda(#usda 1.0

        def "A"
        {
            custom double x = 1
        }

        def "B"
        {
            custom double x = 2
        }

        def "C"
        {
            custom double x = 3
        }
        )usda");
    TF_AXIOM(importedLayer);

    const UsdStageRefPtr stage = UsdStage::Open(layer);
    TF_AXIOM(stage);
    return stage;
}

// Tests that requests built with invalid keys are expired from the start.
static void
TestBuildRequestExpiration()
{
    const UsdStageRefPtr stage = CreateTestStage();
    ExecUsdSystem system(stage);
    InvalidationAccumulator accumulator;

    ExecUsdRequest request = system.BuildRequest({
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/A.x"))},
        ExecUsdValueKey{UsdAttribute()} },
        std::ref(accumulator));
    TF_AXIOM(!request.IsValid());
    // Note that we don't expect to receive any invalidation in this case
    // because the request has not been computed.
    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());
}

// Tests that unprepared requests expire when scene objects become invalid.
static void
TestUnpreparedRequestExpiration()
{
    UsdStageRefPtr stage = CreateTestStage();
    ExecUsdSystem system(stage);
    InvalidationAccumulator accumulator;

    ExecUsdRequest request = system.BuildRequest({
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/A.x"))},
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/B.x"))} },
        std::ref(accumulator));
    TF_AXIOM(request.IsValid());

    stage->RemovePrim(SdfPath("/B"));
    TF_AXIOM(!request.IsValid());
    // Note that we don't expect to receive any invalidation in this case
    // because the request has not been computed.
    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());

    {
        TfErrorMark m;
        system.PrepareRequest(request);
        TF_AXIOM(!m.IsClean());
        m.Clear();
    }

    {
        TfErrorMark m;
        system.Compute(request);
        TF_AXIOM(!m.IsClean());
        m.Clear();
    }
}

// Tests that prepared (but not computed) requests expire when scene objects
// become invalid.
//
static void
TestPreparedRequestExpiration()
{
    UsdStageRefPtr stage = CreateTestStage();
    ExecUsdSystem system(stage);
    InvalidationAccumulator accumulator;

    ExecUsdRequest request = system.BuildRequest({
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/A.x"))},
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/B.x"))} },
        std::ref(accumulator));
    TF_AXIOM(request.IsValid());
    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());

    {
        TfErrorMark m;
        system.PrepareRequest(request);
        TF_AXIOM(m.IsClean());
    }

    stage->RemovePrim(SdfPath("/B"));
    TF_AXIOM(!request.IsValid());
    // Note that we don't expect to receive any invalidation in this case
    // because the request has not been computed.
    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());

    {
        TfErrorMark m;
        system.Compute(request);
        TF_AXIOM(!m.IsClean());
        m.Clear();
    }
}

// Tests that computed requests expire and value invalidation callbacks are
// notified when scene objects become invalid.
//
static void
TestComputedRequestExpiration()
{
    UsdStageRefPtr stage = CreateTestStage();
    ExecUsdSystem system(stage);
    InvalidationAccumulator accumulator;

    ExecUsdRequest request = system.BuildRequest({
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/A.x"))},
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/B.x"))} },
        std::ref(accumulator));
    TF_AXIOM(request.IsValid());
    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());

    {
        TfErrorMark m;
        system.PrepareRequest(request);
        TF_AXIOM(m.IsClean());
        TF_AXIOM(accumulator.GetIndices().empty());
        TF_AXIOM(accumulator.GetInterval().IsEmpty());
    }

    {
        TfErrorMark m;
        system.Compute(request);
        TF_AXIOM(m.IsClean());
        TF_AXIOM(accumulator.GetIndices().empty());
        TF_AXIOM(accumulator.GetInterval().IsEmpty());
    }

    stage->RemovePrim(SdfPath("/B"));
    TF_AXIOM(!request.IsValid());

    TF_AXIOM(accumulator.GetIndices().size() == 1);
    TF_AXIOM(accumulator.GetIndices().count(1));
    TF_AXIOM(accumulator.GetInterval().IsFullInterval());
}

// Checks that a request is not expired when unrelated scene objects become
// invalid.
//
static void
TestUnrelatedSceneObject()
{
    UsdStageRefPtr stage = CreateTestStage();
    ExecUsdSystem system(stage);
    InvalidationAccumulator accumulator;

    ExecUsdRequest request = system.BuildRequest({
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/A.x"))},
        ExecUsdValueKey{stage->GetAttributeAtPath(SdfPath("/B.x"))} },
        std::ref(accumulator));
    TF_AXIOM(request.IsValid());
    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());

    system.Compute(request);

    stage->RemovePrim(SdfPath("/C"));
    TF_AXIOM(request.IsValid());

    TF_AXIOM(accumulator.GetIndices().empty());
    TF_AXIOM(accumulator.GetInterval().IsEmpty());
}

// Check that unexpired indices continue to receive invalidation even after
// another index has been expired.
//
static void
TestInvalidateUnexpiredIndex()
{
    UsdStageRefPtr stage = CreateTestStage();
    ExecUsdSystem system(stage);
    InvalidationAccumulator accumulator;

    UsdAttribute ax = stage->GetAttributeAtPath(SdfPath("/A.x"));
    UsdAttribute bx = stage->GetAttributeAtPath(SdfPath("/B.x"));
    UsdAttribute cx = stage->GetAttributeAtPath(SdfPath("/C.x"));

    ExecUsdRequest request = system.BuildRequest({
        ExecUsdValueKey{ax}, ExecUsdValueKey{bx}, ExecUsdValueKey{cx} },
        std::ref(accumulator));

    system.Compute(request);

    bx.Set(3.0);
    TF_AXIOM(accumulator.GetIndices().size() == 1);
    TF_AXIOM(accumulator.GetIndices().count(1));

    stage->RemovePrim(SdfPath("/C"));
    TF_AXIOM(accumulator.GetIndices().size() == 2);
    TF_AXIOM(accumulator.GetIndices().count(2));

    ax.Set(2.0);
    TF_AXIOM(accumulator.GetIndices().size() == 3);
    TF_AXIOM(accumulator.GetIndices().count(0));
}

int
main(int argc, char* argv[])
{
    TestBuildRequestExpiration();
    TestUnpreparedRequestExpiration();
    TestPreparedRequestExpiration();
    TestComputedRequestExpiration();
    TestUnrelatedSceneObject();
    TestInvalidateUnexpiredIndex();

    return 0;
}
