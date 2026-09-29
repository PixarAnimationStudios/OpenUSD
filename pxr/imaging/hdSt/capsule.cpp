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
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/capsuleSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStCapsule::HdStCapsule(SdfPath const &id)
  : HdStImplicitSurface(
        id,
        HdStNativeImplicitsTokens->capsule,
        HdBufferSpecVector{
            HdBufferSpec(HdStNativeImplicitsTokens->capsuleHeight,
                        HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdStNativeImplicitsTokens->capsuleRadiusTop,
                        HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdStNativeImplicitsTokens->capsuleRadiusBot,
                        HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdStNativeImplicitsTokens->capsuleAxis,
                        HdTupleType{HdTypeUInt32, 1})})
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

    float radiusTop, radiusBot;

    const float height = static_cast<float>(sceneDelegate->Get(GetId(),
                HdCapsuleSchemaTokens->height).Get<double>());

    // radiusTop/radiusBottom are absent on the legacy UsdGeom Capsule.
    // Fall back to the deprecated uniform radius in that case.
    VtValue const &radiusTopVal = sceneDelegate->Get(GetId(),
                                    HdCapsuleSchemaTokens->radiusTop);
    if (radiusTopVal.IsEmpty()) {
        const float radius = static_cast<float>(sceneDelegate->Get(GetId(),
                    HdCapsuleSchemaTokens->radius).Get<double>());
        radiusTop = radius;
        radiusBot = radius;
    } else {
        radiusTop = static_cast<float>(radiusTopVal.Get<double>());
        radiusBot = static_cast<float>(sceneDelegate->Get(GetId(),
                        HdCapsuleSchemaTokens->radiusBottom).Get<double>());
    }

    TfToken const &axisToken = sceneDelegate->Get(GetId(),
                    HdCapsuleSchemaTokens->axis).Get<TfToken>();

    // Map the axis token to the uint the shader expects.
    // Default to Z if unset.
    uint32_t axis = 2;
    if (axisToken == HdCapsuleSchemaTokens->X) {
        axis = 0;
    } else if (axisToken == HdCapsuleSchemaTokens->Y) {
        axis = 1;
    }

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->capsuleHeight, VtValue(height)),
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->capsuleRadiusTop, VtValue(radiusTop)),
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->capsuleRadiusBot, VtValue(radiusBot)),
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->capsuleAxis, VtValue(axis))
    };

    resourceRegistry->AddSources(bar, std::move(sources));
}

PXR_NAMESPACE_CLOSE_SCOPE
