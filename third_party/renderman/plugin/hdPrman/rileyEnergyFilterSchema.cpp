//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/rileyEnergyFilterSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdPrmanRileyEnergyFilterSchemaTokens,
    HD_PRMAN_RILEY_ENERGY_FILTER_SCHEMA_TOKENS);

HdPrmanRileyShadingNodeVectorSchema
HdPrmanRileyEnergyFilterSchema::GetEnergyFilter()
#if HD_API_VERSION >= 66
                                            const
#else

#endif
{
    return HdPrmanRileyShadingNodeVectorSchema(_GetTypedDataSource<HdVectorDataSource>(
        HdPrmanRileyEnergyFilterSchemaTokens->energyFilter));
}

HdPrmanRileyParamListSchema
HdPrmanRileyEnergyFilterSchema::GetAttributes()
#if HD_API_VERSION >= 66
                                            const
#else

#endif
{
    return HdPrmanRileyParamListSchema(_GetTypedDataSource<HdContainerDataSource>(
        HdPrmanRileyEnergyFilterSchemaTokens->attributes));
}

/*static*/
HdContainerDataSourceHandle
HdPrmanRileyEnergyFilterSchema::BuildRetained(
        const HdVectorDataSourceHandle &energyFilter,
        const HdContainerDataSourceHandle &attributes
)
{
    TfToken _names[2];
    HdDataSourceBaseHandle _values[2];

    size_t _count = 0;

    if (energyFilter) {
        _names[_count] = HdPrmanRileyEnergyFilterSchemaTokens->energyFilter;
        _values[_count++] = energyFilter;
    }

    if (attributes) {
        _names[_count] = HdPrmanRileyEnergyFilterSchemaTokens->attributes;
        _values[_count++] = attributes;
    }
    return HdRetainedContainerDataSource::New(_count, _names, _values);
}

HdPrmanRileyEnergyFilterSchema::Builder &
HdPrmanRileyEnergyFilterSchema::Builder::SetEnergyFilter(
    const HdVectorDataSourceHandle &energyFilter)
{
    _energyFilter = energyFilter;
    return *this;
}

HdPrmanRileyEnergyFilterSchema::Builder &
HdPrmanRileyEnergyFilterSchema::Builder::SetAttributes(
    const HdContainerDataSourceHandle &attributes)
{
    _attributes = attributes;
    return *this;
}

HdContainerDataSourceHandle
HdPrmanRileyEnergyFilterSchema::Builder::Build()
{
    return HdPrmanRileyEnergyFilterSchema::BuildRetained(
        _energyFilter,
        _attributes
    );
}

/*static*/
HdPrmanRileyEnergyFilterSchema
HdPrmanRileyEnergyFilterSchema::GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer)
{
    return HdPrmanRileyEnergyFilterSchema(
        fromParentContainer
        ? HdContainerDataSource::Cast(fromParentContainer->Get(
                HdPrmanRileyEnergyFilterSchemaTokens->rileyEnergyFilter))
        : nullptr);
}

/*static*/
const TfToken &
HdPrmanRileyEnergyFilterSchema::GetSchemaToken()
{
    return HdPrmanRileyEnergyFilterSchemaTokens->rileyEnergyFilter;
}

/*static*/
const HdDataSourceLocator &
HdPrmanRileyEnergyFilterSchema::GetDefaultLocator()
{
    static const HdDataSourceLocator locator(GetSchemaToken());
    return locator;
}

/* static */
const HdDataSourceLocator &
HdPrmanRileyEnergyFilterSchema::GetEnergyFilterLocator()
{
    static const HdDataSourceLocator locator =
        GetDefaultLocator().Append(
            HdPrmanRileyEnergyFilterSchemaTokens->energyFilter);
    return locator;
}

/* static */
const HdDataSourceLocator &
HdPrmanRileyEnergyFilterSchema::GetAttributesLocator()
{
    static const HdDataSourceLocator locator =
        GetDefaultLocator().Append(
            HdPrmanRileyEnergyFilterSchemaTokens->attributes);
    return locator;
}

PXR_NAMESPACE_CLOSE_SCOPE
