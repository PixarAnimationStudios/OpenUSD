//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/meshUtil.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

//-------------------------------------------------------------------------
// Triangulation

// Fan triangulation helper function.
template <typename T>
static
bool _FanTriangulate(T *dst, T const *src,
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

static
bool _FanTriangulate(GfVec3i *dst, int const *src,
                     int offset, int index, int size, bool flip)
{
    return _FanTriangulate(dst->data(), src, offset, index, size, flip);
}

void
HdMeshUtil::ComputeTriangleIndices(VtVec3iArray *indices,
                                   VtIntArray *primitiveParams,
                                   VtIntArray *edgeIndices/*=nullptr*/) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for triangulation");
        return;
    }
    if (indices == nullptr ||
        primitiveParams == nullptr) {
        TF_CODING_ERROR("No output buffer provided for triangulation");
        return;
    }

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

    indices->resize(numTris); // vec3 per face
    primitiveParams->resize(numTris); // int per face
    if (edgeIndices) {
        edgeIndices->resize(numTris); // int per face
    }

    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    // reset holeIndex
    holeIndex = 0;

    // i  -> authored face index [0, numFaces)
    // tv -> triangulated face index [0, numTris)
    // v  -> index to the first vertex (index) for face i
    // ev -> edges visited
    for (int i=0,tv=0,v=0,ev=0; i<numFaces; ++i) {
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
            int edgeIndex = ev;
            for (int j=0; j < nv-2; ++j) {
                if (!_FanTriangulate(
                        &(*indices)[tv],
                        vertsPtr, v, j, numVertIndices, flip)) {
                    invalidTopology = true;
                }

                if (nv > 3) {
                    if (j == 0) {
                        if (flip) {
                            // If the topology is flipped, we get the triangle
                            // 021 instead of 012, and we'd hide edge 0-1
                            // instead of 0-2; so we rotate the indices to
                            // produce triangle 210.
                            GfVec3i &index = (*indices)[tv];
                            index.Set(index[1], index[2], index[0]);
                        }
                        edgeFlag = 1;
                    }
                    else if (j == nv-3) {
                        if (flip) {
                            // If the topology is flipped, we get the triangle
                            // 043 instead of 034, and we'd hide edge 0-4
                            // instead of 0-3; so we rotate the indices to
                            // produce triangle 304.
                            GfVec3i &index = (*indices)[tv];
                            index.Set(index[2], index[0], index[1]);
                        }
                        edgeFlag = 2;
                    }
                    else {
                        edgeFlag = 3;
                    }
                    ++edgeIndex;
                }

                (*primitiveParams)[tv] = EncodeCoarseFaceParam(i, edgeFlag);
                if (edgeIndices) {
                    (*edgeIndices)[tv] = edgeIndex;
                }

                ++tv;
            }
        }
        // When the face is degenerate and nv > 0, we need to increment the v
        // pointer to walk past the degenerate verts.
        v += nv;
        ev += nv;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]",
                _id.GetText());
    }
}

// Face-varying triangulation helper function, to deal with type polymorphism.
template <typename T>
static
void _TriangulateFaceVarying(
        SdfPath const& id,
        VtIntArray const &faceVertexCounts,
        VtIntArray const &holeFaces,
        bool flip,
        void const* sourceUntyped,
        int numElements,
        VtValue *triangulated)
{
    T const* source = static_cast<T const*>(sourceUntyped);

    // CPU face-varying triangulation
    bool invalidTopology = false;
    int numFVarValues = 0;
    int holeIndex = 0;
    int numHoleFaces = holeFaces.size();
    for (int i = 0; i < (int)faceVertexCounts.size(); ++i) {
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
    for (int i = 0, v = 0; i < (int)faceVertexCounts.size(); ++i) {
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
                        source, v, j, numElements, flip)) {
                    invalidTopology = true;
                }
                // To keep edge flags consistent, when a face is triangulated
                // and the topology is flipped we rotate the first and last
                // triangle indices. See ComputeTriangleIndices.
                if (nVerts > 3 && flip) {
                    if (j == 0) {
                        std::swap(results[dstIndex], results[dstIndex+1]);
                        std::swap(results[dstIndex+1], results[dstIndex+2]);
                    } else if (j == nVerts-3) {
                        std::swap(results[dstIndex+1], results[dstIndex+2]);
                        std::swap(results[dstIndex], results[dstIndex+1]);
                    }
                }
                dstIndex += 3;
            }
        }
        v += nVerts;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", id.GetText());
    }

    *triangulated = results;
}

