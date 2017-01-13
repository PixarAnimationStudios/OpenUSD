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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferResource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/subdivision.h"
#include "pxr/imaging/hd/subdivision3.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/pxOsd/refinerFactory.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/gf/vec4i.h"

#include <opensubdiv/version.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/osd/cpuVertexBuffer.h>
#include <opensubdiv/osd/cpuEvaluator.h>
#include <opensubdiv/osd/glVertexBuffer.h>
#include <opensubdiv/osd/mesh.h>

#include <boost/scoped_ptr.hpp>

typedef OpenSubdiv::Osd::CpuVertexBuffer Hd_OsdCpuVertexBuffer;

// There's a buffer synchronization bug in driver 331, and apparently fixed in 334.
// Don't enable compute shader kernel until driver updates.

#if OPENSUBDIV_HAS_GLSL_COMPUTE

#include <opensubdiv/osd/glComputeEvaluator.h>
#define HD_ENABLE_GPU_SUBDIVISION 1
typedef OpenSubdiv::Osd::GLStencilTableSSBO Hd_OsdGpuStencilTable;
typedef OpenSubdiv::Osd::GLComputeEvaluator Hd_OsdGpuEvaluator;

#elif OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK

#include <opensubdiv/osd/glXFBEvaluator.h>
#define HD_ENABLE_GPU_SUBDIVISION 1
typedef OpenSubdiv::Osd::GLStencilTableTBO Hd_OsdGpuStencilTable;
typedef OpenSubdiv::Osd::GLXFBEvaluator Hd_OsdGpuEvaluator;

#else

#define HD_ENABLE_GPU_SUBDIVISION 0

#endif


// ---------------------------------------------------------------------------

class Hd_Osd3Subdivision : public Hd_Subdivision {
public:
    /// Construct Hd_Subdivision. It takes an ownership of farmesh.
    Hd_Osd3Subdivision();
    virtual ~Hd_Osd3Subdivision();

    virtual int GetNumVertices() const;

    virtual void RefineCPU(HdBufferSourceSharedPtr const &source,
                           bool varying,
                           void *vertexBuffer);

    virtual void RefineGPU(HdBufferArrayRangeSharedPtr const &range,
                           TfToken const &name);

    // computation factory methods
    virtual HdBufferSourceSharedPtr CreateTopologyComputation(
        HdMeshTopology *topology,
        bool adaptive,
        int level,
        SdfPath const &id);

    virtual HdBufferSourceSharedPtr CreateIndexComputation(
        HdMeshTopology *topology,
        HdBufferSourceSharedPtr const &osdTopology);

    virtual HdBufferSourceSharedPtr CreateRefineComputation(
        HdMeshTopology *topology,
        HdBufferSourceSharedPtr const &source,
        bool varying,
        HdBufferSourceSharedPtr const &osdTopology);

    virtual HdComputationSharedPtr CreateRefineComputationGPU(
        HdMeshTopology *topology,
        TfToken const &name,
        GLenum dataType,
        int numComponents);

    void SetRefinementTables(
        OpenSubdiv::Far::StencilTable const *vertexStencils,
        OpenSubdiv::Far::StencilTable const *varyingStencils,
        OpenSubdiv::Far::PatchTable const *patchTable,
        bool adaptive);

    bool IsAdaptive() const {
        return _adaptive;
    }

    OpenSubdiv::Far::PatchTable const *GetPatchTable() const {
        return _patchTable;
    }

private:
    OpenSubdiv::Far::StencilTable const *_vertexStencils;
    OpenSubdiv::Far::StencilTable const *_varyingStencils;
    OpenSubdiv::Far::PatchTable const *_patchTable;
    bool _adaptive;

#if HD_ENABLE_GPU_SUBDIVISION
    /// Returns GPU stencil table. Creates it if not existed.
    /// A valid GL context has to be made to current before calling this method.
    Hd_OsdGpuStencilTable *_GetGpuStencilTable();
    Hd_OsdGpuStencilTable *_gpuStencilTable;
#endif
};

class Hd_Osd3IndexComputation : public Hd_OsdIndexComputation {
public:
    Hd_Osd3IndexComputation(Hd_Osd3Subdivision *subdivision,
                            HdMeshTopology *topology,
                            HdBufferSourceSharedPtr const &osdTopology);
    /// overrides
    virtual bool Resolve();

private:
    void _PopulateUniformPrimitiveBuffer(
        OpenSubdiv::Far::PatchTable const *patchTable);
    void _PopulateBSplinePrimitiveBuffer(
        OpenSubdiv::Far::PatchTable const *patchTable);
    void _CreatePtexIndexToCoarseFaceIndexMapping(
        std::vector<int> *result);

