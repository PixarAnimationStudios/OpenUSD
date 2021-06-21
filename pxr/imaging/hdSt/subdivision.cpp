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
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/subdivision.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include <opensubdiv/version.h>

PXR_NAMESPACE_OPEN_SCOPE


/*virtual*/
HdSt_Subdivision::~HdSt_Subdivision()
{
}

bool
HdSt_Subdivision::RefinesToTriangles(TfToken const &scheme)
{
    // XXX: Ideally we'd like to delegate this to the concrete class.
    if (scheme == PxOsdOpenSubdivTokens->loop) {
        return true;
    }
    return false;
}

bool
HdSt_Subdivision::RefinesToBSplinePatches(TfToken const &scheme)
{
    return scheme == PxOsdOpenSubdivTokens->catmullClark;
}

bool
HdSt_Subdivision::RefinesToBoxSplineTrianglePatches(TfToken const &scheme)
{
#if OPENSUBDIV_VERSION_NUMBER >= 30400
    // v3.4.0 added support for limit surface patches for loop meshes
    if (scheme == PxOsdOpenSubdivTokens->loop) {
        return true;
    }
#endif
    return false;
}

// ---------------------------------------------------------------------------
HdSt_OsdTopologyComputation::HdSt_OsdTopologyComputation(
    HdSt_MeshTopology *topology, int level, SdfPath const &id)
    : _topology(topology), _level(level), _id(id)
{
}

/*virtual*/
void
HdSt_OsdTopologyComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
}

// ---------------------------------------------------------------------------
HdSt_OsdIndexComputation::HdSt_OsdIndexComputation(
    HdSt_MeshTopology *topology, HdBufferSourceSharedPtr const &osdTopology)
    : _topology(topology)
    , _osdTopology(osdTopology)
{
}

/*virtual*/
void
HdSt_OsdIndexComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    if (_topology->RefinesToBSplinePatches()) {
        // bi-cubic bspline patches
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32, 16});
        // 3+1 (includes sharpness)
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec4, 1});
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec2, 1});
    } else if (_topology->RefinesToBoxSplineTrianglePatches()) {
        // quartic box spline triangle patches
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32, 12});
        // 3+1 (includes sharpness)
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec4, 1});
        // int will suffice, but this unifies it for all the cases
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec2, 1});
    } else if (HdSt_Subdivision::RefinesToTriangles(_topology->GetScheme())) {
        // triangles (loop)
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32Vec3, 1});
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec3, 1});
        // int will suffice, but this unifies it for all the cases
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec2, 1});
    } else {
        // quads (catmark, bilinear)
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32Vec4, 1});
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec3, 1});
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec2, 1});
    }
}

/*virtual*/
bool
HdSt_OsdIndexComputation::HasChainedBuffer() const
{
    return true;
}

/*virtual*/
HdBufferSourceSharedPtrVector
HdSt_OsdIndexComputation::GetChainedBuffers() const
{
    return { _primitiveBuffer, _edgeIndicesBuffer };
}

/*virtual*/
bool
HdSt_OsdIndexComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------
/// OpenSubdiv CPU Refinement
///
///
HdSt_OsdRefineComputation::HdSt_OsdRefineComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    HdBufferSourceSharedPtr const &osdTopology,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel)
    : _topology(topology)
    , _source(source)
    , _osdTopology(osdTopology)
    , _interpolation(interpolation)
    , _fvarChannel(fvarChannel)
{
}

HdSt_OsdRefineComputation::~HdSt_OsdRefineComputation()
{
}

TfToken const &
HdSt_OsdRefineComputation::GetName() const
{
    return _source->GetName();
}

template <class HashState>
void TfHashAppend(HashState &h,
                  HdSt_OsdRefineComputation const &bs)
{
    h.Append(bs.GetInterpolation());
}

size_t
HdSt_OsdRefineComputation::ComputeHash() const
{
    return TfHash()(*this);
}

void const *
HdSt_OsdRefineComputation::GetData() const
{
    return _primvarBuffer.data();
}

HdTupleType
HdSt_OsdRefineComputation::GetTupleType() const
{
    return _source->GetTupleType();
}

size_t
HdSt_OsdRefineComputation::GetNumElements() const
{
    // Stride is measured here in components, not bytes.
    size_t const elementStride =
        HdGetComponentCount(_source->GetTupleType().type);
    return _primvarBuffer.size() / elementStride;
}

HdSt_MeshTopology::Interpolation
HdSt_OsdRefineComputation::GetInterpolation() const
{
    return _interpolation;
}

bool
HdSt_OsdRefineComputation::Resolve()
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
    subdivision->RefineCPU(_source,
                           &_primvarBuffer,
                           _interpolation,
                           _fvarChannel);

    HD_PERF_COUNTER_INCR(HdPerfTokens->subdivisionRefineCPU);

    _SetResolved();
    return true;
}

bool
HdSt_OsdRefineComputation::_CheckValid() const
{
    bool valid = _source->IsValid();

    // _osdTopology is optional
    valid &= _osdTopology ? _osdTopology->IsValid() : true;

    return valid;
}

void
HdSt_OsdRefineComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same buffer specs as source
    _source->GetBufferSpecs(specs);
}

bool
HdSt_OsdRefineComputation::HasPreChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtr
HdSt_OsdRefineComputation::GetPreChainedBuffer() const
{
    return _source;
}

// ---------------------------------------------------------------------------
/// OpenSubdiv GPU Refinement
///
///
HdSt_OsdRefineComputationGPU::HdSt_OsdRefineComputationGPU(
    HdSt_MeshTopology *topology,
    TfToken const &name,
    HdType type,
    HdSt_GpuStencilTableSharedPtr const & gpuStencilTable,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel)
    : _topology(topology)
    , _name(name)
    , _gpuStencilTable(gpuStencilTable)
    , _interpolation(interpolation)
    , _fvarChannel(fvarChannel)
{
}

void
HdSt_OsdRefineComputationGPU::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
    //
    // GPU subdivision requires the source data on GPU in prior to
    // execution, so no need to populate bufferspec on registration.
}

void
HdSt_OsdRefineComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range,
                                      HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSt_Subdivision *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return;

    HdStResourceRegistry* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);

    subdivision->RefineGPU(range, _name,
                           _gpuStencilTable,
                           hdStResourceRegistry);

    HD_PERF_COUNTER_INCR(HdPerfTokens->subdivisionRefineGPU);
}

int
HdSt_OsdRefineComputationGPU::GetNumOutputElements() const
{
    // returns the total number of vertices, including coarse and refined ones
    HdSt_Subdivision const *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return 0;
    if (_interpolation == HdSt_MeshTopology::INTERPOLATE_VERTEX) {
        return subdivision->GetNumVertices();
    } else if (_interpolation == HdSt_MeshTopology::INTERPOLATE_VARYING) {
        return subdivision->GetNumVarying();
    } else {
        return subdivision->GetMaxNumFaceVarying();
    }
}

HdSt_MeshTopology::Interpolation 
HdSt_OsdRefineComputationGPU::GetInterpolation() const
{
    return _interpolation;
}

PXR_NAMESPACE_CLOSE_SCOPE

