//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SMBL_DATA_H
#define PXR_EXEC_VDF_SMBL_DATA_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/outputSpec.h"
#include "pxr/exec/vdf/traits.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfVector;

///////////////////////////////////////////////////////////////////////////////
///
/// \brief VdfSMBLData holds per-output data that is meant to be consumed
/// by the executor. This data is an optional part of VdfExecutorData and it
/// is specific to sparse mung buffer locking.
///
class VdfSMBLData
{
public:
    /// Noncopyable.
    ///
    VdfSMBLData(const VdfSMBLData &) = delete;
    VdfSMBLData &operator=(const VdfSMBLData &) = delete;

    /// Constructs an SMBL data object.
    ///
    VdfSMBLData() :
        _cache(NULL)
    {}

    VDF_API
    ~VdfSMBLData();


    /// \name Memoized mask computations
    ///
    /// @{

    /// Invalidates the executor \p cacheMask given an \p invalidationMask.
    /// Returns the \p cacheMask with the bits in the \p invalidationMask
    /// removed.
    /// This is a memoized computation.
    ///
    inline VdfMask InvalidateCacheMask(
        const VdfMask &cacheMask,
        const VdfMask &invalidationMask);

    /// Extends the \p lockedCacheMask by appending the bits stored in the
    /// executor \p cacheMask.
    /// This is a memoized computation.
    ///
    void ExtendLockedCacheMask(
        VdfMask *lockedCacheMask, 
        const VdfMask &cacheMask) {
        *lockedCacheMask = _cachedExtend(*lockedCacheMask, cacheMask);
    }

    /// Make sure that all the bits in the keepMask are provided by the
    /// cacheMask. Any data bits that are not provided by the cacheMask should
    /// not be contained in the lockedCacheMask, so remove them from the
    /// lockedCacheMask. This makes it so that nodes, which must provide data
    /// to be kept at the output, do not become un-affective.
    ///
    inline void RemoveUncachedMask(
        VdfMask *lockedCacheMask,
        const VdfMask &cacheMask,
        const VdfMask &keepMask);

    /// Computes the affectiveness of the corresponding output given the 
    /// accumulated \p lockedCacheMask and the scheduled \p affectsMask of the
    /// output.
    /// This is a memoized computation.
    ///
    bool ComputeAffectiveness(
        const VdfMask &lockedCacheMask,
        const VdfMask &affectsMask) {
        return !_cachedAffective(lockedCacheMask, affectsMask);
    }

    /// @}


    /// \name Local Cache
    ///
    /// @{

    /// Locally retains the passed in \p cache with the given \p cacheMask.
    /// This method returns a pointer to a (new) cache, which the client is now
    /// free to use.
    /// Consequently, this method passes the ownership of the \p cache pointer
    /// to this object, while giving up the ownership of the returned vector.
    /// This is to avoid a copy of the underlying data.
    ///
    inline VdfVector* Retain(
        const VdfOutputSpec &spec,
        VdfVector *cache,
        const VdfMask &cacheMask);

    /// Releases the cache, which has been retained by this object, if any.
    /// Note, that this method does NOT release ownership of any of its heap
    /// allocated data! It merely demotes the retained cache to a free cache
    /// for future use.
    ///
    void Release() {
        if (!_cacheMask.IsEmpty()) {
            _cacheMask = VdfMask();
        }
    }

    /// Clear any of the data this object is holding on to.
    ///
    VDF_API
    void Clear();

    /// Returns a pointer to the locally retained cache, if any.
    ///
    VdfVector *GetCache() const {
        return _cache;
    }

    /// Returns a mask indicating data available in the locally retained cache.
    ///
    const VdfMask &GetCacheMask() const {
        return _cacheMask;
    }

    /// Returns \c true, if a cache has been retained locally, and \c false
    /// if there is no such cache.
    ///
    bool HasCache() const {
        return _cache && !_cacheMask.IsEmpty();
    }

