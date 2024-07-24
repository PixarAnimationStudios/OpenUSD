//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#include "pxr/usdImaging/usdImaging/extentsHintSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdImagingExtentsHintSchemaTokens,
    USDIMAGINGEXTENTSHINT_SCHEMA_TOKENS);


HdExtentSchema
UsdImagingExtentsHintSchema::GetExtent(const TfToken &purpose)
{
    return HdExtentSchema(_GetTypedDataSource<HdContainerDataSource>(purpose));
}

/*static*/
HdContainerDataSourceHandle
UsdImagingExtentsHintSchema::BuildRetained(
    const size_t count,
    const TfToken * const names,
    const HdDataSourceBaseHandle * const values)
{
    return HdRetainedContainerDataSource::New(count, names, values);
}

/*static*/
UsdImagingExtentsHintSchema
UsdImagingExtentsHintSchema::GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer)
{
    return UsdImagingExtentsHintSchema(
        fromParentContainer
        ? HdContainerDataSource::Cast(
            fromParentContainer->Get(
                GetSchemaToken()))
        : nullptr);
}

/*static*/
const TfToken &
UsdImagingExtentsHintSchema::GetSchemaToken()
{
    return UsdImagingExtentsHintSchemaTokens->extentsHint;
}

/*static*/
const HdDataSourceLocator &
UsdImagingExtentsHintSchema::GetDefaultLocator()
{
    static const HdDataSourceLocator locator(
        UsdImagingExtentsHintSchemaTokens->extentsHint
    );
    return locator;
} 

PXR_NAMESPACE_CLOSE_SCOPE
