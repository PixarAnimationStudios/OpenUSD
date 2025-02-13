//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/materialBindingAPIAdapter.h"

#include "pxr/usdImaging/usdImaging/collectionMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/collectionMaterialBindingsSchema.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingsSchema.h"

#include "pxr/usd/usdShade/materialBindingAPI.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((materialNamespace, "material:"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMaterialBindingAPIAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

namespace
{

template <typename T>
using _RetainedTypedDs = HdRetainedTypedSampledDataSource<T>;

class _CollectionMaterialBindingsContainerDataSource : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_CollectionMaterialBindingsContainerDataSource);

    _CollectionMaterialBindingsContainerDataSource(const UsdPrim &prim)
    : _mbApi(prim) {
    }

    TfTokenVector GetNames() override {
        // XXX This returns all the possible values for material purpose
        //     instead of just the ones for which material bindings are
        //     authored on the prim.
        return _mbApi.GetMaterialPurposes();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        const TfToken &purpose = name;
        return _BuildCollectionBindingsVectorDataSource(purpose);
    }

private:

    HdDataSourceBaseHandle
    _BuildCollectionBindingsVectorDataSource(
        const TfToken &purpose) const
    {
        using _CollectionBindings =
            UsdShadeMaterialBindingAPI::CollectionBindingVector;

        _CollectionBindings bindings = _mbApi.GetCollectionBindings(purpose);
        if (bindings.empty()) {
            return nullptr;
        }

        std::vector<HdDataSourceBaseHandle> bindingsDs;
        bindingsDs.reserve(bindings.size());

        const auto thisPrimPathDs =
            _RetainedTypedDs<SdfPath>::New(_mbApi.GetPath());

        for (auto const &binding : bindings) {
            if (binding.IsValid()) {
                auto const &b = binding;
                bindingsDs.push_back(
                    UsdImagingCollectionMaterialBindingSchema::Builder()
                    .SetCollectionPath(
                        _RetainedTypedDs<SdfPath>::New(b.GetCollectionPath()))
                    .SetMaterialPath(
                        _RetainedTypedDs<SdfPath>::New(b.GetMaterialPath()))
                    .SetBindingStrength(
                        _RetainedTypedDs<TfToken>::New(
                            UsdShadeMaterialBindingAPI::GetMaterialBindingStrength(
                                b.GetBindingRel())))
                    .SetBindingOriginPath(thisPrimPathDs)
                    .Build()
                );
            }
        }

        return HdRetainedSmallVectorDataSource::New(
            bindingsDs.size(), bindingsDs.data());
    }

    UsdShadeMaterialBindingAPI _mbApi;
};
HD_DECLARE_DATASOURCE_HANDLES(_CollectionMaterialBindingsContainerDataSource);


class _DirectMaterialBindingsContainerDataSource : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_DirectMaterialBindingsContainerDataSource);

    _DirectMaterialBindingsContainerDataSource(const UsdPrim &prim)
    : _mbApi(prim) {
    }

    TfTokenVector GetNames() override {
        // XXX This returns all the possible values for material purpose
        //     instead of just the ones for which material bindings are
        //     authored on the prim.
        return _mbApi.GetMaterialPurposes();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        const TfToken &purpose = name;

        UsdRelationship bindingRel = _mbApi.GetDirectBindingRel(purpose);
        if (!bindingRel) {
            return nullptr;
        }
        UsdShadeMaterialBindingAPI::DirectBinding db(bindingRel);
        if (!db.IsBound()) {
            return nullptr;
        }

        return
            UsdImagingDirectMaterialBindingSchema::Builder()
            .SetMaterialPath(
                _RetainedTypedDs<SdfPath>::New(db.GetMaterialPath()))
            .SetBindingStrength(
                _RetainedTypedDs<TfToken>::New(
                    UsdShadeMaterialBindingAPI::GetMaterialBindingStrength(
                        bindingRel)))
            .SetBindingOriginPath(
                _RetainedTypedDs<SdfPath>::New(_mbApi.GetPath()))
            .Build();
    }

private:
    UsdShadeMaterialBindingAPI _mbApi;
};
HD_DECLARE_DATASOURCE_HANDLES(_DirectMaterialBindingsContainerDataSource);


std::pair<bool,bool>
_HasDirectAndCollectionBindings(const UsdPrim &prim)
{
    // Note: GetAuthoredPropertiesInNamespace for "material:binding" returns
    //       "material:binding:*" but not "material:binding". So, we use
    //       "material:" instead to get all bindings.
    //       Collection bindings have a binding name, so using
    //       "material:binding:collection" suffices.
    //
    const std::vector<UsdProperty> colBindingProps =
        prim.GetAuthoredPropertiesInNamespace(
            UsdShadeTokens->materialBindingCollection.GetString());

    const bool hasCollectionBinding = !colBindingProps.empty();
    
    const std::vector<UsdProperty> allBindingProps =
        prim.GetAuthoredPropertiesInNamespace(
            _tokens->materialNamespace.GetString());

    const bool hasDirectBinding =
        allBindingProps.size() > colBindingProps.size();

    return {hasDirectBinding, hasCollectionBinding};
}

} // anonymous namespace

// ----------------------------------------------------------------------------

HdContainerDataSourceHandle
UsdImagingMaterialBindingAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    const std::pair<bool,bool> hasBindings =
        _HasDirectAndCollectionBindings(prim);
    const bool &hasDirectBindings = hasBindings.first;
    const bool &hasCollectionBindings = hasBindings.second;

    return HdRetainedContainerDataSource::New(
        UsdImagingDirectMaterialBindingsSchema::GetSchemaToken(),
        hasDirectBindings
        ? _DirectMaterialBindingsContainerDataSource::New(prim)
        : nullptr,

        UsdImagingCollectionMaterialBindingsSchema::GetSchemaToken(),
        hasCollectionBindings
        ? _CollectionMaterialBindingsContainerDataSource::New(prim)
        : nullptr);
}

HdDataSourceLocatorSet
UsdImagingMaterialBindingAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{

    // QUESTION: We aren't ourselves creating any subprims but do we need to
    //           contribute to them?
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    const auto &dirBindingsLocator =
        UsdImagingDirectMaterialBindingsSchema::GetDefaultLocator();
    const auto &colBindingsLocator =
        UsdImagingCollectionMaterialBindingsSchema::GetDefaultLocator();

    // Edits to the binding path or strength or collection requires
    // reevaluation of the resolved binding.
    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName,
                UsdShadeTokens->materialBindingCollection)) {
            return colBindingsLocator;
        }
        
        if (TfStringStartsWith(propertyName,
                UsdShadeTokens->materialBinding)) {
            return dirBindingsLocator;
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
