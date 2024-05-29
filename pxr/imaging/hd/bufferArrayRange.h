//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_BUFFER_ARRAY_RANGE_H
#define PXR_IMAGING_HD_BUFFER_ARRAY_RANGE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/bufferArray.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using HdBufferSpecVector = std::vector<struct HdBufferSpec>;
using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;
using HdBufferSourceSharedPtr = std::shared_ptr<class HdBufferSource>;

/// \class HdBufferArrayRange
///
/// Interface class for representing range (subset) locator of HdBufferArray.
/// 
/// Each memory management strategy defines a specialized range class which is
/// inherited of this interface so that client (drawItem) can be agnostic about
/// the implementation detail of aggregation.
///
class HdBufferArrayRange 
{
public:

    HD_API
    HdBufferArrayRange();

    /// Destructor (do nothing).
    /// The specialized range class may want to do something for garbage
    /// collection in its destructor. However, be careful not do any
    /// substantial work here (obviously including any kind of GL calls),
    /// since the destructor gets called frequently on various contexts.
    HD_API
    virtual ~HdBufferArrayRange();

    /// Returns true if this range is valid
    virtual bool IsValid() const = 0;

    /// Returns true is the range has been assigned to a buffer
    virtual bool IsAssigned() const = 0;

    /// Returns true if this range is marked as immutable.
    virtual bool IsImmutable() const = 0;

    /// Returns true if this needs a staging buffer for CPU to GPU copies.
    virtual bool RequiresStaging() const = 0;

    /// Resize memory area for this range. Returns true if it causes container
    /// buffer reallocation.
    virtual bool Resize(int numElements) = 0;

    /// Copy source data into buffer
    virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource) = 0;

    /// Read back the buffer content
    virtual VtValue ReadData(TfToken const &name) const = 0;

    /// Returns the offset at which this range begins in the underlying buffer 
    /// array in terms of elements.
    virtual int GetElementOffset() const = 0;
    
    /// Returns the byte offset at which this range begins in the underlying
    /// buffer array for the given resource.
    virtual int GetByteOffset(TfToken const& resourceName) const = 0;
    
    /// Returns the number of elements
    virtual size_t GetNumElements() const = 0;

    /// Returns the version of the buffer array.
    virtual size_t GetVersion() const = 0;

    /// Increment the version of the buffer array. mostly used for notifying
    /// drawbatches to be rebuilt to remove expired BufferArrayRange.
    virtual void IncrementVersion() = 0;

    /// Returns the max number of elements
    virtual size_t GetMaxNumElements() const = 0;

    /// Gets the usage hint on the underlying buffer array
    virtual HdBufferArrayUsageHint GetUsageHint() const = 0;

    /// Sets the buffer array associated with this buffer;
    virtual void SetBufferArray(HdBufferArray *bufferArray) = 0;

    /// Debug output
    virtual void DebugDump(std::ostream &out) const = 0;

    /// Returns true if the underlying buffer array is aggregated to other's
    bool IsAggregatedWith(HdBufferArrayRangeSharedPtr const &other) const {
        return (other && (_GetAggregation() == other->_GetAggregation()));
    }

    /// Gets the bufferSpecs for all resources.
    virtual void GetBufferSpecs(HdBufferSpecVector *bufferSpecs) const = 0;

protected:
    /// Returns the aggregation container to be used in IsAggregatedWith()
    virtual const void *_GetAggregation() const = 0;

    // Don't allow copies
    HdBufferArrayRange(const HdBufferArrayRange &) = delete;
    HdBufferArrayRange &operator=(const HdBufferArrayRange &) = delete;

};

HD_API
std::ostream &operator <<(std::ostream &out,
                          const HdBufferArrayRange &self);

/// \class HdBufferArrayRangeContainer
///
/// A resizable container of HdBufferArrayRanges.
///
class HdBufferArrayRangeContainer
{
public:
    /// Constructor
    HdBufferArrayRangeContainer(int size) : _ranges(size) { }

    /// Set \p range into the container at \p index.
    /// If the size of container is smaller than index, resize it.
    HD_API
    void Set(int index, HdBufferArrayRangeSharedPtr const &range);

    /// Returns the bar at \p index. returns null if either the index
    /// is out of range or not yet set.
    HD_API
    HdBufferArrayRangeSharedPtr const &Get(int index) const;

    /// Resize the buffer array range container to size \p size.
    /// Used to explicitly resize or shrink the container.
    HD_API
    void Resize(int size);

private:
    std::vector<HdBufferArrayRangeSharedPtr> _ranges;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_BUFFER_ARRAY_RANGE_H
