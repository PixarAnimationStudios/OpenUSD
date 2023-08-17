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

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/subdivision.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/pxOsd/refinerFactory.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <opensubdiv/version.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>

PXR_NAMESPACE_OPEN_SCOPE


// ---------------------------------------------------------------------------

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (evalStencils)
    (stencilData)
    (sizes)
    (offsets)
    (indices)
    (weights)
    (baseFaceToRefinedFacesMap)
    (refinedFaceCounts)
);

// The stencil table data is managed using two buffer array ranges
// the first containing the sizes and offsets which are perPoint for
// each refined point, and the second containing the indices and weights
// which are perIndex for each refined point stencil index.
class HdSt_GpuStencilTable
{
public:
    size_t numCoarsePoints;
    size_t numRefinedPoints;
    HdBufferArrayRangeSharedPtr perPointRange;
    HdBufferArrayRangeSharedPtr perIndexRange;
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
class HdSt_OsdIndexComputation final : public HdComputedBufferSource
{
    struct BaseFaceInfo
    {
        int baseFaceParam;
        GfVec2i baseFaceEdgeIndices;
    };

public:
    HdSt_OsdIndexComputation(HdSt_MeshTopology *topology,
                             HdBufferSourceSharedPtr const &osdTopology);
    bool Resolve() override;
    bool HasChainedBuffer() const override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

private:
    bool _CheckValid() const override;

    void _PopulateUniformPrimitiveBuffer(
        HdSt_Subdivision::PatchTable const *patchTable);
    void _PopulatePatchPrimitiveBuffer(
        HdSt_Subdivision::PatchTable const *patchTable);
    void _CreateBaseFaceMapping(
        std::vector<BaseFaceInfo> *result);

    HdSt_MeshTopology *_topology;
    HdBufferSourceSharedPtr _osdTopology;
    HdBufferSourceSharedPtr _primitiveBuffer;
    HdBufferSourceSharedPtr _edgeIndicesBuffer;
};

// ---------------------------------------------------------------------------
/// \class Hd_OsdFvarIndexComputation
class HdSt_OsdFvarIndexComputation final : public HdComputedBufferSource
{

public:
    HdSt_OsdFvarIndexComputation(HdSt_MeshTopology *topology,
                                 HdBufferSourceSharedPtr const &osdTopology,
                                 int channel);
    bool HasChainedBuffer() const override;
    bool Resolve() override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

protected:
    bool _CheckValid() const override;

private:
    void _PopulateFvarPatchParamBuffer(
        HdSt_Subdivision::PatchTable const *patchTable);

    HdSt_MeshTopology *_topology;
    HdBufferSourceSharedPtr _osdTopology;
    HdBufferSourceSharedPtr _fvarPatchParamBuffer;
    int _channel;
    TfToken _indicesName;
    TfToken _patchParamName;
};

// ---------------------------------------------------------------------------
/// \class Hd_OsdTopologyComputation
class HdSt_OsdTopologyComputation final : public HdComputedBufferSource
{
public:
    HdSt_OsdTopologyComputation(HdSt_MeshTopology *topology,
                                SdfPath const &id);

    bool Resolve() override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

protected:
    bool _CheckValid() const override;

private:
    HdSt_MeshTopology *_topology;
    SdfPath const _id;
};

// ---------------------------------------------------------------------------
/// \class HdSt_OsdBaseFaceToRefinedFacesMapComputation
class HdSt_OsdBaseFaceToRefinedFacesMapComputation final : 
    public HdComputedBufferSource
{
public:
    HdSt_OsdBaseFaceToRefinedFacesMapComputation(
        HdSt_Subdivision const *subdivision,
        HdBufferSourceSharedPtr const &osdTopology);

    bool HasChainedBuffer() const override;
    bool Resolve() override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

protected:
    bool _CheckValid() const override;

private:
    HdSt_Subdivision const *_subdivision;
    HdBufferSourceSharedPtr _osdTopology;
    HdBufferSourceSharedPtr _refinedFaceCounts;
};

// ---------------------------------------------------------------------------
/// \class Hd_OsdStencilTableBufferSource
///
/// This class isn't inherited from HdComputedBufferSource.
/// GetData() returns the internal stencil table data to skip unecessary copy.
///
class HdSt_OsdStencilTableBufferSource final : public HdBufferSource
{
public:
    HdSt_OsdStencilTableBufferSource(
        HdSt_Subdivision const *subdivision,
        HdBufferSourceSharedPtr const &osdTopology,
        TfToken const &name,
        HdSt_GpuStencilTableSharedPtr const &gpuStencilTable,
        HdSt_MeshTopology::Interpolation interpolation,
        int fvarChannel = 0);

