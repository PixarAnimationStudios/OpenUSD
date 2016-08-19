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
#ifndef GLF_SIMPLE_SHADOW_ARRAY_H
#define GLF_SIMPLE_SHADOW_ARRAY_H

/// \file glf/simpleShadowArray.h

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/imaging/garch/gl.h"

#include <boost/noncopyable.hpp>
#include <vector>

class GlfSimpleShadowArray : public TfRefBase,
                             public TfWeakBase,
                             boost::noncopyable {
public:
    GlfSimpleShadowArray(GfVec2i const & size, size_t numLayers);
    virtual ~GlfSimpleShadowArray();

    GfVec2i GetSize() const;
    void SetSize(GfVec2i const & size);

    size_t GetNumLayers() const;
    void SetNumLayers(size_t numLayers);

    GfMatrix4d GetViewMatrix(size_t index) const;
    void SetViewMatrix(size_t index, GfMatrix4d const & matrix);

    GfMatrix4d GetProjectionMatrix(size_t index) const;
    void SetProjectionMatrix(size_t index, GfMatrix4d const & matrix);

    GfMatrix4d GetWorldToShadowMatrix(size_t index) const;

    GLuint GetShadowMapTexture() const;
    GLuint GetShadowMapDepthSampler() const;
    GLuint GetShadowMapCompareSampler() const;

    void BeginCapture(size_t index, bool clear);
    void EndCapture(size_t index);

    virtual void Draw(size_t index) const;

private:
    void _AllocTextureArray();
    void _FreeTextureArray();

    void _BindFramebuffer(size_t index);
    void _UnbindFramebuffer();

private:
    GfVec2i _size;
    size_t _numLayers;

    std::vector<GfMatrix4d> _viewMatrix;
    std::vector<GfMatrix4d> _projectionMatrix;

    GLuint _texture;
    GLuint _framebuffer;

    GLuint _shadowDepthSampler;
    GLuint _shadowCompareSampler;

    GLuint _unbindRestoreDrawFramebuffer;
    GLuint _unbindRestoreReadFramebuffer;
};

#endif
