//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/cylinder.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStCylinder::HdStCylinder(SdfPath const &id)
  : HdStImplicitSurface(
        id,
        HdPrimTypeTokens->cylinder,
        HdBufferSpecVector{
            HdBufferSpec(HdCylinderSchemaTokens->height,
                         HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdCylinderSchemaTokens->radiusTop,
                         HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdCylinderSchemaTokens->radiusBottom,
                         HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdCylinderSchemaTokens->axis,
                         HdTupleType{HdTypeInt32, 1})})
{
}

void
HdStCylinder::_PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
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
        GetId(), HdCylinderSchemaTokens->height);
    const VtValue radiusValue = sceneDelegate->Get(
        GetId(), HdCylinderSchemaTokens->radius);
    const VtValue radiusTopValue = sceneDelegate->Get(
        GetId(), HdCylinderSchemaTokens->radiusTop);
    const VtValue radiusBottomValue = sceneDelegate->Get(
        GetId(), HdCylinderSchemaTokens->radiusBottom);
    const VtValue axisValue = sceneDelegate->Get(
        GetId(), HdCylinderSchemaTokens->axis);

    const float height = heightValue.IsHolding<double>()
        ? static_cast<float>(heightValue.UncheckedGet<double>()) : 2.0f;
    const float legacyRadius = radiusValue.IsHolding<double>()
        ? static_cast<float>(radiusValue.UncheckedGet<double>()) : 1.0f;
    const float radiusTop = radiusTopValue.IsHolding<double>()
        ? static_cast<float>(radiusTopValue.UncheckedGet<double>())
        : legacyRadius;
    const float radiusBottom = radiusBottomValue.IsHolding<double>()
        ? static_cast<float>(radiusBottomValue.UncheckedGet<double>())
        : legacyRadius;

    const TfToken axis = axisValue.IsHolding<TfToken>()
        ? axisValue.UncheckedGet<TfToken>() : HdCylinderSchemaTokens->Z;
    const int axisIndex = axis == HdCylinderSchemaTokens->X ? 0
        : axis == HdCylinderSchemaTokens->Y ? 1 : 2;

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdCylinderSchemaTokens->height, VtValue(height)),
        std::make_shared<HdVtBufferSource>(
            HdCylinderSchemaTokens->radiusTop, VtValue(radiusTop)),
        std::make_shared<HdVtBufferSource>(
            HdCylinderSchemaTokens->radiusBottom, VtValue(radiusBottom)),
        std::make_shared<HdVtBufferSource>(
            HdCylinderSchemaTokens->axis, VtValue(axisIndex))
    };

    resourceRegistry->AddSources(bar, std::move(sources));
}

PXR_NAMESPACE_CLOSE_SCOPE
