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
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/work/loops.h"

Hd_VertexAdjacency::Hd_VertexAdjacency()
    : _stride(0)
{
}

template <typename Vec3Type>
class _SmoothNormalsWorker
{
public:
    _SmoothNormalsWorker(Vec3Type const * pointsPtr,
                         std::vector<int> const &entry,
                         int stride,
                         VtArray<Vec3Type> *normals)
    : _pointsPtr(pointsPtr)
    , _entry(entry)
    , _stride(stride)
    , _normals(normals)
    {
    }

    void Compute(size_t begin, size_t end)
    {
        for(size_t i = begin; i < end; ++i) {
            int const * e = &_entry[i * _stride];
            int valence = *e++;
            Vec3Type normal(0);
            Vec3Type const & curr = _pointsPtr[i];
            for (int j=0; j<valence; ++j) {
                Vec3Type const & prev = _pointsPtr[*e++];
                Vec3Type const & next = _pointsPtr[*e++];
                // All meshes have all been converted to rightHanded
                normal += GfCross(next-curr, prev-curr);
            }
            if (true) { // Could defer normalization to shader code
                normal.Normalize();
            }
            (*_normals)[i] = normal;
        }
    }

private:
    Vec3Type const * _pointsPtr;
    std::vector<int> const &_entry;
    int _stride;
    VtArray<Vec3Type> *_normals;
};

/// Returns an array of the same size and type as the source points
/// containing normal vectors computed by averaging the cross products
/// of incident face edges.
template <typename Vec3Type>
VtArray<Vec3Type>
_ComputeSmoothNormals(int numPoints, Vec3Type const * pointsPtr,
                      std::vector<int> const &entry, int stride)
{
    // to be safe.
    // numPoints of input pointer could be different from the number of points
    // in adjacency table.
    numPoints = std::min(numPoints,
                         (int)entry.size()/stride);

    VtArray<Vec3Type> normals(numPoints);

    _SmoothNormalsWorker<Vec3Type> workerState
                                           (pointsPtr, entry, stride, &normals);

    WorkParallelForN(numPoints,
                     boost::bind(&_SmoothNormalsWorker<Vec3Type>::Compute,
                                 workerState, _1, _2));



    return normals;
}

VtArray<GfVec3f>
Hd_VertexAdjacency::ComputeSmoothNormals(int numPoints,
                                         GfVec3f const * pointsPtr) const
{
    return _ComputeSmoothNormals(numPoints, pointsPtr,
                                 _entry, _stride);
}

VtArray<GfVec3d>
Hd_VertexAdjacency::ComputeSmoothNormals(int numPoints,
                                         GfVec3d const * pointsPtr) const
{
    return _ComputeSmoothNormals(numPoints, pointsPtr,
                                 _entry, _stride);
}

HdBufferSourceSharedPtr
Hd_VertexAdjacency::GetSmoothNormalsComputation(
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName)
{
    // if the vertex adjacency is scheduled to be built (and not yet resolved),
    // make a dependency to its builder.
    return HdBufferSourceSharedPtr(
        new Hd_SmoothNormalsComputation(
            this, points, dstName, _adjacencyBuilder.lock()));
}

HdComputationSharedPtr
Hd_VertexAdjacency::GetSmoothNormalsComputationGPU(TfToken const &srcName,
                                                   TfToken const &dstName,
                                                   GLenum dstDataType)
{
    return HdComputationSharedPtr(new Hd_SmoothNormalsComputationGPU(
                                      this, srcName, dstName, dstDataType));
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
    if (_stride > 0) return Hd_AdjacencyBuilderComputationSharedPtr();

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
    if (not _TryLock()) return false;

    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // compute adjacency

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    // compute numPoints from topology indices
    int numPoints = HdMeshTopology::ComputeNumPoints(
        _topology->GetFaceVertexIndices());
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    // Find the max vertex valence, we'll use this 
    // to determine how to lay out our adjacency entries.
    std::vector<int> vertexValence(numPoints);
    int vertIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        for (int j=0; j<nv; ++j) {
            int index = vertsPtr[vertIndex++];
            if (index < 0 or index >= numPoints) {
                TF_CODING_ERROR("vertex index out of range "
                                "index: %d numPoints: %d", index, numPoints);
                return false;
            }
            ++vertexValence[index];
        }
    }
    int maxVertexValence = 0;
    for (int i=0; i<numPoints; ++i) {
        maxVertexValence = std::max(maxVertexValence, vertexValence[i]);
    }

    // Each entry is a count followed by pairs of adjacent vertex indices.
    // We use a uniform entry size for all vertices, this allows faster
    // lookups at the cost of some additional memory.
    _adjacency->_stride = 1+2*maxVertexValence;
    _adjacency->_entry.clear();
    _adjacency->_entry.resize(numPoints * _adjacency->_stride, 0);

    vertIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        for (int j=0; j<nv; ++j) {
            int prev = vertsPtr[vertIndex+(j+nv-1)%nv];
            int curr = vertsPtr[vertIndex+j];
            int next = vertsPtr[vertIndex+(j+1)%nv];
            if (flip) std::swap(prev, next);
            int * entry = &_adjacency->_entry[curr * _adjacency->_stride];
            int index = (*entry)++ * 2 + 1; // increment count
            entry[index+0] = prev;
            entry[index+1] = next;
        }
        vertIndex += nv;
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
    if (not _adjacencyBuilder->IsResolved()) return false;
    if (not _TryLock()) return false;

    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // prepare buffer source to be transferred.
    std::vector<int> const &adjacency = _adjacency->GetEntry();

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
