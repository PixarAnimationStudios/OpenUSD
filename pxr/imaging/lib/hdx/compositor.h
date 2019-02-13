//
// Copyright 2018 Pixar
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
#ifndef HDX_COMPOSITOR_H
#define HDX_COMPOSITOR_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/gf/vec2i.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdStGLSLProgram;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;

/// \class HdxCompositor
///
/// This class is a utility for rendering deep raytracer or aov output
/// (color/depth) to the GL framebuffer.  This lets callers composite results
/// into existing scenes.
///
class HdxCompositor {
public:
    /// Create a new compositor object. Creation of GL resources is deferred
    /// until UpdateColor/UpdateDepth/Draw.
    HDX_API
    HdxCompositor();

    /// Destroy the compositor object, releasing GL resources.
    HDX_API
    ~HdxCompositor();

    /// Upload a new color texture for compositing. The data is expected to be
    /// GL_RGBA/GL_UNSIGNED_BYTE.
    ///   \param width The width of the image.
    ///   \param height The height of the image.
    ///   \param data The color data to use while compositing, in GL_RGBA and
    ///               GL_UNSIGNED_BYTE format.
    HDX_API
    void UpdateColor(int width, int height, uint8_t *data);

    /// Upload a new depth texture for compositing. The data is expected to be
    /// GL_R32F.
    ///   \param width The width of the image.
    ///   \param height The height of the image.
    ///   \param data The depth data to use while compositing, in GL_R32F.
    HDX_API
    void UpdateDepth(int width, int height, uint8_t *data);

    /// Draw the internal color/depth buffers to the bound framebuffer.
    /// This will load the GLSL compositing program on-demand.
    HDX_API
    void Draw();

    /// Draw the provided color/depth buffers to the bound framebuffer.
    /// This will load the GLSL compositing program on-demand.
    HDX_API
    void Draw(GLuint colorId, GLuint depthId, bool remapDepth);

private:
    // Utility function to create a GL texture.
    void _CreateTextureResources(GLuint *texture);
    // Utility function to create a GL program using the compositor source.
    void _CreateShaderResources(bool useDepthProgram);
    // Utility function to create buffer resources.
    void _CreateBufferResources();

    GLuint _colorTexture;
    GLuint _depthTexture;

    HdStGLSLProgramSharedPtr _compositorProgram;
    GLint _locations[5];
    GLuint _vertexBuffer;
    bool _useDepthProgram;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_COMPOSITOR_H
