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

#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/gf/vec4i.h"

HdSt_QuadInfoBuilderComputation::HdSt_QuadInfoBuilderComputation(
    HdSt_MeshTopology *topology, SdfPath const &id)
    : _id(id), _topology(topology)
{
}

bool
HdSt_QuadInfoBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = _topology->GetHoleIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    int numVertIndices = _topology->GetFaceVertexIndices().size();
    int numHoleFaces = _topology->GetHoleIndices().size();
    // compute numPoints from topology indices
    int numPoints = HdSt_MeshTopology::ComputeNumPoints(
        _topology->GetFaceVertexIndices());

    HdSt_QuadInfo *quadInfo = new HdSt_QuadInfo();

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

    // set quadinfo to topology
    // topology takes the ownership of quadinfo so no need to free.
    _topology->SetQuadInfo(quadInfo);

    _SetResolved();
    return true;
}

bool
HdSt_QuadInfoBuilderComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

HdSt_QuadIndexBuilderComputation::HdSt_QuadIndexBuilderComputation(
    HdSt_MeshTopology *topology,
    HdSt_QuadInfoBuilderComputationSharedPtr const &quadInfoBuilder,
    SdfPath const &id)
    : _id(id), _topology(topology), _quadInfoBuilder(quadInfoBuilder)
{
}

void
HdSt_QuadIndexBuilderComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->push_back(HdBufferSpec(HdTokens->indices, GL_INT, 4));
    // coarse-quads uses int2 as primitive param.
    specs->push_back(HdBufferSpec(HdTokens->primitiveParam, GL_INT, 2));
}

bool
HdSt_QuadIndexBuilderComputation::Resolve()
{
    // quadInfoBuilder may or may not exists, depending on how we switched
    // the repr of the mesh. If it exists, we have to wait.
    if (_quadInfoBuilder && !_quadInfoBuilder->IsResolved()) return false;

    if (!_TryLock()) return false;

    // generate quad index buffer

    HD_TRACE_FUNCTION();

    // TODO: create ptex id remapping buffer here.

    int const * numVertsPtr = _topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = _topology->GetFaceVertexIndices().cdata();
    int const * holeFacesPtr = _topology->GetHoleIndices().cdata();
    int numFaces = _topology->GetFaceVertexCounts().size();
    int numVertIndices = _topology->GetFaceVertexIndices().size();
    int numHoleFaces = _topology->GetHoleIndices().size();

    // count num quads
    bool invalidTopology = false;
    int numQuads = HdSt_MeshTopology::ComputeNumQuads(
        _topology->GetFaceVertexCounts(),
        _topology->GetHoleIndices(),
        &invalidTopology);
    if (invalidTopology) {
        TF_WARN("degenerated face found [%s]", _id.GetText());
        invalidTopology = false;
    }

    int holeIndex = 0;
    VtVec4iArray quadsFaceVertexIndices(numQuads);
    VtVec2iArray primitiveParam(numQuads);

    // quadrangulated verts is added to the end.
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);
    int vertIndex = HdSt_MeshTopology::ComputeNumPoints(
        _topology->GetFaceVertexIndices());

    // TODO: We need to support ptex index in addition to coarse indices.
    //int ptexIndex = 0;
    for (int i = 0, qv = 0, v = 0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        if (nv < 3) {
            continue; // skip degenerated face
        }
        if (holeIndex < numHoleFaces &&
            holeFacesPtr[holeIndex] == i) {
            // skip hole faces.
            ++holeIndex;
            continue;
        }

        if (v+nv > numVertIndices) {
            invalidTopology = true;
            if (nv == 4) {
                quadsFaceVertexIndices[qv++] = GfVec4i(0);
            } else {
                for (int j = 0; j < nv; ++j) {
                    quadsFaceVertexIndices[qv++] = GfVec4i(0);
                }
            }
            v += nv;
            continue;
        }

        if (nv == 4) {
            if (flip) {
                quadsFaceVertexIndices[qv][0] = (vertsPtr[v+0]);
                quadsFaceVertexIndices[qv][1] = (vertsPtr[v+3]);
                quadsFaceVertexIndices[qv][2] = (vertsPtr[v+2]);
                quadsFaceVertexIndices[qv][3] = (vertsPtr[v+1]);
            } else {
                quadsFaceVertexIndices[qv][0] = (vertsPtr[v+0]);
                quadsFaceVertexIndices[qv][1] = (vertsPtr[v+1]);
                quadsFaceVertexIndices[qv][2] = (vertsPtr[v+2]);
                quadsFaceVertexIndices[qv][3] = (vertsPtr[v+3]);
            }
            primitiveParam[qv] = GfVec2i(
                HdSt_MeshTopology::EncodeCoarseFaceParam(i, 0), qv);
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
                quadsFaceVertexIndices[qv][0] = vertsPtr[v+j];
                if (flip) {
                    // edge prev
                    quadsFaceVertexIndices[qv][1] = vertIndex + (j+nv-1)%nv;
                    // center
                    quadsFaceVertexIndices[qv][2] = vertIndex + nv;
                    // edge next
                    quadsFaceVertexIndices[qv][3] = vertIndex + j;
                } else {
                    // edge next
                    quadsFaceVertexIndices[qv][1] = vertIndex + j;
                    // center
                    quadsFaceVertexIndices[qv][2] = vertIndex + nv;
                    // edge prev
                    quadsFaceVertexIndices[qv][3] = vertIndex + (j+nv-1)%nv;
                }
                primitiveParam[qv] = GfVec2i(
                    HdSt_MeshTopology::EncodeCoarseFaceParam(i, 0), qv);
                ++qv;
            }
            vertIndex += nv + 1;
        }
        v += nv;
    }
    if (invalidTopology) {
        TF_WARN("numVerts and verts are incosistent [%s]", _id.GetText());
    }

    _SetResult(HdBufferSourceSharedPtr(new HdVtBufferSource(
                                           HdTokens->indices,
                                           VtValue(quadsFaceVertexIndices))));

    _primitiveParam.reset(new HdVtBufferSource(HdTokens->primitiveParam,
                                               VtValue(primitiveParam)));

    _SetResolved();
    return true;
}

