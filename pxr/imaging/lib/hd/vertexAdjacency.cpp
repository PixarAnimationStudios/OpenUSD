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
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE


Hd_VertexAdjacency::Hd_VertexAdjacency()
    : _numPoints(0)
    , _adjacencyTable()
    , _adjacencyRange()
    , _adjacencyBuilder()
{
}

Hd_VertexAdjacency::~Hd_VertexAdjacency()
{
    HD_PERF_COUNTER_SUBTRACT(HdPerfTokens->adjacencyBufSize,
                             _adjacencyTable.size() * sizeof(int));
}


template <typename SrcVec3Type, typename DstType>
class _SmoothNormalsWorker
{
public:
    _SmoothNormalsWorker(SrcVec3Type const * pointsPtr,
                         std::vector<int> const &adjacencyTable,
                         DstType *normals)
    : _pointsPtr(pointsPtr)
    , _adjacencyTable(adjacencyTable)
    , _normals(normals)
    {
    }

    void Compute(size_t begin, size_t end)
    {
        for(size_t i = begin; i < end; ++i) {
            int offsetIdx = i * 2;
            int offset  = _adjacencyTable[offsetIdx];
            int valence = _adjacencyTable[offsetIdx + 1];

            int const * e = &_adjacencyTable[offset];
            SrcVec3Type normal(0);
            SrcVec3Type const & curr = _pointsPtr[i];
            for (int j=0; j<valence; ++j) {
                SrcVec3Type const & prev = _pointsPtr[*e++];
                SrcVec3Type const & next = _pointsPtr[*e++];
                // All meshes have all been converted to rightHanded
                normal += GfCross(next-curr, prev-curr);
            }
            if (true) { // Could defer normalization to shader code
                normal.Normalize();
            }
            _normals[i] = normal;
        }
    }

private:
    SrcVec3Type const * _pointsPtr;
    std::vector<int> const &_adjacencyTable;
    DstType *_normals;
};

/// Returns an array of the same size and type as the source points
/// containing normal vectors computed by averaging the cross products
/// of incident face edges.
template <typename SrcVec3Type, typename DstType=SrcVec3Type>
VtArray<DstType>
_ComputeSmoothNormals(int numPoints, SrcVec3Type const * pointsPtr,
                      std::vector<int> const &entry, int numAdjPoints)
{
    // to be safe.
    // numPoints of input pointer could be different from the number of points
    // in adjacency table.
    numPoints = std::min(numPoints, numAdjPoints);

    VtArray<DstType> normals(numPoints);

    _SmoothNormalsWorker<SrcVec3Type, DstType> workerState
        (pointsPtr, entry, normals.data());

    WorkParallelForN(
        numPoints,
        std::bind(&_SmoothNormalsWorker<SrcVec3Type, DstType>::Compute,
                  std::ref(workerState), std::placeholders::_1, std::placeholders::_2));

    return normals;
}

VtArray<GfVec3f>
Hd_VertexAdjacency::ComputeSmoothNormals(int numPoints,
                                         GfVec3f const * pointsPtr) const
{
    return _ComputeSmoothNormals(
                             numPoints, pointsPtr, _adjacencyTable, _numPoints);
}

VtArray<GfVec3d>
Hd_VertexAdjacency::ComputeSmoothNormals(int numPoints,
                                         GfVec3d const * pointsPtr) const
{
    return _ComputeSmoothNormals(
                             numPoints, pointsPtr, _adjacencyTable, _numPoints);
}

VtArray<HdVec4f_2_10_10_10_REV>
Hd_VertexAdjacency::ComputeSmoothNormalsPacked(int numPoints,
                                         GfVec3f const * pointsPtr) const
{
    return _ComputeSmoothNormals<GfVec3f, HdVec4f_2_10_10_10_REV>(
                             numPoints, pointsPtr, _adjacencyTable, _numPoints);

}

VtArray<HdVec4f_2_10_10_10_REV>
Hd_VertexAdjacency::ComputeSmoothNormalsPacked(int numPoints,
                                         GfVec3d const * pointsPtr) const
{
    return _ComputeSmoothNormals<GfVec3d, HdVec4f_2_10_10_10_REV>(
                             numPoints, pointsPtr, _adjacencyTable, _numPoints);
}

