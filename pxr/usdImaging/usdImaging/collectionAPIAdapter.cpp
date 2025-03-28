//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/collectionAPIAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/collectionMembershipQuery.h"

#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

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

    _CollectionContainerDataSource(const UsdCollectionAPI &api)
    : _api(api)
    {}

    TfTokenVector GetNames() override {
        static TfTokenVector names =
            { HdCollectionSchemaTokens->membershipExpression };
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdCollectionSchemaTokens->membershipExpression) {
            return
                HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                    _ComputePathExpressionFromCollection(_api));
        }

        return nullptr;
    }

private:
    static SdfPathExpression
    _ComputePathExpressionFromCollection(
        const UsdCollectionAPI &api)
    {
        // A collection can be either rule-based (includes, excludes, ..)
        // or path-expression-based.
        // Construct a query object to determine the flavor.
        const UsdCollectionMembershipQuery query = api.ComputeMembershipQuery();

        if (query.UsesPathExpansionRuleMap()) {
            const auto &ruleMap = query.GetAsPathExpansionRuleMap();
            return
                UsdComputePathExpressionFromCollectionMembershipQueryRuleMap(
                    ruleMap);
        }
        
        return api.ResolveCompleteMembershipExpression();
    }

    UsdCollectionAPI _api;
};

HD_DECLARE_DATASOURCE_HANDLES(_CollectionContainerDataSource);


class _CollectionsContainerDataSource : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_CollectionsContainerDataSource);

    _CollectionsContainerDataSource(
        const UsdPrim &prim,
        const TfToken &name)
    : _api(prim, name)
    {}

    TfTokenVector GetNames() override {
        return {_api.GetName()};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _api.GetName()) {
            return _CollectionContainerDataSource::New(_api);
        }
        return nullptr;
    }

private:
    UsdCollectionAPI _api;
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

    // Note: When multiple collections are present, we'll overlay the containers
    //       and thus aggregate the individual collection's.
    return HdRetainedContainerDataSource::New(
        HdCollectionsSchemaTokens->collections,
        _CollectionsContainerDataSource::New(prim, appliedInstanceName)
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
        "collection:%s:", appliedInstanceName.data());

    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), prefix)) {
            return HdDataSourceLocator(
                HdCollectionsSchemaTokens->collections, appliedInstanceName);
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
