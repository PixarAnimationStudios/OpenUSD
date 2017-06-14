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

#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/gf/vec4i.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_QuadInfoBuilderComputation::HdSt_QuadInfoBuilderComputation(
    HdSt_MeshTopology *topology, SdfPath const &id)
    : _id(id), _topology(topology)
{
}

bool
HdSt_QuadInfoBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HdQuadInfo *quadInfo = new HdQuadInfo();
    HdMeshUtil meshUtil(_topology, _id);
    meshUtil.ComputeQuadInfo(quadInfo);

    // Set quadinfo to topology
    // topology takes ownership of quadinfo so no need to free.
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

    HD_TRACE_FUNCTION();

    // generate quad index buffer
    VtVec4iArray quadsFaceVertexIndices;
    VtVec2iArray primitiveParam;
    HdMeshUtil meshUtil(_topology, _id);
    meshUtil.ComputeQuadIndices(
            &quadsFaceVertexIndices,
            &primitiveParam);

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

    HdQuadInfo const *quadInfo = _topology->GetQuadInfo();
    if (!quadInfo) {
        TF_CODING_ERROR("QuadInfo is null.");
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

    HdQuadInfo const *quadInfo = _topology->GetQuadInfo();
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

    VtValue result;
    HdMeshUtil meshUtil(_topology, _id);
    if (meshUtil.ComputeQuadrangulatedPrimvar(quadInfo,
                _source->GetData(),
                _source->GetNumElements(),
                _source->GetGLElementDataType(),
                &result)) {
        HD_PERF_COUNTER_ADD(HdPerfTokens->quadrangulatedVerts,
                quadInfo->numAdditionalPoints);

        _SetResult(HdBufferSourceSharedPtr(
                    new HdVtBufferSource(
                        _source->GetName(),
                        result)));
    } else {
        _SetResult(_source);
    }

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

    // XXX: we could skip this if the mesh is all quads, like above in
    // HdSt_QuadrangulateComputation::Resolve()...

    VtValue result;
    HdMeshUtil meshUtil(_topology, _id);
    if (meshUtil.ComputeQuadrangulatedFaceVaryingPrimvar(
                _source->GetData(),
                _source->GetNumElements(),
                _source->GetGLElementDataType(),
                &result)) {
        _SetResult(HdBufferSourceSharedPtr(
                    new HdVtBufferSource(
                        _source->GetName(),
                        result)));
    } else {
        _SetResult(_source);
    }

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

    HdQuadInfo const *quadInfo = _topology->GetQuadInfo();
    if (!quadInfo) {
        TF_CODING_ERROR("QuadInfo is null.");
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

    HdBufferArrayRangeGLSharedPtr range_ =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (range);

    // buffer resources for GPU computation
    HdBufferResourceSharedPtr primVar_ = range_->GetResource(_name);
    HdBufferResourceGLSharedPtr primVar =
        boost::static_pointer_cast<HdBufferResourceGL> (primVar_);

    HdBufferArrayRangeGLSharedPtr quadrangulateTableRange_ =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (quadrangulateTableRange);

    HdBufferResourceSharedPtr quadrangulateTable_ =
        quadrangulateTableRange_->GetResource();
    HdBufferResourceGLSharedPtr quadrangulateTable =
        boost::static_pointer_cast<HdBufferResourceGL> (quadrangulateTable_);

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
    HdQuadInfo const *quadInfo = _topology->GetQuadInfo();

    if (!quadInfo) {
        TF_CODING_ERROR("QuadInfo is null [%s]", _id.GetText());
        return 0;
    }

    return quadInfo->pointsOffset + quadInfo->numAdditionalPoints;
}


PXR_NAMESPACE_CLOSE_SCOPE

