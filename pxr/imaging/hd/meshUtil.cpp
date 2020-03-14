//
// Copyright 2017 Pixar
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
                                   VtVec3iArray *edgeIndices/*=nullptr*/)
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
        edgeIndices->resize(numTris); // vec3 per face
    }

    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);

    // reset holeIndex
    holeIndex = 0;

    EdgeMap edges;
    if (edgeIndices) {
        edges = ComputeAuthoredEdgeMap(_topology);
    }

    // i  -> authored face index [0, numFaces)
    // tv -> triangulated face index [0, numTris)
    // v  -> index to the first vertex (index) for face i
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
                }


                if (edgeIndices) {
                    GfVec3i const& triIndices = (*indices)[tv];
                    for (int e=0; e<3; e++) {
                        int ev0 = e;
                        int ev1 = (e==2)? 0 : e+1;
                        GfVec2i edge(triIndices[ev0], triIndices[ev1]);

                        // look up the triangle edge in the authored edge map
                        int eId = -1; // non-authored edge
                        auto it = edges.find(edge);
                        if (it != edges.end()) {
                            eId = it->second;
                        }
                        (*edgeIndices)[tv][e] = eId;
                    }
                }
                
                // note that ptex indexing isn't available along with
                // triangulation.
                (*primitiveParams)[tv] = EncodeCoarseFaceParam(i, edgeFlag);
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
                                                  VtValue *triangulated)
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

