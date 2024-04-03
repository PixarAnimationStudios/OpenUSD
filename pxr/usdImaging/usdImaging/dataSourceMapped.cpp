//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingDataSourceMapped_Impl
{

// For a given location within UsdImagingDataSourceMapped,
// information what data source to create.
//
// Such a data source is either an HdSampledDataSource giving
// the value of an Usd attribute. Or a container data source.
using _DataSourceInfo =
    std::variant<UsdImagingDataSourceMapped::AttributeMapping,
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

}

using namespace UsdImagingDataSourceMapped_Impl;

UsdImagingDataSourceMapped::UsdImagingDataSourceMapped(
    UsdPrim const &usdPrim,
    const SdfPath &sceneIndexPath,
    const AttributeMappings &mappings,
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
    if (auto const mapping = std::get_if<AttributeMapping>(&info)) {
        // We create a data source from the UsdAttribute.
        UsdAttribute attr = _usdPrim.GetAttribute(mapping->usdName);
        if (!attr) {
            TF_CODING_ERROR(
                "Expected usd attribute '%s' on prim '%s' to serve "
                "data source at locator '%s'.",
                mapping->usdName.GetText(),
                _usdPrim.GetPath().GetText(),
                mapping->hdLocator.GetString().c_str());
            return nullptr;
        }
        return mapping->factory(
            attr, _stageGlobals, _sceneIndexPath, mapping->hdLocator);
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
    const TfTokenVector &usdNames, const AttributeMappings &mappings)
{
    HdDataSourceLocatorSet locators;
    
    for (const TfToken &usdName : usdNames) {
        for (const AttributeMapping &mapping : mappings._absoluteMappings) {
            if (mapping.usdName == usdName) {
                locators.insert(mapping.hdLocator);
            }
        }
    }

    return locators;
}

// Find or add name as key to given containerMappings - returning the
// _ContainerMappings at that key.
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
            const auto &mapping = 
                std::get<UsdImagingDataSourceMapped::AttributeMapping>(info);
            TF_CODING_ERROR(
                "Adding data source locator when there was already an "
                "ascendant locator added for a Usd attribute with name '%s'.",
                mapping.usdName.GetText());
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
void
_Add(const HdDataSourceLocator &locator,
     const UsdImagingDataSourceMapped::AttributeMapping &mapping,
     _ContainerMappingsSharedPtr containerMappings)
{
    const size_t n = locator.GetElementCount();
    if (n == 0) {
        TF_CODING_ERROR("Expected non-trivial data source locator for "
                        "attribute %s.", mapping.usdName.GetText());
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

    const TfToken &name = mapping.hdLocator.GetLastElement();
    
    // And add the AttributeMapping as leaf.
    const auto itName = std::lower_bound(
        containerMappings->hdNames.begin(),
        containerMappings->hdNames.end(),
        name);
    const auto itInfo =
        containerMappings->dataSourceInfos.begin() +
        (itName - containerMappings->hdNames.begin()); 

    containerMappings->hdNames.insert(itName, name);
    containerMappings->dataSourceInfos.insert(itInfo, mapping);
}

UsdImagingDataSourceMapped::AttributeMappings::AttributeMappings(
    const std::vector<AttributeMapping> &mappings,
    const HdDataSourceLocator &dataSourcePrefix)
{
    _absoluteMappings.reserve(mappings.size());
    _containerMappings = std::make_shared<_ContainerMappings>();

    for (const AttributeMapping &mapping : mappings) {
        // Making locator absolute.
        const AttributeMapping absoluteMapping{
            mapping.usdName,
            dataSourcePrefix.Append(mapping.hdLocator),
            mapping.factory};

        _absoluteMappings.push_back(absoluteMapping);

        _Add(mapping.hdLocator, absoluteMapping, _containerMappings);
    }
}

UsdImagingDataSourceMapped::AttributeMappings::~AttributeMappings() = default;

PXR_NAMESPACE_CLOSE_SCOPE
