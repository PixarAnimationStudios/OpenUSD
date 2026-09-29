//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/conicalFrustum.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/coneSchema.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

template <bool IsCone>
HdStConicalFrustum<IsCone>::HdStConicalFrustum(SdfPath const &id)
  : HdStImplicitSurface(
        id,
        HdStNativeImplicitsTokens->conicalFrustum,
        HdBufferSpecVector{
            HdBufferSpec(HdStNativeImplicitsTokens->conicalFrustumHeight,
                        HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdStNativeImplicitsTokens->conicalFrustumRadiusTop,
                        HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdStNativeImplicitsTokens->conicalFrustumRadiusBot,
                        HdTupleType{HdTypeFloat, 1}),
            HdBufferSpec(HdStNativeImplicitsTokens->conicalFrustumAxis,
                        HdTupleType{HdTypeUInt32, 1})})
{
}


// Determine the schema tokens.
template <bool IsCone>
decltype(auto)
_GetSchemaTokens() {
    if constexpr (IsCone) {
        return (HdConeSchemaTokens);
    } else {
        return (HdCylinderSchemaTokens);
    }
}

template <bool IsCone>
void
HdStConicalFrustum<IsCone>::_PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                                            HdRenderParam *renderParam,
                                                            HdStDrawItem *drawItem)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetConstantPrimvarRange();

    auto const &Tokens = _GetSchemaTokens<IsCone>();

    VtValue const &heightVal = sceneDelegate->Get(GetId(), Tokens->height);

    const float height = static_cast<float>(heightVal.Get<double>());

    VtValue const &axisTokenVal = sceneDelegate->Get(GetId(), Tokens->axis);

    TfToken const &axisToken = axisTokenVal.Get<TfToken>();

    float radiusTop, radiusBot;

    if constexpr (IsCone) {
        radiusTop = 0.0f;

        VtValue const &radiusBotVal = sceneDelegate->Get(GetId(), Tokens->radius);
        radiusBot = static_cast<float>(radiusBotVal.Get<double>());
        
    } else {
        // radiusTop/radiusBottom are absent on the legacy UsdGeom Cylinder.
        // Fall back to the deprecated uniform radius in that case.
        VtValue const &radiusTopVal = sceneDelegate->Get(GetId(), Tokens->radiusTop);

        if (radiusTopVal.IsEmpty()) {
            VtValue const &radiusVal = sceneDelegate->Get(GetId(), Tokens->radius);
            const float radius = static_cast<float>(radiusVal.Get<double>());

            radiusTop = radius;
            radiusBot = radius;
        } else {
            radiusTop = static_cast<float>(radiusTopVal.Get<double>());

            VtValue const &radiusBotVal = sceneDelegate->Get(GetId(), Tokens->radiusBottom); 
            radiusBot = static_cast<float>(radiusBotVal.Get<double>());
        }
    }

    // Map the axis token to the uint the shader expects.
    // Default to Z if unset.
    uint32_t axis = 2;
    if (axisToken == Tokens->X) {
        axis = 0;
    } else if (axisToken == Tokens->Y) {
        axis = 1;
    }

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->conicalFrustumHeight, VtValue(height)),
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->conicalFrustumRadiusTop, VtValue(radiusTop)),
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->conicalFrustumRadiusBot, VtValue(radiusBot)),
        std::make_shared<HdVtBufferSource>(
            HdStNativeImplicitsTokens->conicalFrustumAxis, VtValue(axis))
    };

    resourceRegistry->AddSources(bar, std::move(sources));
}

template class HdStConicalFrustum</*IsCone=*/true>;
template class HdStConicalFrustum</*IsCone=*/false>;

PXR_NAMESPACE_CLOSE_SCOPE
