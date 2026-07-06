//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/irXformablePrimAdapter.h"

#include "pxr/usdImaging/usdExecImaging/requestBuilderInterface.h"
#include "pxr/usdImaging/usdExecImaging/computedDataSource.h"
#include "pxr/usdImaging/usdExecImaging/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

void
UsdExecImaging_IrXformablePrimAdapter::BuildRequest(
    const UsdPrim &prim,
    UsdExecImagingRequestBuilderInterface &requestBuilder) const
{
    requestBuilder.AddValueKey(
        prim.GetAttribute(ExecIrTokens->posedSpace));
}

HdContainerDataSourceHandle
UsdExecImaging_IrXformablePrimAdapter::GetPrimData(
    const SdfPath &primPath,
    const UsdExecImagingRequestAccessorInterfaceSharedPtr &requestAccessor)
        const
{
    return HdRetainedContainerDataSource::New(
        HdXformSchema::GetSchemaToken(),
        HdXformSchema::Builder()
            .SetResetXformStack(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .SetMatrix(
                UsdExecImagingComputedTypedSampledDataSource<GfMatrix4d>::New(
                    requestAccessor,
                    UsdExecImagingValueKey(
                        primPath.AppendProperty(
                            ExecIrTokens->posedSpace),
                        ExecBuiltinComputations->computeValue)))
            .Build());
}

void
UsdExecImaging_IrXformablePrimAdapter::InvalidatePrimData(
    const SdfPath &primPath,
    const UsdExecImagingValueKey &valueKey,
    HdDataSourceLocatorSet *const invalidLocators) const
{
    const UsdExecImagingValueKey posedSpaceValueKey(
        primPath.AppendProperty(ExecIrTokens->posedSpace),
        ExecBuiltinComputations->computeValue);

    // If the invalidated value key is for this prim's posed:space computeValue,
    // then this prim's xform/matrix data source is invalid.
    if (valueKey == posedSpaceValueKey) {
        invalidLocators->append(HdDataSourceLocator(
            HdXformSchema::GetSchemaToken(),
            HdXformSchemaTokens->matrix));
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