    Hd_Osd3Subdivision *_subdivision;
};

class Hd_Osd3TopologyComputation : public Hd_OsdTopologyComputation {
public:
    Hd_Osd3TopologyComputation(Hd_Osd3Subdivision *subdivision,
                               HdMeshTopology *topology,
                               bool adaptive,
                               int level,
                               SdfPath const &id);

    /// overrides
    virtual bool Resolve();

protected:
    virtual bool _CheckValid() const;

private:
    Hd_Osd3Subdivision *_subdivision;
    bool _adaptive;
};

// ---------------------------------------------------------------------------

Hd_Osd3Subdivision::Hd_Osd3Subdivision()
    : _vertexStencils(NULL),
      _varyingStencils(NULL),
      _patchTable(NULL),
      _adaptive(false)
{
#if HD_ENABLE_GPU_SUBDIVISION
    _gpuStencilTable = NULL;
#endif
}

Hd_Osd3Subdivision::~Hd_Osd3Subdivision()
{
    delete _vertexStencils;
    delete _varyingStencils;
    delete _patchTable;
#if HD_ENABLE_GPU_SUBDIVISION
    delete _gpuStencilTable;
#endif
}

void
Hd_Osd3Subdivision::SetRefinementTables(
    OpenSubdiv::Far::StencilTable const *vertexStencils,
    OpenSubdiv::Far::StencilTable const *varyingStencils,
    OpenSubdiv::Far::PatchTable const *patchTable,
    bool adaptive)
{
    if (_vertexStencils) delete _vertexStencils;
    if (_varyingStencils) delete _varyingStencils;
    if (_patchTable) delete _patchTable;

    _vertexStencils = vertexStencils;
    _varyingStencils = varyingStencils;
    _patchTable = patchTable;
    _adaptive = adaptive;
}

/*virtual*/
int
Hd_Osd3Subdivision::GetNumVertices() const
{
    // returns the total number of vertices, including coarse and refined ones.
    if (!TF_VERIFY(_vertexStencils)) return 0;

    return _vertexStencils->GetNumStencils() +
        _vertexStencils->GetNumControlVertices();
}

/*virtual*/
void
Hd_Osd3Subdivision::RefineCPU(HdBufferSourceSharedPtr const &source,
                              bool varying,
                              void *vertexBuffer)
{
    OpenSubdiv::Far::StencilTable const *stencilTable =
        varying ? _varyingStencils : _vertexStencils;

    if (!TF_VERIFY(stencilTable)) return;

    OpenSubdiv::Osd::CpuVertexBuffer *osdVertexBuffer =
        static_cast<OpenSubdiv::Osd::CpuVertexBuffer*>(vertexBuffer);

    int numElements = source->GetNumElements();
    int stride = source->GetNumComponents();

    // NOTE: in osd, GetNumElements() returns how many fields in a vertex
    //          (i.e.  3 for XYZ, and 4 for RGBA)
    //       in hydra, GetNumElements() returns how many vertices
    //       (or faces, etc) in a buffer. We basically follow the hydra
    //       convention in this file.
    TF_VERIFY(stride == osdVertexBuffer->GetNumElements());

    // if the mesh has more vertices than that in use in topology (faceIndices),
    // we need to trim the buffer so that they won't overrun the coarse
    // vertex buffer which we allocated using the stencil table.
    // see Hd_Osd3Subdivision::GetNumVertices()
    if (numElements > stencilTable->GetNumControlVertices()) {
        numElements = stencilTable->GetNumControlVertices();
    }

    // filling coarse vertices
    osdVertexBuffer->UpdateData((const float*)source->GetData(),
                                /*offset=*/0, numElements);

    // if there is no stencil (e.g. torus with adaptive refinement),
    // just return here
    if (stencilTable->GetNumStencils() == 0) return;

    // apply opensubdiv with CPU evaluator.
    OpenSubdiv::Osd::BufferDescriptor srcDesc(0, stride, stride);
    OpenSubdiv::Osd::BufferDescriptor dstDesc(numElements*stride, stride, stride);

    OpenSubdiv::Osd::CpuEvaluator::EvalStencils(
        osdVertexBuffer, srcDesc,
        osdVertexBuffer, dstDesc,
        stencilTable);
}

