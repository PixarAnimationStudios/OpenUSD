//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/motionAPIAdapter.h"

#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/types.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingMotionAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingAPISchemaAdapterFactory<Adapter>>();
}

namespace {

const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetMotionPrimvarMappings()
{
    static const UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
        { HdTokens->blurScale,
          UsdGeomTokens->motionBlurScale,
          HdPrimvarSchemaTokens->constant },
        { HdTokens->nonlinearSampleCount,
          UsdGeomTokens->motionNonlinearSampleCount,
          HdPrimvarSchemaTokens->constant },
    };
    return mappings;
}

} // anonymous namespace

HdContainerDataSourceHandle
UsdImagingMotionAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    // MotionAPI is not a multi-apply schema.
    if (!appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    // This adapter only contributes to the primary prim, not subprims.
    if (!subprim.IsEmpty()) {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        HdPrimvarsSchema::GetSchemaToken(),
        UsdImagingDataSourceCustomPrimvars::New(
            prim.GetPath(),
            prim,
            _GetMotionPrimvarMappings(),
            stageGlobals));
}

HdDataSourceLocatorSet
UsdImagingMotionAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    if (!appliedInstanceName.IsEmpty() || !subprim.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    return UsdImagingDataSourceCustomPrimvars::Invalidate(
        properties, _GetMotionPrimvarMappings());
}

PXR_NAMESPACE_CLOSE_SCOPE