bool
HdMeshUtil::ComputeTriangulatedFaceVaryingPrimvar(void const* source,
                                                  int numElements,
                                                  HdType dataType,
                                                  VtValue *triangulated) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for triangulation");
        return false;
    }
    if (triangulated == nullptr) {
        TF_CODING_ERROR("No output buffer provided for triangulation");
        return false;
    }

    VtIntArray const &faceVertexCounts = _topology->GetFaceVertexCounts();

    // Faces tagged as holes can be skipped over only when not refined.
    VtIntArray const &holeFaces =
        (_topology->GetRefineLevel() > 0)
            ? VtIntArray()
            : _topology->GetHoleIndices();

    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    switch (dataType) {
    case HdTypeFloat:
        _TriangulateFaceVarying<float>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeFloatVec2:
        _TriangulateFaceVarying<GfVec2f>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeFloatVec3:
        _TriangulateFaceVarying<GfVec3f>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeFloatVec4:
        _TriangulateFaceVarying<GfVec4f>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeDouble:
        _TriangulateFaceVarying<double>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeDoubleVec2:
        _TriangulateFaceVarying<GfVec2d>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeDoubleVec3:
        _TriangulateFaceVarying<GfVec3d>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    case HdTypeDoubleVec4:
        _TriangulateFaceVarying<GfVec4d>(_id, faceVertexCounts, holeFaces, flip,
                source, numElements, triangulated);
        break;
    default:
        TF_CODING_ERROR("Unsupported primvar type for triangulation [%s]",
                        _id.GetText());
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------
// Quadrangulation

int
HdMeshUtil::_ComputeNumQuads(VtIntArray const &numVerts,
                             VtIntArray const &holeFaces,
                             bool *invalidFaceFound) const
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

void
HdMeshUtil::ComputeQuadInfo(HdQuadInfo* quadInfo) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for quadrangulation");
        return;
    }
    if (quadInfo == nullptr) {
        TF_CODING_ERROR("No output buffer provided for quadrangulation");
        return;
    }

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = _topology->GetHoleIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    int numVertIndices = _topology->GetFaceVertexIndices().size();
    int numHoleFaces = _topology->GetHoleIndices().size();
    int numPoints = _topology->GetNumPoints();

    quadInfo->numVerts.clear();
    quadInfo->verts.clear();
    quadInfo->pointsOffset = numPoints;

    int vertIndex = 0;
    int numAdditionalPoints = 0;
    int maxNumVert = 0;
    int holeIndex = 0;
    bool invalidTopology = false;
    for (int i = 0; i < numFaces; ++i) {
        int nv = numVertsPtr[i];

        if (nv < 3) {
            vertIndex += nv;
            continue; // skip degenerated face
        }
        if (holeIndex < numHoleFaces &&
            holeFacesPtr[holeIndex] == i) {
            // skip hole faces.
            vertIndex += nv;
            ++holeIndex;
            continue;
        }

        if (nv == 4) {
            vertIndex += nv;
            continue;
        }

        // if it isn't a quad,
        quadInfo->numVerts.push_back(nv);
        for (int j = 0; j < nv; ++j) {
            // store vertex indices into quadinfo
            int index = 0;
            if (vertIndex >= numVertIndices) {
                invalidTopology = true;
            } else {
                index = vertsPtr[vertIndex++];
            }
            quadInfo->verts.push_back(index);
        }
        // nv + 1 (edge + center) additional vertices needed.
        numAdditionalPoints += (nv + 1);

        // remember max numvert for making gpu-friendly table
        maxNumVert = std::max(maxNumVert, nv);
    }
    quadInfo->numAdditionalPoints = numAdditionalPoints;
    quadInfo->maxNumVert = maxNumVert;

    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", _id.GetText());
    }
}

