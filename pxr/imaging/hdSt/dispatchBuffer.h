//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DISPATCH_BUFFER_H
#define PXR_IMAGING_HD_ST_DISPATCH_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;

using HdStDispatchBufferSharedPtr = std::shared_ptr<class HdStDispatchBuffer>;

/// \class HdStDispatchBuffer
///
/// A VBO of a simple array of unsigned integers.
///
/// This buffer is used to prepare data on the GPU for indirect dispatch i.e.
/// to be consumed by MultiDrawIndirect or DispatchComputeIndirect. At the
/// same time, interleaved subsets of the array are bound in several different
/// ways to provide additional data interface to shaders.
///
/// For each binding, we define 'BufferResourceView' on top of the uint array.
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
class HdStDispatchBuffer : public HdBufferArray
{
public:
    /// Constructor. commandNumUints is given in how many integers.
    HDST_API
    HdStDispatchBuffer(HdStResourceRegistry* resourceRegistry,
                       TfToken const &role,
                       int count,
                       unsigned int commandNumUints);

    /// Destructor.
    HDST_API
    ~HdStDispatchBuffer() override;

    /// Update entire buffer data
    HDST_API
    void CopyData(std::vector<uint32_t> const &data);

    /// Add an interleaved view to this buffer.
    HDST_API
    void AddBufferResourceView(TfToken const &name,
                               HdTupleType tupleType, int offset);

    /// Returns the dispatch count
    int GetCount() const { return _count; }

    /// Returns the number of uints in a single draw command.
    unsigned int GetCommandNumUints() const { return _commandNumUints; }

    /// Returns a bar which locates all interleaved resources of the entire
    /// buffer.
    HdStBufferArrayRangeSharedPtr GetBufferArrayRange() const {
        return _bar;
    }

    /// Returns entire buffer as a single HdStBufferResource.
    HdStBufferResourceSharedPtr GetEntireResource() const {
        return _entireResource;
    }

    // HdBufferArray overrides. they are not supported in this class.
    HDST_API
    bool GarbageCollect() override;
    HDST_API
    void Reallocate(
        std::vector<HdBufferArrayRangeSharedPtr> const &,
        HdBufferArraySharedPtr const &) override;

    HDST_API
    void DebugDump(std::ostream &out) const override;

    /// Returns the GPU resource. If the buffer array contains more than one
    /// resource, this method raises a coding error.
    HDST_API
    HdStBufferResourceSharedPtr GetResource() const;

    /// Returns the named GPU resource. This method returns the first found
    /// resource. In HDST_SAFE_MODE it checks all underlying GPU buffers
    /// in _resourceMap and raises a coding error if there are more than
    /// one GPU buffers exist.
    HDST_API
    HdStBufferResourceSharedPtr GetResource(TfToken const& name);

    /// Returns the list of all named GPU resources for this bufferArray.
    HdStBufferResourceNamedList const& GetResources() const {return _resourceList;}

protected:
    /// Adds a new, named GPU resource and returns it.
    HDST_API
    HdStBufferResourceSharedPtr _AddResource(TfToken const& name,
                                               HdTupleType tupleType,
                                               int offset,
                                               int stride);

private:
    HdStResourceRegistry *_resourceRegistry;
    int _count;
    unsigned int _commandNumUints;
    HdStBufferResourceNamedList _resourceList;
    HdStBufferResourceSharedPtr _entireResource;
    HdStBufferArrayRangeSharedPtr _bar;  // Alternative to range list in base class
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_DISPATCH_BUFFER_H