    bool Resolve() override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    TfToken const & GetName() const override {
        return _name;
    }
    size_t ComputeHash() const override {
        return 0;
    }
    void const * GetData() const override {
        return _resultData;
    }
    HdTupleType GetTupleType() const override {
        return _resultTupleType;
    }
    size_t GetNumElements() const override {
        return _resultNumElements;
    }

protected:
    bool _CheckValid() const override;

private:
    HdSt_Subdivision const * _subdivision;
    HdBufferSourceSharedPtr const _osdTopology;
    TfToken _name;
    HdSt_GpuStencilTableSharedPtr _gpuStencilTable;
    HdSt_MeshTopology::Interpolation const _interpolation;
    int const _fvarChannel;
    void const * _resultData;
    size_t _resultNumElements;
    HdTupleType _resultTupleType;
};

HdSt_OsdStencilTableBufferSource::HdSt_OsdStencilTableBufferSource(
    HdSt_Subdivision const *subdivision,
    HdBufferSourceSharedPtr const &osdTopology,
    TfToken const &name,
    HdSt_GpuStencilTableSharedPtr const &gpuStencilTable,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel)
    : _subdivision(subdivision)
    , _osdTopology(osdTopology)
    , _name(name)
    , _gpuStencilTable(gpuStencilTable)
    , _interpolation(interpolation)
    , _fvarChannel(fvarChannel)
    , _resultData(nullptr)
    , _resultNumElements(0)
    , _resultTupleType(HdTupleType{HdTypeInt32, 0})
{
}

bool
HdSt_OsdStencilTableBufferSource::Resolve()
{
    if (_osdTopology && !_osdTopology->IsResolved()) return false;

    if (!_TryLock()) return false;

    HdSt_Subdivision::StencilTable const * stencilTable =
        _subdivision->GetStencilTable(_interpolation, _fvarChannel);

    _gpuStencilTable->numCoarsePoints = stencilTable->GetNumControlVertices();
    _gpuStencilTable->numRefinedPoints = stencilTable->GetNumStencils();

    if (_name == _tokens->sizes) {
        _resultData = stencilTable->GetSizes().data();
        _resultNumElements = stencilTable->GetSizes().size();
        _resultTupleType = HdTupleType{HdTypeInt32, 1};
    } else if (_name == _tokens->offsets) {
        _resultData = stencilTable->GetOffsets().data();
        _resultNumElements = stencilTable->GetOffsets().size();
        _resultTupleType = HdTupleType{HdTypeInt32, 1};
    } else if (_name == _tokens->indices) {
        _resultData = stencilTable->GetControlIndices().data();
        _resultNumElements = stencilTable->GetControlIndices().size();
        _resultTupleType = HdTupleType{HdTypeInt32, 1};
    } else if (_name == _tokens->weights) {
        // Note: weights table may have excess entries, so here we
        // copy only the entries corresponding to control indices.
        _resultData = stencilTable->GetWeights().data();
        _resultNumElements = stencilTable->GetControlIndices().size();
        _resultTupleType = HdTupleType{HdTypeFloat, 1};
    }

    _SetResolved();
    return true;
}

void
HdSt_OsdStencilTableBufferSource::GetBufferSpecs(HdBufferSpecVector *) const
{
    // nothing
}

bool
HdSt_OsdStencilTableBufferSource::_CheckValid() const
{
    return true;
}

HdSt_Subdivision::StencilTable const *
HdSt_Subdivision::GetStencilTable(
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel) const
{
    if (interpolation == HdSt_MeshTopology::INTERPOLATE_FACEVARYING) {
        if (!TF_VERIFY(fvarChannel >= 0)) {
            return nullptr;
        }

        if (!TF_VERIFY(fvarChannel < (int)_faceVaryingStencils.size())) {
            return nullptr;
        }
    }

    return (interpolation == HdSt_MeshTopology::INTERPOLATE_VERTEX) ?
               _vertexStencils.get() :
           (interpolation == HdSt_MeshTopology::INTERPOLATE_VARYING) ?
               _varyingStencils.get() :
           _faceVaryingStencils[fvarChannel].get();
}

HdSt_GpuStencilTableSharedPtr
HdSt_Subdivision::_GetGpuStencilTable(
    HdSt_MeshTopology * topology,
    HdBufferSourceSharedPtr const & osdTopology,
    HdStResourceRegistry * registry,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel)
{
    std::unique_lock<std::mutex> lock(_gpuStencilMutex);

    if (interpolation == HdSt_MeshTopology::INTERPOLATE_VERTEX) {
        if (!_gpuVertexStencils) {
            _gpuVertexStencils = _CreateGpuStencilTable(
                osdTopology, registry, interpolation);
        }
        return _gpuVertexStencils;
    } else if (interpolation == HdSt_MeshTopology::INTERPOLATE_VARYING) {
        if (!_gpuVaryingStencils) {
            _gpuVaryingStencils = _CreateGpuStencilTable(
                osdTopology, registry, interpolation);
        }
        return _gpuVaryingStencils;
    } else {
        if (_gpuFaceVaryingStencils.empty()) {
            _gpuFaceVaryingStencils.resize(
                topology->GetFvarTopologies().size());
        }
        if (!_gpuFaceVaryingStencils[fvarChannel]) {
            _gpuFaceVaryingStencils[fvarChannel] = _CreateGpuStencilTable(
                osdTopology, registry, interpolation, fvarChannel);
        }
        return _gpuFaceVaryingStencils[fvarChannel];
    }
}

HdSt_GpuStencilTableSharedPtr
HdSt_Subdivision::_CreateGpuStencilTable(
    HdBufferSourceSharedPtr const & osdTopology,
    HdStResourceRegistry * registry,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel) const
{
    // Allocate buffer array range for perPoint data
    HdBufferSpecVector perPointSpecs = {
        { _tokens->sizes, HdTupleType{HdTypeInt32, 1} },
        { _tokens->offsets, HdTupleType{HdTypeInt32, 1} },
    };
    HdBufferArrayRangeSharedPtr perPointRange =
        registry->AllocateSingleBufferArrayRange(
            _tokens->stencilData, perPointSpecs, HdBufferArrayUsageHint());

    // Allocate buffer array range for perIndex data
    HdBufferSpecVector perIndexSpecs = {
        { _tokens->indices, HdTupleType{HdTypeInt32, 1} },
        { _tokens->weights, HdTupleType{HdTypeFloat, 1} },
    };
    HdBufferArrayRangeSharedPtr perIndexRange =
        registry->AllocateSingleBufferArrayRange(
            _tokens->stencilData, perIndexSpecs, HdBufferArrayUsageHint());

    HdSt_GpuStencilTableSharedPtr gpuStencilTable =
        std::make_shared<HdSt_GpuStencilTable>(
                HdSt_GpuStencilTable{0, 0, perPointRange, perIndexRange});

    // Register buffer sources for computed perPoint stencil table data
    HdBufferSourceSharedPtrVector perPointSources{
        std::make_shared<HdSt_OsdStencilTableBufferSource>(
            this, osdTopology, _tokens->sizes,
            gpuStencilTable, interpolation, fvarChannel),
        std::make_shared<HdSt_OsdStencilTableBufferSource>(
            this, osdTopology, _tokens->offsets,
            gpuStencilTable, interpolation, fvarChannel)
    };
    registry->AddSources(perPointRange, std::move(perPointSources));

    // Register buffer sources for computed perIndex stencil table data
    HdBufferSourceSharedPtrVector perIndexSources{
        std::make_shared<HdSt_OsdStencilTableBufferSource>(
            this, osdTopology, _tokens->indices,
            gpuStencilTable, interpolation, fvarChannel),
        std::make_shared<HdSt_OsdStencilTableBufferSource>(
            this, osdTopology, _tokens->weights,
            gpuStencilTable, interpolation, fvarChannel)
    };
    registry->AddSources(perIndexRange, std::move(perIndexSources));

    return gpuStencilTable;
}

bool
HdSt_Subdivision::RefinesToTriangles(TfToken const &scheme)
{
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
        if (_topology->TriangulateQuads()) {
            specs->emplace_back(HdTokens->indices,
                                HdTupleType {HdTypeInt32, 6});
        } else {
            specs->emplace_back(HdTokens->indices,
                                HdTupleType {HdTypeInt32, 4});
        }
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
HdSt_OsdRefineComputationCPU::HdSt_OsdRefineComputationCPU(
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

HdSt_OsdRefineComputationCPU::~HdSt_OsdRefineComputationCPU() = default;

TfToken const &
HdSt_OsdRefineComputationCPU::GetName() const
{
    return _source->GetName();
}

template <class HashState>
void TfHashAppend(HashState &h,
                  HdSt_OsdRefineComputationCPU const &bs)
{
    h.Append(bs.GetInterpolation());
}

size_t
HdSt_OsdRefineComputationCPU::ComputeHash() const
{
    return TfHash()(*this);
}

void const *
HdSt_OsdRefineComputationCPU::GetData() const
{
    return _primvarBuffer.data();
}

HdTupleType
HdSt_OsdRefineComputationCPU::GetTupleType() const
{
    return _source->GetTupleType();
}

size_t
HdSt_OsdRefineComputationCPU::GetNumElements() const
{
    // Stride is measured here in components, not bytes.
    size_t const elementStride =
        HdGetComponentCount(_source->GetTupleType().type);
    return _primvarBuffer.size() / elementStride;
}

HdSt_MeshTopology::Interpolation
HdSt_OsdRefineComputationCPU::GetInterpolation() const
{
    return _interpolation;
}

bool
HdSt_OsdRefineComputationCPU::Resolve()
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
HdSt_OsdRefineComputationCPU::_CheckValid() const
{
    bool valid = _source->IsValid();

    // _osdTopology is optional
    valid &= _osdTopology ? _osdTopology->IsValid() : true;

    return valid;
}

void
HdSt_OsdRefineComputationCPU::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same buffer specs as source
    _source->GetBufferSpecs(specs);
}

bool
HdSt_OsdRefineComputationCPU::HasPreChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtr
HdSt_OsdRefineComputationCPU::GetPreChainedBuffer() const
{
    return _source;
}

// ---------------------------------------------------------------------------
/// OpenSubdiv GPU Refinement
///
///
HdSt_OsdRefineComputationGPU::HdSt_OsdRefineComputationGPU(
    HdSt_MeshTopology *topology,
    TfToken const &primvarName,
    HdType type,
    HdSt_GpuStencilTableSharedPtr const & gpuStencilTable,
    HdSt_MeshTopology::Interpolation interpolation)
    : _topology(topology)
    , _primvarName(primvarName)
    , _gpuStencilTable(gpuStencilTable)
    , _interpolation(interpolation)
{
}

HdSt_OsdRefineComputationGPU::~HdSt_OsdRefineComputationGPU() = default;

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

    HdSt_Subdivision * subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return;

    HdStResourceRegistry * hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);

