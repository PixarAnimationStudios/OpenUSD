//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/energyFilterSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdEnergyFilterSchemaTokens,
    HD_ENERGY_FILTER_SCHEMA_TOKENS);

HdMaterialNodeSchema
HdEnergyFilterSchema::GetResource() const
{
    return HdMaterialNodeSchema(_GetTypedDataSource<HdContainerDataSource>(
        HdEnergyFilterSchemaTokens->resource));
}

/*static*/
HdContainerDataSourceHandle
HdEnergyFilterSchema::BuildRetained(
        const HdContainerDataSourceHandle &resource
)
{
    TfToken _names[1];
    HdDataSourceBaseHandle _values[1];

    size_t _count = 0;

    if (resource) {
        _names[_count] = HdEnergyFilterSchemaTokens->resource;
        _values[_count++] = resource;
    }
    return HdRetainedContainerDataSource::New(_count, _names, _values);
}

HdEnergyFilterSchema::Builder &
HdEnergyFilterSchema::Builder::SetResource(
    const HdContainerDataSourceHandle &resource)
{
    _resource = resource;
    return *this;
}

HdContainerDataSourceHandle
HdEnergyFilterSchema::Builder::Build()
{
    return HdEnergyFilterSchema::BuildRetained(
        _resource
    );
}

/*static*/
HdEnergyFilterSchema
HdEnergyFilterSchema::GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer)
{
    return HdEnergyFilterSchema(
        fromParentContainer
        ? HdContainerDataSource::Cast(fromParentContainer->Get(
                HdEnergyFilterSchemaTokens->energyFilter))
        : nullptr);
}

/*static*/
const TfToken &
HdEnergyFilterSchema::GetSchemaToken()
{
    return HdEnergyFilterSchemaTokens->energyFilter;
}

/*static*/
const HdDataSourceLocator &
HdEnergyFilterSchema::GetDefaultLocator()
{
    static const HdDataSourceLocator locator(GetSchemaToken());
    return locator;
}

/* static */
const HdDataSourceLocator &
HdEnergyFilterSchema::GetResourceLocator()
{
    static const HdDataSourceLocator locator =
        GetDefaultLocator().Append(
            HdEnergyFilterSchemaTokens->resource);
    return locator;
}

PXR_NAMESPACE_CLOSE_SCOPE
