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
    WorkParallelForEach(
        _bindingsCache.begin(), 
        _bindingsCache.end(), 
        [](UsdShadeMaterialBindingAPI::BindingsCache::value_type &entry){
            entry.second.reset();
        });

    WorkParallelForEach(
        _collQueryCache.begin(), 
        _collQueryCache.end(), 
        [](UsdShadeMaterialBindingAPI::CollectionQueryCache::value_type &entry){
            entry.second.reset();
        });

    _bindingsCache.clear();
    _collQueryCache.clear();
}

const UsdImaging_BlurScaleStrategy::value_type
UsdImaging_BlurScaleStrategy::invalidValue = { 0.0f, false };

PXR_NAMESPACE_CLOSE_SCOPE