/*static*/ int
HdMeshUtil::ComputeNumQuads(VtIntArray const &numVerts,
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

void
HdMeshUtil::ComputeQuadInfo(HdQuadInfo* quadInfo)
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
HdMeshUtil::ComputeQuadIndices(VtVec4iArray *indices,
                               VtVec2iArray *primitiveParams,
                               VtVec4iArray *quadsEdgeIndices/*=nullptr*/)
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

    // TODO: create ptex id remapping buffer here.

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = _topology->GetHoleIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    int numVertIndices = _topology->GetFaceVertexIndices().size();
    int numHoleFaces = _topology->GetHoleIndices().size();
    int numPoints = _topology->GetNumPoints();

    // count num quads
    bool invalidTopology = false;
    int numQuads = HdMeshUtil::ComputeNumQuads(
        _topology->GetFaceVertexCounts(),
        _topology->GetHoleIndices(),
        &invalidTopology);
    if (invalidTopology) {
        TF_WARN("degenerated face found [%s]", _id.GetText());
        invalidTopology = false;
    }

    int holeIndex = 0;

    indices->resize(numQuads);
    primitiveParams->resize(numQuads);
    if (quadsEdgeIndices) {
        quadsEdgeIndices->resize(numQuads);
    }

    // quadrangulated verts is added to the end.
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);
    int vertIndex = numPoints;

    EdgeMap edges;
    if (quadsEdgeIndices) {
        edges = ComputeAuthoredEdgeMap(_topology);
    }

    // TODO: We need to support ptex index in addition to coarse indices.
    //int ptexIndex = 0;
    
    // i  -> authored face index [0, numFaces)
    // qv -> quadrangulated face index [0, numQuads)
    // v  -> index to the first vertex (index) for face i
    // vertIndex -> index to the start of the additional verts (edge, center)
    //              for face i
    for (int i = 0, qv = 0, v = 0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) {
            v += nv;
            continue; // skip degenerated face
        }
        if (holeIndex < numHoleFaces &&
            holeFacesPtr[holeIndex] == i) {
            // skip hole faces.
            ++holeIndex;
            v += nv;
            continue;
        }

        if (v+nv > numVertIndices) {
            invalidTopology = true;
            if (nv == 4) {
                (*indices)[qv++] = GfVec4i(0);
            } else {
                for (int j = 0; j < nv; ++j) {
                    (*indices)[qv++] = GfVec4i(0);
                }
            }
            v += nv;
            continue;
        }

        if (nv == 4) {
            if (flip) {
                (*indices)[qv][0] = (vertsPtr[v+0]);
                (*indices)[qv][1] = (vertsPtr[v+3]);
                (*indices)[qv][2] = (vertsPtr[v+2]);
                (*indices)[qv][3] = (vertsPtr[v+1]);
            } else {
                (*indices)[qv][0] = (vertsPtr[v+0]);
                (*indices)[qv][1] = (vertsPtr[v+1]);
                (*indices)[qv][2] = (vertsPtr[v+2]);
                (*indices)[qv][3] = (vertsPtr[v+3]);
            }

            //  Case             EdgeFlag    Draw
            //  Quad/Refined face   0        hide common edge for the tri-pair
            //  Non-Quad face       1/2/3    hide common edge for the tri-pair & 
            //                               hide interior quadrangulated edges
            //
            //  The first quad of a non-quad face is marked 1; the last as 2; and
            //  intermediate quads as 3.

            (*primitiveParams)[qv] = GfVec2i(
                EncodeCoarseFaceParam(i, /*edgeFlag=*/0), qv);

            if (quadsEdgeIndices) {
                GfVec4i const& quadIndices = (*indices)[qv];
                for(int e = 0; e < nv; ++e) {
                    GfVec2i edge(quadIndices[e], quadIndices[(e+1)%nv]);
                    // since this is an authored quad, all edges are guaranteed
                    // to exist in the authored edge map
                    auto it = edges.find(edge);
                    TF_VERIFY(it != edges.end());
                    (*quadsEdgeIndices)[qv][e] = it->second; // authored edge id
                }
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
                // vertex
                (*indices)[qv][0] = vertsPtr[v+j];
                if (flip) {
                    // edge prev
                    (*indices)[qv][1] = vertIndex + (j+nv-1)%nv;
                    // center
                    (*indices)[qv][2] = vertIndex + nv;
                    // edge next
                    (*indices)[qv][3] = vertIndex + j;
                } else {
                    // edge next
                    (*indices)[qv][1] = vertIndex + j;
                    // center
                    (*indices)[qv][2] = vertIndex + nv;
                    // edge prev
                    (*indices)[qv][3] = vertIndex + (j+nv-1)%nv;
                }
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
                (*primitiveParams)[qv] = GfVec2i(
                    EncodeCoarseFaceParam(i, edgeFlag), qv);

                if (quadsEdgeIndices) {
                    // only the first (index 0) and last (index 3) edges of the
                    // quad are from the authored edges; the other 2 are the
                    // result of quadrangulation.
                    GfVec2i e0 = GfVec2i(vertsPtr[v+j], vertsPtr[v+(j+1)%nv]);
                    GfVec2i e3 = GfVec2i(vertsPtr[v+(j+nv-1)%nv], vertsPtr[v+j]);
                    auto it = edges.find(e0);
                    TF_VERIFY(it != edges.end());
                    (*quadsEdgeIndices)[qv][0] = it->second;
                    (*quadsEdgeIndices)[qv][1] = -1;
                    (*quadsEdgeIndices)[qv][2] = -1;
                    it = edges.find(e3);
                    TF_VERIFY(it != edges.end());
                    (*quadsEdgeIndices)[qv][3] = it->second;
                }
                
                ++qv;
            }
            vertIndex += nv + 1;
        }
        v += nv;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", _id.GetText());
    }
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

    TF_FOR_ALL (numVertsIt, qi->numVerts) {
        int nv = *numVertsIt;
        T center(0);
        for (int i = 0; i < nv; ++i) {
            int i0 = qi->verts[index+i];
            int i1 = qi->verts[index+(i+1)%nv];

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
                                         VtValue *quadrangulated)
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
            for (int j = 0; j < 4; ++j) {
                if (v+j >= numElements) {
                    invalidTopology = true;
                    results[dstIndex++] = T(0);
                } else {
                    results[dstIndex++] = source[v+j];
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

            // for each quadrant
            for (int j = 0; j < nVerts; ++j) {
                results[dstIndex++] = source[v+j];
                // mid edge
                results[dstIndex++]
                    = (source[v+j] + source[v+(j+1)%nVerts]) * 0.5;
                // center
                results[dstIndex++] = center;
                // mid edge
                results[dstIndex++]
                    = (source[v+j] + source[v+(j+nVerts-1)%nVerts]) * 0.5;
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
        VtValue *quadrangulated)
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

//-------------------------------------------------------------------------
// Authored edge id enumeration
/*static*/HdMeshUtil::EdgeMap
HdMeshUtil::ComputeAuthoredEdgeMap(HdMeshTopology const* topology,
                                   bool skipHoles/*=false*/)
{
    HD_TRACE_FUNCTION();
    int const * numVertsPtr  = topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr     = topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = topology->GetHoleIndices().cdata();
    int numFaces             = topology->GetFaceVertexCounts().size();
    int numHoleFaces         = topology->GetHoleIndices().size();

    EdgeMap edges;
    int edgeId = 0;
    int holeIndex = 0;
    // i  -> authored face index [0, numFaces)
    // v  -> index to the first vertex (index) for face i
    for (int i=0, v=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) {
            // Skip degenerate faces.
        } else if (skipHoles &&
                   holeIndex < numHoleFaces && holeFacesPtr[holeIndex] == i) {
             // Skip hole faces.
             ++holeIndex;
        } else {
            // insert any new edges of the authored face into the edge map
            for (int e = 0; e < nv; ++e) {
                int ev0 = vertsPtr[v + e];
                int ev1 = (e == nv - 1)? vertsPtr[v] : vertsPtr[v + e + 1];
                GfVec2i edge(ev0, ev1);
                auto result = edges.insert(std::make_pair(edge, edgeId));

                if (result.second) { // edge 'edgeId' inserted
                    edgeId++; // next enumerated edge
                }
            }
        }

        v += nv;
    }

    return edges;
}

/*static*/HdMeshUtil::ReverseEdgeMap 
HdMeshUtil::ComputeReverseEdgeMap(const EdgeMap &edgeMap)
{
    HD_TRACE_FUNCTION();
    ReverseEdgeMap res;
    for (const auto& pair : edgeMap) {
        res.insert(std::make_pair(pair.second, pair.first));
    }
    return res;
}

/*static*/ std::pair<bool, GfVec2i>
HdMeshUtil::GetVertexIndicesForEdge(
    const ReverseEdgeMap &rEdgeMap, 
    int authoredEdgeId)
{
    const auto &it = rEdgeMap.find(authoredEdgeId);

    if (it != rEdgeMap.end()) {
        return std::make_pair(true, it->second);
    }

    return std::make_pair(false, GfVec2i(0,0));
}

/*static*/std::pair<bool, int>
HdMeshUtil::GetAuthoredEdgeID(HdMeshTopology const* topology,
                             GfVec2i edge)
{
    // XXX: we should cache this in HdMeshTopology
    EdgeMap authoredEdgeMap = ComputeAuthoredEdgeMap(topology);

    auto it = authoredEdgeMap.find(edge);
    if (it != authoredEdgeMap.end()) {
        return  std::make_pair(true, it->second);
    }
    return std::make_pair(false, -1);
}

PXR_NAMESPACE_CLOSE_SCOPE
