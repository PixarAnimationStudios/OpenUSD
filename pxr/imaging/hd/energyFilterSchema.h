//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_ENERGY_FILTER_SCHEMA_H
#define PXR_IMAGING_HD_ENERGY_FILTER_SCHEMA_H

/// \file

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/materialNodeSchema.h"

#include "pxr/imaging/hd/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_ENERGY_FILTER_SCHEMA_TOKENS \
    (energyFilter) \
    (resource) \

TF_DECLARE_PUBLIC_TOKENS(HdEnergyFilterSchemaTokens, HD_API,
    HD_ENERGY_FILTER_SCHEMA_TOKENS);

//-----------------------------------------------------------------------------

/// \class HdEnergyFilterSchema
///
class HdEnergyFilterSchema : public HdSchema
{
public:
    /// \name Schema retrieval
    /// @{

    HdEnergyFilterSchema(HdContainerDataSourceHandle container)
      : HdSchema(container) {}

    /// Retrieves a container data source with the schema's default name token
    /// "energyFilter" from the parent container and constructs a
    /// HdEnergyFilterSchema instance.
    /// Because the requested container data source may not exist, the result
    /// should be checked with IsDefined() or a bool comparison before use.
    HD_API
    static HdEnergyFilterSchema GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer);

    /// @}

    /// \name Member accessor
    /// @{

    HD_API
    HdMaterialNodeSchema GetResource() const;

    /// @}

    /// \name Schema location
    /// @{

    HD_API
    static const TfToken &GetSchemaToken();

    HD_API
    static const HdDataSourceLocator &GetDefaultLocator();

    /// @}

    /// \name Data source locators for members
    /// @{

    HD_API
    static const HdDataSourceLocator &GetResourceLocator();
    /// @}

    /// \name Schema construction
    /// @{

    HD_API
    static HdContainerDataSourceHandle
    BuildRetained(
        const HdContainerDataSourceHandle &resource
    );

    /// \class HdEnergyFilterSchema::Builder
    class Builder
    {
    public:
        HD_API
        Builder &SetResource(
            const HdContainerDataSourceHandle &resource);

        HD_API
        HdContainerDataSourceHandle Build();

    private:
        HdContainerDataSourceHandle _resource;
    };

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
