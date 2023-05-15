//
// Copyright 2023 Pixar
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
{
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    
    for (UsdImagingAPISchemaAdapterSharedPtr &adapter :
            reg.ConstructKeylessAPISchemaAdapters()) {
        _keylessAdapters.emplace_back(adapter, TfToken());
    }
}

void UsdImaging_AdapterManager::Reset()
{
    _primAdapterMap.clear();
    _apiAdapterMap.clear();
    _adapterSetMap.clear();
}

UsdImaging_AdapterManager::APISchemaAdapters
UsdImaging_AdapterManager::AdapterSetLookup(
        const UsdPrim &prim,
        UsdImagingPrimAdapterSharedPtr *outputPrimAdapter) const
{
    if (!prim) {
        return {};
    }

    const UsdPrimTypeInfo &typeInfo = prim.GetPrimTypeInfo();

    // check for previously cached value of full array
    const _AdapterSetMap::const_iterator it = _adapterSetMap.find(&typeInfo);
    if (it != _adapterSetMap.end()) {
        if (outputPrimAdapter) {
            *outputPrimAdapter = it->second.primAdapter;
        }
        return it->second.allAdapters;
    }

    _AdapterSetEntry result;

    // contains both auto-applied and manually applied schemas
    const TfTokenVector allAppliedSchemas = prim.GetAppliedSchemas();

    result.allAdapters.reserve(allAppliedSchemas.size() + 1 +
        _keylessAdapters.size());

    // first add keyless adapters as they have a stronger opinion than any
    // keyed adapter
    result.allAdapters.insert(result.allAdapters.end(),
        _keylessAdapters.begin(), _keylessAdapters.end());

    // then any prim-type schema
    const TfToken adapterKey = typeInfo.GetSchemaTypeName();
    // If there is an adapter for the type name, include it.
    if (UsdImagingPrimAdapterSharedPtr adapter =
            _PrimAdapterLookup(adapterKey)) {
        // wrap and cache the prim adapter in an API schema interface
        UsdImagingAPISchemaAdapterSharedPtr adapterAdapter;

        const auto it = _apiAdapterMap.find(adapterKey);
        if (it == _apiAdapterMap.end()) {
            adapterAdapter = std::make_shared<
                _PrimAdapterAPISchemaAdapter>(adapter);
            _apiAdapterMap[adapterKey] = adapterAdapter;
        } else {
            adapterAdapter = it->second;
        }
        result.primAdapter = adapter;
        result.allAdapters.emplace_back(adapterAdapter, TfToken());
    } else {
        // use a fallback adapter which calls directly to
        // UsdImagingDataSourcePrim where appropriate
        static const UsdImagingAPISchemaAdapterSharedPtr basePrimAdapter =
             std::make_shared<_BasePrimAdapterAPISchemaAdapter>();

        result.allAdapters.emplace_back(basePrimAdapter, TfToken());
    }

    // then the applied API schemas which are already in their strength order
    for (const TfToken &schemaToken: allAppliedSchemas) {

        std::pair<TfToken, TfToken> tokenPair =
            UsdSchemaRegistry::GetTypeNameAndInstance(schemaToken);
            
        if (UsdImagingAPISchemaAdapterSharedPtr a =
                _APIAdapterLookup(tokenPair.first)) {
            result.allAdapters.emplace_back(a, tokenPair.second);
        }
    }

    _adapterSetMap.insert({&typeInfo, result});
    if (outputPrimAdapter) {
        *outputPrimAdapter = result.primAdapter;
    }
    return result.allAdapters;
}

UsdImagingPrimAdapterSharedPtr
UsdImaging_AdapterManager::_PrimAdapterLookup(const TfToken &adapterKey) const
{
    // Look-up adapter in cache.
    _PrimAdapterMap::const_iterator const it = _primAdapterMap.find(adapterKey);
    if (it != _primAdapterMap.end()) {
        return it->second;
    }

    // Construct and store in cache if not in cache yet.
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    UsdImagingPrimAdapterSharedPtr adapter = reg.ConstructAdapter(adapterKey);
    _primAdapterMap[adapterKey] = adapter;
    return adapter;
}

UsdImagingAPISchemaAdapterSharedPtr
UsdImaging_AdapterManager::_APIAdapterLookup(
    const TfToken &adapterKey) const
{
    _ApiAdapterMap::const_iterator const it = _apiAdapterMap.find(adapterKey);
    if (it != _apiAdapterMap.end()) {
        return it->second;
    }

    // Construct and store in cache if not in cache yet.
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    UsdImagingAPISchemaAdapterSharedPtr adapter =
        reg.ConstructAPISchemaAdapter(adapterKey);
    _apiAdapterMap[adapterKey] = adapter;
    return adapter;
}




PXR_NAMESPACE_CLOSE_SCOPE
