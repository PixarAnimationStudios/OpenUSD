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
/// \file simpleShadowArray.cpp

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/simpleShadowArray.h"
#include "pxr/imaging/glf/debugCodes.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/image.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/stringUtils.h"

#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


GlfSimpleShadowArray::GlfSimpleShadowArray(GfVec2i const & size,
                                           size_t numLayers) :
    _size(size),
    _numLayers(numLayers),
    _viewMatrix(_numLayers),
    _projectionMatrix(_numLayers),
    _texture(0),
    _framebuffer(0),
    _shadowDepthSampler(0),
    _shadowCompareSampler(0),
    _unbindRestoreDrawFramebuffer(0),
    _unbindRestoreReadFramebuffer(0),
    _unbindRestoreViewport{0,0,0,0}
{
}

GlfSimpleShadowArray::~GlfSimpleShadowArray()
{
    _FreeTextureArray();
}

GfVec2i
GlfSimpleShadowArray::GetSize() const
{
    return _size;
}

void
GlfSimpleShadowArray::SetSize(GfVec2i const & size)
{
    if (_size != size) {
        _FreeTextureArray();
        _size = size;
    }
}

size_t
GlfSimpleShadowArray::GetNumLayers() const
{
    return _numLayers;
}

void
GlfSimpleShadowArray::SetNumLayers(size_t numLayers)
{
    if (_numLayers != numLayers) {
        _viewMatrix.resize(numLayers, GfMatrix4d().SetIdentity());
        _projectionMatrix.resize(numLayers, GfMatrix4d().SetIdentity());
        _FreeTextureArray();
        _numLayers = numLayers;
    }
}

GfMatrix4d
GlfSimpleShadowArray::GetViewMatrix(size_t index) const
{
    if (!TF_VERIFY(index < _viewMatrix.size())) {
        return GfMatrix4d(1.0);
    }

    return _viewMatrix[index];
}

void
GlfSimpleShadowArray::SetViewMatrix(size_t index, GfMatrix4d const & matrix)
{
    if (!TF_VERIFY(index < _viewMatrix.size())) {
        return;
    }

    _viewMatrix[index] = matrix;
}

GfMatrix4d
GlfSimpleShadowArray::GetProjectionMatrix(size_t index) const
{
    if (!TF_VERIFY(index < _projectionMatrix.size())) {
        return GfMatrix4d(1.0);
    }

    return _projectionMatrix[index];
}

void
GlfSimpleShadowArray::SetProjectionMatrix(size_t index, GfMatrix4d const & matrix)
{
    if (!TF_VERIFY(index < _projectionMatrix.size())) {
        return;
    }

    _projectionMatrix[index] = matrix;
}

GfMatrix4d
GlfSimpleShadowArray::GetWorldToShadowMatrix(size_t index) const
{
    GfMatrix4d size = GfMatrix4d().SetScale(GfVec3d(0.5, 0.5, 0.5));
    GfMatrix4d center = GfMatrix4d().SetTranslate(GfVec3d(0.5, 0.5, 0.5));
    return GetViewMatrix(index) * GetProjectionMatrix(index) * size * center;
}

GLuint
GlfSimpleShadowArray::GetShadowMapTexture() const
{
    return _texture;
}

GLuint
GlfSimpleShadowArray::GetShadowMapDepthSampler() const
{
    return _shadowDepthSampler;
}

GLuint
GlfSimpleShadowArray::GetShadowMapCompareSampler() const
{
    return _shadowCompareSampler;
}

