//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/capsule.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/capsuleSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStCapsule::HdStCapsule(SdfPath const &id)
  : HdStImplicitSurface(
        id,
        HdPrimTypeTokens->capsule,
        HdBufferSpecVector{
            HdBufferSpec(HdCapsuleSchemaTokens->height,
                         HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdCapsuleSchemaTokens->radiusTop,
                         HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdCapsuleSchemaTokens->radiusBottom,
                         HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdCapsuleSchemaTokens->axis,
                         HdTupleType{HdTypeInt32, 1})})
{
}

void
HdStCapsule::_PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                             HdRenderParam *renderParam,
                                             HdStDrawItem *drawItem)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr resourceRegistry =
    std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetConstantPrimvarRange();

    const VtValue heightValue = sceneDelegate->Get(
        GetId(), HdCapsuleSchemaTokens->height);
    const VtValue radiusValue = sceneDelegate->Get(
        GetId(), HdCapsuleSchemaTokens->radius);
    const VtValue radiusTopValue = sceneDelegate->Get(
        GetId(), HdCapsuleSchemaTokens->radiusTop);
    const VtValue radiusBottomValue = sceneDelegate->Get(
        GetId(), HdCapsuleSchemaTokens->radiusBottom);
    const VtValue axisValue = sceneDelegate->Get(
        GetId(), HdCapsuleSchemaTokens->axis);

    const float height = heightValue.IsHolding<double>()
        ? static_cast<float>(heightValue.UncheckedGet<double>()) : 1.0f;
    const float legacyRadius = radiusValue.IsHolding<double>()
        ? static_cast<float>(radiusValue.UncheckedGet<double>()) : 0.5f;
    const float radiusTop = radiusTopValue.IsHolding<double>()
        ? static_cast<float>(radiusTopValue.UncheckedGet<double>())
        : legacyRadius;
    const float radiusBottom = radiusBottomValue.IsHolding<double>()
        ? static_cast<float>(radiusBottomValue.UncheckedGet<double>())
        : legacyRadius;

    const TfToken axis = axisValue.IsHolding<TfToken>()
        ? axisValue.UncheckedGet<TfToken>() : HdCapsuleSchemaTokens->Z;
    const int axisIndex = axis == HdCapsuleSchemaTokens->X ? 0
        : axis == HdCapsuleSchemaTokens->Y ? 1 : 2;

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdCapsuleSchemaTokens->height, VtValue(height)),
        std::make_shared<HdVtBufferSource>(
            HdCapsuleSchemaTokens->radiusTop, VtValue(radiusTop)),
        std::make_shared<HdVtBufferSource>(
            HdCapsuleSchemaTokens->radiusBottom, VtValue(radiusBottom)),
        std::make_shared<HdVtBufferSource>(
            HdCapsuleSchemaTokens->axis, VtValue(axisIndex))
    };

    resourceRegistry->AddSources(bar, std::move(sources));
}

PXR_NAMESPACE_CLOSE_SCOPE
