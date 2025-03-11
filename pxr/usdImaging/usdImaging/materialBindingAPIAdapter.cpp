//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/materialBindingAPIAdapter.h"

#include "pxr/usdImaging/usdImaging/collectionMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingsSchema.h"

#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE


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

 HdDataSourceBaseHandle
_BuildCollectionBindingsVectorDataSource(
    const UsdShadeMaterialBindingAPI &mbApi,
    const TfToken &purpose)
{
    using _CollectionBindings =
        UsdShadeMaterialBindingAPI::CollectionBindingVector;

    _CollectionBindings bindings = mbApi.GetCollectionBindings(purpose);
    if (bindings.empty()) {
        return nullptr;
    }

    std::vector<HdDataSourceBaseHandle> bindingsDs;
    bindingsDs.reserve(bindings.size());

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
                        UsdShadeMaterialBindingAPI::
                            GetMaterialBindingStrength(b.GetBindingRel())))
                .Build()
            );
        }
    }

    return HdRetainedSmallVectorDataSource::New(
        bindingsDs.size(), bindingsDs.data());
}

HdDataSourceBaseHandle
_BuildDirectMaterialBindingDataSource(
    const UsdShadeMaterialBindingAPI &mbApi,
    const TfToken &purpose)
{
    const UsdRelationship bindingRel = mbApi.GetDirectBindingRel(purpose);
    if (!bindingRel) {
        return nullptr;
    }
    const UsdShadeMaterialBindingAPI::DirectBinding db(bindingRel);
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
        .Build();
}

class _MaterialBindingContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialBindingContainerDataSource);

    _MaterialBindingContainerDataSource(
        const UsdShadeMaterialBindingAPI &mbApi,
        const TfToken &purpose)
    : _mbApi(mbApi)
    , _purpose(purpose) {
    }

    TfTokenVector GetNames() override {
        return 
            {UsdImagingMaterialBindingSchemaTokens->directMaterialBinding,
            UsdImagingMaterialBindingSchemaTokens->collectionMaterialBindings};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == UsdImagingMaterialBindingSchemaTokens->
                directMaterialBinding) {
            return _BuildDirectMaterialBindingDataSource(_mbApi, _purpose);
        }
        if (name == UsdImagingMaterialBindingSchemaTokens->
                collectionMaterialBindings) {
            return _BuildCollectionBindingsVectorDataSource(_mbApi, _purpose);
        }
        return nullptr;
    }

private:
    UsdShadeMaterialBindingAPI _mbApi;
    const TfToken _purpose;
};
HD_DECLARE_DATASOURCE_HANDLES(_MaterialBindingContainerDataSource);

HdVectorDataSourceHandle
_BuildMaterialBindingVectorDataSource(
    const UsdShadeMaterialBindingAPI &mbApi,
    const TfToken &purpose)
{
    if (!mbApi.GetDirectBinding(purpose).IsBound() &&
        mbApi.GetCollectionBindings(purpose).empty()) {
        return nullptr;
    }

    HdDataSourceBaseHandle srcs[] = {
        _MaterialBindingContainerDataSource::New(mbApi, purpose)
    };

    return HdRetainedSmallVectorDataSource::New(std::size(srcs), srcs);
}

class _MaterialBindingsContainerDataSource : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_MaterialBindingsContainerDataSource);

    _MaterialBindingsContainerDataSource(
        const UsdShadeMaterialBindingAPI &mbApi)
    : _mbApi(mbApi) {
    }

    TfTokenVector GetNames() override {
        // XXX This returns all the possible values for material purpose
        //     instead of just the ones for which material bindings are
        //     authored on the prim.
        return _mbApi.GetMaterialPurposes();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        const TfToken &purpose = name;
        return _BuildMaterialBindingVectorDataSource(_mbApi, purpose);
    }

private:
    UsdShadeMaterialBindingAPI _mbApi;
};
HD_DECLARE_DATASOURCE_HANDLES(_MaterialBindingsContainerDataSource);

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

    return HdRetainedContainerDataSource::New(
        UsdImagingMaterialBindingsSchema::GetSchemaToken(),
        _MaterialBindingsContainerDataSource::New(
            UsdShadeMaterialBindingAPI(prim)));
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

    const auto &usdMatBindingsLocator =
        UsdImagingMaterialBindingsSchema::GetDefaultLocator();
  
    for (const TfToken &propertyName : properties) {
        // Edits to the binding path or strength or collection requires
        // reevaluation of the resolved binding.
        // We could be more specific here by checking if the binding is
        // for a purpose. For now, conservatively invalidate bindings for
        // all purposes.
        //
        if (TfStringStartsWith(propertyName,
                UsdShadeTokens->materialBindingCollection)) {
            return usdMatBindingsLocator;
        }
        
        if (TfStringStartsWith(propertyName,
                UsdShadeTokens->materialBinding)) {
            return usdMatBindingsLocator;
        }

        // Edits to a collection authored on the prim may require reevaluation
        // of the resolved binding because the membership may have changed.
        // Conservatively invalidate the material bindings data
        // source on this prim triggering invalidation for all purposes on
        // the prim and its descendants due to flattening.
        // We can certainly improve this by moving invalidation
        // to a scene index and tracking collections referenced by material
        // bindings if this simple approach becomes a bottleneck.
        //
        if (TfStringStartsWith(propertyName, UsdTokens->collection)) {
            return usdMatBindingsLocator;
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
