//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_OUTPUT_VALUE_CACHE_H
#define PXR_EXEC_EF_OUTPUT_VALUE_CACHE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/request.h"

#include <tbb/spin_rw_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

class VdfOutput;
class VdfNode;
class VdfVector;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Ef_OutputValueCache
///
/// \brief An output-to-value storage for caching. The class provides accessor
///        types for thread-safe, as well as unprotected access.
///
class Ef_OutputValueCache
{
public:
    /// Constructor.
    ///
    EF_API
    Ef_OutputValueCache();

    /// Destructor.
    ///
    EF_API
    ~Ef_OutputValueCache();

    /// An accessor that provides exclusive read/write access to the cache. No
    /// other reader or writer will access this cache.
    ///
    class ExclusiveAccess
    {
    public:
        /// Non-copyable.
        ///
        ExclusiveAccess(const ExclusiveAccess &) = delete;
        const ExclusiveAccess &operator=(const ExclusiveAccess &) = delete;

        /// Constructor.
        ///
        ExclusiveAccess(Ef_OutputValueCache *cache) : 
            _cache(cache),
            _lock(cache->_mutex, /* write = */ true)
        {}

        /// Returns \c true if the cache is empty at this time.
        ///
        bool IsEmpty() const {
            return _cache->_IsEmpty();
        }

        /// Returns \c true if any outputs in the \p request are
        /// not currently cached.
        ///
        bool IsUncached(const VdfRequest &request) const {
            return _cache->_IsUncached(request);
        }

        /// Returns a request of outputs that are not currently
        /// cached.
        ///
        VdfRequest GetUncached(
            const VdfRequest &request) const {
            return _cache->_GetUncached(request);
        }

        /// Returns the cached value for a given output and mask, if
        /// it exists.
        ///
        const VdfVector *GetValue(
            const VdfOutput &output,
            const VdfMask &mask) const {
            return _cache->_GetValue(output, mask);
        }

        /// Sets the cached values for a given output and mask.
        /// Returns the number of bytes of additionally allocated storage.
        ///
        /// This will NOT update elements in the vector, which are already
        /// cached. Only uncached data will be merged into values existing
        /// in the cache.
        ///
        size_t SetValue(
            const VdfOutput &output,
            const VdfVector &value,
            const VdfMask &mask) {
            return _cache->_SetValue(output, value, mask);
        }

        /// Invalidate an output by removing all the data stored at the output.
        /// Returns the number of bytes invalidated.
        ///
        size_t Invalidate(const VdfOutput &output) {
            return _cache->_Invalidate(output);
        }

        /// Invalidate a vector of outputs and masks by removing the data from
        /// the cache.
        /// Returns the number of bytes invalidated.
        ///
        size_t Invalidate(const VdfMaskedOutputVector &outputs) {
            return _cache->_Invalidate(outputs);
        }

        /// Clears the entire cache.
        /// Returns the number of bytes that have been removed from the cache.
        ///
        size_t Clear() {
            return _cache->_Clear();
        }

    private:
        Ef_OutputValueCache *_cache;
        tbb::spin_rw_mutex::scoped_lock _lock;

    };

    /// This accessor grants shared read access to the cache, preventing any
    /// concurrent write access.
    ///
    class SharedAccess
    {
    public:
        /// Non-copyable.
        ///
        SharedAccess(const SharedAccess &) = delete;
        const SharedAccess &operator=(const SharedAccess &) = delete;

        /// Constructor.
        ///
        SharedAccess(Ef_OutputValueCache *cache) :
            _cache(cache),
            _lock(cache->_mutex, /* write = */ false)
        {}

        /// Returns the cached value for a given output and mask, if
        /// it exists.
        ///
        const VdfVector *GetValue(
            const VdfOutput &output,
            const VdfMask &mask) const {
            return _cache->_GetValue(output, mask);
        }

    private:
        Ef_OutputValueCache *_cache;
        tbb::spin_rw_mutex::scoped_lock _lock;
        
    };

private:
    // Returns \c true if the given output is contained in this cache.
    bool _ContainsOutput(const VdfOutput &output) const;

    // Marks the given output as contained in the cache.
    void _AddOutput(const VdfOutput &output);

    // Marks the given output as not contained in the cache.
    void _RemoveOutput(const VdfOutput &output);

    // Returns the value stored at the output, or NULL if the value
    // is not available, as determined by the specified mask.
    EF_API
    const VdfVector *_GetValue(
        const VdfOutput &output,
        const VdfMask &mask) const;

    // Sets the value stored at the output. This does not update
    // any elements currently stored! Only uncached elements will be
    // merged into the cache.
    size_t _SetValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask);

    // Invalidate the entire data stored at the given output.
    size_t _Invalidate(const VdfOutput &outut);

    // Invalidate the value stored at the given outputs.
    size_t _Invalidate(const VdfMaskedOutputVector &outputs);

    // Clear the entire cache.
    size_t _Clear();

    // Is this cache empty?
    bool _IsEmpty() const;

    // Are there any uncached outputs in the given request?
    bool _IsUncached(const VdfRequest &request) const;

    // Get all uncached outputs from the given request.
    VdfRequest _GetUncached(const VdfRequest &request) const;

    // The entry stored for each output in the cache.
    class _Entry
    {
    public:
        // Constructor
        _Entry();

        // Destructor
        ~_Entry();

        // Returns the number of bytes stored at this output.
        size_t GetNumBytes() const;

        // Returns the mask of elements stored at this output.
        const VdfMask &GetMask() const {
            return _mask;
        }

        // Returns the value stored at the output, if any.
        const VdfVector *GetValue() const {
            return _value;
        }

        // Returns the value stored at the output, if it exists and
        // contains all elements specified in the given \p mask.
        const VdfVector *GetValue(const VdfMask &mask) const;

        // Sets the value at this output, not overwriting any existing
        // data. This will return the number of bytes allocated to
        // store the additional data.
        size_t SetValue(
            const VdfVector &value,
            const VdfMask &mask);

        // Invalidate the entire data stored at this output. Returns the
        // number of bytes invalidated.
        size_t Invalidate();

        // Invalidate the data stored at this output. Returns the number
        // of bytes invalidated.
        size_t Invalidate(const VdfMask &mask);

    private:
        // The data value stored at this output.
        VdfVector *_value;

        // The mask of data available at this output.
        VdfMask _mask;
    };

    // Retain a reference to the one-one mask as a field in order to avoid
    // contention on any guard variables.
    const VdfMask _oneOneMask;

    // Type of the output cache map
    typedef
        TfHashMap<const VdfOutput *, _Entry, TfHash>
        _OutputsMap;

    // The output cache map
    _OutputsMap _outputs;

    // An acceleration structure with outputs contained in this cache.
    TfBits _outputSet;

    // The mutex protecting concurrent access to this cache.
    tbb::spin_rw_mutex _mutex;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