    subdivision->RefineGPU(range,
                           _primvarName,
                           _gpuStencilTable,
                           hdStResourceRegistry);

    HD_PERF_COUNTER_INCR(HdPerfTokens->subdivisionRefineGPU);
}

int
HdSt_OsdRefineComputationGPU::GetNumOutputElements() const
{
    // returns the total number of vertices, including coarse and refined ones
    HdSt_Subdivision const * subdivision = _topology->GetSubdivision();
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

// ---------------------------------------------------------------------------

HdSt_Subdivision::HdSt_Subdivision(bool adaptive, int refineLevel)
    : _adaptive(adaptive)
    , _refineLevel(refineLevel)
    , _maxNumFaceVarying(0)
{
}

HdSt_Subdivision::~HdSt_Subdivision() = default;

void
HdSt_Subdivision::SetRefinementTables(
    std::unique_ptr<StencilTable const> && vertexStencils,
    std::unique_ptr<StencilTable const> && varyingStencils,
    std::vector<std::unique_ptr<StencilTable const>> && faceVaryingStencils,
    std::unique_ptr<PatchTable const> && patchTable)
{
    _vertexStencils = std::move(vertexStencils);
    _varyingStencils = std::move(varyingStencils);

    _faceVaryingStencils.resize(faceVaryingStencils.size());
    for (size_t i = 0; i < _faceVaryingStencils.size(); ++i) {
        _faceVaryingStencils[i] = std::move(faceVaryingStencils[i]);
    }

    _patchTable = std::move(patchTable);

    _maxNumFaceVarying = 0;
    for (size_t i = 0; i < _faceVaryingStencils.size(); ++i) {
        _maxNumFaceVarying = std::max(_maxNumFaceVarying, GetNumFaceVarying(i));
    }
}

int
HdSt_Subdivision::GetNumVertices() const
{
    // returns the total number of vertices, including coarse and refined ones.
    if (!TF_VERIFY(_vertexStencils)) return 0;

    return _vertexStencils->GetNumStencils() +
        _vertexStencils->GetNumControlVertices();
}

int
HdSt_Subdivision::GetNumVarying() const
{
    // returns the total number of vertices, including coarse and refined ones.
    if (!TF_VERIFY(_varyingStencils)) return 0;

    return _varyingStencils->GetNumStencils() +
        _varyingStencils->GetNumControlVertices();
}

int
HdSt_Subdivision::GetNumFaceVarying(int channel) const
{
    // returns the total number of vertices, including coarse and refined ones.
    if (!TF_VERIFY(_faceVaryingStencils[channel])) return 0;

    return _faceVaryingStencils[channel]->GetNumStencils() +
        _faceVaryingStencils[channel]->GetNumControlVertices();
}

int
HdSt_Subdivision::GetMaxNumFaceVarying() const
{
    // returns the largest total number of face-varying values (coarse and
    // refined) for all the face-varying channels
    return _maxNumFaceVarying;
}

VtIntArray
HdSt_Subdivision::GetRefinedFvarIndices(int channel) const
{
    VtIntArray fvarIndices;
    if (_patchTable && _patchTable->GetNumFVarChannels() > channel) {
        OpenSubdiv::Far::ConstIndexArray indices =
            _patchTable->GetFVarValues(channel);
        for (int i = 0; i < indices.size(); ++i) {
            fvarIndices.push_back(indices[i]);
        }
    }
    return fvarIndices;
}

namespace {

void
_EvalStencilsCPU(
    std::vector<float> * primvarBuffer,
    int const elementStride,
    int const numCoarsePoints,
    int const numRefinedPoints,
    std::vector<int> const & sizes,
    std::vector<int> const & offsets,
    std::vector<int> const & indices,
    std::vector<float> const & weights);

void
_EvalStencilsGPU(
    HdBufferArrayRangeSharedPtr const & range_,
    TfToken const & primvarName,
    int const numCoarsePoints,
    int const numRefinedPoints,
    HdBufferArrayRangeSharedPtr const & perPointRange_,
    HdBufferArrayRangeSharedPtr const & perIndexRange_,
    HdResourceRegistry * resourceRegistry);

};

void
HdSt_Subdivision::RefineCPU(HdBufferSourceSharedPtr const & source,
                                std::vector<float> * primvarBuffer,
                                HdSt_MeshTopology::Interpolation interpolation,
                                int fvarChannel)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSt_Subdivision::StencilTable const * stencilTable =
        GetStencilTable(interpolation, fvarChannel);

    if (!TF_VERIFY(stencilTable)) return;

    // if there is no stencil (e.g. torus with adaptive refinement),
    // just return here
    if (stencilTable->GetNumStencils() == 0) return;

    // Stride is measured here in components, not bytes.
    size_t const elementStride =
        HdGetComponentCount(source->GetTupleType().type);

    size_t const numTotalElements =
        stencilTable->GetNumControlVertices() + stencilTable->GetNumStencils();
    primvarBuffer->resize(numTotalElements * elementStride);

    // if the mesh has more vertices than that in use in topology,
    // we need to trim the buffer so that they won't overrun the coarse
    // vertex buffer which we allocated using the stencil table.
    // see HdSt_Subdivision::GetNumVertices()
    size_t numSrcElements = source->GetNumElements();
    if (numSrcElements > (size_t)stencilTable->GetNumControlVertices()) {
        numSrcElements = stencilTable->GetNumControlVertices();
    }

    float const * srcData = static_cast<float const *>(source->GetData());
    std::copy(srcData, srcData + (numSrcElements * elementStride),
              primvarBuffer->begin());

    _EvalStencilsCPU(
        primvarBuffer,
        elementStride,
        stencilTable->GetNumControlVertices(),
        stencilTable->GetNumStencils(),
        stencilTable->GetSizes(),
        stencilTable->GetOffsets(),
        stencilTable->GetControlIndices(),
        stencilTable->GetWeights()
    );
}

void
HdSt_Subdivision::RefineGPU(
        HdBufferArrayRangeSharedPtr const & primvarRange,
        TfToken const & primvarName,
        HdSt_GpuStencilTableSharedPtr const & gpuStencilTable,
        HdStResourceRegistry * resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(gpuStencilTable)) return;

    // if there is no stencil (e.g. torus with adaptive refinement),
    // just return here
    if (gpuStencilTable->numRefinedPoints == 0)  return;

    _EvalStencilsGPU(
        primvarRange,
        primvarName,
        gpuStencilTable->numCoarsePoints,
        gpuStencilTable->numRefinedPoints,
        gpuStencilTable->perPointRange,
        gpuStencilTable->perIndexRange,
        resourceRegistry);
}

HdBufferSourceSharedPtr
HdSt_Subdivision::CreateTopologyComputation(HdSt_MeshTopology *topology,
                                            SdfPath const &id)
{
    return std::make_shared<HdSt_OsdTopologyComputation>(topology, id);
}