void
HdMeshUtil::_ComputeQuadIndices(VtIntArray *indices,
                                VtIntArray *primitiveParams,
                                VtVec2iArray *edgeIndices/*=nullptr*/,
                                bool triangulate/*=false*/) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for quadrangulation");
        return;
    }
    if (indices == nullptr ||
        primitiveParams == nullptr) {
        TF_CODING_ERROR("No output buffer provided for quadrangulation");
        return;
    }

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = _topology->GetHoleIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    int numVertIndices = _topology->GetFaceVertexIndices().size();
    int numHoleFaces = _topology->GetHoleIndices().size();
    int numPoints = _topology->GetNumPoints();

    // count num quads
    bool invalidTopology = false;
    int numQuads = _ComputeNumQuads(
        _topology->GetFaceVertexCounts(),
        _topology->GetHoleIndices(),
        &invalidTopology);
    if (invalidTopology) {
        TF_WARN("degenerated face found [%s]", _id.GetText());
        invalidTopology = false;
    }

    int holeIndex = 0;

    int const numIndicesPerQuad =
        triangulate
            ? HdMeshTriQuadBuilder::NumIndicesPerTriQuad
            : HdMeshTriQuadBuilder::NumIndicesPerQuad;
    indices->resize(numQuads * numIndicesPerQuad);

    HdMeshTriQuadBuilder outputIndices(indices->data(), triangulate);

    primitiveParams->resize(numQuads);
    if (edgeIndices) {
        edgeIndices->resize(numQuads);
    }

    // quadrangulated verts is added to the end.
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);
    int vertIndex = numPoints;

    // i  -> authored face index [0, numFaces)
    // qv -> quadrangulated face index [0, numQuads)
    // v  -> index to the first vertex (index) for face i
    // ev -> edges visited
    // vertIndex -> index to the start of the additional verts (edge, center)
    //              for face i
    for (int i = 0, qv = 0, v = 0, ev = 0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) {
            v += nv;
            ev += nv;
            continue; // skip degenerated face
        }
        if (holeIndex < numHoleFaces &&
            holeFacesPtr[holeIndex] == i) {
            // skip hole faces.
            ++holeIndex;
            v += nv;
            ev += nv;
            continue;
        }

        if (v+nv > numVertIndices) {
            invalidTopology = true;
            if (nv == 4) {
                outputIndices.EmitQuadFace(GfVec4i(0));
            } else {
                for (int j = 0; j < nv; ++j) {
                    outputIndices.EmitQuadFace(GfVec4i(0));
                }
            }
            v += nv;
            ev += nv;
            continue;
        }

        int edgeIndex = ev;
        if (nv == 4) {
            GfVec4i quadIndices;
            if (flip) {
                quadIndices[0] = (vertsPtr[v+0]);
                quadIndices[1] = (vertsPtr[v+3]);
                quadIndices[2] = (vertsPtr[v+2]);
                quadIndices[3] = (vertsPtr[v+1]);
            } else {
                quadIndices[0] = (vertsPtr[v+0]);
                quadIndices[1] = (vertsPtr[v+1]);
                quadIndices[2] = (vertsPtr[v+2]);
                quadIndices[3] = (vertsPtr[v+3]);
            }
            outputIndices.EmitQuadFace(quadIndices);

            //  Case             EdgeFlag    Draw
            //  Quad/Refined face   0        hide common edge for the tri-pair
            //  Non-Quad face       1/2/3    hide common edge for the tri-pair & 
            //                               hide interior quadrangulated edges
            //
            //  The first quad of a non-quad face is marked 1; the last as 2; and
            //  intermediate quads as 3.

            (*primitiveParams)[qv] = EncodeCoarseFaceParam(i, /*edgeFlag=*/0);

            if (edgeIndices) {
                (*edgeIndices)[qv][0] = edgeIndex;
                (*edgeIndices)[qv][1] = edgeIndex+3;
            }

            ++qv;
        } else {
            // quadrangulate non-quad faces
            // the additional points (edge and center) are stored at the end of
            // original points, as
            //   last point, e0, e1, ..., en, center, e0, e1, ...
            // so each sub-quads become
            // *first non-quad
            //   v0, e0, center, e(-1),
            //   v1, e1, center, e0,
            //...
            // *second non-quad
            //   ...
            for (int j = 0; j < nv; ++j) {
                GfVec4i quadIndices;
                // vertex
                quadIndices[0] = vertsPtr[v+j];
                if (flip) {
                    // edge prev
                    quadIndices[1] = vertIndex + (j+nv-1)%nv;
                    // center
                    quadIndices[2] = vertIndex + nv;
                    // edge next
                    quadIndices[3] = vertIndex + j;
                } else {
                    // edge next
                    quadIndices[1] = vertIndex + j;
                    // center
                    quadIndices[2] = vertIndex + nv;
                    // edge prev
                    quadIndices[3] = vertIndex + (j+nv-1)%nv;
                }
                outputIndices.EmitQuadFace(quadIndices);

                // edge flag != 0 => quad face is from quadrangulation
                // it is used to hide internal edges (edge-center) of the quad
                // The first quad gets flag = 1, intermediate quads get flag = 3
                // and the last quad gets flag = 2, so computations can tell
                // how quads are grouped by looking at edge flags.
                int edgeFlag = 0;
                if (j == 0) {
                    edgeFlag = 1;
                } else if (j == nv - 1) {
                    edgeFlag = 2;
                } else {
                    edgeFlag = 3;
                }
                (*primitiveParams)[qv] = EncodeCoarseFaceParam(i, edgeFlag);

                if (edgeIndices) {
                    if (flip) {
                        (*edgeIndices)[qv][0] = edgeIndex+(j+nv-1)%nv;
                        (*edgeIndices)[qv][1] = edgeIndex+j;
                    } else {
                        (*edgeIndices)[qv][0] = edgeIndex+j;
                        (*edgeIndices)[qv][1] = edgeIndex+(j+nv-1)%nv;
                    }
                }

                ++qv;
            }
            vertIndex += nv + 1;
        }
        v += nv;
        ev += nv;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", _id.GetText());
    }
}

