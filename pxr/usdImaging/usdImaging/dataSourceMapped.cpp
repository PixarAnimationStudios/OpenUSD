//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/usd/usd/relationship.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingDataSourceMapped_Impl
{

using AttributeMapping =
    UsdImagingDataSourceMapped::AttributeMapping;
using RelationshipMapping =
    UsdImagingDataSourceMapped::RelationshipMapping;
using DataSourceRelationshipFactoryFn =
    UsdImagingDataSourceMapped::DataSourceRelationshipFactoryFn;

// For a given location within UsdImagingDataSourceMapped,
// information what data source to create.
//
// Such a data source is either an HdSampledDataSource giving
// the value of an Usd attribute. Or a container data source.
using _DataSourceInfo =
    std::variant<AttributeMapping,
                 RelationshipMapping,
                 _ContainerMappingsSharedPtr>;

// Data for a container data source.
struct _ContainerMappings
{
    // Names within the container data source - sorted lexicographically.
    TfTokenVector hdNames;
    // What data source should be returned for the above name.
    // This vector runs in parallel with hdNames.
    //
    // _DataSourceInfo can store another _ContainerMappings so this
    // data structure is a tree mirroring the tree of given data source
    // locators.
    std::vector<_DataSourceInfo> dataSourceInfos;
};

HdDataSourceBaseHandle
_PathFromRelationshipFactory(
    const UsdRelationship &rel,
    const UsdImagingDataSourceStageGlobals &,
    const SdfPath &,
    const HdDataSourceLocator &)
{
    SdfPathVector result;
    rel.GetForwardedTargets(&result);
    if (result.empty()) {
        return nullptr;
    }
    return HdRetainedTypedSampledDataSource<SdfPath>::New(std::move(result[0]));
}

VtArray<SdfPath>
_ToArray(SdfPathVector &&v)
{
    return VtArray<SdfPath>(
        std::make_move_iterator(v.begin()),
        std::make_move_iterator(v.end()));
}

HdDataSourceBaseHandle
_PathArrayFromRelationshipFactory(
    const UsdRelationship &rel,
    const UsdImagingDataSourceStageGlobals &,
    const SdfPath &,
    const HdDataSourceLocator &)
{
    SdfPathVector result;
    rel.GetForwardedTargets(&result);
    return
        HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
            _ToArray(std::move(result)));
}

}
   
using namespace UsdImagingDataSourceMapped_Impl;

/* static */
const DataSourceRelationshipFactoryFn&
UsdImagingDataSourceMapped::GetPathFromRelationshipDataSourceFactory()
{
    static const DataSourceRelationshipFactoryFn result(
        _PathFromRelationshipFactory);
    return result;
}

/* static */
const DataSourceRelationshipFactoryFn&
UsdImagingDataSourceMapped::GetPathArrayFromRelationshipDataSourceFactory()
{
    static const DataSourceRelationshipFactoryFn result(
        _PathArrayFromRelationshipFactory);
    return result;
}

