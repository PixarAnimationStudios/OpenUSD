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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/triangulate.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"


#include "pxr/base/gf/vec3i.h"

PXR_NAMESPACE_OPEN_SCOPE


template <typename T, typename U>
bool _FanTriangulate(T *dst, U const *src,
                     int offset, int index, int size, bool flip)
{
    // overrun check
    if (offset + index + 2 >= size) {
        *dst++ = T(0);
        *dst++ = T(0);
        *dst   = T(0);
        return false;
    }

    if (flip) {
        *dst++ = src[offset];
        *dst++ = src[offset + index + 2];
        *dst   = src[offset + index + 1];
    } else {
        *dst++ = src[offset];
        *dst++ = src[offset + index + 1];
        *dst   = src[offset + index + 2];
    }
    return true;
}

template <typename T, typename U>
bool _FanTriangulate(T &dst, U const *src,
                     int offset, int index, int size, bool flip)
{
    // overrun check
    if (offset + index + 2 >= size) {
        dst = T(0);
        return false;
    }

    if (flip) {
        dst[0] = src[offset];
        dst[1] = src[offset + index + 2];
        dst[2] = src[offset + index + 1];
    } else {
        dst[0] = src[offset];
        dst[1] = src[offset + index + 1];
        dst[2] = src[offset + index + 2];
    }
    return true;
}

HdSt_TriangleIndexBuilderComputation::HdSt_TriangleIndexBuilderComputation(
    HdSt_MeshTopology *topology, SdfPath const &id)
    : _id(id), _topology(topology)
{
}

void
HdSt_TriangleIndexBuilderComputation::AddBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->push_back(HdBufferSpec(HdTokens->indices, GL_INT, 3));
    // triangles don't support ptex indexing (at least for now).
    specs->push_back(HdBufferSpec(HdTokens->primitiveParam, GL_INT, 1));
}

bool
HdSt_TriangleIndexBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    // generate triangle index buffer

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = _topology->GetHoleIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    int numVertIndices = _topology->GetFaceVertexIndices().size();
    int numTris = 0;
    int numHoleFaces = _topology->GetHoleIndices().size();
    bool invalidTopology = false;
    int holeIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i]-2;
        if (nv < 1) {
            // skip degenerated face
            invalidTopology = true;
        } else if (holeIndex < numHoleFaces && holeFacesPtr[holeIndex] == i) {
            // skip hole face
            ++holeIndex;
        } else {
            numTris += nv;
        }
    }
    if (invalidTopology) {
        TF_WARN("degenerated face found [%s]", _id.GetText());
        invalidTopology = false;
    }

    VtVec3iArray trianglesFaceVertexIndices(numTris);
    VtIntArray primitiveParam(numTris);
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    // reset holeIndex
    holeIndex = 0;

    for (int i=0,tv=0,v=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) {
            // Skip degenerate faces.
        } else if (holeIndex < numHoleFaces && holeFacesPtr[holeIndex] == i) {
            // Skip hole faces.
            ++holeIndex;
        } else {
            // edgeFlag is used for inner-line removal of non-triangle
            // faces on wireframe shading.
            //
            //          0__                0  0   0__
            //        _/|\ \_            _/.  ..   . \_
            //      _/  | \  \_   ->   _/  .  . .   .  \_
            //     /  A |C \ B \_     /  A .  .C .   . B \_
            //    1-----2---3----4   1-----2  1---2   1----2
            //
            //  Type   EdgeFlag    Draw
            //    -       0        show all edges
            //    A       1        hide [2-0]
            //    B       2        hide [0-1]
            //    C       3        hide [0-1] and [2-0]
            //
            int edgeFlag = 0;
            for (int j=0; j < nv-2; ++j) {
                if (nv > 3) {
                    if (j == 0) edgeFlag = flip ? 2 : 1;
                    else if (j == nv-3) edgeFlag = flip ? 1 : 2;
                    else edgeFlag = 3;
                }

                if (!_FanTriangulate(
                        trianglesFaceVertexIndices[tv],
                        vertsPtr, v, j, numVertIndices, flip)) {
                    invalidTopology = true;
                }

                // note that ptex indexing isn't available along with
                // triangulation.
                int coarseFaceParam =
                    HdSt_MeshTopology::EncodeCoarseFaceParam(i, edgeFlag);
                primitiveParam[tv] = coarseFaceParam;
                ++tv;
            }
        }
        // When the face is degenerate and nv > 0, we need to increment the v
        // pointer to walk past the degenerate verts.
        v += nv;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]",
                _id.GetText());
    }

    _SetResult(HdBufferSourceSharedPtr(
                   new HdVtBufferSource(
                       HdTokens->indices,
                       VtValue(trianglesFaceVertexIndices))));

    _primitiveParam.reset(new HdVtBufferSource(
                              HdTokens->primitiveParam,
                              VtValue(primitiveParam)));

    _SetResolved();
    return true;
}