bool
Hd_VertexAdjacency::BuildAdjacencyTable(HdMeshTopology const *topology)
{
    // compute adjacency

    int const * numVertsPtr = topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = topology->GetFaceVertexIndices().cdata();
    int numFaces = topology->GetFaceVertexCounts().size();
    bool flip = (topology->GetOrientation() != HdTokens->rightHanded);

    // compute numPoints from topology indices
    _numPoints = HdMeshTopology::ComputeNumPoints(
        topology->GetFaceVertexIndices());


    // Track the number of entries needed the adjacency table.
    // We start by needing 2 per point (offset and valence).
    size_t numEntries = _numPoints * 2;


    // Compute the size of each entry, so we can work out the offsets.
    std::vector<int> vertexValence(_numPoints);


    int vertIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        for (int j=0; j<nv; ++j) {
            int index = vertsPtr[vertIndex++];
            if (index < 0 || index >= _numPoints) {
                TF_CODING_ERROR("vertex index out of range "
                                "index: %d numPoints: %d", index, _numPoints);
                _numPoints = 0;
                _adjacencyTable.clear();
                return false;
            }
            ++vertexValence[index];
        }

        // Increase the number of entries needed by 2 (prev & next index).
        // for every vertex in the face.
        numEntries += 2 * nv;
    }


    // Each entry is a count followed by pairs of adjacent vertex indices.
    // We use a uniform entry size for all vertices, this allows faster
    // lookups at the cost of some additional memory.
    HD_PERF_COUNTER_SUBTRACT(HdPerfTokens->adjacencyBufSize,
                                          _adjacencyTable.size() * sizeof(int));


    _adjacencyTable.clear();
    _adjacencyTable.resize(numEntries, 0);
    HD_PERF_COUNTER_ADD(HdPerfTokens->adjacencyBufSize,
                                                      numEntries * sizeof(int));

    // Fill out first part of buffer with offsets.  Even though we
    // know counts, don't fill them out now, so we know how many indices
    // we've written so far.
    int currentOffset = _numPoints * 2;
    for (int pointNum = 0; pointNum < _numPoints; ++pointNum) {
        _adjacencyTable[pointNum * 2] = currentOffset;
        currentOffset += 2*vertexValence[pointNum];
    }

    vertIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        for (int j=0; j<nv; ++j) {
            int prev = vertsPtr[vertIndex+(j+nv-1)%nv];
            int curr = vertsPtr[vertIndex+j];
            int next = vertsPtr[vertIndex+(j+1)%nv];
            if (flip) std::swap(prev, next);

            int entryOffset = _adjacencyTable[curr * 2 + 0];
            int &entryCount = _adjacencyTable[curr * 2 + 1];

            int pairOffset = entryOffset + entryCount * 2;
            ++entryCount;

            _adjacencyTable[pairOffset + 0] = prev;
            _adjacencyTable[pairOffset + 1] = next;
        }
        vertIndex += nv;
    }

    return true;
}

HdBufferSourceSharedPtr
Hd_VertexAdjacency::GetSmoothNormalsComputation(
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName, bool packed)
{
    // if the vertex adjacency is scheduled to be built (and not yet resolved),
    // make a dependency to its builder.
    return HdBufferSourceSharedPtr(
        new Hd_SmoothNormalsComputation(
            this, points, dstName, _adjacencyBuilder.lock(), packed));
}

HdComputationSharedPtr
Hd_VertexAdjacency::GetSmoothNormalsComputationGPU(TfToken const &srcName,
                                                   TfToken const &dstName,
                                                   GLenum srcDataType,
                                                   GLenum dstDataType)
{
    return HdComputationSharedPtr(new Hd_SmoothNormalsComputationGPU(
                                      this, srcName, dstName,
                                      srcDataType, dstDataType));
}


HdBufferSourceSharedPtr
Hd_VertexAdjacency::GetAdjacencyBuilderComputation(
    HdMeshTopology const *topology)
{
    // if there's a already requested (and unresolved) adjacency computation,
    // just returns it to make a dependency.
    if (Hd_AdjacencyBuilderComputationSharedPtr builder =
        _adjacencyBuilder.lock()) {

        return builder;
    }

    // if cpu adjacency table exists, no need to compute again
    if (!_adjacencyTable.empty()) {
        return Hd_AdjacencyBuilderComputationSharedPtr();
    }

    Hd_AdjacencyBuilderComputationSharedPtr builder =
        Hd_AdjacencyBuilderComputationSharedPtr(
            new Hd_AdjacencyBuilderComputation(this, topology));

    // store the computation as weak ptr so that it can be referenced
    // by another computation.
    _adjacencyBuilder = builder;

    return builder;
}

HdBufferSourceSharedPtr
Hd_VertexAdjacency::GetAdjacencyBuilderForGPUComputation()
{
    // If the cpu adjacency table has requested to be computed (and not yet
    // solved), make a dependency to its builder.
    HdBufferSourceSharedPtr gpuAdjacecnyBuilder =
        HdBufferSourceSharedPtr(new Hd_AdjacencyBuilderForGPUComputation(
                                    this, _adjacencyBuilder.lock()));

    return gpuAdjacecnyBuilder;
}


// ---------------------------------------------------------------------------

Hd_AdjacencyBuilderComputation::Hd_AdjacencyBuilderComputation(
    Hd_VertexAdjacency *adjacency, HdMeshTopology const *topology)
    : _adjacency(adjacency), _topology(topology)
{
}

bool
Hd_AdjacencyBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_adjacency->BuildAdjacencyTable(_topology)) {
        return false;
    }

    // call base class to mark as resolved.
    _SetResolved();
    return true;
}

bool
Hd_AdjacencyBuilderComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

Hd_AdjacencyBuilderForGPUComputation::Hd_AdjacencyBuilderForGPUComputation(
    Hd_VertexAdjacency const *adjacency,
    Hd_AdjacencyBuilderComputationSharedPtr const &adjacencyBuilder)
    : _adjacency(adjacency), _adjacencyBuilder(adjacencyBuilder)
{
}

bool
Hd_AdjacencyBuilderForGPUComputation::Resolve()
{
    if (!_adjacencyBuilder->IsResolved()) return false;
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // prepare buffer source to be transferred.
    std::vector<int> const &adjacency = _adjacency->GetAdjacencyTable();

    // create buffer source
    VtIntArray array(adjacency.size());
    for (size_t i = 0; i < adjacency.size(); ++i) {
        array[i] = adjacency[i];
    }
    _SetResult(HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->adjacency,
                                                            VtValue(array))));
    _SetResolved();
    return true;
}

void
Hd_AdjacencyBuilderForGPUComputation::AddBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->push_back(HdBufferSpec(HdTokens->adjacency, GL_INT, 1));
}

bool
Hd_AdjacencyBuilderForGPUComputation::_CheckValid() const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

