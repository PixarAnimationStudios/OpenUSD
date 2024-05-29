//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_BUFFER_ARRAY_RANGE_H
#define PXR_IMAGING_HD_ST_BUFFER_ARRAY_RANGE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/bufferArrayRange.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferArrayGL;
class HdStResourceRegistry;

using HdStBufferArrayRangeSharedPtr = 
    std::shared_ptr<class HdStBufferArrayRange>;

class HdStBufferResource;

using HdStBufferResourceSharedPtr = 
    std::shared_ptr<class HdStBufferResource>;
using HdStBufferResourceNamedList =
    std::vector< std::pair<TfToken, HdStBufferResourceSharedPtr> >;

/// \class HdStBufferArrayRange
///
/// Interface class for representing range (subset) locator of HdBufferArray.
/// 
/// Each memory management strategy defines a specialized range class which is
/// inherited of this interface so that client (drawItem) can be agnostic about
/// the implementation detail of aggregation.
///
class HdStBufferArrayRange : public HdBufferArrayRange 
{
public:
    HdStBufferArrayRange(HdStResourceRegistry* resourceRegistry);

    /// Destructor (do nothing).
    /// The specialized range class may want to do something for garbage
    /// collection in its destructor. However, be careful not do any
    /// substantial work here (obviously including any kind of GL calls),
    /// since the destructor gets called frequently on various contexts.
    HDST_API
    virtual ~HdStBufferArrayRange();

    /// Returns the GPU resource. If the buffer array contains more than one
    /// resource, this method raises a coding error.
    virtual HdStBufferResourceSharedPtr GetResource() const = 0;

    /// Returns the named GPU resource.
    virtual HdStBufferResourceSharedPtr GetResource(TfToken const& name) = 0;

    /// Returns the list of all named GPU resources for this bufferArrayRange.
    virtual HdStBufferResourceNamedList const& GetResources() const = 0;

    /// Sets the bufferSpecs for all resources.
    HDST_API
    virtual void GetBufferSpecs(HdBufferSpecVector *bufferSpecs) const override;

    virtual int GetElementStride() const {
        return 0;
    }
    
protected:
    HdStResourceRegistry* GetResourceRegistry();

    HdStResourceRegistry* GetResourceRegistry() const;
    
private:
    HdStResourceRegistry* _resourceRegistry;
};

HDST_API
std::ostream &operator <<(std::ostream &out,
                          const HdStBufferArrayRange &self);

/// \class HdStBufferArrayRangeContainer
///
/// A resizable container of HdBufferArrayRanges.
///
class HdStBufferArrayRangeContainer
{
public:
    /// Constructor
    HdStBufferArrayRangeContainer(int size) : _ranges(size) { }

    /// Set \p range into the container at \p index.
    /// If the size of container is smaller than index, resize it.
    HDST_API
    void Set(int index, HdStBufferArrayRangeSharedPtr const &range);

    /// Returns the bar at \p index. returns null if either the index
    // is out of range or not yet set.
    HDST_API
    HdStBufferArrayRangeSharedPtr const &Get(int index) const;

private:
    std::vector<HdStBufferArrayRangeSharedPtr> _ranges;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_BUFFER_ARRAY_RANGE_GL_H