bool
HdSt_QuadIndexBuilderComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtr
HdSt_QuadIndexBuilderComputation::GetChainedBuffer() const
{
    return _primitiveParam;
}

bool
HdSt_QuadIndexBuilderComputation::_CheckValid() const
{
    return true;
}


// ---------------------------------------------------------------------------

HdSt_QuadrangulateTableComputation::HdSt_QuadrangulateTableComputation(
    HdSt_MeshTopology *topology, HdBufferSourceSharedPtr const &quadInfoBuilder)
    : _topology(topology), _quadInfoBuilder(quadInfoBuilder)
{
}

bool
HdSt_QuadrangulateTableComputation::Resolve()
{
    if (!TF_VERIFY(_quadInfoBuilder)) return false;
    if (!_quadInfoBuilder->IsResolved()) return false;
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    HdSt_QuadInfo const *quadInfo = _topology->GetQuadInfo();
    if (!quadInfo) {
        TF_CODING_ERROR("HdSt_QuadInfo is null.");
        return true;
    }

    // transfer quadrangulation table to GPU
    // for the same reason as cpu quadrangulation, we need a check
    // of IsAllQuads here.
    // see the comment on HdSt_MeshTopology::Quadrangulate()
    if (!quadInfo->IsAllQuads()) {
        int quadInfoStride = quadInfo->maxNumVert + 2;
        int numNonQuads = quadInfo->numVerts.size();

        // create a buffer source for gpu quadinfo table
        VtIntArray array(quadInfoStride * numNonQuads);

        int index = 0, vertIndex = 0, dstOffset = quadInfo->pointsOffset;
        for (int i = 0; i < numNonQuads; ++i) {
            // GPU quadinfo table layout
            //
            // struct NonQuad {
            //     int numVert;
            //     int dstOffset;
            //     int index[maxNumVert];
            // } [numNonQuads]
            //
            int numVert = quadInfo->numVerts[i];
            array[index]   = numVert;
            array[index+1] = dstOffset;
            for (int j = 0; j < numVert; ++j) {
                array[index+j+2] = quadInfo->verts[vertIndex++];
            }
            index += quadInfoStride;
            dstOffset += numVert + 1;  // edge + center
        }

        // sanity check for number of points
        TF_VERIFY(dstOffset ==
                  quadInfo->pointsOffset +
                  quadInfo->numAdditionalPoints);

        // GPU quadrangulate table
        HdBufferSourceSharedPtr table(new HdVtBufferSource(HdTokens->quadInfo,
                                                           VtValue(array)));

        _SetResult(table);
    } else {
        _topology->ClearQuadrangulateTableRange();
    }
    _SetResolved();
    return true;
}

