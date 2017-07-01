//
// Copyright 2016 Pixar
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
#ifndef HD_BUFFER_ARRAY_H
#define HD_BUFFER_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferResource.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <atomic>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferArrayRange;

typedef boost::shared_ptr<class HdBufferArray> HdBufferArraySharedPtr;
typedef boost::shared_ptr<HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef boost::weak_ptr<HdBufferArrayRange> HdBufferArrayRangePtr;
typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;

/// \class HdBufferArray
///
/// Similar to a VAO, this object is a bundle of coherent buffers. This object
/// can be shared across multiple HdRprims, in the context of buffer
/// aggregation.
///
class HdBufferArray : public boost::enable_shared_from_this<HdBufferArray>,
    boost::noncopyable {
public:
    HD_API
    HdBufferArray(TfToken const &role,
                  TfToken const garbageCollectionPerfToken,
                  bool isImmutable=false);

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
        return _isImmutable;
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
    typedef std::vector<HdBufferArrayRangePtr> _RangeList;

    // Vector of ranges assosiated with this buffer
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

    size_t             _maxNumRanges;
    bool               _isImmutable;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_BUFFER_ARRAY_H
