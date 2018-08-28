//
// Copyright 2018 Pixar
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
#include "pxr/imaging/hd/flatNormals.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE


template <typename SrcType, typename DstType>
class _FlatNormalsWorker
{
public:
    _FlatNormalsWorker(VtIntArray const &faceOffsets,
                       VtIntArray const &faceCounts,
                       VtIntArray const &faceIndices,
                       bool flip,
                       SrcType const *points,
                       DstType *normals)
        : _faceOffsets(faceOffsets), _faceCounts(faceCounts),
          _faceIndices(faceIndices), _flip(flip), _points(points),
          _normals(normals)
    {
    }

    void Compute(size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i) {
            SrcType normal(0);
            int offset = _faceOffsets[i];
            int count = _faceCounts[i];

            SrcType const& v0 = _points[_faceIndices[offset+0]];
            for (size_t j = 2; j < count; ++j) {
                SrcType const& v1 = _points[_faceIndices[offset+j-1]];
                SrcType const& v2 = _points[_faceIndices[offset+j]];
                normal += GfCross(v1-v0, v2-v0) * (_flip ? -1.0 : 1.0);
            }
            if (true) { // could defer normalization to shader code
                normal.Normalize();
            }
            _normals[i] = normal;
        }
    }

private:
    VtIntArray const &_faceOffsets;
    VtIntArray const &_faceCounts;
    VtIntArray const &_faceIndices;
    bool _flip;
    SrcType const *_points;
    DstType *_normals;

};

template <typename SrcType, typename DstType=SrcType>
VtArray<DstType>
_ComputeFlatNormals(HdMeshTopology const *topology, SrcType const* points)
{
    int numFaces = topology->GetFaceVertexCounts().size();
    VtArray<DstType> normals(numFaces);

    VtIntArray faceOffsets(numFaces);
    VtIntArray const &faceCounts = topology->GetFaceVertexCounts();
    int offset = 0;
    for (int i = 0; i < numFaces; ++i) {
        faceOffsets[i] = offset;
        offset += faceCounts[i];
    }
    VtIntArray const &faceIndices = topology->GetFaceVertexIndices();
    bool flip = (topology->GetOrientation() != HdTokens->rightHanded);

    _FlatNormalsWorker<SrcType,DstType> workerState(faceOffsets,
        faceCounts, faceIndices, flip, points, normals.data());

    WorkParallelForN(
        numFaces,
        std::bind(&_FlatNormalsWorker<SrcType,DstType>::Compute,
            std::ref(workerState), std::placeholders::_1, std::placeholders::_2));

    return normals;
}

/* static */
VtArray<GfVec3f>
Hd_FlatNormals::ComputeFlatNormals(
    HdMeshTopology const * topology,
    GfVec3f const * pointsPtr)
{
    return _ComputeFlatNormals(topology, pointsPtr);
}

/* static */
VtArray<GfVec3d>
Hd_FlatNormals::ComputeFlatNormals(
    HdMeshTopology const * topology,
    GfVec3d const * pointsPtr)
{
    return _ComputeFlatNormals(topology, pointsPtr);
}

/* static */
VtArray<HdVec4f_2_10_10_10_REV>
Hd_FlatNormals::ComputeFlatNormalsPacked(
    HdMeshTopology const * topology,
    GfVec3f const * pointsPtr)
{
    return _ComputeFlatNormals<GfVec3f, HdVec4f_2_10_10_10_REV>(
        topology, pointsPtr);
}

/* static */
VtArray<HdVec4f_2_10_10_10_REV>
Hd_FlatNormals::ComputeFlatNormalsPacked(
    HdMeshTopology const * topology,
    GfVec3d const * pointsPtr)
{
    return _ComputeFlatNormals<GfVec3d, HdVec4f_2_10_10_10_REV>(
        topology, pointsPtr);
}

Hd_FlatNormalsComputation::Hd_FlatNormalsComputation(
    HdMeshTopology const * topology,
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName,
    bool packed)
    : _topology(topology), _points(points), _dstName(dstName),
      _packed(packed)
{
}

void
Hd_FlatNormalsComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // The datatype of normals is the same as that of points, unless the
    // packed format was requested.
    specs->emplace_back(_dstName,
        _packed ? HdTupleType { HdTypeInt32_2_10_10_10_REV, 1 }
        : _points->GetTupleType() );
}

TfToken const &
Hd_FlatNormalsComputation::GetName() const
{
    return _dstName;
}

bool
Hd_FlatNormalsComputation::Resolve()
{
    if (!_points->IsResolved()) { return false; }
    if (!_TryLock()) { return false; }

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_topology)) return true;

    VtValue normals;

    switch (_points->GetTupleType().type) {
    case HdTypeFloatVec3:
        if (_packed) {
            normals = Hd_FlatNormals::ComputeFlatNormalsPacked(
                _topology, static_cast<const GfVec3f*>(_points->GetData()));
        } else {
            normals = Hd_FlatNormals::ComputeFlatNormals(
                _topology, static_cast<const GfVec3f*>(_points->GetData()));
        }
        break;
    case HdTypeDoubleVec3:
        if (_packed) {
            normals = Hd_FlatNormals::ComputeFlatNormalsPacked(
                _topology, static_cast<const GfVec3d*>(_points->GetData()));
        } else {
            normals = Hd_FlatNormals::ComputeFlatNormals(
                _topology, static_cast<const GfVec3d*>(_points->GetData()));
        }
        break;
    default:
        TF_CODING_ERROR("Unsupported points type for computing flat normals");
        break;
    }

    HdBufferSourceSharedPtr normalsBuffer = HdBufferSourceSharedPtr(
        new HdVtBufferSource(_dstName, VtValue(normals)));
    _SetResult(normalsBuffer);
    _SetResolved();
    return true;
}

bool
Hd_FlatNormalsComputation::_CheckValid() const
{
    bool valid = _points ? _points->IsValid() : false;
    return valid;
}

PXR_NAMESPACE_CLOSE_SCOPE
