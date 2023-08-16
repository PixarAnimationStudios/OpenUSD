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

    struct AdapterEntry
    {
        AdapterEntry(
            const UsdImagingAPISchemaAdapterSharedPtr &adapter,
            const TfToken &appliedInstanceName = TfToken())
          : adapter(adapter)
          , appliedInstanceName(appliedInstanceName)
        {
        }

        // This is either an API schema adapter or the prim adapter
        // wrapped as an API schema adapter.
        UsdImagingAPISchemaAdapterSharedPtr adapter;
        // Instance name for an multi-apply API schema.
        //
        // For example the prepending the apiSchema to CollectionAPI:lightLink
        // a USD prim will use the CollectionAPI adapter with
        // appliedInstancedName "lightLink".
        TfToken appliedInstanceName;
    };

    using AdapterEntries = TfSmallVector<AdapterEntry, 8>;

    struct AdaptersEntry
    {
        // Ordered. And includes the primAdapter wrapped as a an
        // API schema adapter.
        AdapterEntries allAdapters;

        // Just the prim adapter for the prim type.
        UsdImagingPrimAdapterSharedPtr primAdapter;
    };

    // Look-up all adapters needed to serve a prim.
    const AdaptersEntry &LookupAdapters(const UsdPrim &prim);

private:
    const AdaptersEntry &_LookupAdapters(const UsdPrimTypeInfo &typeInfo);

    // A prim adapter together with an API schema wrapping the prim
    // adapter.
    struct _WrappedPrimAdapterEntry
    {
        UsdImagingPrimAdapterSharedPtr primAdapter;
        UsdImagingAPISchemaAdapterSharedPtr apiSchemaAdapter;
    };

    // Concurrent because it could
    // be potentially filled during concurrent GetPrim calls rather than
    // just during single-threaded population.
    using _PrimTypeToWrappedPrimAdapterEntry = tbb::concurrent_unordered_map<
        TfToken, _WrappedPrimAdapterEntry, TfHash>;
    using _SchemaNameToAPISchemaAdapter = tbb::concurrent_unordered_map<
        TfToken, UsdImagingAPISchemaAdapterSharedPtr, TfHash>;
    // Use UsdPrimTypeInfo pointer as key because they are guaranteed to be
    // cached at least as long as the stage is open.
    using _TypeInfoToAdaptersEntry = tbb::concurrent_unordered_map<
        const UsdPrimTypeInfo *, AdaptersEntry, TfHash>;

    const _WrappedPrimAdapterEntry &
    _LookupWrappedPrimAdapter(
        const TfToken &primType);

    UsdImagingAPISchemaAdapterSharedPtr
    _LookupAPISchemaAdapter(
        const TfToken &schemaName);

    AdaptersEntry _ComputeAdapters(const UsdPrimTypeInfo &typeInfo);

    _WrappedPrimAdapterEntry
    _ComputeWrappedPrimAdapter(
        const TfToken &schemaName);

    _PrimTypeToWrappedPrimAdapterEntry _primTypeToWrappedPrimAdapterEntry;
    _SchemaNameToAPISchemaAdapter _schemaNameToAPISchemaAdapter;
    _TypeInfoToAdaptersEntry _typeInfoToAdaptersEntry;

    const std::vector<UsdImagingAPISchemaAdapterSharedPtr>
        _keylessAPISchemaAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
