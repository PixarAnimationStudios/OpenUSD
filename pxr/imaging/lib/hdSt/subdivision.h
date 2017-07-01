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
#ifndef HDST_SUBDIVISION_H
#define HDST_SUBDIVISION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class Hd_Subdivision
///
/// Subdivision struct holding subdivision tables and patch tables.
///
/// This single struct can be used for cpu and gpu subdivision at the same time.
///
class HdSt_Subdivision {
public:
    virtual ~HdSt_Subdivision();

    virtual int GetNumVertices() const = 0;

    virtual void RefineCPU(HdBufferSourceSharedPtr const &source,
                           bool varying,
                           void *vertexBuffer) = 0;
    virtual void RefineGPU(HdBufferArrayRangeSharedPtr const &range,
                           TfToken const &name) = 0;

    // computation factory methods
    virtual HdBufferSourceSharedPtr CreateTopologyComputation(
        HdSt_MeshTopology *topology,
        bool adaptive,
        int level,
        SdfPath const &id) = 0;

    virtual HdBufferSourceSharedPtr CreateIndexComputation(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const &osdTopology) = 0;

    virtual HdBufferSourceSharedPtr CreateRefineComputation(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const &source,
        bool varying,
        HdBufferSourceSharedPtr const &osdTopology) = 0;

    virtual HdComputationSharedPtr CreateRefineComputationGPU(
        HdSt_MeshTopology *topology,
        TfToken const &name,
        GLenum dataType,
        int numComponents) = 0;

    /// Returns true if the subdivision for \a scheme generates triangles,
    /// instead of quads.
    static bool RefinesToTriangles(TfToken const &scheme);

    /// Returns true if the subdivision for \a scheme generates bspline patches.
    static bool RefinesToBSplinePatches(TfToken const &scheme);
};

// ---------------------------------------------------------------------------
/// \class Hd_OsdTopologyComputation
///
/// OpenSubdiv Topology Analysis.
/// Create Hd_Subdivision struct and sets it into HdSt_MeshTopology.
///
class HdSt_OsdTopologyComputation : public HdComputedBufferSource {
public:
    HdSt_OsdTopologyComputation(HdSt_MeshTopology *topology,
                                int level,
                                SdfPath const &id);

    /// overrides
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
    virtual bool Resolve() = 0;

protected:
    HdSt_MeshTopology *_topology;
    int _level;
    SdfPath const _id;
};

// ---------------------------------------------------------------------------
/// \class HdSt_OsdIndexComputation
///
/// OpenSubdiv refined index buffer computation.
///
/// computes index buffer and primitiveParam
///
/// primitiveParam : refined quads to coarse faces mapping buffer
///
/// ----+-----------+-----------+------
/// ... |i0 i1 i2 i3|i4 i5 i6 i7| ...    index buffer (for quads)
/// ----+-----------+-----------+------
/// ... |           |           | ...    primitive param[0] (coarse face index)
/// ... |     p0    |     p1    | ...    primitive param[1] (patch param 0)
/// ... |           |           | ...    primitive param[2] (patch param 1)
/// ----+-----------+-----------+------
///
class HdSt_OsdIndexComputation : public HdComputedBufferSource {
public:
    /// overrides
    virtual bool HasChainedBuffer() const;
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
    virtual HdBufferSourceSharedPtr GetChainedBuffer() const;
    virtual bool Resolve() = 0;

protected:
    HdSt_OsdIndexComputation(HdSt_MeshTopology *topology,
                           HdBufferSourceSharedPtr const &osdTopology);

    virtual bool _CheckValid() const;

    HdSt_MeshTopology *_topology;
    HdBufferSourceSharedPtr _osdTopology;
    HdBufferSourceSharedPtr _primitiveBuffer;
};

// ---------------------------------------------------------------------------
/// \class Hd_OsdRefineComputation
///
/// OpenSubdiv CPU Refinement.
/// This class isn't inherited from HdComputedBufferSource.
/// GetData() returns the internal buffer of Hd_OsdCpuVertexBuffer,
/// so that reducing data copy between osd buffer and HdBufferSource.
///
template <typename VERTEX_BUFFER>
class HdSt_OsdRefineComputation : public HdBufferSource {
public:
    HdSt_OsdRefineComputation(HdSt_MeshTopology *topology,
                            HdBufferSourceSharedPtr const &source,
                            bool varying,
                            HdBufferSourceSharedPtr const &osdTopology);
    virtual ~HdSt_OsdRefineComputation();
    virtual TfToken const &GetName() const;
    virtual size_t ComputeHash() const;
    virtual void const* GetData() const;
    virtual int GetGLComponentDataType() const;
    virtual int GetGLElementDataType() const;
    virtual int GetNumElements() const;
    virtual short GetNumComponents() const;
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
    virtual bool Resolve();
    virtual bool HasPreChainedBuffer() const;
    virtual HdBufferSourceSharedPtr GetPreChainedBuffer() const;

protected:
    virtual bool _CheckValid() const;

private:
    HdSt_MeshTopology *_topology;
    HdBufferSourceSharedPtr _source;
    HdBufferSourceSharedPtr _osdTopology;
    VERTEX_BUFFER *_cpuVertexBuffer;
    bool _varying;
};

