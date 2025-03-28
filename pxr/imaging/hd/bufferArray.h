//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_BUFFER_ARRAY_H
#define PXR_IMAGING_HD_BUFFER_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <atomic>
#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferArrayRange;

using HdBufferArraySharedPtr = std::shared_ptr<class HdBufferArray>;
using HdBufferArrayRangeSharedPtr = std::shared_ptr<HdBufferArrayRange>;
using HdBufferArrayRangePtr = std::weak_ptr<HdBufferArrayRange>;

/// \enum HdBufferArrayUsageHintBits
///
/// Provides a set of flags that provide hints to the memory management system 
/// about the properties of a Buffer Array Range (BAR), so it can efficiently 
/// organize that memory.  For example, the memory manager should probably not 
/// aggregate BARs with different usage hints.
///
/// The flag bits are:
///   - immutable: The BAR will not be modified once created and populated.
///   - sizeVarying: The number of elements in the BAR changes with time.
///   - uniform: The BAR can be used as a uniform buffer.
///   - storage: The BAR can be used as a shader storage buffer.
///   - vertex: The BAR can be used as a vertex buffer.
///   - index: The BAR can be used as an index buffer.
///
/// Some flag bits may not make sense in combination (i.e. mutually exclusive 
/// to each other).  For example, it is logically impossible to be both 
/// immutable (i.e. not changing) and sizeVarying (changing).  However, these 
/// logically impossible combinations are not enforced and remain valid 
/// potential values.
///
enum HdBufferArrayUsageHintBits : uint32_t
{
    HdBufferArrayUsageHintBitsImmutable   = 1 << 0,
    HdBufferArrayUsageHintBitsSizeVarying = 1 << 1,
    HdBufferArrayUsageHintBitsUniform     = 1 << 2,
    HdBufferArrayUsageHintBitsStorage     = 1 << 3,
    HdBufferArrayUsageHintBitsVertex      = 1 << 4,
    HdBufferArrayUsageHintBitsIndex       = 1 << 5,
};
using HdBufferArrayUsageHint = uint32_t;

/// \class HdBufferArray
///
/// Similar to a VAO, this object is a bundle of coherent buffers. This object
/// can be shared across multiple HdRprims, in the context of buffer
/// aggregation.
///
class HdBufferArray : public std::enable_shared_from_this<HdBufferArray> 
{
public:
    HD_API
    HdBufferArray(TfToken const &role,
                  TfToken const garbageCollectionPerfToken,
                  HdBufferArrayUsageHint usageHint);

    HD_API
    virtual ~HdBufferArray();

    /// Returns the role of the GPU data in this bufferArray.
    TfToken const& GetRole() const {return _role;}

    /// Returns the version of this buffer array.
    /// Used to determine when to rebuild outdated indirect dispatch buffers
    size_t GetVersion() const {
        return _version;
    }

    /// Increments the version of this buffer array.
    HD_API
    void IncrementVersion();

    /// Attempts to assign a range to this buffer array.
    /// Multiple threads could be trying to assign to this buffer at the same time.
    /// Returns true is the range is assigned to this buffer otherwise
    /// returns false if the buffer doesn't have space to assign the range.
    HD_API
    bool TryAssignRange(HdBufferArrayRangeSharedPtr &range);

    /// Performs compaction if necessary and returns true if it becomes empty.
    virtual bool GarbageCollect() = 0;

    /// Performs reallocation. After reallocation, the buffer will contain
    /// the specified \a ranges. If these ranges are currently held by a
    /// different buffer array instance, then their data will be copied
    /// from the specified \a curRangeOwner.
    virtual void Reallocate(
        std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
        HdBufferArraySharedPtr const &curRangeOwner) = 0;

    /// Returns the maximum number of elements capacity.
    HD_API
    virtual size_t GetMaxNumElements() const;

    /// Debug output
    virtual void DebugDump(std::ostream &out) const = 0;

    /// How many ranges are attached to the buffer array.
    size_t GetRangeCount() const { return _rangeCount; }

    /// Get the attached range at the specified index.
    HD_API
    HdBufferArrayRangePtr GetRange(size_t idx) const;

    /// Remove any ranges from the range list that have been deallocated
    /// Returns number of ranges after clean-up
    HD_API
    void RemoveUnusedRanges();

    /// Returns true if Reallocate() needs to be called on this buffer array.
    bool NeedsReallocation() const {
        return _needsReallocation;
    }

    /// Returns true if this buffer array is marked as immutable.
    bool IsImmutable() const {
        return _usageHint & HdBufferArrayUsageHintBitsImmutable;
    }

    /// Returns the usage hints for this buffer array.
    HdBufferArrayUsageHint GetUsageHint() const {
        return _usageHint;
    }

protected:
    /// Dirty bit to set when the ranges attached to the buffer
    /// changes.  If set Reallocate() should be called to clean it.
    bool _needsReallocation;

    /// Limits the number of ranges that can be
    /// allocated to this buffer to max.
    void _SetMaxNumRanges(size_t max) { _maxNumRanges = max; }

    /// Swap the rangelist with \p ranges
    HD_API
    void _SetRangeList(std::vector<HdBufferArrayRangeSharedPtr> const &ranges);

private:

    // Do not allow copies.
    HdBufferArray(const HdBufferArray &) = delete;
    HdBufferArray &operator=(const HdBufferArray &) = delete;


    typedef std::vector<HdBufferArrayRangePtr> _RangeList;

    // Vector of ranges associated with this buffer
    // We add values to the list in a multi-threaded fashion
    // but can later remove them in _RemoveUnusedRanges
    // than add more.
    //
    _RangeList         _rangeList;
    std::atomic_size_t _rangeCount;               // how many ranges are valid in list
    std::mutex         _rangeListLock;

    const TfToken _role;
    const TfToken _garbageCollectionPerfToken;

    size_t _version;

    size_t _maxNumRanges;
    HdBufferArrayUsageHint _usageHint;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_BUFFER_ARRAY_H