void
HdSt_QuadrangulateTableComputation::AddBufferSpecs(
    HdBufferSpecVector *specs) const
{
    // quadinfo computation produces an index buffer for quads.
    specs->push_back(HdBufferSpec(HdTokens->quadInfo,
                                  GL_INT,
                                  1));
}

bool
HdSt_QuadrangulateTableComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

template <typename T>
HdBufferSourceSharedPtr
_Quadrangulate(HdBufferSourceSharedPtr const &source,
               HdSt_QuadInfo const *qi)
{
    // CPU quadrangulation

    // original points + quadrangulated points
    VtArray<T> results(qi->pointsOffset + qi->numAdditionalPoints);

    // copy original primVars
    T const *srcPtr = reinterpret_cast<T const*>(source->GetData());
    memcpy(results.data(), srcPtr, sizeof(T)*qi->pointsOffset);

    // compute quadrangulate primVars
    int index = 0;
    // store quadrangulated points at end
    int dstIndex = qi->pointsOffset;

    HD_PERF_COUNTER_ADD(HdPerfTokens->quadrangulatedVerts,
                        qi->numAdditionalPoints);

    TF_FOR_ALL (numVertsIt, qi->numVerts) {
        int nv = *numVertsIt;
        T center(0);
        for (int i = 0; i < nv; ++i) {
            int i0 = qi->verts[index+i];
            int i1 = qi->verts[index+(i+1)%nv];

            // midpoint
            T edge = (srcPtr[i0] + srcPtr[i1]) * 0.5;
            results[dstIndex++] = edge;

            // accumulate center
            center += srcPtr[i0];
        }
        // average center value
        center /= nv;
        results[dstIndex++] = center;

        index += nv;
    }

    return HdBufferSourceSharedPtr(new HdVtBufferSource(
                                       source->GetName(), VtValue(results)));
}


HdSt_QuadrangulateComputation::HdSt_QuadrangulateComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    HdBufferSourceSharedPtr const &quadInfoBuilder,
    SdfPath const &id)
    : _id(id), _topology(topology), _source(source),
      _quadInfoBuilder(quadInfoBuilder)
{
}

bool
HdSt_QuadrangulateComputation::Resolve()
{
    if (!TF_VERIFY(_source)) return false;
    if (!_source->IsResolved()) return false;
    if (_quadInfoBuilder && !_quadInfoBuilder->IsResolved()) return false;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    HD_PERF_COUNTER_INCR(HdPerfTokens->quadrangulateCPU);

    HdSt_QuadInfo const *quadInfo = _topology->GetQuadInfo();
    if (!TF_VERIFY(quadInfo)) return true;

    // If the topology is all quads, just return source.
    // This check is needed since if the topology changes, we don't know
    // whether the topology is all-quads or not until the quadinfo computation
    // is resolved. So we conservatively register primvar quadrangulations
    // on that case, it hits this condition. Once quadinfo resolved on the
    // topology, HdSt_MeshTopology::GetQuadrangulateComputation returns null
    // and nobody calls this function for all-quads prims.
    if (quadInfo->IsAllQuads()) {
        _SetResult(_source);
        _SetResolved();
        return true;
    }

    HdBufferSourceSharedPtr result;

    switch (_source->GetGLElementDataType()) {
    case GL_FLOAT:
        result = _Quadrangulate<float>(_source, quadInfo);
        break;
    case GL_FLOAT_VEC2:
        result = _Quadrangulate<GfVec2f>(_source, quadInfo);
        break;
    case GL_FLOAT_VEC3:
        result = _Quadrangulate<GfVec3f>(_source, quadInfo);
        break;
    case GL_FLOAT_VEC4:
        result = _Quadrangulate<GfVec4f>(_source, quadInfo);
        break;
    case GL_DOUBLE:
        result = _Quadrangulate<double>(_source, quadInfo);
        break;
    case GL_DOUBLE_VEC2:
        result = _Quadrangulate<GfVec2d>(_source, quadInfo);
        break;
    case GL_DOUBLE_VEC3:
        result = _Quadrangulate<GfVec3d>(_source, quadInfo);
        break;
    case GL_DOUBLE_VEC4:
        result = _Quadrangulate<GfVec4d>(_source, quadInfo);
        break;
    default:
        TF_CODING_ERROR("Unsupported points type for quadrangulation [%s]",
                        _id.GetText());
        result = _source;
        break;
    }

    _SetResult(result);
    _SetResolved();
    return true;
}

