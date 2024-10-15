//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/hgiConversions.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((fullscreenVertex,              "FullscreenVertex"))
    ((compositeFragmentNoDepth,      "CompositeFragmentNoDepth"))
    ((compositeFragmentWithDepth,    "CompositeFragmentWithDepth"))
    (fullscreenShader)
);

HdxFullscreenShader::HdxFullscreenShader(
    Hgi* hgi,
    const std::string& debugName)
  : HdxEffectsShader(hgi, debugName.empty() ? "HdxFullscreenShader" : debugName)
{
    // Depth test and write must be on since we may want to transfer depth.
    // Depth test must be on because when off it also disables depth writes.
    // Instead we set the compare function to always.
    _depthStencilState.depthTestEnabled = true;
    _depthStencilState.depthCompareFn = HgiCompareFunctionAlways;

    // We don't use the stencil mask in this task.
    _depthStencilState.stencilTestEnabled = false;

    // Set attachment defaults for load and store
    _colorAttachment.loadOp = HgiAttachmentLoadOpDontCare;
    _colorAttachment.storeOp = HgiAttachmentStoreOpStore;
    _depthAttachment.loadOp = HgiAttachmentLoadOpDontCare;
    _depthAttachment.storeOp = HgiAttachmentStoreOpStore;

    // Alpha to coverage would prevent any pixels that have an alpha of 0.0 from
    // being written. We want to transfer all pixels. Even background
    // pixels that were set with a clearColor alpha of 0.0.
    HgiMultiSampleState multiSampleState;
    multiSampleState.alphaToCoverageEnable = false;
    _SetMultiSampleState(multiSampleState);

    // Setup rasterization state
    HgiRasterizationState rasterizationState;
    rasterizationState.cullMode = HgiCullModeBack;
    rasterizationState.polygonMode = HgiPolygonModeFill;
    rasterizationState.winding = HgiWindingCounterClockwise;
    _SetRasterizationState(rasterizationState);
}

HdxFullscreenShader::~HdxFullscreenShader()
{
    Hgi* hgi = _GetHgi();
    if (!hgi) {
        return;
    }

    if (_shaderProgram) {
        _DestroyShaderProgram(&_shaderProgram);
    }

    if (_defaultSampler) {
        hgi->DestroySampler(&_defaultSampler);
    }
}

void
HdxFullscreenShader::SetProgram(
    TfToken const& glslfxPath,
    TfToken const& shaderName,
    HgiShaderFunctionDesc &fragDesc)
{
    if (_glslfxPath == glslfxPath && _shaderName == shaderName) {
        return;
    }

    const HioGlslfx fragGlslfx(glslfxPath);
    std::string reason;
    if (!fragGlslfx.IsValid(&reason)) {
        TF_CODING_ERROR("Couldn't load fragment shader %s, error: %s",
            fragGlslfx.GetFilePath().c_str(),
            reason.c_str());
    } else {
        _glslfxPath = glslfxPath;
        _shaderName = shaderName;

        const std::string fsCode = fragGlslfx.GetSource(_shaderName);
        TF_VERIFY(!fsCode.empty());
        fragDesc.shaderCode = fsCode.c_str();

        SetProgram(fragDesc);

        fragDesc.shaderCode = nullptr;
    }
}

