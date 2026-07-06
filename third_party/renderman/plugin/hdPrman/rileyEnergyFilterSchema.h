//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_ENERGY_FILTER_SCHEMA_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_ENERGY_FILTER_SCHEMA_H

/// \file

#include "hdPrman/api.h"
#include "hdPrman/rileySchemaTypeDefs.h"
#include "hdPrman/rileyParamListSchema.h"

#include "pxr/imaging/hd/schema.h"
#include "pxr/imaging/hd/version.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_PRMAN_RILEY_ENERGY_FILTER_SCHEMA_TOKENS \
    (rileyEnergyFilter) \
    (energyFilter) \
    (attributes) \

TF_DECLARE_PUBLIC_TOKENS(HdPrmanRileyEnergyFilterSchemaTokens, HDPRMAN_API,
    HD_PRMAN_RILEY_ENERGY_FILTER_SCHEMA_TOKENS);

//-----------------------------------------------------------------------------

class HdPrmanRileyEnergyFilterSchema : public HdSchema
{
public:
    HdPrmanRileyEnergyFilterSchema(HdContainerDataSourceHandle container)
      : HdSchema(container) {}

    HDPRMAN_API
    static HdPrmanRileyEnergyFilterSchema GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer);

    HDPRMAN_API
    HdPrmanRileyShadingNodeVectorSchema GetEnergyFilter()
#if HD_API_VERSION >= 66
                                            const;
#else
                                                 ;
#endif

    HDPRMAN_API
    HdPrmanRileyParamListSchema GetAttributes()
#if HD_API_VERSION >= 66
                                            const;
#else
                                                 ;
#endif

    HDPRMAN_API
    static const TfToken &GetSchemaToken();

    HDPRMAN_API
    static const HdDataSourceLocator &GetDefaultLocator();

    HDPRMAN_API
    static const HdDataSourceLocator &GetEnergyFilterLocator();

    HDPRMAN_API
    static const HdDataSourceLocator &GetAttributesLocator();

    HDPRMAN_API
    static HdContainerDataSourceHandle
    BuildRetained(
        const HdVectorDataSourceHandle &energyFilter,
        const HdContainerDataSourceHandle &attributes
    );

    class Builder
    {
    public:
        HDPRMAN_API
        Builder &SetEnergyFilter(
            const HdVectorDataSourceHandle &energyFilter);
        HDPRMAN_API
        Builder &SetAttributes(
            const HdContainerDataSourceHandle &attributes);

        HDPRMAN_API
        HdContainerDataSourceHandle Build();

    private:
        HdVectorDataSourceHandle _energyFilter;
        HdContainerDataSourceHandle _attributes;
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