    /// @}

private:
    // This is a helper class used for memoizing expensive mask computations.
    //
    // For sparse mung buffer locking mask operations on a specific output are
    // expected to always yield the same results for any but the first run of
    // the executor. Memoization allows us to hold on to these results without
    // having to worry about invalidation. Note, that we exploit the fact that
    // masks are flyweighted, and hence very cheap to store, as well as
    // equality compare.
    template < typename R, typename OP >
    class _MaskOpMemoizer
    {
    public:
        _MaskOpMemoizer() :
            // Initialize the default result to avoid correctness problems
            _result(OP()(_opA, _opB))
        {}

        VdfByValueOrConstRef<R>
        operator()(const VdfMask &opA, const VdfMask &opB) {
            // Cache miss?
            if (_opA != opA || _opB != opB) {
                _opA = opA;
                _opB = opB;
                _result = OP()(opA, opB);
            }

            // Cache hit!
            return _result;
        }

    private:
        // Operand A
        VdfMask _opA;

        // Operand B
        VdfMask _opB;

        // Cached result
        R _result;

    };

    // Functor used for a memoized mask subtraction
    struct _MaskSubtract {
        VdfMask operator()(const VdfMask &lhs, const VdfMask &rhs) const {
            return lhs - rhs;
        }
    };

    // Functor used for a memoized append of two masks
    struct _MaskSetOrAppend {
        VdfMask operator()(const VdfMask &lhs, const VdfMask &rhs) const {
            VdfMask res(lhs);
            res.SetOrAppend(rhs);
            return res;
        }
    };

    // Functor used for computing the "Contains" method on a mask
    struct _MaskContains {
        bool operator()(const VdfMask &lhs, const VdfMask &rhs) const {
            return lhs.Contains(rhs);
        }
    };

    // Memoized result of the invalid cache mask
    _MaskOpMemoizer<VdfMask, _MaskSubtract> _cachedInvalidate;

    // Memoized result of the extended locked cache mask
    _MaskOpMemoizer<VdfMask, _MaskSetOrAppend> _cachedExtend;

    // Memoized result of the affective-ness flag
    _MaskOpMemoizer<bool, _MaskContains> _cachedAffective;

    // Memoized result of keepMask - cacheMask. This contains the bits
    // that are required to be stored at the output.
    _MaskOpMemoizer<VdfMask, _MaskSubtract> _cachedRequiredMask;

    // Memoized result of the locked cache mask with all the uncached,
    // but required bits removed.
    _MaskOpMemoizer<VdfMask, _MaskSubtract> _cachedRequiredLockedCache;

    // Locally retained cache and cache mask
    VdfVector *_cache;
    VdfMask _cacheMask;

};


VdfMask
VdfSMBLData::InvalidateCacheMask(
    const VdfMask &cacheMask, 
    const VdfMask &invalidationMask)
{
    return cacheMask.IsEmpty()
        ? cacheMask
        : _cachedInvalidate(cacheMask, invalidationMask);
}

void 
VdfSMBLData::RemoveUncachedMask(
    VdfMask *lockedCacheMask,
    const VdfMask &cacheMask,
    const VdfMask &keepMask)
{
    // Determine which bits in the keep mask are not available in the
    // local executor cache. These are the bits that we have to remove from
    // the executor cache mask, if necessary.
    const VdfMask &uncached =
        cacheMask.IsEmpty()
            ? keepMask
            : _cachedRequiredMask(keepMask, cacheMask);

    // Remove the uncached bits, if any.
    *lockedCacheMask = _cachedRequiredLockedCache(*lockedCacheMask, uncached);
}

VdfVector* 
VdfSMBLData::Retain(
    const VdfOutputSpec &spec,
    VdfVector *cache,
    const VdfMask &cacheMask) 
{
    // The local cache is always a free cache. If a local cache has not
    // been allocated, yet, allocate one here.
    if (!_cache) {
        _cache = spec.AllocateCache();
    }

    // Store a pointer to the current cache, which is a always a free cache.
    VdfVector *freeCache = _cache;

    // Swap the current cache with the passed in cache, to retain it.
    _cache = cache;
    _cacheMask = cacheMask;

    // Return the pointer to the free cache to be re-used by the client.
    return freeCache;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_EXEC_VDF_SMBL_DATA_H */