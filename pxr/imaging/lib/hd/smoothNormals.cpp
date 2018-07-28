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
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"
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

Hd_SmoothNormalsComputation::Hd_SmoothNormalsComputation(
    Hd_VertexAdjacency const *adjacency,
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName,
    HdBufferSourceSharedPtr const &adjacencyBuilder,
    bool packed)
    : _adjacency(adjacency), _points(points), _dstName(dstName),
      _adjacencyBuilder(adjacencyBuilder), _packed(packed)
{
}

void
Hd_SmoothNormalsComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // The datatype of normals is the same as that of points,
    // unless the packed format was requested.
    specs->emplace_back(_dstName,
        _packed ? HdTupleType { HdTypeInt32_2_10_10_10_REV, 1 }
        : _points->GetTupleType() );
}

TfToken const &
Hd_SmoothNormalsComputation::GetName() const
{
    return _dstName;
}

bool
Hd_SmoothNormalsComputation::Resolve()
{
    // dependency check first
    if (_adjacencyBuilder) {
        if (!_adjacencyBuilder->IsResolved()) return false;
    }
    if (!_points->IsResolved()) return false;
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_adjacency)) return true;

    size_t numPoints = _points->GetNumElements();

    HdBufferSourceSharedPtr normals;

    switch (_points->GetTupleType().type) {
    case HdTypeFloatVec3:
        if (_packed) {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormalsPacked(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3f*>(_points->GetData())))));
        } else {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormals(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3f*>(_points->GetData())))));
        }
        break;
    case HdTypeDoubleVec3:
        if (_packed) {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormalsPacked(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3d*>(_points->GetData())))));
        } else {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormals(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3d*>(_points->GetData())))));
        }
        break;
    default:
        TF_CODING_ERROR("Unsupported points type for computing smooth normals");
        break;
    }

    _SetResult(normals);

    // call base class to mark as resolved.
    _SetResolved();
    return true;
}

bool
Hd_SmoothNormalsComputation::_CheckValid() const
{
    bool valid = _points ? _points->IsValid() : false;

    // _adjacencyBuilder is an optional source
    valid &= _adjacencyBuilder ? _adjacencyBuilder->IsValid() : true;

    return valid;
}

PXR_NAMESPACE_CLOSE_SCOPE