void
HdMeshUtil::ComputeQuadIndices(VtIntArray *indices,
                               VtIntArray *primitiveParams,
                               VtVec2iArray *edgeIndices/*=nullptr*/) const
{
    _ComputeQuadIndices(
        indices, primitiveParams, edgeIndices);
}

void
HdMeshUtil::ComputeTriQuadIndices(VtIntArray *indices,
                                  VtIntArray *primitiveParams,
                                  VtVec2iArray *edgeIndices/*=nullptr*/) const
{
    _ComputeQuadIndices(
        indices, primitiveParams, edgeIndices, /*triangulate=*/true);
}

template <typename T>
static void
_Quadrangulate(SdfPath const& id,
               void const* sourceUntyped,
               int numElements,
               HdQuadInfo const *qi,
               VtValue *quadrangulated)
{
    // CPU quadrangulation

    // original points + quadrangulated points
    VtArray<T> results(qi->pointsOffset + qi->numAdditionalPoints);

    // copy original primvars
    T const *source = reinterpret_cast<T const*>(sourceUntyped);
    if (numElements >= qi->pointsOffset) {
        memcpy(results.data(), source, sizeof(T)*qi->pointsOffset);
    } else {
        TF_WARN("source.numElements and pointsOffset are inconsistent [%s]",
                id.GetText());
        memcpy(results.data(), source, sizeof(T)*numElements);
        for (int i = numElements; i < qi->pointsOffset; ++i) {
            results[i] = T(0);
        }
    }

    // compute quadrangulate primvars
    int index = 0;
    // store quadrangulated points at end
    int dstIndex = qi->pointsOffset;

    for (const int nv : qi->numVerts) {
        T center(0);
        for (int i = 0; i < nv; ++i) {
            const int i0 = qi->verts[index+i];
            const int i1 = qi->verts[index+(i+1)%nv];

            // midpoint
            T edge = (results[i0] + results[i1]) * 0.5;
            results[dstIndex++] = edge;

            // accumulate center
            center += results[i0];
        }
        // average center value
        center /= nv;
        results[dstIndex++] = center;

        index += nv;
    }

    *quadrangulated = results;
}