/*virtual*/
void
Hd_Osd3Subdivision::RefineGPU(HdBufferArrayRangeSharedPtr const &range,
                              TfToken const &name)
{
#if HD_ENABLE_GPU_SUBDIVISION
    // filling coarse vertices has been done at resource registry.

    // vertex buffer wrapper for OpenSubdiv API
    Hd_OsdRefineComputationGPU::VertexBuffer vertexBuffer(
        range->GetResource(name));

    // vertex buffer is not interleaved, but aggregated.
    // we need an offset to locate the current range.
    int stride = vertexBuffer.GetNumElements();
    int numCoarseVertices = _vertexStencils->GetNumControlVertices();

    OpenSubdiv::Osd::BufferDescriptor srcDesc(
        /*offset=*/range->GetOffset() * stride,
        /*length=*/stride,
        /*stride=*/stride);
    OpenSubdiv::Osd::BufferDescriptor dstDesc(
        /*offset=*/(range->GetOffset() + numCoarseVertices) * stride,
        /*length=*/stride,
        /*stride=*/stride);

    // GPU evaluator can be static, as long as it's called sequentially.
    static OpenSubdiv::Osd::EvaluatorCacheT<Hd_OsdGpuEvaluator> evaluatorCache;

    Hd_OsdGpuEvaluator const *instance =
        OpenSubdiv::Osd::GetEvaluator<Hd_OsdGpuEvaluator>(
            &evaluatorCache, srcDesc, dstDesc, (void*)NULL /*deviceContext*/);

    instance->EvalStencils(&vertexBuffer, srcDesc,
                           &vertexBuffer, dstDesc,
                           _GetGpuStencilTable());
#else
    TF_CODING_ERROR("No GPU kernel available.\n");
#endif
}

/*virtual*/
HdBufferSourceSharedPtr
Hd_Osd3Subdivision::CreateTopologyComputation(HdMeshTopology *topology,
                                              bool adaptive,
                                              int level,
                                              SdfPath const &id)
{
    return HdBufferSourceSharedPtr(new Hd_Osd3TopologyComputation(
                                       this, topology, adaptive, level, id));

}

/*virtual*/
HdBufferSourceSharedPtr
Hd_Osd3Subdivision::CreateIndexComputation(HdMeshTopology *topology,
                                           HdBufferSourceSharedPtr const &osdTopology)
{
    return HdBufferSourceSharedPtr(new Hd_Osd3IndexComputation(
                                       this, topology, osdTopology));
}

/*virtual*/
HdBufferSourceSharedPtr
Hd_Osd3Subdivision::CreateRefineComputation(HdMeshTopology *topology,
                                            HdBufferSourceSharedPtr const &source,
                                            bool varying,
                                            HdBufferSourceSharedPtr const &osdTopology)
{
    return HdBufferSourceSharedPtr(new Hd_OsdRefineComputation<Hd_OsdCpuVertexBuffer>(
                                       topology, source, varying, osdTopology));
}

/*virtual*/
HdComputationSharedPtr
Hd_Osd3Subdivision::CreateRefineComputationGPU(HdMeshTopology *topology,
                                           TfToken const &name,
                                           GLenum dataType,
                                           int numComponents)
{
    return HdComputationSharedPtr(new Hd_OsdRefineComputationGPU(
                                      topology, name, dataType, numComponents));
}

#if HD_ENABLE_GPU_SUBDIVISION
Hd_OsdGpuStencilTable *
Hd_Osd3Subdivision::_GetGpuStencilTable()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_gpuStencilTable) {
        _gpuStencilTable = Hd_OsdGpuStencilTable::Create(
            _vertexStencils, NULL);
    }

    return _gpuStencilTable;
}
#endif

// ---------------------------------------------------------------------------

Hd_Osd3TopologyComputation::Hd_Osd3TopologyComputation(
    Hd_Osd3Subdivision *subdivision,
    HdMeshTopology *topology,
    bool adaptive, int level, SdfPath const &id)
    : Hd_OsdTopologyComputation(topology, level, id),
      _subdivision(subdivision), _adaptive(adaptive)
{
}

