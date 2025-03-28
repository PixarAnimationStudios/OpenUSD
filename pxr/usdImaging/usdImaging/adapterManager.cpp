//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/adapterManager.h"

#include "pxr/usdImaging/usdImaging/adapterRegistry.h"
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Because auto-applied schemas have weaker opinions than type-based prim
// adapters, it interweaves the opinion strength of prim and API schemas.
// In order to present that to all consumers as a single ordered list of
// potential contributors, this class satisfies UsdImagingAPISchemaAdapter
// by ignoring appliedInstanceName (which will always be empty as built) and
// calling through to equivalent methods on a UsdImagingPrimAdapter
class _PrimAdapterAPISchemaAdapter : public UsdImagingAPISchemaAdapter
{
public:
    _PrimAdapterAPISchemaAdapter(
            const UsdImagingPrimAdapterSharedPtr &primAdapter)
    : _primAdapter(primAdapter)
    {}

    TfTokenVector GetImagingSubprims(
            UsdPrim const& prim,
            TfToken const& appliedInstanceName) override {
        return _primAdapter->GetImagingSubprims(prim);
    }

    TfToken GetImagingSubprimType(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName) override {

        return _primAdapter->GetImagingSubprimType(prim, subprim);
    }

    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override {
        return _primAdapter->GetImagingSubprimData(prim, subprim, stageGlobals);
    }

    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties,
            const UsdImagingPropertyInvalidationType invalidationType) override {

        return _primAdapter->InvalidateImagingSubprim(
            prim, subprim, properties, invalidationType);
    }

private:
    UsdImagingPrimAdapterSharedPtr _primAdapter;
};


// If no prim type adapter is present, this will use UsdImagingDataSourcePrim
class _BasePrimAdapterAPISchemaAdapter : public UsdImagingAPISchemaAdapter
{
public:

    _BasePrimAdapterAPISchemaAdapter()
    {}

    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override {

        if (subprim.IsEmpty()) {
            return UsdImagingDataSourcePrim::New(
                prim.GetPath(), prim, stageGlobals);
        }
        return nullptr;
    }

    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties,
            const UsdImagingPropertyInvalidationType invalidationType) override {

        return UsdImagingDataSourcePrim::Invalidate(
            prim, subprim,properties, invalidationType);
    }
};

} //anonymous namespace

UsdImaging_AdapterManager::UsdImaging_AdapterManager()
  : _keylessAPISchemaAdapters(
      UsdImagingAdapterRegistry::GetInstance()
        .ConstructKeylessAPISchemaAdapters())
{
}

void UsdImaging_AdapterManager::Reset()
{
    _primTypeToWrappedPrimAdapterEntry.clear();
    _schemaNameToAPISchemaAdapter.clear();
    _typeInfoToAdaptersEntry.clear();
}

const UsdImaging_AdapterManager::AdaptersEntry &
UsdImaging_AdapterManager::LookupAdapters(const UsdPrim &prim)
{
    if (!prim) {
        static const AdaptersEntry empty;
        return empty;
    }

    return _LookupAdapters(prim.GetPrimTypeInfo());
}

const UsdImaging_AdapterManager::AdaptersEntry &
UsdImaging_AdapterManager::_LookupAdapters(const UsdPrimTypeInfo &typeInfo)
{
    const UsdPrimTypeInfo * const key = std::addressof(typeInfo);
    
    // check for previously cached value of full array
    const auto it = _typeInfoToAdaptersEntry.find(key);
    if (it != _typeInfoToAdaptersEntry.end()) {
        return it->second;
    }

    const auto itAndBool =
        _typeInfoToAdaptersEntry.insert(
            { key, _ComputeAdapters(typeInfo) } );

    return itAndBool.first->second;
}

UsdImaging_AdapterManager::AdaptersEntry
UsdImaging_AdapterManager::_ComputeAdapters(
    const UsdPrimTypeInfo &typeInfo)
{
    AdaptersEntry result;

    // contains both auto-applied and manually applied schemas
    const TfTokenVector appliedSchemas =
        typeInfo.GetPrimDefinition().GetAppliedAPISchemas();

    result.allAdapters.reserve(
        _keylessAPISchemaAdapters.size() + 1 + appliedSchemas.size());

    // first add keyless adapters as they have a stronger opinion than any
    // keyed adapter
    result.allAdapters.insert(
        result.allAdapters.end(),
        _keylessAPISchemaAdapters.begin(), _keylessAPISchemaAdapters.end());

    // Then any prim-type schema - using the _BasePrimAdapterAPISchemaAdapter
    // If no prim adapter was registered.
    const _WrappedPrimAdapterEntry &entry = _LookupWrappedPrimAdapter(
        typeInfo.GetSchemaTypeName());
    result.primAdapter = entry.primAdapter;
    // Adds prim adapter wrapped as API schema adapter.
    result.allAdapters.emplace_back(entry.apiSchemaAdapter);

    for (const TfToken &schemaToken : appliedSchemas) {
        const std::pair<TfToken, TfToken> tokenPair =
            UsdSchemaRegistry::GetTypeNameAndInstance(schemaToken);
        if (UsdImagingAPISchemaAdapterSharedPtr const a =
                _LookupAPISchemaAdapter(tokenPair.first)) {
            result.allAdapters.emplace_back(a, tokenPair.second);
        }
    }

    return result;
}

const UsdImaging_AdapterManager::_WrappedPrimAdapterEntry &
UsdImaging_AdapterManager::_LookupWrappedPrimAdapter(
    const TfToken &primType)
{
    // Look-up adapter in cache.
    const auto it = _primTypeToWrappedPrimAdapterEntry.find(primType);
    if (it != _primTypeToWrappedPrimAdapterEntry.end()) {
        return it->second;
    }

    const auto itAndBool =
        _primTypeToWrappedPrimAdapterEntry.insert(
            {primType, _ComputeWrappedPrimAdapter(primType)});
    
    return itAndBool.first->second;
}

UsdImaging_AdapterManager::_WrappedPrimAdapterEntry
UsdImaging_AdapterManager::_ComputeWrappedPrimAdapter(
    const TfToken &schemaName)
{
    // Construct and store in cache if not in cache yet.
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    UsdImaging_AdapterManager::_WrappedPrimAdapterEntry entry;
    entry.primAdapter = reg.ConstructAdapter(schemaName);
    if (entry.primAdapter) {
        entry.apiSchemaAdapter =
            std::make_shared<_PrimAdapterAPISchemaAdapter>(entry.primAdapter);
    } else {
        // use a fallback adapter which calls directly to
        // UsdImagingDataSourcePrim where appropriate
        static const UsdImagingAPISchemaAdapterSharedPtr basePrimAdapter =
             std::make_shared<_BasePrimAdapterAPISchemaAdapter>();
        entry.apiSchemaAdapter = basePrimAdapter;
    }

    return entry;
}

UsdImagingAPISchemaAdapterSharedPtr
UsdImaging_AdapterManager::_LookupAPISchemaAdapter(const TfToken &schemaName)
{
    const auto it = _schemaNameToAPISchemaAdapter.find(schemaName);
    if (it != _schemaNameToAPISchemaAdapter.end()) {
        return it->second;
    }

    // Construct and store in cache if not in cache yet.
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    UsdImagingAPISchemaAdapterSharedPtr const adapter =
        reg.ConstructAPISchemaAdapter(schemaName);
    _schemaNameToAPISchemaAdapter[schemaName] = adapter;
    return adapter;
}




PXR_NAMESPACE_CLOSE_SCOPE
