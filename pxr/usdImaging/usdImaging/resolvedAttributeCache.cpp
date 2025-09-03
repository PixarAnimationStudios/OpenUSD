//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/resolvedAttributeCache.h"

#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

void
UsdImaging_MaterialBindingImplData::ClearCaches()
{
    TRACE_FUNCTION();

    // Speed up destruction of the cache by resetting the unique_ptrs held 
    // within in parallel.
    using BindCacheRange = 
        UsdShadeMaterialBindingAPI::BindingsCache::range_type;
    WorkParallelForTBBRange(_bindingsCache.range(), 
        []( const BindCacheRange &range) {
            for (auto entryIt = range.begin(); entryIt != range.end(); 
                ++entryIt) {
                entryIt->second.reset();
            }
        }
    );

    using CollQueryRange =
        UsdShadeMaterialBindingAPI::CollectionQueryCache::range_type;
    WorkParallelForTBBRange(_collQueryCache.range(), 
        []( const CollQueryRange &range) {
            for (auto entryIt = range.begin(); entryIt != range.end(); 
                ++entryIt) {
                entryIt->second.reset();
            }
        }
    );

    _bindingsCache.clear();
    _collQueryCache.clear();
}

const UsdImaging_BlurScaleStrategy::value_type
UsdImaging_BlurScaleStrategy::invalidValue = { 0.0f, false };

PXR_NAMESPACE_CLOSE_SCOPE