bool
Hd_Osd3TopologyComputation::Resolve()
{
    using namespace OpenSubdiv;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // do far analysis and set stencils and patch table into Hd_Subdivision.

    // create topology refiner
    PxOsdTopologyRefinerSharedPtr refiner;

    if (!TF_VERIFY(_topology)) {
        _SetResolved();
        return true;
    }

    // for empty topology, we don't need to refine anything.
    // but still need to return the typed buffer for codegen
    if (_topology->GetFaceVertexCounts().size() == 0) {
        // leave refiner empty
    } else {
        refiner = PxOsdRefinerFactory::Create(_topology->GetPxOsdMeshTopology(),
                                              TfToken(_id.GetText()));
    }

    if (!TF_VERIFY(_subdivision)) {
        _SetResolved();
        return true;
    }

    // refine
    //  and
    // create stencil/patch table
    Far::StencilTable const *vertexStencils = NULL;
    Far::StencilTable const *varyingStencils = NULL;
    Far::PatchTable const *patchTable = NULL;

    if (refiner) {
        // split trace scopes.
        {
            HD_TRACE_SCOPE("refine");
            if (_adaptive) {
                refiner->RefineAdaptive(_level);
            } else {
                refiner->RefineUniform(_level);
            }
        }
        {
            HD_TRACE_SCOPE("stencil factory");
            Far::StencilTableFactory::Options options;
            options.generateOffsets = true;
            options.generateIntermediateLevels = _adaptive;
            options.interpolationMode =
                Far::StencilTableFactory::INTERPOLATE_VERTEX;
            vertexStencils = Far::StencilTableFactory::Create(*refiner, options);

            options.interpolationMode =
                Far::StencilTableFactory::INTERPOLATE_VARYING;
            varyingStencils = Far::StencilTableFactory::Create(*refiner, options);
        }
        {
            HD_TRACE_SCOPE("patch factory");
            Far::PatchTableFactory::Options options;
            if (_adaptive) {
                options.endCapType =
                    Far::PatchTableFactory::Options::ENDCAP_BSPLINE_BASIS;
            }

            patchTable = Far::PatchTableFactory::Create(*refiner, options);
        }
    }

    // merge endcap
    if (patchTable && patchTable->GetLocalPointStencilTable()) {
        // append stencils
        if (Far::StencilTable const *vertexStencilsWithLocalPoints =
            Far::StencilTableFactory::AppendLocalPointStencilTable(
                *refiner,
                vertexStencils,
                patchTable->GetLocalPointStencilTable())) {
            delete vertexStencils;
            vertexStencils = vertexStencilsWithLocalPoints;
        }
        if (Far::StencilTable const *varyingStencilsWithLocalPoints =
            Far::StencilTableFactory::AppendLocalPointStencilTable(
                *refiner,
                varyingStencils,
                patchTable->GetLocalPointStencilTable())) {
            delete varyingStencils;
            varyingStencils = varyingStencilsWithLocalPoints;
        }
    }

    // set tables to topology
    // Hd_Subdivision takes an ownership of stencilTable and patchTable.
    _subdivision->SetRefinementTables(vertexStencils, varyingStencils,
                                      patchTable, _adaptive);

    _SetResolved();
    return true;
}

bool
Hd_Osd3TopologyComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

Hd_Osd3IndexComputation::Hd_Osd3IndexComputation (
    Hd_Osd3Subdivision *subdivision,
    HdMeshTopology *topology,
    HdBufferSourceSharedPtr const &osdTopology)
    : Hd_OsdIndexComputation(topology, osdTopology)
    , _subdivision(subdivision)
{
}

bool
Hd_Osd3IndexComputation::Resolve()
{
    using namespace OpenSubdiv;

    if (_osdTopology && !_osdTopology->IsResolved()) return false;

    if (!_TryLock()) return false;

    Hd_Subdivision *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) {
        _SetResolved();
        return true;
    }

    Far::PatchTable const *patchTable = _subdivision->GetPatchTable();

    Far::Index const *firstIndex = NULL;
    size_t ptableSize = 0;
    if (patchTable) {
        ptableSize = patchTable->GetPatchControlVerticesTable().size();
        if (ptableSize > 0) {
            firstIndex = &patchTable->GetPatchControlVerticesTable()[0];
        }
    }

    TfToken const& scheme = _topology->GetScheme();

    if (Hd_Subdivision::RefinesToTriangles(scheme)) {
        // populate refined triangle indices.
        VtArray<GfVec3i> indices(ptableSize/3);
        memcpy(indices.data(), firstIndex, ptableSize * sizeof(int));

        HdBufferSourceSharedPtr triIndices(
            new HdVtBufferSource(HdTokens->indices, VtValue(indices)));
        _SetResult(triIndices);

        _PopulateUniformPrimitiveBuffer(patchTable);
    } else if (_subdivision->IsAdaptive() &&
               Hd_Subdivision::RefinesToBSplinePatches(scheme)) {

        VtArray<Hd_BSplinePatchIndex> indices(ptableSize/16);
        memcpy(indices.data(), firstIndex, ptableSize * sizeof(int));

        HdBufferSourceSharedPtr patchIndices(
            new HdVtBufferSource(HdTokens->indices, VtValue(indices)));
        _SetResult(patchIndices);

        _PopulateBSplinePrimitiveBuffer(patchTable);
    } else {
        // populate refined quad indices.
        VtArray<GfVec4i> indices(ptableSize/4);
        memcpy(indices.data(), firstIndex, ptableSize * sizeof(int));

        // refined quads index buffer
        HdBufferSourceSharedPtr quadIndices(
            new HdVtBufferSource(HdTokens->indices, VtValue(indices)));
        _SetResult(quadIndices);

        _PopulateUniformPrimitiveBuffer(patchTable);
    }

    _SetResolved();
    return true;
}