HdBufferSourceSharedPtr
HdSt_Subdivision::CreateIndexComputation(HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &osdTopology)
{
    return std::make_shared<HdSt_OsdIndexComputation>(topology, osdTopology);
}

HdBufferSourceSharedPtr
HdSt_Subdivision::CreateFvarIndexComputation(HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &osdTopology, int channel)
{
    return std::make_shared<HdSt_OsdFvarIndexComputation>(
        topology, osdTopology, channel);
}

HdBufferSourceSharedPtr
HdSt_Subdivision::CreateRefineComputationCPU(HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    HdBufferSourceSharedPtr const &osdTopology,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel)
{
    return std::make_shared<HdSt_OsdRefineComputationCPU>(
        topology, source, osdTopology, interpolation, fvarChannel);
}

HdStComputationSharedPtr
HdSt_Subdivision::CreateRefineComputationGPU(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &osdTopology,
    TfToken const &name,
    HdType dataType,
    HdStResourceRegistry *resourceRegistry,
    HdSt_MeshTopology::Interpolation interpolation,
    int fvarChannel)
{
    HdSt_GpuStencilTableSharedPtr gpuStencilTable =
        _GetGpuStencilTable(topology, osdTopology,
                            resourceRegistry, interpolation, fvarChannel);

    return std::make_shared<HdSt_OsdRefineComputationGPU>(
        topology, name, dataType, gpuStencilTable, interpolation);
}

HdBufferSourceSharedPtr
HdSt_Subdivision::CreateBaseFaceToRefinedFacesMapComputation(
    HdBufferSourceSharedPtr const &osdTopology)
{
    return std::make_shared<HdSt_OsdBaseFaceToRefinedFacesMapComputation>(
        this, osdTopology);
}

// ---------------------------------------------------------------------------
HdSt_OsdTopologyComputation::HdSt_OsdTopologyComputation(
    HdSt_MeshTopology *topology,
    SdfPath const &id)
    : _topology(topology)
    , _id(id)
{
}

void
HdSt_OsdTopologyComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
}

bool
HdSt_OsdTopologyComputation::Resolve()
{
    using namespace OpenSubdiv;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // do far analysis and set stencils and patch table into HdSt_Subdivision.

    if (!TF_VERIFY(_topology)) {
        _SetResolved();
        return true;
    }

    HdSt_Subdivision * subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) {
        _SetResolved();
        return true;
    }

    // create topology refiner
    PxOsdTopologyRefinerSharedPtr refiner;

    // for empty topology, we don't need to refine anything.
    // but still need to return the typed buffer for codegen
    int numFvarChannels = 0;
    if (_topology->GetFaceVertexCounts().size() == 0) {
        // leave refiner empty
    } else {
        refiner = PxOsdRefinerFactory::Create(_topology->GetPxOsdMeshTopology(),
                                              _topology->GetFvarTopologies(),
                                              TfToken(_id.GetText()));
        numFvarChannels = refiner->GetNumFVarChannels();
    }

    std::unique_ptr<HdSt_Subdivision::StencilTable const> vertexStencils;
    std::unique_ptr<HdSt_Subdivision::StencilTable const> varyingStencils;
    std::vector<std::unique_ptr<HdSt_Subdivision::StencilTable const>>
        faceVaryingStencils(numFvarChannels);
    std::unique_ptr<HdSt_Subdivision::PatchTable const> patchTable;

    // refine topology and create stencil tables and patch table
    if (refiner) {
        bool const adaptive = subdivision->IsAdaptive();
        int const level = subdivision->GetRefineLevel();

        Far::PatchTableFactory::Options patchOptions(level);
        if (numFvarChannels > 0) {
            patchOptions.generateFVarTables = true;
            patchOptions.includeFVarBaseLevelIndices = true;
            patchOptions.generateFVarLegacyLinearPatches = !adaptive;
        }
        if (adaptive) {
            patchOptions.endCapType =
                Far::PatchTableFactory::Options::ENDCAP_BSPLINE_BASIS;
#if OPENSUBDIV_VERSION_NUMBER >= 30400
            // Improve fidelity when refining to limit surface patches
            // These options supported since v3.1.0 and v3.2.0 respectively.
            patchOptions.useInfSharpPatch = true;
            patchOptions.generateLegacySharpCornerPatches = false;
#endif
        }

        // split trace scopes.
        {
            HD_TRACE_SCOPE("refine");
            if (adaptive) {
                Far::TopologyRefiner::AdaptiveOptions adaptiveOptions(level);
#if OPENSUBDIV_VERSION_NUMBER >= 30400
                adaptiveOptions =  patchOptions.GetRefineAdaptiveOptions();
#endif
                refiner->RefineAdaptive(adaptiveOptions);
            } else {
                refiner->RefineUniform(level);
            }
        }
        {
            HD_TRACE_SCOPE("stencil factory");
            Far::StencilTableFactory::Options options;
            options.generateOffsets = true;
            options.generateIntermediateLevels = adaptive;
            options.interpolationMode =
                Far::StencilTableFactory::INTERPOLATE_VERTEX;
            vertexStencils.reset(
                Far::StencilTableFactory::Create(*refiner, options));

            options.interpolationMode =
                Far::StencilTableFactory::INTERPOLATE_VARYING;
            varyingStencils.reset(
                Far::StencilTableFactory::Create(*refiner, options));

            options.interpolationMode =
                Far::StencilTableFactory::INTERPOLATE_FACE_VARYING;
            for (int i = 0; i < numFvarChannels; ++i) {
                options.fvarChannel = i;
                faceVaryingStencils[i].reset(
                    Far::StencilTableFactory::Create(*refiner, options));
            }
        }
        {
            HD_TRACE_SCOPE("patch factory");
            patchTable.reset(
                Far::PatchTableFactory::Create(*refiner, patchOptions));
        }
    }

    // merge local point stencils
    if (patchTable && patchTable->GetLocalPointStencilTable()) {
        // append stencils
        if (Far::StencilTable const *vertexStencilsWithLocalPoints =
            Far::StencilTableFactory::AppendLocalPointStencilTable(
                *refiner,
                vertexStencils.get(),
                patchTable->GetLocalPointStencilTable())) {
            vertexStencils.reset(vertexStencilsWithLocalPoints);
        }
    }
    if (patchTable && patchTable->GetLocalPointVaryingStencilTable()) {
        // append stencils
        if (Far::StencilTable const *varyingStencilsWithLocalPoints =
            Far::StencilTableFactory::AppendLocalPointStencilTableVarying(
                *refiner,
                varyingStencils.get(),
                patchTable->GetLocalPointVaryingStencilTable())) {
            varyingStencils.reset(varyingStencilsWithLocalPoints);
        }
    }
    for (int i = 0; i < numFvarChannels; ++i) {
        if (patchTable && patchTable->GetLocalPointFaceVaryingStencilTable(i)) {
            // append stencils
            if (Far::StencilTable const *faceVaryingStencilsWithLocalPoints =
                Far::StencilTableFactory
                        ::AppendLocalPointStencilTableFaceVarying(
                    *refiner,
                    faceVaryingStencils[i].get(),
                    patchTable->GetLocalPointFaceVaryingStencilTable(i),
                    i)) {
                faceVaryingStencils[i].reset(
                        faceVaryingStencilsWithLocalPoints);
            }
        }
    }

    // set tables to topology
    // HdSt_Subdivision takes an ownership of stencilTables and patchTable.
    subdivision->SetRefinementTables(std::move(vertexStencils),
                                     std::move(varyingStencils),
                                     std::move(faceVaryingStencils),
                                     std::move(patchTable));

    _SetResolved();
    return true;
}

