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
#include "pxr/imaging/hd/tokens.h"

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
            for (int j = 2; j < count; ++j) {
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


PXR_NAMESPACE_CLOSE_SCOPE