bool
HdSt_TriangleIndexBuilderComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtr
HdSt_TriangleIndexBuilderComputation::GetChainedBuffer() const
{
    return _primitiveParam;
}

bool
HdSt_TriangleIndexBuilderComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

template <typename T>
HdBufferSourceSharedPtr
_TriangulateFaceVarying(HdBufferSourceSharedPtr const &source,
                        VtIntArray const &faceVertexCounts,
                        VtIntArray const &holeFaces,
                        bool flip,
                        SdfPath const &id)
{
    T const *srcPtr = reinterpret_cast<T const *>(source->GetData());
    int numElements = source->GetNumElements();

    // CPU face-varying triangulation
    bool invalidTopology = false;
    int numFVarValues = 0;
    int holeIndex = 0;
    int numHoleFaces = holeFaces.size();
    for (int i = 0; i < faceVertexCounts.size(); ++i) {
        int nv = faceVertexCounts[i] - 2;
        if (nv < 1) {
            // skip degenerated face
            invalidTopology = true;
        } else if (holeIndex < numHoleFaces && holeFaces[holeIndex] == i) {
            // skip hole face
            ++holeIndex;
        } else {
            numFVarValues += 3 * nv;
        }
    }
    if (invalidTopology) {
        TF_WARN("degenerated face found [%s]", id.GetText());
        invalidTopology = false;
    }

    VtArray<T> results(numFVarValues);
    // reset holeIndex
    holeIndex = 0;

    int dstIndex = 0;
    for (int i = 0, v = 0; i < faceVertexCounts.size(); ++i) {
        int nVerts = faceVertexCounts[i];

        if (nVerts < 3) {
            // Skip degenerate faces.
        } else if (holeIndex < numHoleFaces && holeFaces[holeIndex] == i) {
            // Skip hole faces.
            ++holeIndex;
        } else {
            // triangulate.
            // apply same triangulation as index does
            for (int j=0; j < nVerts-2; ++j) {
                if (!_FanTriangulate(&results[dstIndex],
                                        srcPtr, v, j, numElements, flip)) {
                    invalidTopology = true;
                }
                dstIndex += 3;
            }
        }
        v += nVerts;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", id.GetText());
    }

    return HdBufferSourceSharedPtr(new HdVtBufferSource(
                                       source->GetName(), VtValue(results)));
}

HdSt_TriangulateFaceVaryingComputation::HdSt_TriangulateFaceVaryingComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    SdfPath const &id)
    : _id(id), _topology(topology), _source(source)
{
}

bool
HdSt_TriangulateFaceVaryingComputation::Resolve()
{
    if (!TF_VERIFY(_source)) return false;
    if (!_source->IsResolved()) return false;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HD_PERF_COUNTER_INCR(HdPerfTokens->triangulateFaceVarying);

    VtIntArray const &faceVertexCounts = _topology->GetFaceVertexCounts();
    VtIntArray const &holeFaces = _topology->GetHoleIndices();
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);
    HdBufferSourceSharedPtr result;

    switch (_source->GetGLElementDataType()) {
    case GL_FLOAT:
        result = _TriangulateFaceVarying<float>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_FLOAT_VEC2:
        result = _TriangulateFaceVarying<GfVec2f>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_FLOAT_VEC3:
        result = _TriangulateFaceVarying<GfVec3f>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_FLOAT_VEC4:
        result = _TriangulateFaceVarying<GfVec4f>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE:
        result = _TriangulateFaceVarying<double>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE_VEC2:
        result = _TriangulateFaceVarying<GfVec2d>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE_VEC3:
        result = _TriangulateFaceVarying<GfVec3d>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE_VEC4:
        result = _TriangulateFaceVarying<GfVec4d>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    default:
        TF_CODING_ERROR("Unsupported primvar type for triangulation [%s]",
                        _id.GetText());
        result = _source;
        break;
    }

    _SetResult(result);
    _SetResolved();
    return true;
}

void
HdSt_TriangulateFaceVaryingComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same spec buffer as source
    _source->AddBufferSpecs(specs);
}

bool
HdSt_TriangulateFaceVaryingComputation::_CheckValid() const
{
    return (_source->IsValid());
}

PXR_NAMESPACE_CLOSE_SCOPE