bool
HdSt_OsdTopologyComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

HdSt_OsdIndexComputation::HdSt_OsdIndexComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &osdTopology)
    : _topology(topology)
    , _osdTopology(osdTopology)
{
}

bool
HdSt_OsdIndexComputation::Resolve()
{
    using namespace OpenSubdiv;

    if (_osdTopology && !_osdTopology->IsResolved()) return false;

    if (!_TryLock()) return false;

    HdSt_Subdivision * subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) {
        _SetResolved();
        return true;
    }

    HdSt_Subdivision::PatchTable const * patchTable =
                                            subdivision->GetPatchTable();

    Far::Index const *firstIndex = NULL;
    size_t ptableSize = 0;
    if (patchTable) {
        ptableSize = patchTable->GetPatchControlVerticesTable().size();
        if (ptableSize > 0) {
            firstIndex = &patchTable->GetPatchControlVerticesTable()[0];
        }
    }

    TfToken const & scheme = _topology->GetScheme();

    if (_topology->RefinesToBSplinePatches() ||
        _topology->RefinesToBoxSplineTrianglePatches()) {

        // Bundle groups of 12 or 16 patch control vertices.
        int const arraySize = patchTable
            ? patchTable->GetPatchArrayDescriptor(0).GetNumControlVertices()
            : 0;

        VtArray<int> indices(ptableSize);
        memcpy(indices.data(), firstIndex, ptableSize * sizeof(int));

        HdBufferSourceSharedPtr patchIndices =
            std::make_shared<HdVtBufferSource>(
                HdTokens->indices, VtValue(indices), arraySize);

        _SetResult(patchIndices);

        _PopulatePatchPrimitiveBuffer(patchTable);
    } else if (HdSt_Subdivision::RefinesToTriangles(scheme)) {
        // populate refined triangle indices.
        VtArray<GfVec3i> indices(ptableSize/3);
        memcpy(reinterpret_cast<Far::Index*>(indices.data()),
                firstIndex, ptableSize * sizeof(int));

        HdBufferSourceSharedPtr triIndices =
            std::make_shared<HdVtBufferSource>(
                HdTokens->indices, VtValue(indices));
        _SetResult(triIndices);

        _PopulateUniformPrimitiveBuffer(patchTable);
    } else {
        // populate refined quad indices.
        size_t const numQuads = ptableSize / 4;

        int const numIndicesPerQuad =
            _topology->TriangulateQuads()
                ? HdMeshTriQuadBuilder::NumIndicesPerTriQuad
                : HdMeshTriQuadBuilder::NumIndicesPerQuad;
        VtIntArray indices(numQuads * numIndicesPerQuad);

        if (numIndicesPerQuad == 4) {
            memcpy(indices.data(), firstIndex, ptableSize * sizeof(int));
        } else {
            HdMeshTriQuadBuilder outputIndices(indices.data(), true);
            for (size_t i=0; i<numQuads; ++i) {
                GfVec4i quadIndices(&firstIndex[i*4]);
                outputIndices.EmitQuadFace(quadIndices);
            }
        }

        // refined quads index buffer
        HdBufferSourceSharedPtr quadIndices =
            std::make_shared<HdVtBufferSource>(
                HdTokens->indices, VtValue(indices), numIndicesPerQuad);
        _SetResult(quadIndices);

        _PopulateUniformPrimitiveBuffer(patchTable);
    }

    _SetResolved();
    return true;
}

void
HdSt_OsdIndexComputation::_CreateBaseFaceMapping(
    std::vector<HdSt_OsdIndexComputation::BaseFaceInfo> *result)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(result)) return;

    int const * numVertsPtr  = _topology->GetFaceVertexCounts().cdata();
    int const numFaces       = _topology->GetFaceVertexCounts().size();
    int const numVertIndices = _topology->GetFaceVertexIndices().size();

    result->clear();
    result->reserve(numFaces);

    int regFaceSize = 4;
    if (HdSt_Subdivision::RefinesToTriangles( _topology->GetScheme() )) {
        regFaceSize = 3;
    }

    for (int i = 0, v = 0, ev = 0; i<numFaces; ++i) {
        int const nv = numVertsPtr[i];

        if (v+nv > numVertIndices) break;

        if (nv == regFaceSize) {
            BaseFaceInfo info;
            info.baseFaceParam =
                HdMeshUtil::EncodeCoarseFaceParam(i, /*edgeFlag=*/0);
            info.baseFaceEdgeIndices = GfVec2i(ev, 0);
            result->push_back(info);
        } else if (nv < 3) {
            int const numBaseFaces = (regFaceSize == 4) ? nv : nv - 2;
            for (int f = 0; f < numBaseFaces; ++f) {
                BaseFaceInfo info;
                info.baseFaceParam =
                    HdMeshUtil::EncodeCoarseFaceParam(i, /*edgeFlag=*/0);
                info.baseFaceEdgeIndices = GfVec2i(-1, -1);
                result->push_back(info);
            }
        } else {
            for (int j = 0; j < nv; ++j) {
                int edgeFlag = 0;
                if (j == 0) {
                    edgeFlag = 1;
                } else if (j == nv - 1) {
                    edgeFlag = 2;
                } else {
                    edgeFlag = 3;
                }

                BaseFaceInfo info;
                info.baseFaceParam =
                    HdMeshUtil::EncodeCoarseFaceParam(i, edgeFlag);
                info.baseFaceEdgeIndices = GfVec2i(ev+j, ev+(j+nv-1)%nv);
                result->push_back(info);
            }
        }

        v += nv;
        ev += nv;
    }

    result->shrink_to_fit();
}

void
HdSt_OsdIndexComputation::_PopulateUniformPrimitiveBuffer(
    HdSt_Subdivision::PatchTable const * patchTable)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    std::vector<BaseFaceInfo> patchFaceToBaseFaceMapping;
    _CreateBaseFaceMapping(&patchFaceToBaseFaceMapping);

    size_t numPatches = patchTable
        ? patchTable->GetPatchParamTable().size()
        : 0;
    VtVec3iArray primitiveParam(numPatches);
    VtVec2iArray edgeIndices(numPatches);

    for (size_t i = 0; i < numPatches; ++i) {
        OpenSubdiv::Far::PatchParam const &patchParam =
            patchTable->GetPatchParamTable()[i];

        int patchFaceIndex = patchParam.GetFaceId();
        BaseFaceInfo const & info = patchFaceToBaseFaceMapping[patchFaceIndex];

        unsigned int field0 = patchParam.field0;
        unsigned int field1 = patchParam.field1;
        primitiveParam[i][0] = info.baseFaceParam;
        primitiveParam[i][1] = *((int*)&field0);
        primitiveParam[i][2] = *((int*)&field1);

        edgeIndices[i] = info.baseFaceEdgeIndices;
    }

    _primitiveBuffer.reset(new HdVtBufferSource(
                               HdTokens->primitiveParam,
                               VtValue(primitiveParam)));

    _edgeIndicesBuffer.reset(new HdVtBufferSource(
                           HdTokens->edgeIndices,
                           VtValue(edgeIndices)));

}

