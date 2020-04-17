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
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/pipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

/// \class HdxFullscreenShader
///
/// This class is a utility for rendering deep raytracer or aov output
/// (color/depth) to a hgi texture. This lets callers composite results
/// into existing scenes.
///
class HdxFullscreenShader {
public:
    typedef std::map<TfToken, HgiTextureHandle> TextureMap;
    typedef std::map<uint32_t, HgiBufferHandle> BufferMap;

    /// Create a new fullscreen shader object. Creation of GPU resources is
    /// deferred...
    /// 'debugName' is assigned to the fullscreen pass as gpu debug group that
    /// is helpful when inspecting the frame on a gpu debugger.
    HDX_API
    HdxFullscreenShader(
        Hgi* hgi,
        std::string const& debugName);

    /// Destroy the fullscreen shader object, releasing GPU resources.
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

    /// Bind a (externally managed) buffer to the shader program.
    /// This function can be used to bind buffers to a custom shader program.
    /// The lifetime of the buffer is managed by the caller. HdxFullscreenShader
    /// does not take ownership. To update values in the buffer, the client can
    /// use a blitCmds to copy new data into their buffer.
    /// If an invalid 'buffer' is passed, the binding will be cleared.
    HDX_API
    void SetBuffer(HgiBufferHandle const& buffer, uint32_t bindingIndex);

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

    /// Customize the pipeline state, such as setting the blend mode that will
    /// by used when rendering the triangle. Note that the ShaderProgram and
    /// ResourceBindings in the provided descriptor are ignored. Those are
    /// internally managed based on SetProgram, SetBuffers and SetTextures.
    HDX_API
    void CreatePipeline(HgiPipelineDesc pipeDesc);

    /// Customize the blend state that is used during draw.
    HDX_API
    void SetBlendState(
        bool enableBlending,
        HgiBlendFactor srcColorBlendFactor,
        HgiBlendFactor dstColorBlendFactor,
        HgiBlendOp colorBlendOp,
        HgiBlendFactor srcAlphaBlendFactor,
        HgiBlendFactor dstAlphaBlendFactor,
        HgiBlendOp alphaBlendOp);

    /// Draw the internal textures to the provided destination textures.
    /// `depth` is optional.
    HDX_API
    void Draw(HgiTextureHandle const& colorDst,
              HgiTextureHandle const& depthDst);

    /// Draw the provided textures into the provided destination textures.
    /// `depth` is optional.
    HDX_API
    void Draw(TextureMap const& textures, 
              HgiTextureHandle const& colorDst,
              HgiTextureHandle const& depthDst);

protected:
    // XXX We don't want tasks to use DrawToFramebuffer, but during hgi transition
    // we need to make a few exceptions.
    friend class HdxPresentTask;
    friend class HdxColorizeTask;

    /// Draw the internal textures to the global framebuffer.
    /// This API exists to help with Hgi transition to let the PresentTask
    /// Draw directly to the gl framebuffer. In the future this will be
    /// handled by HgiInterop.
    HDX_API
    void DrawToFramebuffer(TextureMap const& textures = TextureMap());

private:
    HdxFullscreenShader() = delete;

    // Utility function to create buffer resources.
    void _CreateBufferResources();

    // Destroy shader program and the shader functions it holds.
    void _DestroyShaderProgram();

    // Utility to create resource bindings
    bool _CreateResourceBindings(TextureMap const& textures);

    // Utility to create a pipeline
    bool _CreateDefaultPipeline(
        HgiTextureHandle const& colorDst,
        HgiTextureHandle const& depthDst,
        bool depthWrite);

    // Internal draw method
    void _Draw(TextureMap const& textures, 
              HgiTextureHandle const& colorDst,
              HgiTextureHandle const& depthDst,
              bool depthWrite);

    // Print shader compile errors.
    void _PrintCompileErrors();

    class Hgi* _hgi;

    std::string _debugName;

    TextureMap _textures;
    BufferMap _buffers;

    TfToken _glslfx;
    TfToken _technique;

    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;
    HgiShaderProgramHandle _shaderProgram;
    HgiResourceBindingsHandle _resourceBindings;
    HgiPipelineHandle _pipeline;

    bool _blendingEnabled;
    HgiBlendFactor _srcColorBlendFactor;
    HgiBlendFactor _dstColorBlendFactor;
    HgiBlendOp _colorBlendOp;
    HgiBlendFactor _srcAlphaBlendFactor;
    HgiBlendFactor _dstAlphaBlendFactor;
    HgiBlendOp _alphaBlendOp;

    HgiAttachmentDesc _attachment0;
    HgiAttachmentDesc _depthAttachment;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_FULLSCREENSHADER_H
