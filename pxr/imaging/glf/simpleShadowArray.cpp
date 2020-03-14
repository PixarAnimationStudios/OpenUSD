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
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stringUtils.h"

#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(GLF_ENABLE_BINDLESS_SHADOW_TEXTURES, false,
                      "Enable use of bindless shadow maps");

GlfSimpleShadowArray::GlfSimpleShadowArray() :
    // bindful state
    _size(0),
    _numLayers(0),
    _bindfulTexture(0),
    _shadowDepthSampler(0),
    // common state
    _framebuffer(0),
    _shadowCompareSampler(0),
    _unbindRestoreDrawFramebuffer(0),
    _unbindRestoreReadFramebuffer(0),
    _unbindRestoreViewport{0,0,0,0}
{
}

GlfSimpleShadowArray::~GlfSimpleShadowArray()
{
    _FreeResources();
}

/*static*/
bool
GlfSimpleShadowArray::GetBindlessShadowMapsEnabled()
{
    // Note: We do not test the GL context caps for the availability of the
    // bindless texture and int64 extensions.
    static bool usingBindlessShadowMaps =
        TfGetEnvSetting(GLF_ENABLE_BINDLESS_SHADOW_TEXTURES);

    return usingBindlessShadowMaps;
}


// --------- (public) Bindful API ----------
void
GlfSimpleShadowArray::SetSize(GfVec2i const & size)
{
    if (GetBindlessShadowMapsEnabled()) {
        TF_CODING_ERROR("Using bindful API %s when bindless "
            "shadow maps are enabled\n", TF_FUNC_NAME().c_str());
        return;
    }
    if (_size != size) {
        _FreeBindfulTextures();
        _size = size;
    }
}

void
GlfSimpleShadowArray::SetNumLayers(size_t numLayers)
{
    if (GetBindlessShadowMapsEnabled()) {
        TF_CODING_ERROR("Using bindful API %s when bindless "
            "shadow maps are enabled\n", TF_FUNC_NAME().c_str());
        return;
    }

    if (_numLayers != numLayers) {
        _viewMatrix.resize(numLayers, GfMatrix4d().SetIdentity());
        _projectionMatrix.resize(numLayers, GfMatrix4d().SetIdentity());
        _FreeBindfulTextures();
        _numLayers = numLayers;
    }
}

GLuint
GlfSimpleShadowArray::GetShadowMapTexture() const
{
    if (GetBindlessShadowMapsEnabled()) {
        TF_CODING_ERROR("Using bindful API in %s when bindless "
            "shadow maps are enabled\n",  TF_FUNC_NAME().c_str());
        return -1;
    }
    return _bindfulTexture;
}

GLuint
GlfSimpleShadowArray::GetShadowMapDepthSampler() const
{
    if (GetBindlessShadowMapsEnabled()) {
        TF_CODING_ERROR("Using bindful API in %s when bindless "
            "shadow maps are enabled\n",  TF_FUNC_NAME().c_str());
        return -1;
    }
    return _shadowDepthSampler;
}

GLuint
GlfSimpleShadowArray::GetShadowMapCompareSampler() const
{
    if (GetBindlessShadowMapsEnabled()) {
        TF_CODING_ERROR("Using bindful API in %s when bindless "
            "shadow maps are enabled\n",  TF_FUNC_NAME().c_str());
        return -1;
    }
    return _shadowCompareSampler;
}

// --------- (public) Bindless API ----------
void
GlfSimpleShadowArray::SetShadowMapResolutions(
    std::vector<GfVec2i> const& resolutions)
{
    if (_resolutions == resolutions) {
        return;
    }

    _resolutions = resolutions;

    _FreeBindlessTextures();

    size_t numShadowMaps = _resolutions.size();
    if (_viewMatrix.size() != numShadowMaps ||
        _projectionMatrix.size() != numShadowMaps) {
        _viewMatrix.resize(numShadowMaps, GfMatrix4d().SetIdentity());
        _projectionMatrix.resize(numShadowMaps, GfMatrix4d().SetIdentity());
    }

}

std::vector<uint64_t> const&
GlfSimpleShadowArray::GetBindlessShadowMapHandles() const
{
    return _bindlessTextureHandles;
}

// --------- (public) Common API ----------
size_t
GlfSimpleShadowArray::GetNumShadowMapPasses() const
{
    // In both the bindful and bindless cases, we require one pass per shadow
    // map.
    if (GetBindlessShadowMapsEnabled()) {
        return _resolutions.size();
    } else {
        return _numLayers;
    }
}