void
HdSt_OsdIndexComputation::_PopulatePatchPrimitiveBuffer(
    HdSt_Subdivision::PatchTable const * patchTable)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    std::vector<BaseFaceInfo> patchFaceToBaseFaceMapping;
    _CreateBaseFaceMapping(&patchFaceToBaseFaceMapping);

    size_t numPatches = patchTable
        ? patchTable->GetPatchParamTable().size()
        : 0;
    VtVec4iArray primitiveParam(numPatches);
    VtVec2iArray edgeIndices(numPatches);

    for (size_t i = 0; i < numPatches; ++i) {
        OpenSubdiv::Far::PatchParam const &patchParam =
            patchTable->GetPatchParamTable()[i];

        float sharpness = 0.0;
        if (i < patchTable->GetSharpnessIndexTable().size()) {
            OpenSubdiv::Far::Index sharpnessIndex =
                patchTable->GetSharpnessIndexTable()[i];
            if (sharpnessIndex >= 0)
                sharpness = patchTable->GetSharpnessValues()[sharpnessIndex];
        }

        int patchFaceIndex = patchParam.GetFaceId();
        BaseFaceInfo const & info = patchFaceToBaseFaceMapping[patchFaceIndex];

        unsigned int field0 = patchParam.field0;
        unsigned int field1 = patchParam.field1;
        primitiveParam[i][0] = info.baseFaceParam;
        primitiveParam[i][1] = *((int*)&field0);
        primitiveParam[i][2] = *((int*)&field1);

        int sharpnessAsInt = static_cast<int>(sharpness);
        primitiveParam[i][3] = sharpnessAsInt;

        edgeIndices[i] = info.baseFaceEdgeIndices;
    }
    _primitiveBuffer.reset(new HdVtBufferSource(
                               HdTokens->primitiveParam,
                               VtValue(primitiveParam)));

    _edgeIndicesBuffer.reset(new HdVtBufferSource(
                           HdTokens->edgeIndices,
                           VtValue(edgeIndices)));
}

// ---------------------------------------------------------------------------

HdSt_OsdFvarIndexComputation::HdSt_OsdFvarIndexComputation (
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &osdTopology,
    int channel)
    : _topology(topology)
    , _osdTopology(osdTopology)
    , _channel(channel)
{
    _indicesName = TfToken(
        HdStTokens->fvarIndices.GetString() + std::to_string(_channel));
    _patchParamName = TfToken(
        HdStTokens->fvarPatchParam.GetString() + std::to_string(_channel));
}

bool
HdSt_OsdFvarIndexComputation::Resolve()
{
    using namespace OpenSubdiv;

    if (_osdTopology && !_osdTopology->IsResolved()) return false;

    if (!_TryLock()) return false;

    HdSt_Subdivision * subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) {
        _SetResolved();
        return true;
    }

    VtIntArray fvarIndices = subdivision->GetRefinedFvarIndices(_channel);
    if (!TF_VERIFY(!fvarIndices.empty())) {
        _SetResolved();
        return true;
    }

    Far::Index const * firstIndex = fvarIndices.cdata();
    Far::PatchTable const * patchTable = subdivision->GetPatchTable();
    size_t numPatches = patchTable ? patchTable->GetNumPatchesTotal() : 0;

    TfToken const & scheme = _topology->GetScheme();

    if (_topology->RefinesToBSplinePatches() ||
        _topology->RefinesToBoxSplineTrianglePatches()) {

        // Bundle groups of 12 or 16 patch control vertices
        int const arraySize = patchTable ?
            patchTable->GetFVarPatchDescriptor(_channel).GetNumControlVertices()
            : 0;

        VtIntArray indices(arraySize * numPatches);
        memcpy(indices.data(), firstIndex,
               arraySize * numPatches * sizeof(int));

        HdBufferSourceSharedPtr patchIndices =
            std::make_shared<HdVtBufferSource>(
                _indicesName, VtValue(indices), arraySize);

        _SetResult(patchIndices);
        _PopulateFvarPatchParamBuffer(patchTable);
    } else if (HdSt_Subdivision::RefinesToTriangles(scheme)) {
        // populate refined triangle indices.
        VtArray<GfVec3i> indices(numPatches);
        memcpy(reinterpret_cast<Far::Index*>(indices.data()),
                firstIndex, 3 * numPatches * sizeof(int));

        HdBufferSourceSharedPtr triIndices =
            std::make_shared<HdVtBufferSource>(_indicesName, VtValue(indices));
        _SetResult(triIndices);
    } else {
        // populate refined quad indices.
        VtArray<GfVec4i> indices(numPatches);
        memcpy(reinterpret_cast<Far::Index*>(indices.data()),
                firstIndex, 4 * numPatches * sizeof(int));

        HdBufferSourceSharedPtr quadIndices =
            std::make_shared<HdVtBufferSource>(_indicesName, VtValue(indices));
        _SetResult(quadIndices);
    }

    _SetResolved();
    return true;
}

void
HdSt_OsdFvarIndexComputation::_PopulateFvarPatchParamBuffer(
    HdSt_Subdivision::PatchTable const * patchTable)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtVec2iArray fvarPatchParam(0);

    if (patchTable) {
        OpenSubdiv::Far::ConstPatchParamArray patchParamArray =
            patchTable->GetFVarPatchParams(_channel);
        size_t numPatches = patchParamArray.size();
        fvarPatchParam.resize(numPatches);

        for (size_t i = 0; i < numPatches; ++i) {
            OpenSubdiv::Far::PatchParam const &patchParam = patchParamArray[i];
            fvarPatchParam[i][0] = patchParam.field0;
            fvarPatchParam[i][1] = patchParam.field1;
        }
    }

    _fvarPatchParamBuffer.reset(new HdVtBufferSource(
                                    _patchParamName, VtValue(fvarPatchParam)));
}

void
HdSt_OsdFvarIndexComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    if (_topology->RefinesToBSplinePatches()) {
        // bi-cubic bspline patches
        specs->emplace_back(_indicesName, HdTupleType {HdTypeInt32, 16});
        specs->emplace_back(_patchParamName, HdTupleType {HdTypeInt32Vec2, 1});
    } else if (_topology->RefinesToBoxSplineTrianglePatches()) {
        // quartic box spline triangle patches
        specs->emplace_back(_indicesName, HdTupleType {HdTypeInt32, 12});
        specs->emplace_back(_patchParamName, HdTupleType {HdTypeInt32Vec2, 1});
    } else if (HdSt_Subdivision::RefinesToTriangles(_topology->GetScheme())) {
        // triangles (loop)
        specs->emplace_back(_indicesName, HdTupleType {HdTypeInt32Vec3, 1});
    } else {
        // quads (catmark, bilinear)
        specs->emplace_back(_indicesName, HdTupleType {HdTypeInt32Vec4, 1});
    }
}

bool
HdSt_OsdFvarIndexComputation::HasChainedBuffer() const
{
    return (_topology->RefinesToBSplinePatches() ||
            _topology->RefinesToBoxSplineTrianglePatches());
}

HdBufferSourceSharedPtrVector
HdSt_OsdFvarIndexComputation::GetChainedBuffers() const
{
    if (_topology->RefinesToBSplinePatches() ||
        _topology->RefinesToBoxSplineTrianglePatches()) {
        return { _fvarPatchParamBuffer };
    } else {
        return {};
    }
}

bool
HdSt_OsdFvarIndexComputation::_CheckValid() const {
    return true;
}

// ---------------------------------------------------------------------------

