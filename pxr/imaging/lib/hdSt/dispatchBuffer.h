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
#ifndef HDST_DISPATCH_BUFFER_H
#define HDST_DISPATCH_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdStDispatchBuffer> HdStDispatchBufferSharedPtr;

/// \class HdStDispatchBuffer
///
/// A VBO of a simple array of GLuint.
///
/// This buffer is used to prepare data on the GPU for indirect dispatch i.e.
/// to be consumed by glMultiDrawIndirect or glDispatchComputeIndirect. At the
/// same time, interleaved subsets of the array are bound in several different
/// ways to provide additional data interface to shaders.
///
/// For each binding, we define 'BufferResourceView' on top of the GLuint array.
/// HdBufferArray aggregates those views and HdResourceBinder binds them
/// with specified binding method and interleaved offset.
///
/// Example:
///    DrawElements + Instance culling : 14 integers for each drawitem
///
///                              BufferResourceViews    BufferResourceViews
///                                 for draw               for cull
///
/// +----draw item 0----+----------------------------> destination buffer
/// | count             | --+
/// | instanceCount     |   |
/// | first             |   |----> MDI dispatch
/// | baseVertex        |   |
/// | baseInstance      | --+-------------------------> drawitem index
/// | cullCount         | ----+
/// | cullInstanceCount |     |------------------------> MDI dispatch
/// | cullFirstVertex   |     |
/// | cullBaseInstance  | ----+
/// | modelDC           | --+
/// | constantDC        |   |----> DrawingCoord0 -------> DrawingCoord0
/// | elementDC         |   |
/// | primitiveDC       | --+
/// | fvarDC            | --+
/// | instanceIndexDC   |   |----> DrawingCoord1 -------> DrawingCoord1
/// | shaderDC          | --+
/// | (instanceDC[0])   | --+
/// | (instanceDC[1])   |   |----> DrawingCoordI -------> DrawingCoordI
/// | (instanceDC[2])   |   |
/// | ...               | --+
/// +----draw item 1----+
/// | count             |
/// | instanceCount     |
/// | ...               |
///
/// XXX: it would be better to generalize this class not only for dispatch
/// buffer, if we see other similar use-cases.
///
class HdStDispatchBuffer : public HdBufferArray {
public:
    /// Constructor. commandNumUints is given in how many integers.
    HDST_API
    HdStDispatchBuffer(TfToken const &role, int count,
                     unsigned int commandNumUints);

    /// Destructor.
    HDST_API
    ~HdStDispatchBuffer();

    /// Update entire buffer data
    HDST_API
    void CopyData(std::vector<GLuint> const &data);

    /// Add an interleaved view to this buffer.
    HDST_API
    void AddBufferResourceView(TfToken const &name, GLenum glDataType,
                               int numComponents, int offset);

    /// Returns the dispatch count
    int GetCount() const { return _count; }

    /// Returns the number of GLuints in a single draw command.
    unsigned int GetCommandNumUints() const { return _commandNumUints; }

    /// Returns a bar which locates all interleaved resources of the entire
    /// buffer.
    HdStBufferArrayRangeGLSharedPtr GetBufferArrayRange() const {
        return _bar;
    }

    /// Returns entire buffer as a single HdBufferResource.
    HdStBufferResourceGLSharedPtr GetEntireResource() const {
        return _entireResource;
    }

    // HdBufferArray overrides. they are not supported in this class.
    HDST_API
    virtual bool GarbageCollect();
    HDST_API
    virtual void Reallocate(
        std::vector<HdBufferArrayRangeSharedPtr> const &,
        HdBufferArraySharedPtr const &);

    HDST_API
    virtual void DebugDump(std::ostream &out) const;

    /// Returns the GPU resource. If the buffer array contains more than one
    /// resource, this method raises a coding error.
    HDST_API
    HdStBufferResourceGLSharedPtr GetResource() const;

    /// Returns the named GPU resource. This method returns the first found
    /// resource. In HDST_SAFE_MODE it checkes all underlying GL buffers
    /// in _resourceMap and raises a coding error if there are more than
    /// one GL buffers exist.
    HDST_API
    HdStBufferResourceGLSharedPtr GetResource(TfToken const& name);

    /// Returns the list of all named GPU resources for this bufferArray.
    HdStBufferResourceGLNamedList const& GetResources() const {return _resourceList;}

protected:
    /// Adds a new, named GPU resource and returns it.
    HDST_API
    HdStBufferResourceGLSharedPtr _AddResource(TfToken const& name,
                                           int glDataType,
                                           short numComponents,
                                           int arraySize,
                                           int offset,
                                           int stride);

private:
    int _count;
    unsigned int _commandNumUints;
    HdStBufferResourceGLNamedList _resourceList;
    HdStBufferResourceGLSharedPtr _entireResource;
    HdStBufferArrayRangeGLSharedPtr _bar;  // Alternative to range list in base class
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_DISPATCH_BUFFER_H