void
GlfSimpleShadowArray::BeginCapture(size_t index, bool clear)
{
    _BindFramebuffer(index);

    if (clear) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    // save the current viewport
    glGetIntegerv(GL_VIEWPORT, _unbindRestoreViewport);

    glViewport(0, 0, GetSize()[0], GetSize()[1]);

    // depth 1.0 means infinity (no occluders).
    // This value is also used as a border color
    glDepthRange(0, 0.99999);
    glEnable(GL_DEPTH_CLAMP);

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::EndCapture(size_t index)
{
    // reset to GL default, except viewport
    glDepthRange(0, 1.0);
    glDisable(GL_DEPTH_CLAMP);

    if (TfDebug::IsEnabled(GLF_DEBUG_SHADOW_TEXTURES)) {
        GlfImage::StorageSpec storage;
        storage.width = GetSize()[0];
        storage.height = GetSize()[1];
        storage.format = GL_DEPTH_COMPONENT;
        storage.type = GL_FLOAT;

        // In OpenGL, (0, 0) is the lower left corner.
        storage.flipped = true;

        const int numPixels = storage.width * storage.height;
        std::vector<GLfloat> pixelData(static_cast<size_t>(numPixels));
        storage.data = static_cast<void*>(pixelData.data());

        glReadPixels(0,
                     0,
                     storage.width,
                     storage.height,
                     storage.format,
                     storage.type,
                     storage.data);

        GLfloat minValue = std::numeric_limits<float>::max();
        GLfloat maxValue = -std::numeric_limits<float>::max();
        for (int i = 0; i < numPixels; ++i) {
            const GLfloat pixelValue = pixelData[i];
            if (pixelValue < minValue) {
                minValue = pixelValue;
            }
            if (pixelValue > maxValue) {
                maxValue = pixelValue;
            }
        }

        // Remap the pixel data so that the furthest depth sample is white and
        // the nearest depth sample is black.
        for (int i = 0; i < numPixels; ++i) {
            pixelData[i] = (pixelData[i] - minValue) / (maxValue - minValue);
        }

        const std::string outputImageFile = ArchNormPath(
            TfStringPrintf("%s/GlfSimpleShadowArray.index_%zu.tif",
                           ArchGetTmpDir(),
                           index));
        GlfImageSharedPtr image = GlfImage::OpenForWriting(outputImageFile);
        if (image->Write(storage)) {
            TF_DEBUG(GLF_DEBUG_SHADOW_TEXTURES).Msg(
                "Wrote shadow texture: %s\n", outputImageFile.c_str());
        } else {
            TF_DEBUG(GLF_DEBUG_SHADOW_TEXTURES).Msg(
                "Failed to write shadow texture: %s\n", outputImageFile.c_str());
        }
    }

    _UnbindFramebuffer();

    // restore viewport
    glViewport(_unbindRestoreViewport[0],
               _unbindRestoreViewport[1],
               _unbindRestoreViewport[2],
               _unbindRestoreViewport[3]);

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::_AllocTextureArray()
{
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                 _size[0], _size[1], _numLayers, 0, 
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    GLfloat border[] = {1, 1, 1, 1};

    glGenSamplers(1, &_shadowDepthSampler);
    glSamplerParameteri(_shadowDepthSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(_shadowDepthSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(_shadowDepthSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(_shadowDepthSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameterfv(_shadowDepthSampler, GL_TEXTURE_BORDER_COLOR, border);

    glGenSamplers(1, &_shadowCompareSampler);
    glSamplerParameteri(_shadowCompareSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(_shadowCompareSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(_shadowCompareSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(_shadowCompareSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameterfv(_shadowCompareSampler, GL_TEXTURE_BORDER_COLOR, border);
    glSamplerParameteri(_shadowCompareSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
    glSamplerParameteri(_shadowCompareSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

    glFramebufferTextureLayer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT, _texture, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void
GlfSimpleShadowArray::_FreeTextureArray()
{
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;

    if (_texture) {
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
    if (_framebuffer) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    if (_shadowDepthSampler) {
        glDeleteSamplers(1, &_shadowDepthSampler);
        _shadowDepthSampler = 0;
    }
    if (_shadowCompareSampler) {
        glDeleteSamplers(1, &_shadowCompareSampler);
        _shadowCompareSampler = 0;
    }
}

void
GlfSimpleShadowArray::_BindFramebuffer(size_t index)
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                  (GLint*)&_unbindRestoreDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,
                  (GLint*)&_unbindRestoreReadFramebuffer);

    if (!_framebuffer || !_texture) {
        _AllocTextureArray();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT, _texture, 0, index);
}

void
GlfSimpleShadowArray::_UnbindFramebuffer()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _unbindRestoreDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _unbindRestoreReadFramebuffer);
}

PXR_NAMESPACE_CLOSE_SCOPE

