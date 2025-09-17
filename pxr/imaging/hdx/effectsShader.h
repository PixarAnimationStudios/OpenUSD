//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_EFFECTS_SHADER_H
#define PXR_IMAGING_HDX_EFFECTS_SHADER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

#include "pxr/base/gf/vec4i.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

using HgiGraphicsCmdsUniquePtr = std::unique_ptr<class HgiGraphicsCmds>;


/// \class HdxEffectsShader
///
/// This class provides functionality to create and manage a single
/// HgiGraphicsPipeline instance and to issue draw calls to that instance.
///
/// Sub-classes should define the actual interface for issuing the draw call
/// leveraging the common functionality this class provides to facilitate that.
///
/// It is primarily intended to be used for full screen passes that perform a
/// sceen-space effect.  As an example, the HdxFullscreenShader class inherits
/// from this class and makes use of the functions defined here to set up its
/// pipeline and issue draw commands.
///
class HdxEffectsShader
{
public:
    HDX_API
    virtual ~HdxEffectsShader();

    // Print shader compile errors for the shader function.
    static void PrintCompileErrors(
        const HgiShaderFunctionHandle& shaderFn);

    // Print shader compile errors for the shader program and any functions
    // it references.
    static void PrintCompileErrors(
        const HgiShaderProgramHandle& shaderProgram);

protected:
    HdxEffectsShader() = delete;
    HdxEffectsShader(const HdxEffectsShader&) = delete;
    HdxEffectsShader& operator=(const HdxEffectsShader&) = delete;

    /// Create a new shader object.
    ///   \param hgi Hgi instance to use to create any GPU resources.
    ///   \param debugName Name used to tag GPU resources to aid in debugging.
    HDX_API
    HdxEffectsShader(
        Hgi* hgi,
        const std::string& debugName);

    HDX_API
    void _SetColorAttachments(
        const HgiAttachmentDescVector& colorAttachmentDescs);

    HDX_API
    void _SetDepthAttachment(
        const HgiAttachmentDesc& depthAttachmentDesc);

    HDX_API
    void _SetPrimitiveType(
        HgiPrimitiveType primitiveType);

    HDX_API
    void _SetShaderProgram(
        const HgiShaderProgramHandle& shaderProgram);

    HDX_API
    void _SetVertexBufferDescs(
        const HgiVertexBufferDescVector& vertexBufferDescs);

    HDX_API
    void _SetDepthStencilState(
        const HgiDepthStencilState& depthStencilState);

    HDX_API
    void _SetMultiSampleState(
        const HgiMultiSampleState& multiSampleState);

    HDX_API
    void _SetRasterizationState(
        const HgiRasterizationState& rasterizationState);

    HDX_API
    void _SetShaderConstants(
        uint32_t byteSize,
        const void* data,
        HgiShaderStage stageUsage);

    HDX_API
    void _SetTextureBindings(
        const HgiTextureBindDescVector& textures);

    HDX_API
    void _SetBufferBindings(
        const HgiBufferBindDescVector& buffers);

    // Creates a graphics commands object, records draw commands to it via the
    // overridden _RecordDrawCmds, and then submits them.
    HDX_API
    void _CreateAndSubmitGraphicsCmds(
        const HgiTextureHandleVector& colorTextures,
        const HgiTextureHandleVector& colorResolveTextures,
        const HgiTextureHandle& depthTexture,
        const HgiTextureHandle& depthResolveTexture,
        const GfVec4i& viewport);

    // Sub-classes should override and invoke one or more calls to
    // _DrawNonIndexed or _DrawIndexed.
    HDX_API
    virtual void _RecordDrawCmds() = 0;

    // Sets the vertex buffer and invokes HgiGraphicsCmds::Draw.
    HDX_API
    void _DrawNonIndexed(
        const HgiBufferHandle& vertexBuffer,
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance);

    // Sets the vertex buffer and invokes HgiGraphicsCmds::DrawIndexed with the
    // provided index buffer.
    HDX_API
    void _DrawIndexed(
        const HgiBufferHandle& vertexBuffer,
        const HgiBufferHandle& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance);

    HDX_API
    Hgi* _GetHgi() const;

    HDX_API
    void _DestroyShaderProgram(HgiShaderProgramHandle* shaderProgram);

    HDX_API
    const std::string& _GetDebugName() const;

private:
    void _CreatePipeline(
        const HgiTextureHandleVector& colorTextures,
        const HgiTextureHandleVector& colorResolveTextures,
        const HgiTextureHandle& depthTexture,
        const HgiTextureHandle& depthResolveTexture);
    void _DestroyPipeline();

    void _CreateResourceBindings();
    void _DestroyResourceBindings();

    Hgi* _hgi;
    const std::string _debugName;
    HgiGraphicsPipelineDesc _pipelineDesc;
    HgiGraphicsPipelineHandle _pipeline;
    std::vector<uint8_t> _constantsData;
    HgiResourceBindingsDesc _resourceBindingsDesc;
    HgiResourceBindingsHandle _resourceBindings;
    HgiGraphicsCmdsUniquePtr _gfxCmds;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_EFFECTS_SHADER_H