HdSt_OsdBaseFaceToRefinedFacesMapComputation::
    HdSt_OsdBaseFaceToRefinedFacesMapComputation(
        HdSt_Subdivision const *subdivision,
        HdBufferSourceSharedPtr const &osdTopology) :
    _subdivision(subdivision), _osdTopology(osdTopology)
{}

void HdSt_OsdBaseFaceToRefinedFacesMapComputation::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->emplace_back(_tokens->baseFaceToRefinedFacesMap, 
        HdTupleType {HdTypeInt32, 1});
    specs->emplace_back(_tokens->refinedFaceCounts, 
        HdTupleType {HdTypeInt32, 1});
}

bool
HdSt_OsdBaseFaceToRefinedFacesMapComputation::Resolve()
{
    if (_osdTopology && !_osdTopology->IsResolved()) return false;

    if (!_TryLock()) return false;

    if (!TF_VERIFY(_subdivision)) {
        _SetResolved();
        return true;
    }
    
    HdSt_Subdivision::PatchTable const *patchTable =
        _subdivision->GetPatchTable();
    const size_t numPatches = patchTable ? 
        patchTable->GetPatchParamTable().size() : 0;
    const size_t numBaseFaces = patchTable ? 
        patchTable->GetNumPtexFaces() : 0;

    std::vector<std::vector<int>> baseFaceToRefinedFacesMap(numBaseFaces);

    for (size_t i = 0; i < numPatches; ++i) {
        OpenSubdiv::Far::PatchParam const &patchParam =
            patchTable->GetPatchParamTable()[i];
        baseFaceToRefinedFacesMap[patchParam.GetFaceId()].push_back(i);
    }

    VtIntArray baseFaceToRefinedFacesArr;
    VtIntArray refinedFaceCounts(numBaseFaces);
    size_t currRefinedFaceCount = 0;

    for (size_t i = 0; i < numBaseFaces; ++i) {
        std::vector<int> refinedFaces = baseFaceToRefinedFacesMap[i];
        currRefinedFaceCount += refinedFaces.size();
        
        for (size_t j = 0; j < refinedFaces.size(); ++j) {
            baseFaceToRefinedFacesArr.push_back(refinedFaces[j]);
        }
        refinedFaceCounts[i] = currRefinedFaceCount;
    }

    HdBufferSourceSharedPtr source = std::make_shared<HdVtBufferSource>(
        _tokens->baseFaceToRefinedFacesMap, VtValue(baseFaceToRefinedFacesArr));
    _SetResult(source);

    _refinedFaceCounts = std::make_shared<HdVtBufferSource>(
        _tokens->refinedFaceCounts, VtValue(refinedFaceCounts));

    _SetResolved();
    return true;
}

bool
HdSt_OsdBaseFaceToRefinedFacesMapComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtrVector
HdSt_OsdBaseFaceToRefinedFacesMapComputation::GetChainedBuffers() const
{
    return { _refinedFaceCounts };
}

bool
HdSt_OsdBaseFaceToRefinedFacesMapComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

namespace {

void
_EvalStencilsCPU(
    std::vector<float> * primvarBuffer,
    int const elementStride,
    int const numCoarsePoints,
    int const numRefinedPoints,
    std::vector<int> const & sizes,
    std::vector<int> const & offsets,
    std::vector<int> const & indices,
    std::vector<float> const  & weights)
{
    int const numElements = elementStride;
    std::vector<float> dst(numElements);

    for (int pointIndex = 0; pointIndex < numRefinedPoints; ++pointIndex) {
        for (int element = 0; element < numElements; ++element) {
            dst[element] = 0;
        }

        int const stencilSize = sizes[pointIndex];
        int const stencilOffset = offsets[pointIndex];

        for (int stencil = 0; stencil < stencilSize; ++stencil) {
            int const index = indices[stencil + stencilOffset];
            float const weight = weights[stencil + stencilOffset];
            int const srcIndex = index * elementStride;
            for (int element = 0; element < numElements; ++element) {
                dst[element] += weight * (*primvarBuffer)[srcIndex + element];
            }
        }

        int const dstIndex = (pointIndex + numCoarsePoints) * elementStride;
        for (int element = 0; element < numElements; ++element) {
            (*primvarBuffer)[dstIndex + element] = dst[element];
        }
    }
}

enum {
    BufferBinding_Uniforms,
    BufferBinding_Sizes,
    BufferBinding_Offsets,
    BufferBinding_Indices,
    BufferBinding_Weights,
    BufferBinding_Primvar,
};

HgiResourceBindingsSharedPtr
_CreateResourceBindings(
    Hgi* hgi,
    HgiBufferHandle const & sizes,
    HgiBufferHandle const & offsets,
    HgiBufferHandle const & indices,
    HgiBufferHandle const & weights,
    HgiBufferHandle const & primvar)
{
    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "EvalStencils";

    if (sizes) {
        HgiBufferBindDesc bufBind0;
        bufBind0.bindingIndex = BufferBinding_Sizes;
        bufBind0.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind0.stageUsage = HgiShaderStageCompute;
        bufBind0.writable = false;
        bufBind0.offsets.push_back(0);
        bufBind0.buffers.push_back(sizes);
        resourceDesc.buffers.push_back(std::move(bufBind0));
    }

    if (offsets) {
        HgiBufferBindDesc bufBind1;
        bufBind1.bindingIndex = BufferBinding_Offsets;
        bufBind1.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind1.stageUsage = HgiShaderStageCompute;
        bufBind1.writable = false;
        bufBind1.offsets.push_back(0);
        bufBind1.buffers.push_back(offsets);
        resourceDesc.buffers.push_back(std::move(bufBind1));
    }

    if (indices) {
        HgiBufferBindDesc bufBind2;
        bufBind2.bindingIndex = BufferBinding_Indices;
        bufBind2.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind2.stageUsage = HgiShaderStageCompute;
        bufBind2.writable = false;
        bufBind2.offsets.push_back(0);
        bufBind2.buffers.push_back(indices);
        resourceDesc.buffers.push_back(std::move(bufBind2));
    }

    if (weights) {
        HgiBufferBindDesc bufBind3;
        bufBind3.bindingIndex = BufferBinding_Weights;
        bufBind3.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind3.stageUsage = HgiShaderStageCompute;
        bufBind3.writable = false;
        bufBind3.offsets.push_back(0);
        bufBind3.buffers.push_back(weights);
        resourceDesc.buffers.push_back(std::move(bufBind3));
    }

    if (primvar) {
        HgiBufferBindDesc bufBind4;
        bufBind4.bindingIndex = BufferBinding_Primvar;
        bufBind4.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind4.stageUsage = HgiShaderStageCompute;
        bufBind4.writable = true;
        bufBind4.offsets.push_back(0);
        bufBind4.buffers.push_back(primvar);
        resourceDesc.buffers.push_back(std::move(bufBind4));
    }

    return std::make_shared<HgiResourceBindingsHandle>(
        hgi->CreateResourceBindings(resourceDesc));
}

HgiComputePipelineSharedPtr
_CreatePipeline(
    Hgi * hgi,
    uint32_t constantValuesSize,
    HgiShaderProgramHandle const & program)
{
    HgiComputePipelineDesc desc;
    desc.debugName = "EvalStencils";
    desc.shaderProgram = program;
    desc.shaderConstantsDesc.byteSize = constantValuesSize;
    return std::make_shared<HgiComputePipelineHandle>(
        hgi->CreateComputePipeline(desc));
}

void
_EvalStencilsGPU(
    HdBufferArrayRangeSharedPtr const & primvarRange_,
    TfToken const & primvarName,
    int const numCoarsePoints,
    int const numRefinedPoints,
    HdBufferArrayRangeSharedPtr const & perPointRange_,
    HdBufferArrayRangeSharedPtr const & perIndexRange_,
    HdResourceRegistry * resourceRegistry)
{
    // select shader by datatype
    TfToken shaderToken = _tokens->evalStencils;
    if (!TF_VERIFY(!shaderToken.IsEmpty())) return;

    HdStBufferArrayRangeSharedPtr primvarRange =
        std::static_pointer_cast<HdStBufferArrayRange> (primvarRange_);

    HdStBufferResourceSharedPtr primvar =
        primvarRange->GetResource(primvarName);
    size_t const elementOffset = primvarRange->GetElementOffset();
    size_t const elementStride =
        HdGetComponentCount(primvar->GetTupleType().type);

    struct Uniform {
        int pointIndexStart;
        int pointIndexEnd;
        int srcBase;
        int srcStride;
        int dstBase;
        int dstStride;
        int sizesBase;
        int offsetsBase;
        int indicesBase;
        int weightsBase;
    } uniform;

    HdStResourceRegistry* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);
    std::string const defines =
        TfStringPrintf("#define EVAL_STENCILS_NUM_ELEMENTS %zd\n",
                       elementStride);
    HdStGLSLProgramSharedPtr computeProgram
        = HdStGLSLProgram::GetComputeProgram(shaderToken,
          defines,
          hdStResourceRegistry,
          [&](HgiShaderFunctionDesc &computeDesc) {
            computeDesc.debugName = shaderToken.GetString();
            computeDesc.shaderStage = HgiShaderStageCompute;
            computeDesc.computeDescriptor.localSize = GfVec3i(64, 1, 1);

            HgiShaderFunctionAddBuffer(&computeDesc,
                "sizes", HdStTokens->_int,
                BufferBinding_Sizes, HgiBindingTypePointer);
            HgiShaderFunctionAddBuffer(&computeDesc,
                "offsets", HdStTokens->_int,
                BufferBinding_Offsets, HgiBindingTypePointer);
            HgiShaderFunctionAddBuffer(&computeDesc,
                "indices", HdStTokens->_int,
                BufferBinding_Indices, HgiBindingTypePointer);
            HgiShaderFunctionAddBuffer(&computeDesc,
                "weights", HdStTokens->_float,
                BufferBinding_Weights, HgiBindingTypePointer);
            HgiShaderFunctionAddWritableBuffer(&computeDesc,
                "primvar", HdStTokens->_float,
                BufferBinding_Primvar);

            static const std::string params[] = {
                "pointIndexStart",
                "pointIndexEnd",
                "srcBase",
                "srcStride",
                "dstBase",
                "dstStride",
                "sizesBase",
                "offsetsBase",
                "indicesBase",
                "weightsBase",
            };
            static_assert((sizeof(Uniform) / sizeof(int)) ==
                          (sizeof(params) / sizeof(params[0])), "");
            for (std::string const & param : params) {
                HgiShaderFunctionAddConstantParam(
                    &computeDesc, param, HdStTokens->_int);
            }
            HgiShaderFunctionAddStageInput(
                &computeDesc, "hd_GlobalInvocationID", "uvec3",
                HgiShaderKeywordTokens->hdGlobalInvocationID);
        });

