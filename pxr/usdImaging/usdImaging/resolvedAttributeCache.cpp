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

    // Speed up destuction of the cache by resetting the unique_ptrs held 
    // within in parallel.
    tbb::parallel_for(_bindingsCache.range(), 
        []( decltype(_bindingsCache)::range_type &range) {
            for (auto entryIt = range.begin(); entryIt != range.end(); 
                    ++entryIt) {
                entryIt->second.reset();
            }
        }
    );

    tbb::parallel_for(_collQueryCache.range(), 
        []( decltype(_collQueryCache)::range_type &range) {
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

