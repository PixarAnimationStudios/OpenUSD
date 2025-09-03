//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executorBufferData.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfExecutorBufferData::~VdfExecutorBufferData()
{
    _Free();
}

void
VdfExecutorBufferData::Reset()
{
    _Free();
    _cacheAndFlags.store(nullptr, std::memory_order_release);
    _mask = VdfMask();
}

void
VdfExecutorBufferData::Clone(VdfExecutorBufferData *dest) const
{
    // Deallocate all the destination data
    dest->_Free();

    // Get the cache pointer with the flags.
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);

    // If the source cache is set, clone its contents. Otherwise, reset
    // the cache pointer.
    VdfVector *newCache = nullptr;
    if (VdfVector *cache = _GetCache(cacheAndFlags)) {
        newCache = new VdfVector(*cache);
    }

    // Clone the occupied flag, but always take ownership of the newly
    // constructed vector.
    const uintptr_t newFlags = _GetFlags(cacheAndFlags) | _IsOwnedFlag;

    // Store the duplicated cache and flags.
    dest->_cacheAndFlags.store(
        _SetFlags(newCache, newFlags), std::memory_order_release);

    // Clone the cache mask.
    dest->_mask = _mask;
}

void
VdfExecutorBufferData::RetainExecutorCache(
    const VdfOutputSpec &spec,
    VdfSMBLData *smblData)
{
    // Get the cache pointer with the flags.
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);

    // It is an error if there is already data retained for this cache, or if
    // the cache is not owned or occupied at this output.
    TF_DEV_AXIOM(
        smblData && 
        smblData->GetCacheMask().IsEmpty() &&
        _GetFlags(cacheAndFlags) == (_IsOwnedFlag | _IsOccupiedFlag));

    // Retain the current cache and return a new cache for us to use. 
    VdfVector *newCache =
        smblData->Retain(spec, _GetCache(cacheAndFlags), _mask);

    // Store the new cache, but reset the occupation state.
    _cacheAndFlags.store(
        _SetFlags(newCache, _IsOwnedFlag), std::memory_order_release);

    // Reset the cache mask.
    _mask = VdfMask();
}

VdfMask
VdfExecutorBufferData::ReleaseExecutorCache(VdfSMBLData *smblData)
{
    // If there is no smbl data or if there is no cache retained, bail out.
    if (!smblData || !smblData->HasCache()) {
        return VdfMask();
    }

    // Merge the retained data into the executor cache.
    VdfMask mergeMask(smblData->GetCacheMask());
    GetExecutorCache()->Merge(*smblData->GetCache(), mergeMask);

    // Release the previously retained cache.
    smblData->Release();

    // Return the merge mask.
    return mergeMask;
}

void
VdfExecutorBufferData::_Free()
{
    // Free the memory only if the buffer owns the vector.
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);
    if (_GetFlags(cacheAndFlags) & _IsOwnedFlag) {
        delete _GetCache(cacheAndFlags);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
