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
#ifndef HD_BUFFER_ARRAY_RANGE_H
#define HD_BUFFER_ARRAY_RANGE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/bufferResource.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferArray;

typedef std::vector<struct HdBufferSpec> HdBufferSpecVector;
typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;
typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;

/// \class HdBufferArrayRange
///
/// Interface class for representing range (subset) locator of HdBufferArray.
/// 
/// Each memory management strategy defines a specialized range class which is
/// inherited of this interface so that client (drawItem) can be agnostic about
/// the implementation detail of aggregation.
///
class HdBufferArrayRange : boost::noncopyable {
public:
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

    /// Resize memory area for this range. Returns true if it causes container
    /// buffer reallocation.
    virtual bool Resize(int numElements) = 0;

    /// Copy source data into buffer
    virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource) = 0;

    /// Read back the buffer content
    virtual VtValue ReadData(TfToken const &name) const = 0;

    /// Returns the relative offset for this range
    virtual int GetOffset() const = 0;

    /// Returns the index for this range
    virtual int GetIndex() const = 0;

    /// Returns the number of elements
    virtual int GetNumElements() const = 0;

    /// Returns the version of the buffer array.
    virtual size_t GetVersion() const = 0;

    /// Increment the version of the buffer array. mostly used for notifying
    /// drawbatches to be rebuilt to remove expired BufferArrayRange.
    virtual void IncrementVersion() = 0;

    /// Returns the max number of elements
    virtual size_t GetMaxNumElements() const = 0;

    /// Returns the GPU resource. If the buffer array contains more than one
    /// resource, this method raises a coding error.
    virtual HdBufferResourceSharedPtr GetResource() const = 0;

    /// Returns the named GPU resource.
    virtual HdBufferResourceSharedPtr GetResource(TfToken const& name) = 0;

    /// Returns the list of all named GPU resources for this bufferArrayRange.
    virtual HdBufferResourceNamedList const& GetResources() const = 0;

    /// Sets the buffer array assosiated with this buffer;
    virtual void SetBufferArray(HdBufferArray *bufferArray) = 0;

    /// Debug output
    virtual void DebugDump(std::ostream &out) const = 0;

    /// Returns true if the underlying buffer array is aggregated to other's
    bool IsAggregatedWith(HdBufferArrayRangeSharedPtr const &other) const {
        return (other && (_GetAggregation() == other->_GetAggregation()));
    }

    /// Sets the bufferSpecs for all resources.
    HD_API
    void AddBufferSpecs(HdBufferSpecVector *bufferSpecs) const;

protected:
    /// Returns the aggregation container to be used in IsAggregatedWith()
    virtual const void *_GetAggregation() const = 0;

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
    // is out of range or not yet set.
    HD_API
    HdBufferArrayRangeSharedPtr const &Get(int index) const;

private:
    std::vector<HdBufferArrayRangeSharedPtr> _ranges;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_BUFFER_ARRAY_RANGE_H