GfVec2i
GlfSimpleShadowArray::GetShadowMapSize(size_t index) const
{
    GfVec2i shadowMapSize(0);
    if (GetBindlessShadowMapsEnabled()) {
        if (TF_VERIFY(index < _resolutions.size())) {
            shadowMapSize = _resolutions[index];
        }
    } else {
        // In the bindful case, all shadow map textures use the same size.
        shadowMapSize = _size;
    }

    return shadowMapSize;
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

void
GlfSimpleShadowArray::BeginCapture(size_t index, bool clear)
{
    _BindFramebuffer(index);

    if (clear) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    // save the current viewport
    glGetIntegerv(GL_VIEWPORT, _unbindRestoreViewport);

    GfVec2i resolution = GetShadowMapSize(index);
    glViewport(0, 0, resolution[0], resolution[1]);

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

    if (TfDebug::IsEnabled(GLF_DEBUG_DUMP_SHADOW_TEXTURES)) {
        GlfImage::StorageSpec storage;
        GfVec2i resolution = GetShadowMapSize(index);
        storage.width = resolution[0];
        storage.height = resolution[1];
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
            TfDebug::Helper().Msg(
                "Wrote shadow texture: %s\n", outputImageFile.c_str());
        } else {
            TfDebug::Helper().Msg(
                "Failed to write shadow texture: %s\n", outputImageFile.c_str()
            );
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

// --------- private helpers ----------
bool
GlfSimpleShadowArray::_ShadowMapExists() const
{
    return GetBindlessShadowMapsEnabled() ? !_bindlessTextures.empty() : 
                                             _bindfulTexture;
}

void
GlfSimpleShadowArray::_AllocResources()
{
    // Samplers
    GLfloat border[] = {1, 1, 1, 1};

    if (!_shadowDepthSampler) {
        glGenSamplers(1, &_shadowDepthSampler);
        glSamplerParameteri(
            _shadowDepthSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(
            _shadowDepthSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(
            _shadowDepthSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(
            _shadowDepthSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameterfv(
            _shadowDepthSampler, GL_TEXTURE_BORDER_COLOR, border);
    }

    if (!_shadowCompareSampler) {
        glGenSamplers(1, &_shadowCompareSampler);
        glSamplerParameteri(
            _shadowCompareSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(
            _shadowCompareSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(
            _shadowCompareSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(
            _shadowCompareSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameterfv(
            _shadowCompareSampler, GL_TEXTURE_BORDER_COLOR, border);
        glSamplerParameteri(
            _shadowCompareSampler, GL_TEXTURE_COMPARE_MODE, 
            GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(
            _shadowCompareSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
    }

    // Shadow maps
    if (GetBindlessShadowMapsEnabled()) {
        _AllocBindlessTextures();
    } else {
       _AllocBindfulTextures();
    }

    // Framebuffer
    if (!_framebuffer) {
        glGenFramebuffers(1, &_framebuffer);
    }
}

void
GlfSimpleShadowArray::_AllocBindfulTextures()
{
    glGenTextures(1, &_bindfulTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _bindfulTexture);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                _size[0], _size[1], _numLayers, 0, 
                GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    TF_DEBUG(GLF_DEBUG_SHADOW_TEXTURES).Msg(
        "Created bindful shadow map texture array with %lu %dx%d textures\n"
        , _numLayers, _size[0], _size[1]);
}

void
GlfSimpleShadowArray::_AllocBindlessTextures()
{
    if (!TF_VERIFY(_shadowCompareSampler) ||
        !TF_VERIFY(_bindlessTextures.empty()) ||
        !TF_VERIFY(_bindlessTextureHandles.empty())) {
        TF_CODING_ERROR("Unexpected entry state in %s\n",
                        TF_FUNC_NAME().c_str());
        return;
    }

    // Commenting out the line below results in the residency check in
    // _FreeBindlessTextures failing.
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;

    // XXX: Currently, we allocate/reallocate ALL shadow maps each time.
    for (GfVec2i const& size : _resolutions) {
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
            size[0], size[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        _bindlessTextures.push_back(id);

        GLuint64 handle =
            glGetTextureSamplerHandleARB(id, _shadowCompareSampler);
        
        _bindlessTextureHandles.push_back(handle);

        if (TF_VERIFY(!glIsTextureHandleResidentARB(handle))) {
            glMakeTextureHandleResidentARB(handle);
        } else {
            GLF_POST_PENDING_GL_ERRORS();
        }

        TF_DEBUG(GLF_DEBUG_SHADOW_TEXTURES).Msg(
            "Created bindless shadow map texture of size %dx%d "
            "(id %#x, handle %#lx)\n" , size[0], size[1], id, handle);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void
GlfSimpleShadowArray::_FreeResources()
{
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;

    if (GetBindlessShadowMapsEnabled()) {
        _FreeBindlessTextures();
    } else {
        _FreeBindfulTextures();
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
GlfSimpleShadowArray::_FreeBindfulTextures()
{
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;

    if (_bindfulTexture) {
        glDeleteTextures(1, &_bindfulTexture);
        _bindfulTexture = 0;
    }

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::_FreeBindlessTextures()
{
    GlfSharedGLContextScopeHolder sharedContextScopeHolder;
    // XXX: Ideally, we don't deallocate all textures, and only those that have
    // resolution modified.

    if (!_bindlessTextureHandles.empty()) {
        for (uint64_t handle : _bindlessTextureHandles) {
            // Handles are made resident on creation.
            if (TF_VERIFY(glIsTextureHandleResidentARB(handle))) {
                glMakeTextureHandleNonResidentARB(handle);
            }
        }
        _bindlessTextureHandles.clear();
    }

    for (GLuint const& id : _bindlessTextures) {
        if (id) {
            glDeleteTextures(1, &id);
        }
    }
    _bindlessTextures.clear();
    
    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::_BindFramebuffer(size_t index)
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                  (GLint*)&_unbindRestoreDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,
                  (GLint*)&_unbindRestoreReadFramebuffer);

    if (!_framebuffer || !_ShadowMapExists()) {
        _AllocResources();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    if (GetBindlessShadowMapsEnabled()) {
        glFramebufferTexture(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT, _bindlessTextures[index], 0);
    } else {
        glFramebufferTextureLayer(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT, _bindfulTexture, 0, index);
    }

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::_UnbindFramebuffer()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _unbindRestoreDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _unbindRestoreReadFramebuffer);

    GLF_POST_PENDING_GL_ERRORS();
}


PXR_NAMESPACE_CLOSE_SCOPE

