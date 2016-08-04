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

#include "pxr/imaging/hd/smoothNormals.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/vt/array.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"

Hd_SmoothNormalsComputation::Hd_SmoothNormalsComputation(
    Hd_VertexAdjacency const *adjacency,
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName,
    HdBufferSourceSharedPtr const &adjacencyBuilder)
    : _adjacency(adjacency), _points(points), _dstName(dstName),
      _adjacencyBuilder(adjacencyBuilder)
{
}

void
Hd_SmoothNormalsComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // the datatype of normals is same as points (float or double).
    specs->push_back(HdBufferSpec(_dstName,
                                  _points->GetGLComponentDataType(),
                                  3));
}

TfToken const &
Hd_SmoothNormalsComputation::GetName() const
{
    return _dstName;
}

int
Hd_SmoothNormalsComputation::GetGLComponentDataType() const
{
    return _points->GetGLComponentDataType();
}

bool
Hd_SmoothNormalsComputation::Resolve()
{
    // dependency check first
    if (_adjacencyBuilder) {
        if (not _adjacencyBuilder->IsResolved()) return false;
    }
    if (_points) {
        if (not _points->IsResolved()) return false;
    }
    if (not _TryLock()) return false;

    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (not TF_VERIFY(_adjacency)) return true;

    int numPoints = _points->GetNumElements();

    HdBufferSourceSharedPtr normals;
    switch (_points->GetGLElementDataType()) {
    case GL_FLOAT_VEC3:
        normals = HdBufferSourceSharedPtr(
            new HdVtBufferSource(
                _dstName, VtValue(
                    _adjacency->ComputeSmoothNormals(
                        numPoints,
                        static_cast<const GfVec3f*>(_points->GetData())))));
        break;
    case GL_DOUBLE_VEC3:
        normals = HdBufferSourceSharedPtr(
            new HdVtBufferSource(
                _dstName, VtValue(
                    _adjacency->ComputeSmoothNormals(
                        numPoints,
                        static_cast<const GfVec3d*>(_points->GetData())))));
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
// ---------------------------------------------------------------------------

Hd_SmoothNormalsComputationGPU::Hd_SmoothNormalsComputationGPU(
    Hd_VertexAdjacency const *adjacency,
    TfToken const &srcName, TfToken const &dstName,
    GLenum dstDataType)
    : _adjacency(adjacency), _srcName(srcName), _dstName(dstName),
      _dstDataType(dstDataType)
{
    if (dstDataType != GL_FLOAT and dstDataType != GL_DOUBLE) {
        TF_CODING_ERROR("Unsupported points type for computing smooth normals");
    }
}

void
Hd_SmoothNormalsComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const &range)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (not glDispatchCompute)
        return;

    // XXX: workaround until the shading stuff is implemeted.
    // The drawing program is owned and set by testHdBasicDrawing now,
    // so it has to be restored if it's changed in hd.
    GLint restoreProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &restoreProgram);

    int numPoints = range->GetNumElements();

    TF_VERIFY(_adjacency);
    HdBufferArrayRangeSharedPtr const &adjacencyRange = _adjacency->GetAdjacencyRange();
    TF_VERIFY(adjacencyRange);

    // select shader by datatype
    TfToken shaderToken = (_dstDataType == GL_FLOAT ?
                           HdGLSLProgramTokens->smoothNormalsFloat :
                           HdGLSLProgramTokens->smoothNormalsDouble);

    HdGLSLProgramSharedPtr computeProgram = HdGLSLProgram::GetComputeProgram(shaderToken);
    if (not computeProgram) return;

    GLuint program = computeProgram->GetProgram().GetId();

    // buffer resources for GPU computation
    HdBufferResourceSharedPtr points = range->GetResource(_srcName);
    HdBufferResourceSharedPtr normals = range->GetResource(_dstName);
    HdBufferResourceSharedPtr adjacency = adjacencyRange->GetResource();

    // prepare uniform buffer for GPU computation
    struct Uniform {
        int vertexOffset;
        int adjacencyStride;
        int adjacencyOffset;
        int padding;
        int pointsOffset;
        int pointsStride;
        int normalsOffset;
        int normalsStride;
    } uniform;

    // coherent vertex offset in aggregated buffer array
    uniform.vertexOffset = range->GetOffset();
    // adjacency offset/stride in aggregated adjacency table
    uniform.adjacencyStride = _adjacency->GetStride();
    uniform.adjacencyOffset = adjacencyRange->GetOffset();
    uniform.padding = 0;
    // interleaved offset/stride to points
    // note: this code (and the glsl smooth normal compute shader) assumes
    // components in interleaved vertex array are always same data type.
    // i.e. it can't handle an interleaved array which interleaves
    // float/double, float/int etc.
    uniform.pointsOffset = static_cast<int>(points->GetOffset() / points->GetComponentSize());
    uniform.pointsStride = static_cast<int>(points->GetStride() / points->GetComponentSize());
    // interleaved offset/stride to normals
    uniform.normalsOffset = static_cast<int>(normals->GetOffset() / points->GetComponentSize());
    uniform.normalsStride = static_cast<int>(normals->GetStride() / points->GetComponentSize());

    // transfer uniform buffer
    GLuint ubo = computeProgram->GetGlobalUniformBuffer().GetId();
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
    // XXX: workaround for 319.xx driver bug of glNamedBufferDataEXT on UBO
    // XXX: move this workaround to renderContextCaps
    if (false and caps.directStateAccessEnabled) {
        glNamedBufferDataEXT(ubo, sizeof(uniform), &uniform, GL_STATIC_DRAW);
    } else {
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(uniform), &uniform, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, points->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, normals->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, adjacency->GetId());

    // dispatch compute kernel
    glUseProgram(program);

    glDispatchCompute(numPoints, 1, 1);

    glUseProgram(0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);

    // XXX: workaround until shading stuff implemeted.
    glUseProgram(restoreProgram);
}

void
Hd_SmoothNormalsComputationGPU::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->push_back(HdBufferSpec(_dstName, _dstDataType, 3));
}