bool
HdMeshUtil::ComputeQuadrangulatedPrimvar(HdQuadInfo const* qi,
                                         void const* source,
                                         int numElements,
                                         HdType dataType,
                                         VtValue *quadrangulated) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for quadrangulation");
        return false;
    }
    if (quadrangulated == nullptr) {
        TF_CODING_ERROR("No output buffer provided for quadrangulation");
        return false;
    }

    switch (dataType) {
    case HdTypeFloat:
        _Quadrangulate<float>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeFloatVec2:
        _Quadrangulate<GfVec2f>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeFloatVec3:
        _Quadrangulate<GfVec3f>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeFloatVec4:
        _Quadrangulate<GfVec4f>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeDouble:
        _Quadrangulate<double>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeDoubleVec2:
        _Quadrangulate<GfVec2d>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeDoubleVec3:
        _Quadrangulate<GfVec3d>(_id, source, numElements, qi, quadrangulated);
        break;
    case HdTypeDoubleVec4:
        _Quadrangulate<GfVec4d>(_id, source, numElements, qi, quadrangulated);
        break;
    default:
        TF_CODING_ERROR("Unsupported points type for quadrangulation [%s]",
                        _id.GetText());
        return false;
    }

    return true;
}

template <typename T>
static void
_QuadrangulateFaceVarying(SdfPath const& id,
                          VtIntArray const &faceVertexCounts,
                          VtIntArray const &holeFaces,
                          bool flip,
                          void const* sourceUntyped,
                          int numElements,
                          VtValue *quadrangulated)
{
    T const* source = static_cast<T const*>(sourceUntyped);

    // CPU face-varying quadrangulation
    bool invalidTopology = false;
    int numFVarValues = 0;
    int holeIndex = 0;
    int numHoleFaces = holeFaces.size();
    for (int i = 0; i < (int)faceVertexCounts.size(); ++i) {
        int nVerts = faceVertexCounts[i];
        if (nVerts < 3) {
            // skip degenerated face
            invalidTopology = true;
        } else if (holeIndex < numHoleFaces && holeFaces[holeIndex] == i) {
            // skip hole face
            ++holeIndex;
        } else if (nVerts == 4) {
            numFVarValues += 4;
        } else {
            numFVarValues += 4 * nVerts;
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
    for (int i = 0, v = 0; i < (int)faceVertexCounts.size(); ++i) {
        int nVerts = faceVertexCounts[i];
        if (nVerts < 3) {
            // skip degenerated faces.
        } else if (holeIndex < numHoleFaces && holeFaces[holeIndex] == i) {
            // skip hole faces.
            ++holeIndex;
        } else if (nVerts == 4) {
            // copy
            if (v+nVerts > numElements) {
                invalidTopology = true;
                results[dstIndex++] = T(0);
                results[dstIndex++] = T(0);
                results[dstIndex++] = T(0);
                results[dstIndex++] = T(0);
            } else {
                results[dstIndex++] = source[v];
                if (flip) {
                    results[dstIndex++] = source[v+3]; 
                    results[dstIndex++] = source[v+2]; 
                    results[dstIndex++] = source[v+1]; 
                } else {
                    results[dstIndex++] = source[v+1]; 
                    results[dstIndex++] = source[v+2]; 
                    results[dstIndex++] = source[v+3]; 
                }
            }
        } else {
            // quadrangulate
            // compute the center first

            // early out if overrunning
            if (v+nVerts > numElements) {
                invalidTopology = true;
                for (int j = 0; j < nVerts; ++j) {
                    results[dstIndex++] = T(0);
                    results[dstIndex++] = T(0);
                    results[dstIndex++] = T(0);
                    results[dstIndex++] = T(0);
                }
                continue;
            } 

            T center(0);
            for (int j = 0; j < nVerts; ++j) {
                center += source[v+j];
            }
            center /= nVerts;

            // mid edges
            T e0 = (source[v] + source[v+1]) * 0.5;
            T e1 = (source[v] + source[v+(nVerts-1)%nVerts]) * 0.5;

            results[dstIndex++] = source[v];
            if (flip) {
                results[dstIndex++] = e1; 
                results[dstIndex++] = center; 
                results[dstIndex++] = e0; 

                for (int j = nVerts - 1; j > 0; --j) {
                    e0 = (source[v+j] + source[v+(j+1)%nVerts]) * 0.5;
                    e1 = (source[v+j] + source[v+(j+nVerts-1)%nVerts]) * 0.5;

                    results[dstIndex++] = source[v+j];
                    results[dstIndex++] = e1; 
                    results[dstIndex++] = center; 
                    results[dstIndex++] = e0; 
                }
            } else {
                results[dstIndex++] = e0; 
                results[dstIndex++] = center;
                results[dstIndex++] = e1; 

                for (int j = 1; j < nVerts; ++j) {
                    e0 = (source[v+j] + source[v+(j+1)%nVerts]) * 0.5;
                    e1 = (source[v+j] + source[v+(j+nVerts-1)%nVerts]) * 0.5;

                    results[dstIndex++] = source[v+j];
                    results[dstIndex++] = e0; 
                    results[dstIndex++] = center;
                    results[dstIndex++] = e1; 
                }
            }
        }
        v += nVerts;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", id.GetText());
    }

    *quadrangulated = results;
}

bool
HdMeshUtil::ComputeQuadrangulatedFaceVaryingPrimvar(
        void const* source,
        int numElements,
        HdType dataType,
        VtValue *quadrangulated) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for quadrangulation");
        return false;
    }
    if (quadrangulated == nullptr) {
        TF_CODING_ERROR("No output buffer provided for quadrangulation");
        return false;
    }

    VtIntArray const &faceVertexCounts = _topology->GetFaceVertexCounts();

    // Faces tagged as holes can be skipped over only when not refined.
    VtIntArray const &holeFaces =
        (_topology->GetRefineLevel() > 0)
            ? VtIntArray()
            : _topology->GetHoleIndices();

    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    switch (dataType) {
    case HdTypeFloat:
        _QuadrangulateFaceVarying<float>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeFloatVec2:
        _QuadrangulateFaceVarying<GfVec2f>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeFloatVec3:
        _QuadrangulateFaceVarying<GfVec3f>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeFloatVec4:
        _QuadrangulateFaceVarying<GfVec4f>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeDouble:
        _QuadrangulateFaceVarying<double>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeDoubleVec2:
        _QuadrangulateFaceVarying<GfVec2d>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeDoubleVec3:
        _QuadrangulateFaceVarying<GfVec3d>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    case HdTypeDoubleVec4:
        _QuadrangulateFaceVarying<GfVec4d>(
            _id, faceVertexCounts, holeFaces, flip,
            source, numElements, quadrangulated);
        break;
    default:
        TF_CODING_ERROR("Unsupported primvar type for quadrangulation [%s]",
                        _id.GetText());
        return false;
    }

    return true;
}