void
HdxFullscreenShader::SetProgram(
    const HgiShaderFunctionDesc& fragDesc)
{
    _DestroyShaderProgram(&_shaderProgram);

    // Set up the vertex shader
    const HioGlslfx vertGlslfx(HdxPackageFullscreenShader());
    std::string reason;
    if (!vertGlslfx.IsValid(&reason)) {
        TF_CODING_ERROR("Couldn't load vertex shader %s, error: %s",
            vertGlslfx.GetFilePath().c_str(),
            reason.c_str());
        return;
    }

    HgiShaderFunctionDesc vertDesc;
    vertDesc.debugName = _tokens->fullscreenVertex;
    vertDesc.shaderStage = HgiShaderStageVertex;
    HgiShaderFunctionAddStageInput(
            &vertDesc, "hd_VertexID", "uint",
            HgiShaderKeywordTokens->hdVertexID);
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "gl_Position", "vec4", "position");
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "uvOut", "vec2");

    const std::string vsCode = vertGlslfx.GetSource(_tokens->fullscreenVertex);
    TF_VERIFY(!vsCode.empty());
    vertDesc.shaderCode = vsCode.c_str();
    HgiShaderFunctionHandle vertFn = _GetHgi()->CreateShaderFunction(vertDesc);

    // Create up the fragment shader
    HgiShaderFunctionHandle fragFn = _GetHgi()->CreateShaderFunction(fragDesc);

    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName = _tokens->fullscreenShader.GetString();
    programDesc.shaderFunctions.push_back(std::move(vertFn));
    programDesc.shaderFunctions.push_back(std::move(fragFn));
    _shaderProgram = _GetHgi()->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create HdxFullscreenShader shader program");
        HdxEffectsShader::PrintCompileErrors(_shaderProgram);
        _DestroyShaderProgram(&_shaderProgram);
    }

    // Set the shader program to either a valid program or an empty handle.
    _SetShaderProgram(_shaderProgram);
}

void
HdxFullscreenShader::BindBuffers(
    HgiBufferHandleVector const& buffers)
{
    _buffers = buffers;
}

void
HdxFullscreenShader::SetDepthState(HgiDepthStencilState const& state)
{
    _depthStencilState = state;
}

void
HdxFullscreenShader::SetBlendState(
    bool enableBlending,
    HgiBlendFactor srcColorBlendFactor,
    HgiBlendFactor dstColorBlendFactor,
    HgiBlendOp colorBlendOp,
    HgiBlendFactor srcAlphaBlendFactor,
    HgiBlendFactor dstAlphaBlendFactor,
    HgiBlendOp alphaBlendOp)
{
    _colorAttachment.blendEnabled = enableBlending;
    _colorAttachment.srcColorBlendFactor = srcColorBlendFactor;
    _colorAttachment.dstColorBlendFactor = dstColorBlendFactor;
    _colorAttachment.colorBlendOp = colorBlendOp;
    _colorAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
    _colorAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
    _colorAttachment.alphaBlendOp = alphaBlendOp;
}

void
HdxFullscreenShader::SetAttachmentLoadStoreOp(
    HgiAttachmentLoadOp attachmentLoadOp,
    HgiAttachmentStoreOp attachmentStoreOp)
{
    _colorAttachment.loadOp = attachmentLoadOp;
    _colorAttachment.storeOp = attachmentStoreOp;
}

void
HdxFullscreenShader::SetShaderConstants(
    uint32_t byteSize,
    const void* data)
{
    _SetShaderConstants(byteSize, data, HgiShaderStageFragment);
}

void
HdxFullscreenShader::BindTextures(
    HgiTextureHandleVector const& textures,
    HgiSamplerHandleVector const& samplers)
{
    if (!samplers.empty() && textures.size() != samplers.size()) {
        TF_CODING_ERROR("Samplers vector must be empty, or match the size of "
                        "the provided textures vector.");
        _textures.clear();
        _samplers.clear();
        return;
    }

    _textures = textures;
    if (samplers.empty()) {
        _samplers.assign(_textures.size(), _GetDefaultSampler());
    } else {
        _samplers.resize(samplers.size());
        for (size_t i = 0; i < samplers.size(); ++i) {
            _samplers[i] = samplers[i] ? samplers[i] : _GetDefaultSampler();
        }
    }
}

void
HdxFullscreenShader::_SetResourceBindings()
{
    HgiTextureBindDescVector textureBindings;
    textureBindings.reserve(_textures.size());
    for (uint32_t index = 0; index < _textures.size(); ++index) {
        const HgiTextureHandle& texture = _textures[index];
        if (!texture) {
            continue;
        }
        HgiTextureBindDesc texBind;
        texBind.bindingIndex = index;
        texBind.stageUsage = HgiShaderStageFragment;
        texBind.writable = false;
        texBind.textures.push_back(texture);
        texBind.samplers.push_back(_samplers[index]);
        textureBindings.push_back(std::move(texBind));
    }
    _SetTextureBindings(textureBindings);

    HgiBufferBindDescVector bufferBindings;
    bufferBindings.reserve(_buffers.size());
    for (uint32_t index = 0; index < _buffers.size(); ++index) {
        const HgiBufferHandle& buffer = _buffers[index];
        if (!buffer) {
            continue;
        }
        HgiBufferBindDesc bufBind;
        bufBind.bindingIndex = index;
        bufBind.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind.stageUsage = HgiShaderStageFragment;
        bufBind.writable = false;
        bufBind.offsets.push_back(0);
        bufBind.buffers.push_back(buffer);
        bufferBindings.push_back(std::move(bufBind));
    }
    _SetBufferBindings(bufferBindings);
}

