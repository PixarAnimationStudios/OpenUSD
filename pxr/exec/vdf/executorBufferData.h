//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_BUFFER_DATA_H
#define PXR_EXEC_VDF_EXECUTOR_BUFFER_DATA_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/smblData.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class VdfOutputSpec;
class VdfVector;

///////////////////////////////////////////////////////////////////////////////
///
/// \brief This object is responsible for storing the executor buffer data,
/// comprised of the executor cache vector, as well as a mask that denotes the
/// available data.
///
class VdfExecutorBufferData
{
public:
    /// Noncopyable.
    ///
    VdfExecutorBufferData(const VdfExecutorBufferData &) = delete;
    VdfExecutorBufferData &operator=(const VdfExecutorBufferData &) = delete;

    /// Constructor
    /// 
    VdfExecutorBufferData() : _cacheAndFlags() {}

    /// Destructor.
    ///
    VDF_API
    ~VdfExecutorBufferData();

    /// Reset the instance to its original, newly constructed state.
    ///
    VDF_API
    void Reset();

    /// Clones this VdfExecutorBufferData instance to \p dest.
    ///
    VDF_API
    void Clone(VdfExecutorBufferData *dest) const;

    /// Creates a new executor cache for this buffer.
    ///
    inline VdfVector *CreateExecutorCache(const VdfOutputSpec &spec);

    /// Creates a new executor cache for this buffer. The executor cache will
    /// be sized to accommodate all the entries set in the given \p bits.
    ///
    inline VdfVector *CreateExecutorCache(
        const VdfOutputSpec &spec,
        const VdfMask::Bits &bits);

    /// Swaps the executor cache at this buffer, with that of another buffer.
    ///
    inline VdfVector *SwapExecutorCache(VdfExecutorBufferData *rhs);

    /// Returns the executor cache stored at this buffer data instance.
    ///
    inline VdfVector *GetExecutorCache() const;

    /// Get the available mask.
    ///
    inline const VdfMask &GetExecutorCacheMask() const;

    /// Sets the available mask.
    ///
    inline void SetExecutorCacheMask(const VdfMask &mask);

    /// Reset the executor cache without releasing any memory and set the
    /// executor cache mask to \p mask.
    ///
    inline void ResetExecutorCache(const VdfMask &mask);

    /// Reset the executor cache without releasing any memory.
    ///
    inline void ResetExecutorCache();

    /// Takes the existing executor cache and retains it within the 
    /// existing VdfSMBLData object.
    ///
    VDF_API
    void RetainExecutorCache(const VdfOutputSpec &spec, VdfSMBLData *smblData);

    /// Merges the executor cache previously retained in \p smblData into
    /// this cache and releases the SMBL data. Returns the mask denoting the
    /// data merged into the executor cache.
    ///
    VDF_API
    VdfMask ReleaseExecutorCache(VdfSMBLData *smblData);

    /// Returns \c true if the buffer owns the executor cache. The owner of the
    /// cache is responsible for its lifetime management.
    ///
    inline bool HasOwnership() const;

    /// Yields ownership of the internal vector, i.e. the vector will no
    /// longer be deallocated when this object goes out of scope.
    ///
    inline void YieldOwnership();

    /// Yields ownership of the given \p vector. Note, this method deallocates
    /// any vector previously owned by this instance.
    ///
    inline void YieldOwnership(VdfVector *v);

    /// Assumes ownership of the given vector. Note, this will cause the
    /// given vector to be deallocated when this instance goes out of scope.
    /// The client must ensure that only a single buffer instance maintains
    /// ownership over any vector.
    ///
    inline void TakeOwnership(VdfVector *v);

private:

    // Flag denotes whether the VdfVector is owned by this buffer.
    static constexpr uintptr_t _IsOwnedFlag    = 1 << 0;

    // Flag denotes whether the cache is occupied, rather than merely allocated.
    static constexpr uintptr_t _IsOccupiedFlag = 1 << 1;

    // Masks the flags in the cache pointer.
    static constexpr uintptr_t _FlagsMask = _IsOwnedFlag | _IsOccupiedFlag;

    // Casts the integer value to a pointer value.
    static VdfVector *_CastToPointer(uintptr_t v) {
        return reinterpret_cast<VdfVector *>(reinterpret_cast<void *>(v));
    }

    // Casts the pointer value to an integer value.
    static uintptr_t _CastToInteger(VdfVector *v) {
        return reinterpret_cast<uintptr_t>(reinterpret_cast<void *>(v));
    }

    // Returns the cache pointer from the cache-with-flags value.
    static VdfVector *_GetCache(VdfVector *cacheAndFlags) {
        return _CastToPointer(_CastToInteger(cacheAndFlags) & ~_FlagsMask);
    }

    // Retuns the flags from the cache-with-flags value.
    static uintptr_t _GetFlags(VdfVector *cacheAndFlags) {
        return _CastToInteger(cacheAndFlags) & _FlagsMask;
    }

    // Sets the flags on the cache-with-flags value.
    static VdfVector *_SetFlags(
        VdfVector *cacheAndFlags, uintptr_t flags) {
        return _CastToPointer(_CastToInteger(cacheAndFlags) | flags);
    }

