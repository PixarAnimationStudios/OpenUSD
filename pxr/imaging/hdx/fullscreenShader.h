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
#ifndef PXR_IMAGING_HDX_FULLSCREENSHADER_H
#define PXR_IMAGING_HDX_FULLSCREENSHADER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/gf/vec2i.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdStGLSLProgram;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;

/// \class HdxFullscreenShader
///
/// This class is a utility for rendering deep raytracer or aov output
/// (color/depth) to the GL framebuffer.  This lets callers composite results
/// into existing scenes.
///
class HdxFullscreenShader {
public:
    /// Create a new fullscreen shader object. Creation of GL resources is
    /// deferred...
    HDX_API
    HdxFullscreenShader();

    /// Destroy the fullscreen shader object, releasing GL resources.
    HDX_API
    ~HdxFullscreenShader();

    /// Set the program for the class to use for its fragment shader.
    /// The vertex shader is always hdx/shaders/fullscreen.glslfx,
    /// "FullScreenVertex", which draws a full-screen triangle.
    /// The fragment shader should expect a varying called "uv", and
    /// whatever textures or uniforms have been passed in by the caller.
    ///   \param glslfx The name of the glslfx file where the fragment shader
    ///                 is located.
    ///   \param technique The technique name of the fragment shader.
    HDX_API
    void SetProgram(TfToken const& glslfx, TfToken const& technique);

    /// Set the program for the class to use the built-in
    /// compositing shader, optionally with depth support.
    ///   \param depthAware Whether the compositor should expect a depth buffer.
    HDX_API
    void SetProgramToCompositor(bool depthAware);

    /// Add a value to be bound as a uniform.  Currently supported are:
    /// - float, int, unsigned int
    /// - GfVec[2-4]i, f
    /// - GfMatrix[2-4]f
    /// - Arrays of the above.
    /// If the VtValue is empty, the binding is removed.
    ///   \param name The GLSL name of the uniform binding.
    ///   \param value The data to be bound.
    HDX_API
    void SetUniform(TfToken const& name, VtValue const& data);

    /// Upload a named texture with a given format. These textures will
    /// be used by Draw() called with no arguments. If width == 0,
    /// height == 0, or data == nullptr, the image is removed from the bindings.
    ///   \param name The name of the texture (used to look up GLSL binding).
    ///   \param width The width of the image.
    ///   \param height The height of the image.
    ///   \param format The data format. Currently supported are:
    ///                 F32x4, F16x4, Unorm8x4, F32x1.
    ///   \param data The image data to upload.
    HDX_API
    void SetTexture(TfToken const& name, int width, int height,
                       HdFormat format, void *data);

    /// Draw the internal textures to the bound framebuffer.
    /// This will load the GLSL compositing program on-demand.
    HDX_API
    void Draw();

    typedef std::map<TfToken, GLuint> TextureMap;

    /// Draw the provided textures to the bound framebuffer.
    /// This will load the GLSL program on-demand.
    HDX_API
    void Draw(TextureMap const& textures);

private:
    // Utility function to create a GL texture.
    void _CreateTextureResources(GLuint *texture);
    // Utility function to create buffer resources.
    void _CreateBufferResources();
    // Utility function to set uniform values.
    void _SetUniform(GLuint programId,
                     TfToken const& name, VtValue const& value);

    typedef std::map<TfToken, VtValue> UniformMap;

    UniformMap _uniforms;
    TextureMap _textures;

    HdStGLSLProgramSharedPtr _program;
    TfToken _glslfx;
    TfToken _technique;
    GLuint _vertexBuffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_FULLSCREENSHADER_H