UsdImagingDataSourceMapped::UsdImagingDataSourceMapped(
    UsdPrim const &usdPrim,
    const SdfPath &sceneIndexPath,
    const PropertyMappings &mappings,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
  : UsdImagingDataSourceMapped(
      usdPrim,
      sceneIndexPath,
      mappings._containerMappings,
      stageGlobals)
{
}

UsdImagingDataSourceMapped::UsdImagingDataSourceMapped(
    UsdPrim const &usdPrim,
    const SdfPath &sceneIndexPath,
    const _ContainerMappingsSharedPtr &containerMappings,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _usdPrim(usdPrim)
  , _sceneIndexPath(sceneIndexPath)
  , _containerMappings(containerMappings)
  , _stageGlobals(stageGlobals)
{
}

UsdImagingDataSourceMapped::~UsdImagingDataSourceMapped() = default;

TfTokenVector
UsdImagingDataSourceMapped::GetNames()
{
    return _containerMappings->hdNames;
}

HdDataSourceBaseHandle
UsdImagingDataSourceMapped::Get(const TfToken &name)
{
    // Look for name in hdNames.

    const auto itName = std::lower_bound(
        _containerMappings->hdNames.begin(),
        _containerMappings->hdNames.end(),
        name);

    if (!(itName != _containerMappings->hdNames.end() &&
          *itName == name)) {
        return nullptr;
    }
    
    const auto itInfo =
        _containerMappings->dataSourceInfos.begin() +
        (itName - _containerMappings->hdNames.begin()); 

    const _DataSourceInfo &info = *itInfo;
    if (auto const attrMapping = std::get_if<AttributeMapping>(&info)) {
        // We create a data source from the UsdAttribute.
        const UsdAttribute attr = _usdPrim.GetAttribute(attrMapping->usdName);
        if (!attr) {
            TF_CODING_ERROR(
                "Expected usd attribute '%s' on prim '%s' to serve "
                "data source at locator '%s'.",
                attrMapping->usdName.GetText(),
                _usdPrim.GetPath().GetText(),
                attrMapping->hdLocator.GetString().c_str());
            return nullptr;
        }
        if (!attrMapping->factory) {
            TF_CODING_ERROR(
                "No factory given to convert usd attribute '%s' on prim '%s' "
                "to serve data source at locator '%s'.",
                attrMapping->usdName.GetText(),
                _usdPrim.GetPath().GetText(),
                attrMapping->hdLocator.GetString().c_str());
            return nullptr;
        }
        return attrMapping->factory(
            attr, _stageGlobals, _sceneIndexPath, attrMapping->hdLocator);
    } else if (auto const relMapping =
                                std::get_if<RelationshipMapping>(&info)) {
        const UsdRelationship rel =
            _usdPrim.GetRelationship(relMapping->usdName);
        if (!rel) {
            TF_CODING_ERROR(
                "Expected usd relationship '%s' on prim '%s' to serve "
                "data source at locator '%s'.",
                relMapping->usdName.GetText(),
                _usdPrim.GetPath().GetText(),
                relMapping->hdLocator.GetString().c_str());
            return nullptr;
        }
        if (!relMapping->factory) {
            TF_CODING_ERROR(
                "No factory given to convert usd relationship '%s' on prim '%s' "
                "to serve data source at locator '%s'.",
                relMapping->usdName.GetText(),
                _usdPrim.GetPath().GetText(),
                relMapping->hdLocator.GetString().c_str());
            return nullptr;
        }
        return relMapping->factory(
            rel, _stageGlobals, _sceneIndexPath, relMapping->hdLocator);
    } else {
        // We are in the nested case.
        return UsdImagingDataSourceMapped::New(
            _usdPrim,
            _sceneIndexPath,
            std::get<_ContainerMappingsSharedPtr>(info),
            _stageGlobals);
    }
}

HdDataSourceLocatorSet
UsdImagingDataSourceMapped::Invalidate(
    const TfTokenVector &usdNames, const PropertyMappings &mappings)
{
    HdDataSourceLocatorSet locators;
    
    for (const TfToken &usdName : usdNames) {
        for (const PropertyMappingBase &mapping : mappings._absoluteMappings) {
            if (mapping.usdName == usdName) {
                locators.insert(mapping.hdLocator);
            }
        }
    }

    return locators;
}

static
TfToken
_GetUsdName(const _DataSourceInfo &info)
{
    if (auto const attrMapping = std::get_if<AttributeMapping>(&info)) {
        return attrMapping->usdName;
    }
    if (auto const relMapping = std::get_if<RelationshipMapping>(&info)) {
        return relMapping->usdName;
    }
    return TfToken("<UNKNOWN>");
}

// Find or add name as key to given containerMappings - returning the
// _ContainerMappings at that key.
static
_ContainerMappingsSharedPtr
_FindOrCreateChild(const TfToken &name,
                  _ContainerMappingsSharedPtr const &containerMappings)
{
    const auto itName = std::lower_bound(
        containerMappings->hdNames.begin(),
        containerMappings->hdNames.end(),
        name);
    const auto itInfo =
        containerMappings->dataSourceInfos.begin() +
        (itName - containerMappings->hdNames.begin()); 

    if (itName != containerMappings->hdNames.end() && *itName == name) {
        const _DataSourceInfo &info = *itInfo;
        auto * const child = std::get_if<_ContainerMappingsSharedPtr>(&info);
        if (!child) {
            TF_CODING_ERROR(
                "Adding data source locator when there was already an "
                "ascendant locator added for a Usd attribute with name '%s'.",
                _GetUsdName(info).GetText());
            return nullptr;
        }
        return *child;
    }

    // Create new item.
    _ContainerMappingsSharedPtr child = std::make_shared<_ContainerMappings>();

    containerMappings->hdNames.insert(itName, name);
    containerMappings->dataSourceInfos.insert(itInfo, child);

    return child;
}

// Add mapping to containerMappins at given locator.
static
void
_Add(const HdDataSourceLocator &locator,
     _DataSourceInfo &&info,
     _ContainerMappingsSharedPtr containerMappings)
{
    const size_t n = locator.GetElementCount();
    if (n == 0) {
        TF_CODING_ERROR("Expected non-trivial data source locator for "
                        "attribute %s.", _GetUsdName(info).GetText());
        return;
    }

    // Find or create the nested container data sources.
    for (size_t i = 0; i < n - 1; ++i) {
        const TfToken &hdName = locator.GetElement(i);
        containerMappings = _FindOrCreateChild(hdName, containerMappings);
        if (!containerMappings) {
            return;
        }
    }

    const TfToken &name = locator.GetLastElement();
    
    // And add the AttributeMapping as leaf.
    const auto itName = std::lower_bound(
        containerMappings->hdNames.begin(),
        containerMappings->hdNames.end(),
        name);
    const auto itInfo =
        containerMappings->dataSourceInfos.begin() +
        (itName - containerMappings->hdNames.begin()); 

    containerMappings->hdNames.insert(itName, name);
    containerMappings->dataSourceInfos.insert(itInfo, std::move(info));
}

UsdImagingDataSourceMapped::PropertyMappings::PropertyMappings(
    const std::vector<PropertyMapping> &mappings,
    const HdDataSourceLocator &dataSourcePrefix)
{
    _absoluteMappings.reserve(mappings.size());
    _containerMappings = std::make_shared<_ContainerMappings>();

    for (const PropertyMapping &mapping : mappings) {
        if (auto const attrMapping = std::get_if<AttributeMapping>(&mapping)) {
            // Making locator absolute. 
           const HdDataSourceLocator locator =
                dataSourcePrefix.Append(attrMapping->hdLocator);
            _absoluteMappings.push_back(
                { attrMapping->usdName, locator });
            _Add(
                attrMapping->hdLocator,
                AttributeMapping{
                    attrMapping->usdName, locator, attrMapping->factory },
                _containerMappings);
        } else if (auto const &relMapping =
                                std::get_if<RelationshipMapping>(&mapping)) {
            // Making locator absolute. 
           const HdDataSourceLocator locator =
                dataSourcePrefix.Append(relMapping->hdLocator);
            _absoluteMappings.push_back(
                { relMapping->usdName, locator });
            _Add(
                relMapping->hdLocator,
                RelationshipMapping{
                    relMapping->usdName, locator, relMapping->factory },
                _containerMappings);
        }
    }
}

UsdImagingDataSourceMapped::PropertyMappings::~PropertyMappings() = default;

PXR_NAMESPACE_CLOSE_SCOPE