    // Unsets the flags on the cache-with-flags value.
    static VdfVector *_UnsetFlags(
        VdfVector *cacheAndFlags, uintptr_t flags) {
        return _CastToPointer(_CastToInteger(cacheAndFlags) & ~flags);
    }

    // Free all the data allocated by this object
    VDF_API
    void _Free();

    // The VdfVector, as well as two bits denoting ownership and cache
    // occupation.
    // Note, even though this is an atomic, VdfExecutorBufferData makes no
    // thread-safety guarantees beyond concurrent read access to the data. The
    // only reason this is an atomic is so that the flags can be stored along
    // with the cache pointer, while still being able to modify the flags
    // concurrently to reading the cache pointer (as if those were two separate
    // member variables) without technically triggering undefined behavior.
    std::atomic<VdfVector *> _cacheAndFlags;

    // Mask of the entries computed in _cacheAndFlags.
    VdfMask _mask;

};

////////////////////////////////////////////////////////////////////////////////

VdfVector *
VdfExecutorBufferData::CreateExecutorCache(const VdfOutputSpec &spec)
{
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);

    // If this buffer maintains ownership over a previously allocated vector,
    // make it occupy the executor cache.
    if (_GetCache(cacheAndFlags) && _GetFlags(cacheAndFlags) & _IsOwnedFlag) {
        _cacheAndFlags.store(
            _SetFlags(cacheAndFlags, _IsOccupiedFlag),
            std::memory_order_release);
        return _GetCache(cacheAndFlags);
    }

    // If the buffer does not have ownership of the cache, or has not
    // previously allocated a cache, allocate a new one. Take ownership
    // of the new buffer.
    VdfVector *newCache = spec.AllocateCache();
    _cacheAndFlags.store(
        _SetFlags(newCache, _IsOwnedFlag | _IsOccupiedFlag),
        std::memory_order_release);

    // Return the newly allocated cache.
    return newCache;
}

VdfVector *
VdfExecutorBufferData::CreateExecutorCache(
    const VdfOutputSpec &spec,
    const VdfMask::Bits &bits)
{
    VdfVector *v = CreateExecutorCache(spec);
    spec.ResizeCache(v, bits);
    return v;
}

VdfVector *
VdfExecutorBufferData::SwapExecutorCache(VdfExecutorBufferData *rhs)
{
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);
    VdfVector *rhsCacheAndFlags =
        rhs->_cacheAndFlags.load(std::memory_order_acquire);

    _cacheAndFlags.store(rhsCacheAndFlags, std::memory_order_release);
    rhs->_cacheAndFlags.store(cacheAndFlags, std::memory_order_release);

    return _GetCache(rhsCacheAndFlags);
}

VdfVector *
VdfExecutorBufferData::GetExecutorCache() const
{
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);
    return (_GetFlags(cacheAndFlags) & _IsOccupiedFlag)
        ? _GetCache(cacheAndFlags)
        : nullptr;
}

const VdfMask &
VdfExecutorBufferData::GetExecutorCacheMask() const
{ 
    return _mask;
}

void
VdfExecutorBufferData::ResetExecutorCache(const VdfMask &mask)
{
    // Untoggle the occupation flag of the executor cache without
    // modifying the ownership flag.
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);
    if (_GetFlags(cacheAndFlags) & _IsOccupiedFlag) {
        _cacheAndFlags.store(
            _UnsetFlags(cacheAndFlags, _IsOccupiedFlag),
            std::memory_order_release);
    }

    // Reset the executor cache mask to the mask provided.
    if (_mask != mask) {
        _mask = mask;
    }
}

void
VdfExecutorBufferData::ResetExecutorCache()
{
    ResetExecutorCache(VdfMask());
}

void
VdfExecutorBufferData::SetExecutorCacheMask(const VdfMask &mask)
{ 
    _mask = mask; 
}

bool
VdfExecutorBufferData::HasOwnership() const
{
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_relaxed);
    return (_GetFlags(cacheAndFlags) & _IsOwnedFlag) != 0;
}

void
VdfExecutorBufferData::YieldOwnership()
{
    VdfVector *cacheAndFlags = _cacheAndFlags.load(std::memory_order_acquire);
    _cacheAndFlags.store(
        _UnsetFlags(cacheAndFlags, _IsOwnedFlag), std::memory_order_release);
}

void
VdfExecutorBufferData::YieldOwnership(VdfVector *v)
{
    // The call to _Free won't actually deallocate the cache if it's not
    // owned by this instance, so it's okay to "self-assign" in that case.
    TF_DEV_AXIOM(v != GetExecutorCache() || !HasOwnership());

    _Free();
    _cacheAndFlags.store(
        _SetFlags(v, _IsOccupiedFlag), std::memory_order_release);
}

void
VdfExecutorBufferData::TakeOwnership(VdfVector *v)
{
    // The call to _Free won't actually deallocate the cache if it's not
    // owned by this instance, so it's okay to "self-assign" in that case.
    TF_DEV_AXIOM(v != GetExecutorCache() || !HasOwnership());
    
    _Free();
    _cacheAndFlags.store(
        _SetFlags(v, _IsOccupiedFlag | _IsOwnedFlag),
        std::memory_order_release);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
