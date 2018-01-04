//
// Copyright 2017 Pixar
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
#ifndef HDST_BUFFER_ARRAY_RANGE_GL_H
#define HDST_BUFFER_ARRAY_RANGE_GL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/bufferArrayRange.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferArrayGL;

typedef boost::shared_ptr<class HdStBufferArrayRangeGL> HdStBufferArrayRangeGLSharedPtr;

class HdStBufferResourceGL;

typedef boost::shared_ptr<class HdStBufferResourceGL> HdStBufferResourceGLSharedPtr;
typedef std::vector<
    std::pair<TfToken, HdStBufferResourceGLSharedPtr> > HdStBufferResourceGLNamedList;

/// \class HdStBufferArrayRangeGL
///
/// Interface class for representing range (subset) locator of HdBufferArray.
/// 
/// Each memory management strategy defines a specialized range class which is
/// inherited of this interface so that client (drawItem) can be agnostic about
/// the implementation detail of aggregation.
///
class HdStBufferArrayRangeGL : public HdBufferArrayRange 
{
public:
    /// Destructor (do nothing).
    /// The specialized range class may want to do something for garbage
    /// collection in its destructor. However, be careful not do any
    /// substantial work here (obviously including any kind of GL calls),
    /// since the destructor gets called frequently on various contexts.
    HDST_API
    virtual ~HdStBufferArrayRangeGL();

    /// Returns the GPU resource. If the buffer array contains more than one
    /// resource, this method raises a coding error.
    virtual HdStBufferResourceGLSharedPtr GetResource() const = 0;

    /// Returns the named GPU resource.
    virtual HdStBufferResourceGLSharedPtr GetResource(TfToken const& name) = 0;

    /// Returns the list of all named GPU resources for this bufferArrayRange.
    virtual HdStBufferResourceGLNamedList const& GetResources() const = 0;

    /// Sets the bufferSpecs for all resources.
    HDST_API
    virtual void AddBufferSpecs(HdBufferSpecVector *bufferSpecs) const;
};

HDST_API
std::ostream &operator <<(std::ostream &out,
                          const HdStBufferArrayRangeGL &self);

/// \class HdStBufferArrayRangeGLContainer
///
/// A resizable container of HdBufferArrayRanges.
///
class HdStBufferArrayRangeGLContainer
{
public:
    /// Constructor
    HdStBufferArrayRangeGLContainer(int size) : _ranges(size) { }

    /// Set \p range into the container at \p index.
    /// If the size of container is smaller than index, resize it.
    HDST_API
    void Set(int index, HdStBufferArrayRangeGLSharedPtr const &range);

    /// Returns the bar at \p index. returns null if either the index
    // is out of range or not yet set.
    HDST_API
    HdStBufferArrayRangeGLSharedPtr const &Get(int index) const;

private:
    std::vector<HdStBufferArrayRangeGLSharedPtr> _ranges;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_BUFFER_ARRAY_RANGE_GL_H
