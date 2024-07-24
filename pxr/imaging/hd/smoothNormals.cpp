//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE


template <typename SrcVec3Type, typename DstType>
class _SmoothNormalsWorker
{
public:
    _SmoothNormalsWorker(SrcVec3Type const * pointsPtr,
                         VtIntArray const &adjacencyTable,
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
    VtIntArray const &_adjacencyTable;
    DstType *_normals;
};

/// Returns an array of the same size and type as the source points
/// containing normal vectors computed by averaging the cross products
/// of incident face edges.
template <typename SrcVec3Type, typename DstType=SrcVec3Type>
VtArray<DstType>
_ComputeSmoothNormals(int numPoints, SrcVec3Type const * pointsPtr,
                      VtIntArray const &adjacencyTable, int numAdjPoints)
{
    // to be safe.
    // numPoints of input pointer could be different from the number of points
    // in adjacency table.
    numPoints = std::min(numPoints, numAdjPoints);

    VtArray<DstType> normals(numPoints);

    _SmoothNormalsWorker<SrcVec3Type, DstType> workerState
        (pointsPtr, adjacencyTable, normals.data());

    WorkParallelForN(
        numPoints,
        std::bind(&_SmoothNormalsWorker<SrcVec3Type, DstType>::Compute,
                  std::ref(workerState), std::placeholders::_1, std::placeholders::_2));

    return normals;
}

/* static */
VtArray<GfVec3f>
Hd_SmoothNormals::ComputeSmoothNormals(
    Hd_VertexAdjacency const * adjacency,
    int numPoints,
    GfVec3f const * pointsPtr)
{
    return _ComputeSmoothNormals(numPoints, pointsPtr,
        adjacency->GetAdjacencyTable(), adjacency->GetNumPoints());
}

/* static */
VtArray<GfVec3d>
Hd_SmoothNormals::ComputeSmoothNormals(
    Hd_VertexAdjacency const * adjacency,
    int numPoints,
    GfVec3d const * pointsPtr)
{
    return _ComputeSmoothNormals(numPoints, pointsPtr,
        adjacency->GetAdjacencyTable(), adjacency->GetNumPoints());
}

/* static */
VtArray<HdVec4f_2_10_10_10_REV>
Hd_SmoothNormals::ComputeSmoothNormalsPacked(
    Hd_VertexAdjacency const * adjacency,
    int numPoints,
    GfVec3f const * pointsPtr)
{
    return _ComputeSmoothNormals<GfVec3f, HdVec4f_2_10_10_10_REV>(
        numPoints, pointsPtr, adjacency->GetAdjacencyTable(),
        adjacency->GetNumPoints());
}

/* static */
VtArray<HdVec4f_2_10_10_10_REV>
Hd_SmoothNormals::ComputeSmoothNormalsPacked(
    Hd_VertexAdjacency const * adjacency,
    int numPoints,
    GfVec3d const * pointsPtr)
{
    return _ComputeSmoothNormals<GfVec3d, HdVec4f_2_10_10_10_REV>(
        numPoints, pointsPtr, adjacency->GetAdjacencyTable(),
        adjacency->GetNumPoints());
}


PXR_NAMESPACE_CLOSE_SCOPE