void
HdMeshUtil::EnumerateEdges(
    std::vector<GfVec2i> * edgeVerticesOut,
    std::vector<int> * firstEdgeIndexForFacesOut) const
{
    HD_TRACE_FUNCTION();

    if (_topology == nullptr) {
        TF_CODING_ERROR("No topology provided for edge vertices");
        return;
    }
    if (edgeVerticesOut == nullptr) {
        TF_CODING_ERROR("No output buffer provided for edge vertices");
        return;
    }

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const numFaces = _topology->GetFaceVertexCounts().size();

    if (firstEdgeIndexForFacesOut) {
        firstEdgeIndexForFacesOut->resize(numFaces);
    }

    int numEdges = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        numEdges += nv;
    }
    edgeVerticesOut->resize(numEdges);

    bool const flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    for (int i=0, v=0, ev=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (firstEdgeIndexForFacesOut) {
            (*firstEdgeIndexForFacesOut)[i] = ev;
        }
        if (flip) {
            for (int j=nv; j>0; --j) {
                int v0 = vertsPtr[v+j%nv];
                int v1 = vertsPtr[v+(j+nv-1)%nv];
                if (v0 < v1) {
                    std::swap(v0, v1);
                }
                (*edgeVerticesOut)[ev++] = GfVec2i(v0, v1);
            }
        } else {
            for (int j=0; j<nv; ++j) {
                int v0 = vertsPtr[v+j];
                int v1 = vertsPtr[v+(j+1)%nv];
                if (v0 < v1) {
                    std::swap(v0, v1);
                }
                (*edgeVerticesOut)[ev++] = GfVec2i(v0, v1);
            }
        }
        v += nv;
    }
}