void
Hd_Osd3IndexComputation::_CreatePtexIndexToCoarseFaceIndexMapping(
    std::vector<int> *result)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(result)) return;

    int const * numVertsPtr =
        _topology->GetFaceVertexCounts().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();

    // hole faces shouldn't affect ptex id.
    // passing empty array to count num quads.
    int numQuads = HdMeshTopology::ComputeNumQuads(
        _topology->GetFaceVertexCounts(),
        /*holeFaces=*/VtIntArray());
    result->clear();
    result->reserve(numQuads);

    for (int i = 0; i < numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) continue;
        if (nv == 4) {
            result->push_back(i);
        } else {
            for (int j = 0; j < nv; ++j)
                result->push_back(i);
        }
    }
}

void
Hd_Osd3IndexComputation::_PopulateUniformPrimitiveBuffer(
    OpenSubdiv::Far::PatchTable const *patchTable)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // primitiveParam from patchtable contains a map of
    // gl_PrimitiveID to PtexIndex. It should be reinterpreted
    // to face index if necessary.
    std::vector<int> ptexIndexToFaceIndexMapping;
    _CreatePtexIndexToCoarseFaceIndexMapping(&ptexIndexToFaceIndexMapping);

    // store faceIndex, ptexIndex and edgeFlag(=0)
    size_t numPatches = patchTable
        ? patchTable->GetPatchParamTable().size()
        : 0;
    VtVec3iArray primitiveParam(numPatches);

    // ivec3
    for (size_t i = 0; i < numPatches; ++i) {
        OpenSubdiv::Far::PatchParam const &patchParam =
            patchTable->GetPatchParamTable()[i];

        int ptexIndex = patchParam.GetFaceId();
        int faceIndex = ptexIndexToFaceIndexMapping[ptexIndex];
        unsigned int field0 = patchParam.field0;
        unsigned int field1 = patchParam.field1;
        primitiveParam[i][0] =
            HdMeshTopology::EncodeCoarseFaceParam(faceIndex, 0);
        primitiveParam[i][1] = *((int*)&field0);
        primitiveParam[i][2] = *((int*)&field1);
    }

    _primitiveBuffer.reset(new HdVtBufferSource(
                               HdTokens->primitiveParam,
                               VtValue(primitiveParam)));
}

void
Hd_Osd3IndexComputation::_PopulateBSplinePrimitiveBuffer(
    OpenSubdiv::Far::PatchTable const *patchTable)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    std::vector<int> ptexIndexToFaceIndexMapping;
    _CreatePtexIndexToCoarseFaceIndexMapping(&ptexIndexToFaceIndexMapping);

    // BSPLINES
    size_t numPatches = patchTable
        ? patchTable->GetPatchParamTable().size()
        : 0;
    VtVec4iArray primitiveParam(numPatches);

    // ivec4
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

        int ptexIndex = patchParam.GetFaceId();
        int faceIndex = ptexIndexToFaceIndexMapping[ptexIndex];
        unsigned int field0 = patchParam.field0;
        unsigned int field1 = patchParam.field1;
        primitiveParam[i][0] =
            HdMeshTopology::EncodeCoarseFaceParam(faceIndex, 0);
        primitiveParam[i][1] = *((int*)&field0);
        primitiveParam[i][2] = *((int*)&field1);

        int sharpnessAsInt = static_cast<int>(sharpness);
        primitiveParam[i][3] = sharpnessAsInt;
    }
    _primitiveBuffer.reset(new HdVtBufferSource(
                               HdTokens->primitiveParam,
                               VtValue(primitiveParam)));
}

// ---------------------------------------------------------------------------

Hd_Subdivision *
Hd_Osd3Factory::CreateSubdivision()
{
    return new Hd_Osd3Subdivision();
}

