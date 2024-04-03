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
#include "pxr/imaging/hdx/effectsShader.h"

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

#include "pxr/base/gf/vec4i.h"

#include "pxr/base/tf/token.h"

#include <map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HioGlslfx;

/// \class HdxFullscreenShader
///
/// This class is a utility for rendering deep raytracer or aov output
/// (color/depth) to a hgi texture. This lets callers composite results
/// into existing scenes.
///
class HdxFullscreenShader : public HdxEffectsShader
{
public:
    /// Create a new fullscreen shader object.
    /// 'debugName' is assigned to the fullscreen pass as gpu debug group that
    /// is helpful when inspecting the frame on a gpu debugger.
    HDX_API
    HdxFullscreenShader(Hgi* hgi, const std::string& debugName);

    /// Destroy the fullscreen shader object, releasing GPU resources.
    HDX_API
    ~HdxFullscreenShader() override;

    /// Set the program for the class to use for its fragment shader.
    /// The vertex shader is always hdx/shaders/fullscreen.glslfx,
    /// "FullScreenVertex", which draws a full-screen triangle.
    /// The fragment shader should expect an interpolated input parameter with
    /// the name "uvOut", and whatever textures, constants, or buffers it
    /// requires.
    ///   \param glslfxPath The path to the glslfx file where the fragment
    ///                     shader is located.
    ///   \param shaderName The (technique) name of the fragment shader.
    ///   \param fragDesc Describes inputs, outputs and stage of fragment
    ///                   shader.
    HDX_API
    void SetProgram(
        const TfToken& glslfxPath,
        const TfToken& shaderName,
        HgiShaderFunctionDesc& fragDesc);

    /// Bypasses any cache checking or HioGlslfx processing and just re-creates
    /// the shader program using the "FullScreenVertex" shader and the provided
    /// fragment shader description.
    HDX_API
    void SetProgram(
        const HgiShaderFunctionDesc& fragDesc);

    /// Bind (externally managed) buffers to the shader program.
    /// This function can be used to bind buffers to a custom shader program.
    /// The lifetime of buffers is managed by the caller. HdxFullscreenShader
    /// does not take ownership. To update values in the buffer, the client can
    /// use a blitCmds to copy new data into their buffer.
    /// Buffers will be bound at the indices corresponding to their position in
    /// the provided vector.
    HDX_API
    void BindBuffers(HgiBufferHandleVector const& buffers);

    /// Bind (externally managed) textures to the shader program.
    /// This function can be used to bind textures to a custom shader program.
    /// The lifetime of textures is managed by the caller. HdxFullscreenShader
    /// does not take ownership.
    /// Textures will be bound at the indices corresponding to their position in
    /// the provided vector.
    HDX_API
    void BindTextures(
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

    /// By default HdxFullscreenShader uses LoadOpDontCare and StoreOpStore.
    /// This function allows you to override the attachment load and store op.
    HDX_API
    void SetAttachmentLoadStoreOp(
        HgiAttachmentLoadOp attachmentLoadOp,
        HgiAttachmentStoreOp attachmentStoreOp);

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

    HDX_API
    void Draw(HgiTextureHandle const& colorDst,
              HgiTextureHandle const& colorResolveDst,
              HgiTextureHandle const& depthDst,
              HgiTextureHandle const& depthResolveDst,
              GfVec4i const& viewport);

private:
    HdxFullscreenShader() = delete;
    HdxFullscreenShader(const HdxFullscreenShader&) = delete;
    HdxFullscreenShader& operator=(const HdxFullscreenShader&) = delete;

    // Utility function to create buffer resources.
    void _CreateBufferResources();

    // Utility to set resource bindings.
    void _SetResourceBindings();

    // Utility to create default vertex buffer descriptor.
    void _SetVertexBufferDescriptor();

    // Utility to create a texture sampler.
    bool _CreateSampler();

    // Utility to set the default program.
    void _SetDefaultProgram(bool writeDepth);

    // Internal draw method
    void _Draw(
        HgiTextureHandle const& colorDst,
        HgiTextureHandle const& colorResolveDst,
        HgiTextureHandle const& depthDst,
        HgiTextureHandle const& depthResolveDst,
        GfVec4i const &viewport);

    void _RecordDrawCmds() override;

    // Print shader compile errors.
    void _PrintCompileErrors();

    HgiTextureHandleVector _textures;
    HgiBufferHandleVector _buffers;

    TfToken _glslfxPath;
    TfToken _shaderName;

    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;
    HgiShaderProgramHandle _shaderProgram;
    HgiSamplerHandle _sampler;

    HgiDepthStencilState _depthStencilState;

    HgiAttachmentDesc _colorAttachment;
    HgiAttachmentDesc _depthAttachment;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_FULLSCREENSHADER_H