HgiSamplerHandle
HdxFullscreenShader::_GetDefaultSampler()
{
    if (!_defaultSampler) {
        HgiSamplerDesc sampDesc;
        sampDesc.magFilter = HgiSamplerFilterLinear;
        sampDesc.minFilter = HgiSamplerFilterLinear;
        sampDesc.addressModeU = HgiSamplerAddressModeClampToEdge;
        sampDesc.addressModeV = HgiSamplerAddressModeClampToEdge;
        _defaultSampler = _GetHgi()->CreateSampler(sampDesc);
    }

    return _defaultSampler;
}

void
HdxFullscreenShader::Draw(
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& depthDst)
{
    if (!colorDst) {
        TF_CODING_ERROR("Color texture must be provided.");
        return;
    }

    const GfVec3i dimensions = colorDst->GetDescriptor().dimensions;
    const GfVec4i viewport(0, 0, dimensions[0], dimensions[1]);
    _Draw(colorDst, {}, depthDst, {}, viewport);
}

void
HdxFullscreenShader::Draw(
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& colorResolveDst,
    HgiTextureHandle const& depthDst,
    HgiTextureHandle const& depthResolveDst,
    GfVec4i const& viewport)
{
    _Draw(colorDst, colorResolveDst, depthDst, depthResolveDst, viewport);
}

void
HdxFullscreenShader::_SetDefaultProgram(
    bool writeDepth)
{
    const TfToken& fragShader =
        writeDepth ? _tokens->compositeFragmentWithDepth :
                     _tokens->compositeFragmentNoDepth;

    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = fragShader.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn", /*bindIndex = */0);


    if (writeDepth) {
        HgiShaderFunctionAddStageOutput(
            &fragDesc, "gl_FragDepth", "float", "depth(any)");
        HgiShaderFunctionAddTexture(
            &fragDesc, "depth", /*bindIndex = */1);
    }

    SetProgram(HdxPackageFullscreenShader(), fragShader, fragDesc);
}

void 
HdxFullscreenShader::_Draw(
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& colorResolveDst,
    HgiTextureHandle const& depthDst,
    HgiTextureHandle const& depthResolveDst,
    GfVec4i const &viewport)
{
    const bool writeDepth(depthDst);

    // If the user has not set a custom shader program, pick default program.
    if (!_shaderProgram) {
        _SetDefaultProgram(writeDepth);
    }

    _SetPrimitiveType(HgiPrimitiveTypeTriangleList);

    // Set or update the resource bindings (textures may have changed)
    _SetResourceBindings();

    const HgiAttachmentDescVector colorAttachmentDescs(1, _colorAttachment);
    _SetColorAttachments(colorAttachmentDescs);
    _SetDepthAttachment(_depthAttachment);

    _depthStencilState.depthWriteEnabled = writeDepth;
    _SetDepthStencilState(_depthStencilState);

    HgiTextureHandleVector colorTextures;
    if (colorDst) {
        colorTextures.push_back(colorDst);
    }
    HgiTextureHandleVector colorResolveTextures;
    if (colorResolveDst) {
        colorResolveTextures.push_back(colorResolveDst);
    }

    HdxEffectsShader::_CreateAndSubmitGraphicsCmds(
        colorTextures, colorResolveTextures,
        depthDst, depthResolveDst,
        viewport);
}

void
HdxFullscreenShader::_RecordDrawCmds()
{
    _DrawWithoutVertexBuffer(3, 0, 1, 0);
}

PXR_NAMESPACE_CLOSE_SCOPE
