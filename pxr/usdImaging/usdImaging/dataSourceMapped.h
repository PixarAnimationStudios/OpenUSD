//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MAPPED_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MAPPED_H

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingDataSourceMapped_Impl
{
using _ContainerMappingsSharedPtr = std::shared_ptr<struct _ContainerMappings>;
}

/// \class UsdImagingDataSourceMapped
///
/// A data source that is a (potentially nested) container for the values of
/// the attributes of a given UsdPrim.
///
/// That is, this container data source contains `HdSampledDataSource`s giving
/// the value of an attribute, either directly or in a nested fashion by
/// containing container data sources itself.
///
/// The mapping of attributes to locations in the (nested) container data
/// source can be specified through AttributeMapping. These locations are
/// relative to the UsdImagingDataSourceMapped itself. To compute the correct
/// data source locators for invalidation purposes, the location of the
/// UsdImagingDataSourceMapped needs to be given to AttributeMappings so that
/// absolute locators (that is relative to the prim data source) can be
/// computed.
///
class UsdImagingDataSourceMapped : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMapped);

    /// Base class to specify how a property on the given Usd prim maps to a
    /// data source in this (nested) container data source.
    struct PropertyMappingBase
    {
        /// Name of attribute on Usd Prim.
        ///
        TfToken usdName;

        /// Corresponding location in this data source.
        ///
        /// Has to be non-empty. If length is greater than one, nested
        /// container data sources will be created.
        ///
        HdDataSourceLocator hdLocator;
    };
    
    /// Signature of function to compute data source from attribute.
    ///
    /// This could be generalized to HdDataSourceBaseHandle but we use
    /// HdSampledDataSourceHandle as result instead so that we can use
    /// the UsdImagingDataSourceAttributeNew function pointer.
    ///
    using DataSourceAttributeFactory =
        HdSampledDataSourceHandle(const UsdAttribute &,
                                  const UsdImagingDataSourceStageGlobals &,
                                  const SdfPath &,
                                  const HdDataSourceLocator &);
    using DataSourceAttributeFactoryFn =
        std::function<DataSourceAttributeFactory>;
    using DataSourceAttributeFactoryPtr =
        DataSourceAttributeFactory *;

    /// Specify how an attribute on the given Usd prim maps to a data source in
    /// this (nested) container data source.
    struct AttributeMapping final : PropertyMappingBase
    {
        /// Function to compute data source from UsdAttribute.
        ///
        /// Defaults to the appropriate overload of
        /// UsdImagingDataSourceAttributeNew.
        ///
        /// Clients can implement custom behavior. For example, following
        /// the connection of a UsdShadeOutput and return the path of the
        /// connected prim.
        ///
        DataSourceAttributeFactoryFn factory =
            DataSourceAttributeFactoryPtr(
                UsdImagingDataSourceAttributeNew);
    };

    using DataSourceRelationshipFactory =
        HdDataSourceBaseHandle(const UsdRelationship &,
                               const UsdImagingDataSourceStageGlobals &,
                               const SdfPath &,
                               const HdDataSourceLocator &);
    using DataSourceRelationshipFactoryFn =
        std::function<DataSourceRelationshipFactory>;
    using DataSourceRelationshipFactoryPtr =
        DataSourceRelationshipFactory *;

    USDIMAGING_API
    static
    const DataSourceRelationshipFactoryFn&
    GetPathFromRelationshipDataSourceFactory();

    USDIMAGING_API
    static
    const DataSourceRelationshipFactoryFn&
    GetPathArrayFromRelationshipDataSourceFactory();

    /// Specify how a relationship on the given Usd prim maps to a data source
    /// in this (nested) container data source.
    struct RelationshipMapping final : PropertyMappingBase
    {
        /// Function to compute data source from UsdRelationship.
        DataSourceRelationshipFactoryFn factory;
    };

    using PropertyMapping =
        std::variant<AttributeMapping, RelationshipMapping>;
    
    /// Specify how attributes on given Usd prim maps to data sources in
    /// this (nested) container data source.
    class PropertyMappings final
    {
    public:
        /// dataSourcePrefix is the location of this
        /// UsdImagingDataSourceMapped within a prim data source.
        USDIMAGING_API
        PropertyMappings(
            const std::vector<PropertyMapping> &mappings,
            const HdDataSourceLocator &datasourcePrefix);

        USDIMAGING_API
        ~PropertyMappings();
        
    private:
        friend class UsdImagingDataSourceMapped;

        using _ContainerMappingsSharedPtr =
            UsdImagingDataSourceMapped_Impl::_ContainerMappingsSharedPtr;

        // Flat list with absolute locators for invalidation.
        std::vector<PropertyMappingBase> _absoluteMappings;
        // Nested list to implement HdContainerDataSource::Get.
        _ContainerMappingsSharedPtr _containerMappings;
    };

    USDIMAGING_API
    TfTokenVector GetNames() override;

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(const TfTokenVector &usdNames,
               const PropertyMappings &mappings);

    USDIMAGING_API
    ~UsdImagingDataSourceMapped() override;

private:
    using _ContainerMappingsSharedPtr =
        UsdImagingDataSourceMapped_Impl::_ContainerMappingsSharedPtr;

    USDIMAGING_API
    UsdImagingDataSourceMapped(
        UsdPrim const &usdPrim,
        const SdfPath &sceneIndexPath,
        const PropertyMappings &mappings,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

    UsdImagingDataSourceMapped(
        UsdPrim const &usdPrim,
        const SdfPath &sceneIndexPath,
        const _ContainerMappingsSharedPtr &containerMappings,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

    UsdPrim _usdPrim;
    const SdfPath _sceneIndexPath;
    _ContainerMappingsSharedPtr const _containerMappings;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