HdMeshEdgeIndexTable::HdMeshEdgeIndexTable(HdMeshTopology const * topology)
    : _topology(topology)
{
    HdMeshUtil meshUtil(_topology, SdfPath());

    meshUtil.EnumerateEdges(&_edgeVertices, &_firstEdgeIndexForFaces);

    _edgesByIndex.resize(_edgeVertices.size());
    for (size_t i=0; i<_edgeVertices.size(); ++i) {
        _edgesByIndex[i] = _Edge(_edgeVertices[i], i);
    }

    std::sort(_edgesByIndex.begin(),
              _edgesByIndex.end(),
              _CompareEdgeVertices());
}

HdMeshEdgeIndexTable::~HdMeshEdgeIndexTable() = default;

bool
HdMeshEdgeIndexTable::GetVerticesForEdgeIndex(int edgeIndex,
                                              GfVec2i * edgeVerticesOut) const
{
    if (edgeIndex < 0 || edgeIndex >= (int)_edgeVertices.size()) {
        return false;
    }

    *edgeVerticesOut = _edgeVertices[edgeIndex];

    return true;
}

bool
HdMeshEdgeIndexTable::GetVerticesForEdgeIndices(
        std::vector<int> const & edgeIndices,
        std::vector<GfVec2i> * edgeVerticesOut) const
{
    std::unordered_set<GfVec2i, _EdgeVerticesHash> result;
    for (int edgeIndex : edgeIndices) {
        GfVec2i edgeVertices;
        if (GetVerticesForEdgeIndex(edgeIndex, &edgeVertices)) {
            result.insert(edgeVertices);
        }
    }
    edgeVerticesOut->assign(result.begin(), result.end());

    return !edgeVerticesOut->empty();
}

bool
HdMeshEdgeIndexTable::GetEdgeIndices(GfVec2i const & edgeVertices,
                                     std::vector<int> * edgeIndicesOut) const
{
    const _Edge edge(edgeVertices);
    auto matchingEdges = std::equal_range(_edgesByIndex.begin(),
                                          _edgesByIndex.end(),
                                          edge, _CompareEdgeVertices());

    if (matchingEdges.first == matchingEdges.second) {
        return false;
    }

    const size_t numIndices = std::distance(matchingEdges.first,
                                            matchingEdges.second);
    edgeIndicesOut->resize(numIndices);

    int e = 0;
    for (auto i = matchingEdges.first; i!=matchingEdges.second; ++i) {
        (*edgeIndicesOut)[e++] = i->index;
    }

    return !edgeIndicesOut->empty();
}

VtIntArray
HdMeshEdgeIndexTable::CollectFaceEdgeIndices(
    VtIntArray const &faceIndices) const
{
    std::vector<int> result;

    for (int const face : faceIndices) {

        int const firstEdgeIndex = _firstEdgeIndexForFaces[face];
        int const numEdges = _topology->GetFaceVertexCounts()[face];

        for (int e=0; e<numEdges; ++e) {

            // Edges are identified by their vertex indices.
            GfVec2i const &edgeVertices = _edgeVertices[firstEdgeIndex+e];

            std::vector<int> edgeIndices;
            GetEdgeIndices(edgeVertices, &edgeIndices);

            result.insert(result.end(),
                          edgeIndices.begin(), edgeIndices.end());
        }
    }

    return VtIntArray(result.begin(), result.end());
}


PXR_NAMESPACE_CLOSE_SCOPE
