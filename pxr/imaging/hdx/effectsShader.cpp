//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/effectsShader.h"
#include "pxr/imaging/hdx/hgiConversions.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdxEffectsShader::HdxEffectsShader(
    Hgi* hgi,
    const std::string& debugName)
  : _hgi(hgi)
  , _debugName(debugName.empty() ? "HdxEffectsShader" : debugName)
{
    _pipelineDesc.debugName = _debugName;
    _resourceBindingsDesc.debugName = _debugName;
}

HdxEffectsShader::~HdxEffectsShader()
{
    if (!_hgi) {
        return;
    }

    _DestroyResourceBindings();
    _DestroyPipeline();
}

/* static */
void
HdxEffectsShader::PrintCompileErrors(
    const HgiShaderFunctionHandle& shaderFn)
{
    if (!shaderFn->IsValid()) {
        std::cout << shaderFn->GetCompileErrors() << std::endl;
    }
}

/* static */
void
HdxEffectsShader::PrintCompileErrors(
    const HgiShaderProgramHandle& shaderProgram)
{
    for (HgiShaderFunctionHandle fn : shaderProgram->GetShaderFunctions()) {
        PrintCompileErrors(fn);
    }

    if (!shaderProgram->IsValid()) {
        std::cout << shaderProgram->GetCompileErrors() << std::endl;
    }
}

static
HgiAttachmentDesc
_PartialAttachmentCopy(
    const HgiAttachmentDesc& inDesc)
{
    // Create a copy that ignores those fields in the HgiAttachmentDesc that
    // will come from the associated texture.
    HgiAttachmentDesc outDesc = inDesc;
    outDesc.format = HgiFormatInvalid;
    outDesc.usage = 0;
    return outDesc;
}

static
bool
_MatchesAttachment(
    const HgiAttachmentDesc& oldDesc,
    const HgiAttachmentDesc& newDesc)
{
    const HgiAttachmentDesc oldDescCopy = _PartialAttachmentCopy(oldDesc);
    const HgiAttachmentDesc newDescCopy = _PartialAttachmentCopy(newDesc);

    return oldDescCopy == newDescCopy;
}

static
bool
_MatchesAttachments(
    const HgiAttachmentDescVector& oldDescs,
    const HgiAttachmentDescVector& newDescs)
{
    if (oldDescs.size() != newDescs.size()) {
        return false;
    }

    for (size_t i = 0; i < oldDescs.size(); ++i) {
        if (!_MatchesAttachment(oldDescs[i], newDescs[i])) {
            return false;
        }
    }

    return true;
}

void
HdxEffectsShader::_SetColorAttachments(
    const HgiAttachmentDescVector& colorAttachmentDescs)
{
    if (_MatchesAttachments(_pipelineDesc.colorAttachmentDescs,
                            colorAttachmentDescs)) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.colorAttachmentDescs = colorAttachmentDescs;
}

void
HdxEffectsShader::_SetDepthAttachment(
    const HgiAttachmentDesc& depthAttachmentDesc)
{
    if (_MatchesAttachment(_pipelineDesc.depthAttachmentDesc,
                           depthAttachmentDesc)) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.depthAttachmentDesc = depthAttachmentDesc;
}


void
HdxEffectsShader::_SetPrimitiveType(
    HgiPrimitiveType primitiveType)
{
    if (_pipelineDesc.primitiveType == primitiveType) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.primitiveType = primitiveType;
}

void
HdxEffectsShader::_SetShaderProgram(
    const HgiShaderProgramHandle& shaderProgram)
{
    if (_pipelineDesc.shaderProgram == shaderProgram) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.shaderProgram = shaderProgram;
}


void
HdxEffectsShader::_SetVertexBufferDescs(
    const HgiVertexBufferDescVector& vertexBufferDescs)
{
    if (_pipelineDesc.vertexBuffers == vertexBufferDescs) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.vertexBuffers = vertexBufferDescs;
}

void
HdxEffectsShader::_SetDepthStencilState(
    const HgiDepthStencilState& depthStencilState)
{
    if (_pipelineDesc.depthState == depthStencilState) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.depthState = depthStencilState;
}

static
HgiMultiSampleState
_PartialMultiSampleStateCopy(
    const HgiMultiSampleState& inState)
{
    // Create a copy that ignores those fields in the HgiMultiSampleState that
    // will come from the color and/or depth texture.
    HgiMultiSampleState outState = inState;
    outState.multiSampleEnable = false;
    outState.sampleCount = HgiSampleCount1;
    return outState;
}

void
HdxEffectsShader::_SetMultiSampleState(
    const HgiMultiSampleState& multiSampleState)
{
    const HgiMultiSampleState pipelineMultiSampleStateCopy =
        _PartialMultiSampleStateCopy(_pipelineDesc.multiSampleState);
    const HgiMultiSampleState multiSampleStateCopy =
        _PartialMultiSampleStateCopy(multiSampleState);
    if (pipelineMultiSampleStateCopy == multiSampleStateCopy) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.multiSampleState = multiSampleState;
}

