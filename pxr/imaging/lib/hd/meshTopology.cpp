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

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/quadrangulate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/subdivision.h"
#include "pxr/imaging/hd/subdivision3.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/triangulate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

TF_DEFINE_ENV_SETTING(HD_ENABLE_OPENSUBDIV3_ADAPTIVE, 0,
                      "Enables OpenSubdiv 3 Adaptive Tessellation");

HdMeshTopology::HdMeshTopology()
    : _refineLevel(0)
    , _quadInfo(NULL)
    , _subdivision(NULL)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::HdMeshTopology(const HdMeshTopology &src, int refineLevel)
    : _topology(src.GetPxOsdMeshTopology())
    , _refineLevel(refineLevel)
    , _quadInfo(NULL)
    , _subdivision(NULL)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::HdMeshTopology(const PxOsdMeshTopology &topo, int refineLevel)
    : _topology(topo)
    , _refineLevel(refineLevel)
    , _quadInfo(NULL)
    , _subdivision(NULL)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::HdMeshTopology(
    TfToken scheme,
    TfToken orientation,
    VtIntArray faceVertexCounts, 
    VtIntArray faceVertexIndices,
    int refineLevel)
    : _topology(scheme,
                orientation,
                faceVertexCounts,
                faceVertexIndices)
    , _refineLevel(refineLevel)
    , _quadInfo(NULL)
    , _subdivision(NULL)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::HdMeshTopology(
    TfToken scheme,
    TfToken orientation,
    VtIntArray faceVertexCounts, 
    VtIntArray faceVertexIndices,
    VtIntArray holeIndices,
    int refineLevel)
    : _topology(scheme,
                orientation,
                faceVertexCounts,
                faceVertexIndices,
                holeIndices)
    , _refineLevel(refineLevel)
    , _quadInfo(NULL)
    , _subdivision(NULL)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::~HdMeshTopology()
{
    HD_PERF_COUNTER_DECR(HdPerfTokens->meshTopology);

    delete _quadInfo;
    delete _subdivision;
}

bool
HdMeshTopology::IsEnabledAdaptive()
{
    return TfGetEnvSetting(HD_ENABLE_OPENSUBDIV3_ADAPTIVE) == 1;
}

bool
HdMeshTopology::operator==(HdMeshTopology const &other) const {

    TRACE_FUNCTION();

    // no need to compare _adajency and _quadInfo
    return (_topology == other._topology);
}

int
HdMeshTopology::GetNumFaces() const
{
    return (int)_topology.GetFaceVertexCounts().size();
}

int
HdMeshTopology::GetNumFaceVaryings() const
{
    return (int)_topology.GetFaceVertexIndices().size();
}

int
HdMeshTopology::ComputeNumPoints() const
{
    return HdMeshTopology::ComputeNumPoints(_topology.GetFaceVertexIndices());
}

/*static*/ int
HdMeshTopology::ComputeNumPoints(VtIntArray const &verts)
{
    HD_TRACE_FUNCTION();

    // compute numPoints from topology indices
    int numIndices = verts.size();
    int numPoints = -1;
    int const * vertsPtr = verts.cdata();
    for (int i= 0;i <numIndices; ++i) {
        // find the max vertex index in face verts
        numPoints = std::max(numPoints, vertsPtr[i]);
    }
    // numPoints = max vertex index + 1
    return numPoints + 1;
}

/*static*/ int
HdMeshTopology::ComputeNumQuads(VtIntArray const &numVerts,
                                VtIntArray const &holeFaces,
                                bool *invalidFaceFound)
{
    HD_TRACE_FUNCTION();

    int numFaces = numVerts.size();
    int numHoleFaces = holeFaces.size();
    int numQuads = 0;
    int const *numVertsPtr = numVerts.cdata();
    int const * holeFacesPtr = holeFaces.cdata();
    int holeIndex = 0;

    for (int i = 0; i < numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) {
            // skip degenerated face
            if (invalidFaceFound) *invalidFaceFound = true;
        } else if (holeIndex < numHoleFaces && holeFacesPtr[holeIndex] == i) {
            // skip hole face
            ++holeIndex;
        } else {
            // non-quad n-gons are quadrangulated into n-quads.
            numQuads += (nv == 4 ? 1 : nv);
        }
    }
    return numQuads;
}

HdTopology::ID
HdMeshTopology::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    HdTopology::ID id =_topology.ComputeHash();
    boost::hash_combine(id, _refineLevel);

    return id;
}

void
HdMeshTopology::SetQuadInfo(Hd_QuadInfo const *quadInfo)
{
    delete _quadInfo;
    _quadInfo = quadInfo;
}

HdBufferSourceSharedPtr
HdMeshTopology::GetPointsIndexBuilderComputation()
{
    // this is simple enough to return the result right away.
    int numPoints = ComputeNumPoints();
    VtIntArray indices(numPoints);
    for (int i = 0; i < numPoints; ++i) indices[i] = i;

    return HdBufferSourceSharedPtr(
        new HdVtBufferSource(HdTokens->indices, VtValue(indices)));
}

HdBufferSourceSharedPtr
HdMeshTopology::GetTriangleIndexBuilderComputation(SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new Hd_TriangleIndexBuilderComputation(this, id));
}

