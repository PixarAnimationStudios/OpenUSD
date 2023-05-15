//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/collectionAPIAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (usdCollections)
    (includes)
    (excludes)
    (expansionRule)
    (includeRoot)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCollectionAPIAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

namespace
{

class _CollectionContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CollectionContainerDataSource);

    _CollectionContainerDataSource(const UsdCollectionAPI &api,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _api(api)
    , _stageGlobals(stageGlobals){
    }

    TfTokenVector GetNames() override {
        TfTokenVector result;
        result.reserve(4);

        if (_api.GetExpansionRuleAttr()) {
            result.push_back(_tokens->expansionRule);
        }

        if (_api.GetIncludeRootAttr()) {
            result.push_back(_tokens->includeRoot);
        }

        if (_api.GetIncludesRel()) {
            result.push_back(_tokens->includes);
        }

        if (_api.GetExcludesRel()) {
            result.push_back(_tokens->excludes);
        }

        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _tokens->expansionRule) {
            if (UsdAttribute attr = _api.GetExpansionRuleAttr()) {
                return UsdImagingDataSourceAttributeNew(attr, _stageGlobals);
            }
        } else if (name == _tokens->includeRoot) {
             if (UsdAttribute attr = _api.GetIncludeRootAttr()) {
                // not intended to be time-varying so not providing a 
                // path/locator.
                return UsdImagingDataSourceAttributeNew(attr, _stageGlobals);
            }
        } else if (name == _tokens->includes) {
            if (UsdRelationship rel = _api.GetIncludesRel()) {
                return _BuildRelationshipDataSource(rel);
            }
        } else if (name == _tokens->excludes) {
            if (UsdRelationship rel = _api.GetExcludesRel()) {
                return _BuildRelationshipDataSource(rel);
            }
        }

        return nullptr;
    }

private:

    HdDataSourceBaseHandle _BuildRelationshipDataSource(
            const UsdRelationship &rel) {

        SdfPathVector targets;
        rel.GetForwardedTargets(&targets);
        if (targets.empty()) {
            return nullptr;
        }
        VtArray<SdfPath> targetArray(targets.begin(), targets.end());
        return HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
            targetArray);
    }


    UsdCollectionAPI _api;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_CollectionContainerDataSource);


class _CollectionsContainerDataSource : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_CollectionsContainerDataSource);

    _CollectionsContainerDataSource(
        const UsdPrim &prim,
        const TfToken &name,
        const UsdImagingDataSourceStageGlobals &stageGlobals
    ) : _api(prim, name)
    , _stageGlobals(stageGlobals) {
    }

    TfTokenVector GetNames() override {
        return {_api.GetName()};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _api.GetName()) {
            return _CollectionContainerDataSource::New(_api, _stageGlobals);
        }
        return nullptr;
    }

private:
    UsdCollectionAPI _api;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_CollectionsContainerDataSource);

} // anonymous namespace

// ----------------------------------------------------------------------------

HdContainerDataSourceHandle
UsdImagingCollectionAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        _tokens->usdCollections,
        _CollectionsContainerDataSource::New(
            prim, appliedInstanceName, stageGlobals)
    );
}

HdDataSourceLocatorSet
UsdImagingCollectionAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    std::string prefix = TfStringPrintf(
        "collections:%s:", appliedInstanceName.data());

    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), prefix)) {
            return HdDataSourceLocator(
                _tokens->usdCollections, appliedInstanceName);
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
