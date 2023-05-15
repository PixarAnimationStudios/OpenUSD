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
#ifndef PXR_USD_IMAGING_USD_IMAGING_ADAPTER_MANAGER_H
#define PXR_USD_IMAGING_USD_IMAGING_ADAPTER_MANAGER_H

#include "pxr/pxr.h"

#include "pxr/usd/usd/prim.h"

#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

using UsdImagingAPISchemaAdapterSharedPtr =
    std::shared_ptr<class UsdImagingAPISchemaAdapter>;
using UsdImagingPrimAdapterSharedPtr =
    std::shared_ptr<class UsdImagingPrimAdapter>;

/// \class UsdImaging_AdapterManager
///
/// Computes the prim and API schema adapters that are needed to compute the
/// HdSceneIndexPrim from a UsdPrim.
///
class UsdImaging_AdapterManager
{
public:
    UsdImaging_AdapterManager();
    void Reset();

    using APISchemaEntry =
        std::pair<UsdImagingAPISchemaAdapterSharedPtr, TfToken>;
    using APISchemaAdapters = TfSmallVector<APISchemaEntry, 8>;

    // Adapter delegation.
    APISchemaAdapters AdapterSetLookup(const UsdPrim &prim,
            // optionally return the prim adapter (which will also be
            // included in wrapped form as  part of the ordered main result)
            UsdImagingPrimAdapterSharedPtr *outputPrimAdapter=nullptr) const;

private:
    APISchemaAdapters _AdapterSetLookup(const UsdPrimTypeInfo &primTypeInfo,
            UsdImagingPrimAdapterSharedPtr *outputPrimAdapter=nullptr) const;

    UsdImagingAPISchemaAdapterSharedPtr _APIAdapterLookup(
            const TfToken &adapterKey) const;

    UsdImagingPrimAdapterSharedPtr _PrimAdapterLookup(
            const TfToken &adapterKey) const;

    // Usd Prim Type to Adapter lookup table, concurrent because it could
    // be potentially filled during concurrent GetPrim calls rather than
    // just during single-threaded population.
    using _PrimAdapterMap = tbb::concurrent_unordered_map<
        TfToken, UsdImagingPrimAdapterSharedPtr, TfHash>;

    mutable _PrimAdapterMap _primAdapterMap;

    using _ApiAdapterMap = tbb::concurrent_unordered_map<
        TfToken, UsdImagingAPISchemaAdapterSharedPtr, TfHash>;

    mutable _ApiAdapterMap _apiAdapterMap;

    struct _AdapterSetEntry
    {
         // ordered and inclusive of primAdapter
        APISchemaAdapters allAdapters;

        // for identifying prim adapter within same lookup
        UsdImagingPrimAdapterSharedPtr primAdapter; 
    };

    // Use UsdPrimTypeInfo pointer as key because they are guaranteed to be
    // cached at least as long as the stage is open.
    using _AdapterSetMap = tbb::concurrent_unordered_map<
        const UsdPrimTypeInfo *, _AdapterSetEntry, TfHash>;

    mutable _AdapterSetMap _adapterSetMap;

    APISchemaAdapters _keylessAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