void
HdSt_QuadrangulateComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same spec buffer as source
    _source->AddBufferSpecs(specs);
}

int
HdSt_QuadrangulateComputation::GetGLComponentDataType() const
{
    return _source->GetGLComponentDataType();
}

bool
HdSt_QuadrangulateComputation::_CheckValid() const
{
    return (_source->IsValid());
}

// ---------------------------------------------------------------------------

template <typename T>
HdBufferSourceSharedPtr
_QuadrangulateFaceVarying(HdBufferSourceSharedPtr const &source,
                          VtIntArray const &faceVertexCounts,
                          VtIntArray const &holeFaces,
                          bool flip,
                          SdfPath const &id)
{
    T const *srcPtr = reinterpret_cast<T const *>(source->GetData());
    int numElements = source->GetNumElements();

    // CPU face-varying quadrangulation
    bool invalidTopology = false;
    int numFVarValues = 0;
    int holeIndex = 0;
    int numHoleFaces = holeFaces.size();
    for (int i = 0; i < faceVertexCounts.size(); ++i) {
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
    for (int i = 0, v = 0; i < faceVertexCounts.size(); ++i) {
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
                    results[dstIndex++] = srcPtr[v+j];
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
                center += srcPtr[v+j];
            }
            center /= nVerts;

            // for each quadrant
            for (int j = 0; j < nVerts; ++j) {
                results[dstIndex++] = srcPtr[v+j];
                // mid edge
                results[dstIndex++]
                    = (srcPtr[v+j] + srcPtr[v+(j+1)%nVerts]) * 0.5;
                // center
                results[dstIndex++] = center;
                // mid edge
                results[dstIndex++]
                    = (srcPtr[v+j] + srcPtr[v+(j+nVerts-1)%nVerts]) * 0.5;
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

HdSt_QuadrangulateFaceVaryingComputation::HdSt_QuadrangulateFaceVaryingComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
    : _id(id), _topology(topology), _source(source)
{
}

bool
HdSt_QuadrangulateFaceVaryingComputation::Resolve()
{
    if (!TF_VERIFY(_source)) return false;
    if (!_source->IsResolved()) return false;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HD_PERF_COUNTER_INCR(HdPerfTokens->quadrangulateFaceVarying);

    VtIntArray const &faceVertexCounts = _topology->GetFaceVertexCounts();
    VtIntArray const &holeFaces = _topology->GetHoleIndices();
    bool flip = (_topology->GetOrientation() != HdTokens->rightHanded);
    HdBufferSourceSharedPtr result;

    switch (_source->GetGLElementDataType()) {
    case GL_FLOAT:
        result = _QuadrangulateFaceVarying<float>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_FLOAT_VEC2:
        result = _QuadrangulateFaceVarying<GfVec2f>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_FLOAT_VEC3:
        result = _QuadrangulateFaceVarying<GfVec3f>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_FLOAT_VEC4:
        result = _QuadrangulateFaceVarying<GfVec4f>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE:
        result = _QuadrangulateFaceVarying<double>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE_VEC2:
        result = _QuadrangulateFaceVarying<GfVec2d>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE_VEC3:
        result = _QuadrangulateFaceVarying<GfVec3d>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    case GL_DOUBLE_VEC4:
        result = _QuadrangulateFaceVarying<GfVec4d>(
            _source, faceVertexCounts, holeFaces, flip, _id);
        break;
    default:
        TF_CODING_ERROR("Unsupported primvar type for quadrangulation [%s]",
                        _id.GetText());
        result = _source;
        break;
    }

    _SetResult(result);
    _SetResolved();
    return true;
}

void
HdSt_QuadrangulateFaceVaryingComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same spec buffer as source
    _source->AddBufferSpecs(specs);
}


bool
HdSt_QuadrangulateFaceVaryingComputation::_CheckValid() const
{
    return (_source->IsValid());
}

// ---------------------------------------------------------------------------

HdSt_QuadrangulateComputationGPU::HdSt_QuadrangulateComputationGPU(
    HdSt_MeshTopology *topology, TfToken const &sourceName, GLenum dataType,
    SdfPath const &id)
    : _id(id), _topology(topology), _name(sourceName), _dataType(dataType)
{
    if (dataType != GL_FLOAT && dataType != GL_DOUBLE) {
        TF_CODING_ERROR("Unsupported primvar type for quadrangulation [%s]",
                        _id.GetText());
    }
}

void
HdSt_QuadrangulateComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range)
{
    if (!TF_VERIFY(_topology))
        return;

    HD_TRACE_FUNCTION();

    HD_PERF_COUNTER_INCR(HdPerfTokens->quadrangulateGPU);

    // if this topology doesn't contain non-quad faces, quadInfoRange is null.
    HdBufferArrayRangeSharedPtr const &quadrangulateTableRange =
        _topology->GetQuadrangulateTableRange();
    if (!quadrangulateTableRange) return;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSt_QuadInfo const *quadInfo = _topology->GetQuadInfo();
    if (!quadInfo) {
        TF_CODING_ERROR("HdSt_QuadInfo is null.");
        return;
    }

    if (!glDispatchCompute)
        return;

    // select shader by datatype
    TfToken shaderToken = (_dataType == GL_FLOAT ?
                           HdGLSLProgramTokens->quadrangulateFloat :
                           HdGLSLProgramTokens->quadrangulateDouble);

    HdGLSLProgramSharedPtr computeProgram =
        HdGLSLProgram::GetComputeProgram(shaderToken);
    if (!computeProgram) return;

    GLuint program = computeProgram->GetProgram().GetId();

    // buffer resources for GPU computation
    HdBufferResourceSharedPtr primVar = range->GetResource(_name);
    HdBufferResourceSharedPtr quadrangulateTable =
        quadrangulateTableRange->GetResource();

    // prepare uniform buffer for GPU computation
    struct Uniform {
        int vertexOffset;
        int quadInfoStride;
        int quadInfoOffset;
        int maxNumVert;
        int primVarOffset;
        int primVarStride;
        int numComponents;
    } uniform;

    int quadInfoStride = quadInfo->maxNumVert + 2;

    // coherent vertex offset in aggregated buffer array
    uniform.vertexOffset = range->GetOffset();
    // quadinfo offset/stride in aggregated adjacency table
    uniform.quadInfoStride = quadInfoStride;
    uniform.quadInfoOffset = quadrangulateTableRange->GetOffset();
    uniform.maxNumVert = quadInfo->maxNumVert;
    // interleaved offset/stride to points
    // note: this code (and the glsl smooth normal compute shader) assumes
    // components in interleaved vertex array are always same data type.
    // i.e. it can't handle an interleaved array which interleaves
    // float/double, float/int etc.
    uniform.primVarOffset = primVar->GetOffset() / primVar->GetComponentSize();
    uniform.primVarStride = primVar->GetStride() / primVar->GetComponentSize();
    uniform.numComponents = primVar->GetNumComponents();

    // transfer uniform buffer
    GLuint ubo = computeProgram->GetGlobalUniformBuffer().GetId();
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
    // XXX: workaround for 319.xx driver bug of glNamedBufferDataEXT on UBO
    // XXX: move this workaround to renderContextCaps
    if (false && caps.directStateAccessEnabled) {
        glNamedBufferDataEXT(ubo, sizeof(uniform), &uniform, GL_STATIC_DRAW);
    } else {
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(uniform), &uniform, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, primVar->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, quadrangulateTable->GetId());

    // dispatch compute kernel
    glUseProgram(program);

    int numNonQuads = (int)quadInfo->numVerts.size();

    glDispatchCompute(numNonQuads, 1, 1);

    glUseProgram(0);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

    HD_PERF_COUNTER_ADD(HdPerfTokens->quadrangulatedVerts,
                        quadInfo->numAdditionalPoints);
}

void
HdSt_QuadrangulateComputationGPU::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
    //
    // GPU quadrangulation requires the source data on GPU in prior to
    // execution, so no need to populate bufferspec on registration.
}

int
HdSt_QuadrangulateComputationGPU::GetNumOutputElements() const
{
    HdSt_QuadInfo const *quadInfo = _topology->GetQuadInfo();

    if (!quadInfo) {
        TF_CODING_ERROR("HdSt_QuadInfo is null [%s]", _id.GetText());
        return 0;
    }

    return quadInfo->pointsOffset + quadInfo->numAdditionalPoints;
}

