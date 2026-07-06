//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/geomXformablePrimAdapter.h"

#include "pxr/usdImaging/usdExecImaging/computedDataSource.h"
#include "pxr/usdImaging/usdExecImaging/requestAccessorInterface.h"
#include "pxr/usdImaging/usdExecImaging/requestBuilderInterface.h"
#include "pxr/usdImaging/usdExecImaging/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/exec/execGeom/tokens.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

// Get a retained data source that provides the value of xform/resetXformStack.
// Our computed transforms are always in world space, so this data source should
// always produce a value of true.
static HdBoolDataSourceHandle
_GetResetXformStackDataSource()
{
    static auto dataSource = HdRetainedTypedSampledDataSource<bool>::New(true);
    return dataSource;
}

void
UsdExecImaging_GeomXformablePrimAdapter::BuildRequest(
    const UsdPrim &prim,
    UsdExecImagingRequestBuilderInterface &requestBuilder) const
{
    // Add a value key for computing this prim's local-to-world transform.
    requestBuilder.AddValueKey(
        prim, ExecGeomXformableTokens->computeLocalToWorldTransform);
}

HdContainerDataSourceHandle
UsdExecImaging_GeomXformablePrimAdapter::GetPrimData(
    const SdfPath &primPath,
    const UsdExecImagingRequestAccessorInterfaceSharedPtr &requestAccessor)
        const
{
    // Produce a data source conforming to HdXformSchema. The 'matrix' value of
    // the xform schema gets its value from the exec request.
    return HdRetainedContainerDataSource::New(
        HdXformSchema::GetSchemaToken(),
        HdXformSchema::Builder()
            .SetMatrix(
                UsdExecImagingComputedTypedSampledDataSource<GfMatrix4d>::New(
                    requestAccessor,
                    UsdExecImagingValueKey(
                        primPath,
                        ExecGeomXformableTokens->computeLocalToWorldTransform)))
            .SetResetXformStack(_GetResetXformStackDataSource())
            .Build());
}

void
UsdExecImaging_GeomXformablePrimAdapter::InvalidatePrimData(
    const SdfPath &primPath,
    const UsdExecImagingValueKey &valueKey,
    HdDataSourceLocatorSet *const invalidLocators) const
{
    const UsdExecImagingValueKey localToWorldTransformValueKey(
        primPath, ExecGeomXformableTokens->computeLocalToWorldTransform);

    // If the invalidated value key is for computeLocalToWorldTransform on this
    // prim, then this prim's xform/matrix data source is invalid.
    if (valueKey == localToWorldTransformValueKey) {
        invalidLocators->append(HdDataSourceLocator(
            HdXformSchemaTokens->xform,
            HdXformSchemaTokens->matrix));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