Hd_QuadInfoBuilderComputationSharedPtr
HdMeshTopology::GetQuadInfoBuilderComputation(
    bool gpu, SdfPath const &id, HdResourceRegistry *resourceRegistry)
{
    Hd_QuadInfoBuilderComputationSharedPtr builder(
        new Hd_QuadInfoBuilderComputation(this, id));

    // store as a weak ptr.
    _quadInfoBuilder = builder;

    if (gpu) {
        if (!TF_VERIFY(resourceRegistry)) {
            TF_CODING_ERROR("resource registry must be non-null "
                            "if gpu quadinfo is requested.");
            return builder;
        }

        HdBufferSourceSharedPtr quadrangulateTable(
            new Hd_QuadrangulateTableComputation(this, builder));

        // allocate quadrangulation table on GPU
        HdBufferSpecVector bufferSpecs;
        quadrangulateTable->AddBufferSpecs(&bufferSpecs);

        _quadrangulateTableRange =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs);

        resourceRegistry->AddSource(_quadrangulateTableRange, quadrangulateTable);
    }
    return builder;
}

HdBufferSourceSharedPtr
HdMeshTopology::GetQuadIndexBuilderComputation(SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new Hd_QuadIndexBuilderComputation(this, _quadInfoBuilder.lock(), id));
}

HdBufferSourceSharedPtr
HdMeshTopology::GetQuadrangulateComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    // check if the quad table is already computed as all-quads.
    if (_quadInfo && _quadInfo->IsAllQuads()) {
        // no need of quadrangulation.
        return HdBufferSourceSharedPtr();
    }

    // Make a dependency to quad info, in case if the topology
    // is chaging and the quad info hasn't been populated.
    //
    // It can be null for the second or later primvar animation.
    // Don't call GetQuadInfoBuilderComputation instead. It may result
    // unregisterd computation.
    HdBufferSourceSharedPtr quadInfo = _quadInfoBuilder.lock();

    return HdBufferSourceSharedPtr(
        new Hd_QuadrangulateComputation(this, source, quadInfo, id));
}

HdComputationSharedPtr
HdMeshTopology::GetQuadrangulateComputationGPU(
    TfToken const &name, GLenum dataType, SdfPath const &id)
{
    // check if the quad table is already computed as all-quads.
    if (_quadInfo && _quadInfo->IsAllQuads()) {
        // no need of quadrangulation.
        return HdComputationSharedPtr();
    }
    return HdComputationSharedPtr(
        new Hd_QuadrangulateComputationGPU(this, name, dataType, id));
}

HdBufferSourceSharedPtr
HdMeshTopology::GetQuadrangulateFaceVaryingComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new Hd_QuadrangulateFaceVaryingComputation(this, source, id));
}

HdBufferSourceSharedPtr
HdMeshTopology::GetTriangulateFaceVaryingComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new Hd_TriangulateFaceVaryingComputation(this, source, id));
}

bool
HdMeshTopology::RefinesToTriangles() const
{
    return Hd_Subdivision::RefinesToTriangles(_topology.GetScheme());
}

bool
HdMeshTopology::RefinesToBSplinePatches() const
{
    return (IsEnabledAdaptive() &&
            Hd_Subdivision::RefinesToBSplinePatches(_topology.GetScheme()));
}

HdBufferSourceSharedPtr
HdMeshTopology::GetOsdTopologyComputation(SdfPath const &id)
{
    if (HdBufferSourceSharedPtr builder = _osdTopologyBuilder.lock()) {
        return builder;
    }

    // this has to be the first instance.
    if (!TF_VERIFY(!_subdivision)) return HdBufferSourceSharedPtr();

    // create Hd_Subdivision
    _subdivision = Hd_Osd3Factory::CreateSubdivision();

    if (!TF_VERIFY(_subdivision)) return HdBufferSourceSharedPtr();

    bool adaptive = RefinesToBSplinePatches();

    // create a topology computation for Hd_Subdivision
    HdBufferSourceSharedPtr builder =
        _subdivision->CreateTopologyComputation(this, adaptive,
                                                _refineLevel, id);
    _osdTopologyBuilder = builder; // retain weak ptr
    return builder;
}

HdBufferSourceSharedPtr
HdMeshTopology::GetOsdIndexBuilderComputation()
{
    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();
    return _subdivision->CreateIndexComputation(this, topologyBuilder);
}

HdBufferSourceSharedPtr
HdMeshTopology::GetOsdRefineComputation(HdBufferSourceSharedPtr const &source,
                                        bool varying)
{
    // Make a dependency to far mesh.
    // (see comment on GetQuadrangulateComputation)
    //
    // It can be null for the second or later primvar animation.
    // Don't call GetOsdTopologyComputation instead. It may result
    // unregisterd computation.

    // for empty topology, we don't need to refine anything.
    // source will be scheduled at the caller
    if (_topology.GetFaceVertexCounts().size() == 0) return source;

    if (!TF_VERIFY(_subdivision)) {
        TF_CODING_ERROR("GetOsdTopologyComputation should be called before "
                        "GetOsdRefineComputation.");
        return source;
    }

    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();
    resourceRegistry->AddSource(source);

    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();

    return _subdivision->CreateRefineComputation(this, source, varying,
                                                 topologyBuilder);
}

HdComputationSharedPtr
HdMeshTopology::GetOsdRefineComputationGPU(TfToken const &name,
                                           GLenum dataType,
                                           int numComponents)
{
    // for empty topology, we don't need to refine anything.
    if (_topology.GetFaceVertexCounts().size() == 0) {
        return HdComputationSharedPtr();
    }

    if (!TF_VERIFY(_subdivision)) return HdComputationSharedPtr();

    return _subdivision->CreateRefineComputationGPU(
        this, name, dataType, numComponents);
}