// ---------------------------------------------------------------------------
/// \class HdSt_OsdRefineComputationGPU
///
/// OpenSubdiv GPU Refinement.
///
class HdSt_OsdRefineComputationGPU : public HdComputation {
public:
    HdSt_OsdRefineComputationGPU(HdSt_MeshTopology *topology,
                               TfToken const &name,
                               GLenum dataType,
                               int numComponents);

    virtual void Execute(HdBufferArrayRangeSharedPtr const &range);
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
    virtual int GetNumOutputElements() const;

    // A wrapper class to bridge between HdBufferResource and OpenSubdiv
    // vertex buffer API.
    //
    class VertexBuffer {
    public:
        VertexBuffer(HdBufferResourceSharedPtr const &resource) { 
            _resource =
                boost::static_pointer_cast<HdBufferResourceGL> (resource);
        }

        // bit confusing, osd expects 'GetNumElements()' returns the num components,
        // in hydra sense
        int GetNumElements() const {
            return _resource->GetNumComponents();
        }
        GLuint BindVBO() {
            return _resource->GetId();
        }
        HdBufferResourceGLSharedPtr _resource;
    };

private:
    HdSt_MeshTopology *_topology;
    TfToken _name;
    GLenum  _dataType;
    int _numComponents;
};

// ---------------------------------------------------------------------------
// template implementations
template <typename VERTEX_BUFFER>
HdSt_OsdRefineComputation<VERTEX_BUFFER>::HdSt_OsdRefineComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    bool varying,
    HdBufferSourceSharedPtr const &osdTopology)
    : _topology(topology), _source(source), _osdTopology(osdTopology),
      _cpuVertexBuffer(NULL), _varying(varying)
{
}

template <typename VERTEX_BUFFER>
HdSt_OsdRefineComputation<VERTEX_BUFFER>::~HdSt_OsdRefineComputation()
{
    delete _cpuVertexBuffer;
}

template <typename VERTEX_BUFFER>
TfToken const &
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetName() const
{
    return _source->GetName();
}

template <typename VERTEX_BUFFER>
size_t
HdSt_OsdRefineComputation<VERTEX_BUFFER>::ComputeHash() const
{
    return 0;
}

template <typename VERTEX_BUFFER>
void const*
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetData() const
{
    return _cpuVertexBuffer->BindCpuBuffer();
}

template <typename VERTEX_BUFFER>
int
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetGLComponentDataType() const
{
    return _source->GetGLComponentDataType();
}

template <typename VERTEX_BUFFER>
int
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetGLElementDataType() const
{
    return _source->GetGLElementDataType();
}

template <typename VERTEX_BUFFER>
int
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetNumElements() const
{
    return _cpuVertexBuffer->GetNumVertices();
}

template <typename VERTEX_BUFFER>
short
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetNumComponents() const
{
    return _cpuVertexBuffer->GetNumElements();
}

template <typename VERTEX_BUFFER>
bool
HdSt_OsdRefineComputation<VERTEX_BUFFER>::Resolve()
{
    if (_source && !_source->IsResolved()) return false;
    if (_osdTopology && !_osdTopology->IsResolved()) return false;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSt_Subdivision *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) {
        _SetResolved();
        return true;
    }

    // prepare cpu vertex buffer including refined vertices
    TF_VERIFY(!_cpuVertexBuffer);
    _cpuVertexBuffer = VERTEX_BUFFER::Create(_source->GetNumComponents(),
                                             subdivision->GetNumVertices());

    subdivision->RefineCPU(_source, _varying, _cpuVertexBuffer);

    HD_PERF_COUNTER_INCR(HdPerfTokens->subdivisionRefineCPU);

    _SetResolved();
    return true;
}

template <typename VERTEX_BUFFER>
bool
HdSt_OsdRefineComputation<VERTEX_BUFFER>::_CheckValid() const
{
    bool valid = _source->IsValid();

    // _osdTopology is optional
    valid &= _osdTopology ? _osdTopology->IsValid() : true;

    return valid;
}

template <typename VERTEX_BUFFER>
void
HdSt_OsdRefineComputation<VERTEX_BUFFER>::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same spec buffer as source
    _source->AddBufferSpecs(specs);
}

template <typename VERTEX_BUFFER>
bool
HdSt_OsdRefineComputation<VERTEX_BUFFER>::HasPreChainedBuffer() const
{
    return true;
}

template <typename VERTEX_BUFFER>
HdBufferSourceSharedPtr
HdSt_OsdRefineComputation<VERTEX_BUFFER>::GetPreChainedBuffer() const
{
    return _source;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_SUBDIVISION_H