    if (!computeProgram) return;

    HdStBufferArrayRangeSharedPtr perPointRange =
        std::static_pointer_cast<HdStBufferArrayRange> (perPointRange_);
    HdStBufferArrayRangeSharedPtr perIndexRange =
        std::static_pointer_cast<HdStBufferArrayRange> (perIndexRange_);

    // prepare uniform buffer for GPU computation
    uniform.pointIndexStart = 0;
    uniform.pointIndexEnd = numRefinedPoints;
    int const numPoints = uniform.pointIndexEnd - uniform.pointIndexStart;

    uniform.srcBase = elementOffset;
    uniform.srcStride = elementStride;

    uniform.dstBase = elementOffset + numCoarsePoints;
    uniform.dstStride = elementStride;

    uniform.sizesBase = perPointRange->GetElementOffset();
    uniform.offsetsBase = perPointRange->GetElementOffset();
    uniform.indicesBase = perIndexRange->GetElementOffset();
    uniform.weightsBase = perIndexRange->GetElementOffset();

    // buffer resources for GPU computation
    HdStBufferResourceSharedPtr sizes = perPointRange->GetResource(
                                                            _tokens->sizes);
    HdStBufferResourceSharedPtr offsets = perPointRange->GetResource(
                                                            _tokens->offsets);
    HdStBufferResourceSharedPtr indices = perIndexRange->GetResource(
                                                            _tokens->indices);
    HdStBufferResourceSharedPtr weights = perIndexRange->GetResource(
                                                            _tokens->weights);

    Hgi* hgi = hdStResourceRegistry->GetHgi();

    // Generate hash for resource bindings and pipeline.
    // XXX Needs fingerprint hash to avoid collisions
    uint64_t const rbHash = (uint64_t) TfHash::Combine(
        sizes->GetHandle().Get(),
        offsets->GetHandle().Get(),
        indices->GetHandle().Get(),
        weights->GetHandle().Get(),
        primvar->GetHandle().Get());

    uint64_t const pHash = (uint64_t) TfHash::Combine(
        computeProgram->GetProgram().Get(),
        sizeof(uniform));

    // Get or add resource bindings in registry.
    HdInstance<HgiResourceBindingsSharedPtr> resourceBindingsInstance =
        hdStResourceRegistry->RegisterResourceBindings(rbHash);
    if (resourceBindingsInstance.IsFirstInstance()) {
        HgiResourceBindingsSharedPtr rb =
            _CreateResourceBindings(hgi,
                                    sizes->GetHandle(),
                                    offsets->GetHandle(),
                                    indices->GetHandle(),
                                    weights->GetHandle(),
                                    primvar->GetHandle());
        resourceBindingsInstance.SetValue(rb);
    }

    HgiResourceBindingsSharedPtr const & resourceBindingsPtr =
        resourceBindingsInstance.GetValue();
    HgiResourceBindingsHandle resourceBindings = *resourceBindingsPtr.get();

    // Get or add pipeline in registry.
    HdInstance<HgiComputePipelineSharedPtr> computePipelineInstance =
        hdStResourceRegistry->RegisterComputePipeline(pHash);
    if (computePipelineInstance.IsFirstInstance()) {
        HgiComputePipelineSharedPtr pipe = _CreatePipeline(
            hgi, sizeof(uniform), computeProgram->GetProgram());
        computePipelineInstance.SetValue(pipe);
    }

    HgiComputePipelineSharedPtr const & pipelinePtr =
        computePipelineInstance.GetValue();
    HgiComputePipelineHandle pipeline = *pipelinePtr.get();

    HgiComputeCmds* computeCmds = hdStResourceRegistry->GetGlobalComputeCmds();
    computeCmds->PushDebugGroup("EvalStencils Cmds");
    computeCmds->BindResources(resourceBindings);
    computeCmds->BindPipeline(pipeline);

    // transfer uniform buffer
    computeCmds->SetConstantValues(
        pipeline, BufferBinding_Uniforms, sizeof(uniform), &uniform);

    // dispatch compute kernel
    computeCmds->Dispatch(numPoints, 1);

    // submit the work
    computeCmds->PopDebugGroup();
}

} // Anonymous namespace


PXR_NAMESPACE_CLOSE_SCOPE
