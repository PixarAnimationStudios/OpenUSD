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
#include "pxr/base/gf/vec2i.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

/// \class HdxFullscreenShader
///
/// This class is a utility for rendering deep raytracer or aov output
/// (color/depth) to a hgi texture. This lets callers composite results
/// into existing scenes.
///
class HdxFullscreenShader
{
public:
    /// Create a new fullscreen shader object.
    /// 'debugName' is assigned to the fullscreen pass as gpu debug group that
    /// is helpful when inspecting the frame on a gpu debugger.
    HDX_API
    HdxFullscreenShader(Hgi* hgi, std::string const& debugName);

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
    ///   \param shaderName The (technique) name of the fragment shader.
    ///   \param vertDesc Describes inputs, outputs and stage of vertex shader.
    ///   \param fragDesc Describes inputs, outputs and stage of fragment shader.
    HDX_API
    void SetProgram(
        TfToken const& glslfx,
        TfToken const& shaderName,
        HgiShaderFunctionDesc &fragDesc,
        HgiShaderFunctionDesc vertDesc = GetFullScreenVertexDesc()
        );

    /// Bind a (externally managed) buffer to the shader program.
    /// This function can be used to bind buffers to a custom shader program.
    /// The lifetime of the buffer is managed by the caller. HdxFullscreenShader
    /// does not take ownership. To update values in the buffer, the client can
    /// use a blitCmds to copy new data into their buffer.
    /// If an invalid 'buffer' is passed, the binding will be cleared.
    HDX_API
    void BindBuffer(HgiBufferHandle const& buffer, uint32_t bindingIndex);

    /// Bind (externally managed) textures to the shader program.
    /// This function can be used to bind textures to a custom shader program.
    /// The lifetime of textures is managed by the caller. HdxFullscreenShader
    /// does not take ownership.
    /// If an invalid 'texture' is passed, the binding will be cleared.
    HDX_API
    void BindTextures(
        TfTokenVector const& names,
        HgiTextureHandleVector const& textures);

    /// By default HdxFullscreenShader creates a pipeline object that enables
    /// depth testing and enables depth write if there is a depth texture.
    /// This function allows you to override the depth and stencil state.
    HDX_API
    void SetDepthState(HgiDepthStencilState const& state);

    /// By default HdxFullscreenShader uses no blending (opaque).
    /// This function allows you to override blend state (e.g. alpha blending)
    HDX_API
    void SetBlendState(
        bool enableBlending,
        HgiBlendFactor srcColorBlendFactor,
        HgiBlendFactor dstColorBlendFactor,
        HgiBlendOp colorBlendOp,
        HgiBlendFactor srcAlphaBlendFactor,
        HgiBlendFactor dstAlphaBlendFactor,
        HgiBlendOp alphaBlendOp);

    /// Provide the shader constant values (uniforms).
    /// The data values are copied, so you do not have to set them
    /// each frame if they do not change in value.
    HDX_API
    void SetShaderConstants(
        uint32_t byteSize,
        const void* data);

    /// Draw the internal textures to the provided destination textures.
    /// `depth` is optional.
    HDX_API
    void Draw(HgiTextureHandle const& colorDst,
              HgiTextureHandle const& depthDst);

private:
    HdxFullscreenShader() = delete;

    using TextureMap = std::map<TfToken, HgiTextureHandle>;
    using BufferMap = std::map<uint32_t, HgiBufferHandle>;

    // Utility function to create buffer resources.
    void _CreateBufferResources();

    // Destroy shader program and the shader functions it holds.
    void _DestroyShaderProgram();

    // Utility to create resource bindings
    bool _CreateResourceBindings(TextureMap const& textures);

    // Utility to create default vertex buffer descriptor
    void _CreateVertexBufferDescriptor();

    // Utility to create a pipeline
    bool _CreatePipeline(
        HgiTextureHandle const& colorDst,
        HgiTextureHandle const& depthDst,
        bool depthWrite);

    // Utility to create a texture sampler
    bool _CreateSampler();

    // Internal draw method
    void _Draw(TextureMap const& textures, 
              HgiTextureHandle const& colorDst,
              HgiTextureHandle const& depthDst,
              bool depthWrite);
    
    static HgiShaderFunctionDesc GetFullScreenVertexDesc();

    // Print shader compile errors.
    void _PrintCompileErrors();

    class Hgi* _hgi;

    std::string _debugName;

    TextureMap _textures;
    BufferMap _buffers;

    TfToken _glslfx;
    TfToken _shaderName;

    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;
    HgiShaderProgramHandle _shaderProgram;
    HgiResourceBindingsHandle _resourceBindings;
    HgiGraphicsPipelineHandle _pipeline;
    HgiSamplerHandle _sampler;
    HgiVertexBufferDesc _vboDesc;

    HgiDepthStencilState _depthState;

    bool _blendingEnabled;
    HgiBlendFactor _srcColorBlendFactor;
    HgiBlendFactor _dstColorBlendFactor;
    HgiBlendOp _colorBlendOp;
    HgiBlendFactor _srcAlphaBlendFactor;
    HgiBlendFactor _dstAlphaBlendFactor;
    HgiBlendOp _alphaBlendOp;

    HgiAttachmentDesc _attachment0;
    HgiAttachmentDesc _depthAttachment;

    std::vector<uint8_t> _constantsData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_FULLSCREENSHADER_H
