//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/sphere.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStSphere::HdStSphere(SdfPath const &id)
  : HdStImplicitSurface(
        id,
        HdPrimTypeTokens->sphere,
        HdBufferSpecVector{HdBufferSpec(HdSphereSchemaTokens->radius,
                                        HdTupleType{HdTypeFloat, 1})})
{
}

void
HdStSphere::_PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                            HdRenderParam *renderParam,
                                            HdStDrawItem *drawItem)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr resourceRegistry =
    std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetConstantPrimvarRange();

    float radius = sceneDelegate->Get(GetId(),
                    HdSphereSchemaTokens->radius).Get<double>();

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdSphereSchemaTokens->radius, VtValue(radius))
    };

    resourceRegistry->AddSources(bar, std::move(sources));
}

PXR_NAMESPACE_CLOSE_SCOPE
