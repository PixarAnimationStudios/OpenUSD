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
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"

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
    _unbindRestoreReadFramebuffer(0)
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
    if (not TF_VERIFY(index < _viewMatrix.size())) {
        return GfMatrix4d(1.0);
    }

    return _viewMatrix[index];
}

void
GlfSimpleShadowArray::SetViewMatrix(size_t index, GfMatrix4d const & matrix)
{
    if (not TF_VERIFY(index < _viewMatrix.size())) {
        return;
    }

    _viewMatrix[index] = matrix;
}

GfMatrix4d
GlfSimpleShadowArray::GetProjectionMatrix(size_t index) const
{
    if (not TF_VERIFY(index < _projectionMatrix.size())) {
        return GfMatrix4d(1.0);
    }

    return _projectionMatrix[index];
}

void
GlfSimpleShadowArray::SetProjectionMatrix(size_t index, GfMatrix4d const & matrix)
{
    if (not TF_VERIFY(index < _projectionMatrix.size())) {
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

    glViewport(0, 0, GetSize()[0], GetSize()[1]);

    // depth 1.0 means infinity (no occluders).
    // This value is also used as a border color
    glDepthRange(0, 0.99999);
    glEnable(GL_DEPTH_CLAMP);

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::EndCapture(size_t)
{
    glDisable(GL_DEPTH_CLAMP);

    _UnbindFramebuffer();

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfSimpleShadowArray::Draw(size_t index) const
{
    GfVec4d nearPlane[4] = {
        GfVec4d( 1.0, 1.0, -1.0, 1.0), GfVec4d(-1.0, 1.0, -1.0, 1.0),
        GfVec4d(-1.0,-1.0, -1.0, 1.0), GfVec4d( 1.0,-1.0, -1.0, 1.0)
    };
    GfVec4d farPlane[4] = {
        GfVec4d( 1.0, 1.0, 1.0, 1.0), GfVec4d(-1.0, 1.0, 1.0, 1.0),
        GfVec4d(-1.0,-1.0, 1.0, 1.0), GfVec4d( 1.0,-1.0, 1.0, 1.0)
    };

    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex3dv(nearPlane[0].GetArray());
    glVertex3dv(nearPlane[1].GetArray());
    glVertex3dv(nearPlane[2].GetArray());
    glVertex3dv(nearPlane[3].GetArray());
    glEnd();

    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex3dv(farPlane[0].GetArray());
    glVertex3dv(farPlane[1].GetArray());
    glVertex3dv(farPlane[2].GetArray());
    glVertex3dv(farPlane[3].GetArray());
    glEnd();

    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINES);
    glVertex3dv(nearPlane[0].GetArray());
    glVertex3dv(farPlane[0].GetArray());
    glVertex3dv(nearPlane[1].GetArray());
    glVertex3dv(farPlane[1].GetArray());
    glVertex3dv(nearPlane[2].GetArray());
    glVertex3dv(farPlane[2].GetArray());
    glVertex3dv(nearPlane[3].GetArray());
    glVertex3dv(farPlane[3].GetArray());
    glEnd();
}

void
GlfSimpleShadowArray::_AllocTextureArray()
{
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                 _size[0], _size[1], static_cast<GLsizei>(_numLayers), 0, 
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

    if (not _framebuffer or not _texture) {
        _AllocTextureArray();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT, _texture, 0, static_cast<GLint>(index));
}

void
GlfSimpleShadowArray::_UnbindFramebuffer()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _unbindRestoreDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _unbindRestoreReadFramebuffer);
}
