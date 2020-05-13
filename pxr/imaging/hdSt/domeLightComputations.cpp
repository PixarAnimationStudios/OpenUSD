//
// Copyright 2019 Pixar
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

#include "pxr/imaging/hdSt/domeLightComputations.h"
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_DomeLightComputationGPU::HdSt_DomeLightComputationGPU(
    const TfToken &shaderToken,
    const unsigned int sourceGLTextureName,
    HdStSimpleLightingShaderPtr const &lightingShader,
    const bool wrapRepeat,
    const unsigned int numLevels,
    const unsigned int level, 
    const float roughness) 
  : _shaderToken(shaderToken), 
    _sourceGLTextureName(sourceGLTextureName),
    _lightingShader(lightingShader),
    _wrapRepeat(wrapRepeat),
    _numLevels(numLevels), 
    _level(level), 
    _roughness(roughness)
{
}

uint32_t
HdSt_DomeLightComputationGPU::_CreateGLTexture(
    const int32_t width, const int32_t height) const
{
    uint32_t result;
    glGenTextures(1, &result);

    glBindTexture(GL_TEXTURE_2D, result);

    const GLenum wrapMode = _wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    (_numLevels > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexStorage2D(GL_TEXTURE_2D, _numLevels, GL_RGBA16F, width, height);
    
    return result;
}

void
HdSt_DomeLightComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range,
                                      HdResourceRegistry * const resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!glDispatchCompute) {
        return;
    }

    HdStGLSLProgramSharedPtr const computeProgram = 
        HdStGLSLProgram::GetComputeProgram(
            HdStPackageDomeLightShader(), 
            _shaderToken,
            static_cast<HdStResourceRegistry*>(resourceRegistry));
    if (!TF_VERIFY(computeProgram)) {
        return;
    }

    const GLuint programId = computeProgram->GetProgram().GetId();

    HdStSimpleLightingShaderSharedPtr const shader = _lightingShader.lock();
    if (!TF_VERIFY(shader)) {
        return;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _sourceGLTextureName);

    // Get size of source texture
    GLint srcWidth  = 0;
    GLint srcHeight = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &srcWidth );
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &srcHeight);

    // Size of texture to be created.
    const GLint width  = srcWidth  / 2;
    const GLint height = srcHeight / 2;

    // Get texture name from lighting shader.
    uint32_t dstGLTextureName = shader->GetGLTextureName(_shaderToken);

    // Computation for level 0 is responsible for freeing/allocating
    // the texture.
    if (_level == 0) {
        if (dstGLTextureName) {
            // Free previously allocated texture.
            glDeleteTextures(1, &dstGLTextureName);
        }

        // Create new texture.
        dstGLTextureName = _CreateGLTexture(width, height);
        // And set on shader.
        shader->SetGLTextureName(_shaderToken, dstGLTextureName);
    }

    // Now bind the textures and launch GPU computation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _sourceGLTextureName);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, dstGLTextureName);
    glBindImageTexture(1, dstGLTextureName,
                       _level,
                       /* layered = */ GL_FALSE,
                       /* layer = */ 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    glUseProgram(programId);

    // if we are calculating the irradiance map we do not need to send over
    // the roughness value to the shader
    // flagged this with a negative roughness value
    if (_roughness >= 0.0) {
        glUniform1f(glGetUniformLocation(programId, "roughness"), _roughness);
    }

    // dispatch compute kernel
    glDispatchCompute( (GLuint)width / 32, (GLuint)height / 32, 1);

    glUseProgram(0);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

PXR_NAMESPACE_CLOSE_SCOPE