void
HdxEffectsShader::_SetRasterizationState(
    const HgiRasterizationState& rasterizationState)
{
    if (_pipelineDesc.rasterizationState == rasterizationState) {
        return;
    }

    _DestroyPipeline();

    _pipelineDesc.rasterizationState = rasterizationState;
}

void
HdxEffectsShader::_SetShaderConstants(
    uint32_t byteSize,
    const void* data,
    HgiShaderStage stageUsage)
{
    // Check if this constant data requires us to re-create the pipeline.
    if (byteSize != _constantsData.size() ||
        stageUsage != _pipelineDesc.shaderConstantsDesc.stageUsage) {

        _DestroyPipeline();

        _pipelineDesc.shaderConstantsDesc.byteSize = byteSize;
        _pipelineDesc.shaderConstantsDesc.stageUsage = stageUsage;
    }

    // But we always want to grab the new data, even if we didn't have
    // to re-create the pipeline.
    const uint8_t* uint8Data = static_cast<const uint8_t *>(data);
    _constantsData.assign(uint8Data, uint8Data + byteSize);
}

void
HdxEffectsShader::_SetTextureBindings(
    const HgiTextureBindDescVector& textures)
{
    if (_resourceBindingsDesc.textures == textures) {
        return;
    }

    _DestroyResourceBindings();

    _resourceBindingsDesc.textures = textures;
}

void
HdxEffectsShader::_SetBufferBindings(
    const HgiBufferBindDescVector& buffers)
{
    if (_resourceBindingsDesc.buffers == buffers) {
        return;
    }

    _DestroyResourceBindings();

    _resourceBindingsDesc.buffers = buffers;
}

void
HdxEffectsShader::_CreateAndSubmitGraphicsCmds(
    const HgiTextureHandleVector& colorTextures,
    const HgiTextureHandleVector& colorResolveTextures,
    const HgiTextureHandle& depthTexture,
    const HgiTextureHandle& depthResolveTexture,
    const GfVec4i& viewport)
{
    // Ensure the pipeline is ready to be used and the attachment descriptors
    // are correct.
    _CreatePipeline(colorTextures, colorResolveTextures, depthTexture,
        depthResolveTexture);

    // Ensure the resource bindings are ready to be used.
    _CreateResourceBindings();

    // Now we can create the HgiGraphicsCmds
    HgiGraphicsCmdsDesc gfxDesc;
    gfxDesc.colorAttachmentDescs = _pipelineDesc.colorAttachmentDescs;
    gfxDesc.depthAttachmentDesc = _pipelineDesc.depthAttachmentDesc;
    gfxDesc.colorTextures = colorTextures;
    gfxDesc.colorResolveTextures = colorResolveTextures;
    gfxDesc.depthTexture = depthTexture;
    gfxDesc.depthResolveTexture = depthResolveTexture;

    _gfxCmds = _hgi->CreateGraphicsCmds(gfxDesc);

    _gfxCmds->PushDebugGroup(_debugName.c_str());
    _gfxCmds->BindPipeline(_pipeline);
    _gfxCmds->SetViewport(viewport);
    _gfxCmds->BindResources(_resourceBindings);
    if (!_constantsData.empty()) {
        _gfxCmds->SetConstantValues(
            _pipeline, _pipelineDesc.shaderConstantsDesc.stageUsage, 0,
            _constantsData.size(), _constantsData.data());
    }

    // Invoke the sub-class override
    _RecordDrawCmds();

    _gfxCmds->PopDebugGroup();

    _hgi->SubmitCmds(_gfxCmds.get());

    _gfxCmds.reset();
}

void
HdxEffectsShader::_DrawNonIndexed(
    const HgiBufferHandle& vertexBuffer,
    uint32_t vertexCount,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    _gfxCmds->BindVertexBuffers({{vertexBuffer, 0, 0}});

    _gfxCmds->Draw(vertexCount, baseVertex, instanceCount, baseInstance);
}

void
HdxEffectsShader::_DrawIndexed(
    const HgiBufferHandle& vertexBuffer,
    const HgiBufferHandle& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    _gfxCmds->BindVertexBuffers({{vertexBuffer, 0, 0}});

    _gfxCmds->DrawIndexed(indexBuffer, indexCount, indexBufferByteOffset,
                          baseVertex, instanceCount, baseInstance);
}

Hgi*
HdxEffectsShader::_GetHgi() const
{
    return _hgi;
}

void HdxEffectsShader::_DestroyShaderProgram(
    HgiShaderProgramHandle* shaderProgram)
{
    if (!shaderProgram || !(*shaderProgram)) {
        return;
    }

    for (HgiShaderFunctionHandle fn : (*shaderProgram)->GetShaderFunctions()) {
        _hgi->DestroyShaderFunction(&fn);
    }
    _hgi->DestroyShaderProgram(shaderProgram);
}

const std::string&
HdxEffectsShader::_GetDebugName() const
{
    return _debugName;
}

static
bool
_MatchesFormatAndSampleCount(
    const HgiTextureHandle& texture,
    const HgiAttachmentDesc& attachment,
    const HgiSampleCount sampleCount)
{
    if (texture) {
        const HgiTextureDesc& textureDesc = texture->GetDescriptor();
        return attachment.format == textureDesc.format &&
               sampleCount == textureDesc.sampleCount;
    } else {
        return attachment.format == HgiFormatInvalid;
    }
}

static
bool
_MatchesFormatAndSampleCount(
    const HgiTextureHandleVector& textures,
    const HgiAttachmentDescVector& attachments,
    const HgiSampleCount sampleCount)
{
    if (textures.size() != attachments.size()) {
        return false;
    }

    for (size_t i = 0; i < textures.size(); ++i) {
        if (!_MatchesFormatAndSampleCount(textures[i],
                                          attachments[i],
                                          sampleCount)) {
            return false;
        }
    }

    return true;
}

static
void
_UpdateFormatAndUsage(
    const HgiTextureHandle& texture,
    HgiAttachmentDesc* desc)
{
    if (texture) {
        const HgiTextureDesc& texDesc = texture->GetDescriptor();
        desc->format = texDesc.format;
        desc->usage = texDesc.usage;
    } else {
        desc->format = HgiFormatInvalid;
    }
}

static
void
_UpdateFormatAndUsage(
    const HgiTextureHandleVector& textures,
    HgiAttachmentDescVector* descs)
{
    for (size_t i = 0; i < textures.size(); ++i) {
        _UpdateFormatAndUsage(textures[i], &(*descs)[i]);
    }
}

static void
_UpdateSampleCount(
    const HgiTextureHandleVector& colorTextures,
    const HgiTextureHandle& depthTexture,
    HgiGraphicsPipelineDesc* desc)
{
    HgiSampleCount sampleCount = HgiSampleCount1;
    if (!colorTextures.empty()) {
        sampleCount = colorTextures[0]->GetDescriptor().sampleCount;
    } else if (depthTexture) {
        sampleCount = depthTexture->GetDescriptor().sampleCount;
    }
    desc->multiSampleState.sampleCount = sampleCount;
    desc->multiSampleState.multiSampleEnable = sampleCount != HgiSampleCount1;
}

void
HdxEffectsShader::_CreatePipeline(
    const HgiTextureHandleVector& colorTextures,
    const HgiTextureHandleVector& colorResolveTextures,
    const HgiTextureHandle& depthTexture,
    const HgiTextureHandle& depthResolveTexture)
{
    if (_pipeline) {
        const HgiSampleCount sampleCount =
            _pipelineDesc.multiSampleState.sampleCount;
        if (_MatchesFormatAndSampleCount(colorTextures,
                _pipelineDesc.colorAttachmentDescs, sampleCount) &&
            _MatchesFormatAndSampleCount(colorResolveTextures,
                _pipelineDesc.colorAttachmentDescs, HgiSampleCount1) &&
            _MatchesFormatAndSampleCount(depthTexture,
                _pipelineDesc.depthAttachmentDesc, sampleCount) &&
            _MatchesFormatAndSampleCount(depthResolveTexture,
                _pipelineDesc.depthAttachmentDesc, HgiSampleCount1)) {
            return;
        }

        _DestroyPipeline();
    }

    _UpdateSampleCount(colorTextures, depthTexture, &_pipelineDesc);

    _UpdateFormatAndUsage(colorTextures,
                          &_pipelineDesc.colorAttachmentDescs);
    _UpdateFormatAndUsage(depthTexture,
                          &_pipelineDesc.depthAttachmentDesc);
    
    if ((!colorResolveTextures.empty() && colorResolveTextures[0]) ||
        depthResolveTexture) {
        _pipelineDesc.resolveAttachments = true;
    }
    
    _pipeline = _hgi->CreateGraphicsPipeline(_pipelineDesc);
}

void
HdxEffectsShader::_DestroyPipeline()
{
    if (_pipeline) {
        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }
}

void
HdxEffectsShader::_CreateResourceBindings()
{
    if (_resourceBindings) {
        return;
    }

    _resourceBindings = _hgi->CreateResourceBindings(_resourceBindingsDesc);
}

void
HdxEffectsShader::_DestroyResourceBindings()
{
    if (_resourceBindings) {
        _hgi->DestroyResourceBindings(&_resourceBindings);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
