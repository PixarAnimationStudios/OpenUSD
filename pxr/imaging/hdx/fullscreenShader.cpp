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
#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/hgiConversions.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <iostream>

// XXX Remove includes when entire task is using Hgi. We do not want to refer
// to any specific Hgi implementation.
#include "pxr/imaging/hgiGL/pipeline.h"

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
    std::string const& debugName)
    : _hgi(hgi)
    , _debugName(debugName)
    , _indexBuffer()
    , _vertexBuffer()
    , _shaderProgram()
    , _resourceBindings()
    , _pipeline()
    , _blendingEnabled(false)
    , _srcColorBlendFactor(HgiBlendFactorZero)
    , _dstColorBlendFactor(HgiBlendFactorZero)
    , _colorBlendOp(HgiBlendOpAdd)
    , _srcAlphaBlendFactor(HgiBlendFactorZero)
    , _dstAlphaBlendFactor(HgiBlendFactorZero)
    , _alphaBlendOp(HgiBlendOpAdd)
{
    if (_debugName.empty()) {
        _debugName = "HdxFullscreenShader";
    }
}

HdxFullscreenShader::~HdxFullscreenShader()
{
    if (!_hgi) {
        return;
    }

    if (_vertexBuffer) {
        _hgi->DestroyBuffer(&_vertexBuffer);
    }

    if (_indexBuffer) {
        _hgi->DestroyBuffer(&_indexBuffer);
    }

    for (auto& texture : _textures) {
        _hgi->DestroyTexture(&texture.second);
    }

    if (_shaderProgram) {
        _DestroyShaderProgram();
    }

    if (_resourceBindings) {
        _hgi->DestroyResourceBindings(&_resourceBindings);
    }

    if (_pipeline) {
        _hgi->DestroyPipeline(&_pipeline);
    }
}

void
HdxFullscreenShader::SetProgram(
    TfToken const& glslfx, 
    TfToken const& technique) 
{
    if (_glslfx == glslfx && _technique == technique) {
        return;
    }

    _glslfx = glslfx;
    _technique = technique;

    if (_shaderProgram) {
        _DestroyShaderProgram();
    }

    HioGlslfx vsGlslfx(HdxPackageFullscreenShader());
    HioGlslfx fsGlslfx(glslfx);

    // Setup the vertex shader
    HgiShaderFunctionDesc vertDesc;
    vertDesc.debugName = _tokens->fullscreenVertex.GetString();
    vertDesc.shaderStage = HgiShaderStageVertex;
    vertDesc.shaderCode = vsGlslfx.GetSource(_tokens->fullscreenVertex);
    HgiShaderFunctionHandle vertFn = _hgi->CreateShaderFunction(vertDesc);

    // Setup the fragment shader
    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = technique.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    fragDesc.shaderCode = fsGlslfx.GetSource(technique);
    HgiShaderFunctionHandle fragFn = _hgi->CreateShaderFunction(fragDesc);

    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName = _tokens->fullscreenShader.GetString();
    programDesc.shaderFunctions.emplace_back(std::move(vertFn));
    programDesc.shaderFunctions.emplace_back(std::move(fragFn));
    _shaderProgram = _hgi->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create HdxFullscreenShader shader program");
        _PrintCompileErrors();
        _DestroyShaderProgram();
        return;
    }
}

void
HdxFullscreenShader::SetBuffer(
    HgiBufferHandle const& buffer,
    uint32_t bindingIndex)
{
    _buffers[bindingIndex] = buffer;
}

void
HdxFullscreenShader::CreatePipeline(HgiPipelineDesc pipeDesc)
{
    // Pipeline not changed, abort.
    if (_pipeline && _pipeline.Get()->GetDescriptor() == pipeDesc) {
        return;
    }

    if (pipeDesc.debugName.empty()) {
        pipeDesc.debugName = "HdxFullscreenShader Pipeline";
    }
    pipeDesc.pipelineType = HgiPipelineTypeGraphics;
    pipeDesc.resourceBindings = _resourceBindings;
    pipeDesc.shaderProgram = _shaderProgram;

    if (_pipeline) {
        _hgi->DestroyPipeline(&_pipeline);
    }

    _pipeline = _hgi->CreatePipeline(pipeDesc);
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
    _blendingEnabled = enableBlending;
    _srcColorBlendFactor = srcColorBlendFactor;
    _dstColorBlendFactor = dstColorBlendFactor;
    _colorBlendOp = colorBlendOp;
    _srcAlphaBlendFactor = srcAlphaBlendFactor;
    _dstAlphaBlendFactor = dstAlphaBlendFactor;
    _alphaBlendOp = alphaBlendOp;
}

void
HdxFullscreenShader::_CreateBufferResources()
{
    if (_vertexBuffer) {
        return;
    }

    /* For the fullscreen pass, we draw a triangle:
     *
     * |\
     * |_\
     * | |\
     * |_|_\
     *
     * The vertices are at (-1, 3) [top left]; (-1, -1) [bottom left];
     * and (3, -1) [bottom right]; UVs are assigned so that the bottom left
     * is (0,0) and the clipped vertices are 2 on their axis, so that:
     * x=-1 => s = 0; x = 3 => s = 2, which means x = 1 => s = 1.
     *
     * This maps the texture space [0,1]^2 to the clip space XY [-1,1]^2.
     * The parts of the triangle extending past NDC space are clipped before
     * rasterization.
     *
     * This has the advantage (over rendering a quad) that we don't render
     * the diagonal twice.
     *
     * Note that we're passing in NDC positions, and we don't expect the vertex
     * shader to transform them.  Also note: the fragment shader can optionally
     * read depth from a texture, but otherwise the depth is -1, meaning near
     * plane.
     */
    static const size_t elementsPerVertex = 6;

    static const float vertices[elementsPerVertex * 3] = 
    //      positions     |  uvs
        { -1,  3, 0, 1,     0, 2,
          -1, -1, 0, 1,     0, 0,
           3, -1, 0, 1,     2, 0 };

    HgiBufferDesc vboDesc;
    vboDesc.debugName = "HdxFullscreenShader VertexBuffer";
    vboDesc.usage = HgiBufferUsageVertex;
    vboDesc.initialData = vertices;
    vboDesc.byteSize = sizeof(vertices) * sizeof(vertices[0]);
    vboDesc.vertexStride = elementsPerVertex * sizeof(vertices[0]);
    _vertexBuffer = _hgi->CreateBuffer(vboDesc);

    static const int32_t indices[3] = {0,1,2};

    HgiBufferDesc iboDesc;
    iboDesc.debugName = "HdxFullscreenShader IndexBuffer";
    iboDesc.usage = HgiBufferUsageIndex32;
    iboDesc.initialData = indices;
    iboDesc.byteSize = sizeof(indices) * sizeof(indices[0]);
    _indexBuffer = _hgi->CreateBuffer(iboDesc);
}

void
HdxFullscreenShader::SetTexture(
    TfToken const& name, 
    int width, 
    int height,
    HdFormat format,
    void *data)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (width == 0 || height == 0 || data == nullptr) {
        auto it = _textures.find(name);
        if (it != _textures.end()) {
            _hgi->DestroyTexture(&it->second);
            _textures.erase(it);
        }
        return;
    }

    size_t pixelByteSize = HdDataSizeOfFormat(format);

    // If we already had the texture, destroy it since we have new pixels.
    auto it = _textures.find(name);
    if (it != _textures.end()) {
        _hgi->DestroyTexture(&it->second);
    }

    HgiTextureDesc texDesc;
    texDesc.debugName = "HdxFullscreenShader texture " + name.GetString();
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.format = HdxHgiConversions::GetHgiFormat(format);
    texDesc.initialData = data;
    texDesc.layerCount = 1;
    texDesc.mipLevels = 1;
    texDesc.pixelsByteSize = width * height * pixelByteSize;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsShaderRead;
    HgiTextureHandle tex = _hgi->CreateTexture(texDesc);

    _textures[name] = tex;
}

bool
HdxFullscreenShader::_CreateResourceBindings(TextureMap const& textures)
{
    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "HdxFullscreenShader";
    resourceDesc.pipelineType = HgiPipelineTypeGraphics;

    // XXX OpenGL / Metal both re-use slot indices between buffers and textures.
    // Vulkan uses unique slot indices in descriptor set.
    // We probably don't want to express bind indices in the descriptor and
    // use codegen to produce the right values for the different APIs.
    size_t bindSlots = 0;

    for (auto const& texture : textures) {
        HgiTextureHandle texHandle = texture.second;
        if (!texHandle) continue;
        HgiTextureBindDesc texBind;
        texBind.bindingIndex = bindSlots++;
        texBind.resourceType = HgiBindResourceTypeCombinedImageSampler;
        texBind.stageUsage = HgiShaderStageFragment;
        texBind.textures.push_back(texHandle);
        resourceDesc.textures.emplace_back(std::move(texBind));
    }

    for (auto const& buffer : _buffers) {
        HgiBufferHandle bufferHandle = buffer.second;
        if (!bufferHandle) continue;
        HgiBufferBindDesc bufBind;
        bufBind.bindingIndex = buffer.first;
        bufBind.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind.stageUsage = HgiShaderStageFragment;
        bufBind.offsets.push_back(0);
        bufBind.buffers.push_back(bufferHandle);
        resourceDesc.buffers.emplace_back(std::move(bufBind));
    }

    // If nothing has changed in the descriptor we avoid re-creating the
    // resource bindings object.
    if (_resourceBindings) {
        HgiResourceBindingsDesc const& desc= _resourceBindings->GetDescriptor();
        if (desc == resourceDesc) {
            return true;
        }
    }

    _resourceBindings = _hgi->CreateResourceBindings(resourceDesc);

    return true;
}

bool
HdxFullscreenShader::_CreateDefaultPipeline(
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& depthDst,
    bool depthWrite)
{
    if (_pipeline) {
        if ((!colorDst || (_attachment0.format ==
                          colorDst.Get()->GetDescriptor().format)) &&
            (!depthDst || (_depthAttachment.format ==
                           depthDst.Get()->GetDescriptor().format))) {
            return true;
        }
             
        _hgi->DestroyPipeline(&_pipeline);
    }
    
    _attachment0.blendEnabled = _blendingEnabled;
    _attachment0.loadOp = HgiAttachmentLoadOpDontCare;
    _attachment0.storeOp = HgiAttachmentStoreOpStore;
    _attachment0.srcColorBlendFactor = _srcColorBlendFactor;
    _attachment0.dstColorBlendFactor = _dstColorBlendFactor;
    _attachment0.colorBlendOp = _colorBlendOp;
    _attachment0.srcAlphaBlendFactor = _srcAlphaBlendFactor;
    _attachment0.dstAlphaBlendFactor = _dstAlphaBlendFactor;
    _attachment0.alphaBlendOp = _alphaBlendOp;
    if (colorDst) {
        _attachment0.format = colorDst.Get()->GetDescriptor().format;
    }

    _depthAttachment.loadOp = HgiAttachmentLoadOpDontCare;
    _depthAttachment.storeOp = HgiAttachmentStoreOpStore;
    if (depthDst) {
        _depthAttachment.format = depthDst.Get()->GetDescriptor().format;
    }

    HgiPipelineDesc desc;
    desc.debugName = "HdxFullscreenShader Pipeline";
    desc.pipelineType = HgiPipelineTypeGraphics;
    desc.resourceBindings = _resourceBindings;
    desc.shaderProgram = _shaderProgram;
    desc.colorAttachmentDescs.emplace_back(_attachment0);
    desc.depthAttachmentDesc = _depthAttachment;
    
    // Describe the vertex buffer
    HgiVertexAttributeDesc posAttr;
    posAttr.format = HgiFormatFloat32Vec3;
    posAttr.offset = 0;
    posAttr.shaderBindLocation = 0;

    HgiVertexAttributeDesc uvAttr;
    uvAttr.format = HgiFormatFloat32Vec2;
    uvAttr.offset = sizeof(float) * 4; // after posAttr
    uvAttr.shaderBindLocation = 1;

// todo OpenGL and Metal both re-use slot indices between buffers and textures.
// Vulkan does not allow this and each bound resource must have a unique index.
// We need to clarify the Hgi API. We probably want to follow the Vulkan rules,
// because when we do we still have both pieces of information.
// Metal and GL can look in the 'textures' vector to find the bindIndex.
// Vulkan can use the provided 'bindIndex' to determine the index in the
// descriptor set.
// However we still have a problem with the glsl.
// In there we will have written the 'binding=xx' value and it the same glsl
// won't be compatible between opengl and vulkan...
    size_t bindSlots = 0;

    HgiVertexBufferDesc vboDesc;
    vboDesc.bindingIndex = bindSlots++;
    vboDesc.vertexStride = sizeof(float) * 6; // pos, uv
    vboDesc.vertexAttributes.push_back(posAttr);
    vboDesc.vertexAttributes.push_back(uvAttr);

    desc.vertexBuffers.emplace_back(std::move(vboDesc));

    // Depth test and write must be on since we may want to transfer depth.
    // Depth test must be on because when off it also disables depth writes.
    // Instead we set the compare function to always.
    desc.depthState.depthTestEnabled = true;
    desc.depthState.depthWriteEnabled = depthWrite;
    desc.depthState.depthCompareFn = HgiCompareFunctionAlways;

    // We don't use the stencil mask in this task.
    desc.depthState.stencilTestEnabled = false;

    // Alpha to coverage would prevent any pixels that have an alpha of 0.0 from
    // being written. We want to transfer all pixels. Even background
    // pixels that were set with a clearColor alpha of 0.0.
    desc.multiSampleState.alphaToCoverageEnable = false;

    // Setup raserization state
    desc.rasterizationState.cullMode = HgiCullModeBack;
    desc.rasterizationState.polygonMode = HgiPolygonModeFill;
    desc.rasterizationState.winding = HgiWindingCounterClockwise;

    CreatePipeline(desc);

    return true;
}

void 
HdxFullscreenShader::Draw(
    TextureMap const& textures,
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& depthDst)
{
    bool depthWrite = depthDst.Get() != nullptr;
    _Draw(textures, colorDst, depthDst, depthWrite);
}

void
HdxFullscreenShader::Draw(
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& depthDst)
{
    bool depthWrite = depthDst.Get() != nullptr;
    _Draw(_textures, colorDst, depthDst, depthWrite);
}

void
HdxFullscreenShader::DrawToFramebuffer(TextureMap const& textures)
{
    // Destination textures are null since we are drawing into framebuffer.
    // depthWrite is true: we want to transfer depth from aov's to framebuffer.
    bool depthWrite = true;
    TextureMap const& texs = textures.empty() ? _textures : textures;
    _Draw(texs, HgiTextureHandle(), HgiTextureHandle(), depthWrite);
}

void
HdxFullscreenShader::_DestroyShaderProgram()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        _hgi->DestroyShaderFunction(&fn);
    }
    _hgi->DestroyShaderProgram(&_shaderProgram);
}

void 
HdxFullscreenShader::_Draw(
    TextureMap const& textures,
    HgiTextureHandle const& colorDst,
    HgiTextureHandle const& depthDst,
    bool writeDepth)
{
    // If the user has not set a custom shader program, pick default program.
    if (!_shaderProgram) {
        auto const& it = textures.find(TfToken("depth"));
        bool depthAware = it != textures.end();
        SetProgram(HdxPackageFullscreenShader(),
            depthAware ? _tokens->compositeFragmentWithDepth :
                         _tokens->compositeFragmentNoDepth);
    }

    // Create draw buffers if they haven't been created yet.
    if (!_vertexBuffer) {
        _CreateBufferResources();
    }

    // Create or update the resource bindings (textures may have changed)
    _CreateResourceBindings(textures);

    // create pipeline (first time)
    _CreateDefaultPipeline(colorDst, depthDst, writeDepth);

    // If a destination color target is provided we can use it as the
    // dimensions of the backbuffer. If not destination textures are provided
    // it means we are rendering to the framebuffer.
    // In that case we use one of the provided input texture dimensions.
    // XXX Remove this once HgiInterop is in place in PresentTask. We should
    // error out if 'colorDst' is not provided.
    HgiTextureHandle dimensionSrc = colorDst;
    if (!dimensionSrc) {
        auto const& it = textures.find(TfToken("color"));
        if (it != textures.end()) {
            dimensionSrc = it->second;
        }
    }

    GfVec3i dimensions = GfVec3i(1);
    if (dimensionSrc) {
        dimensions = dimensionSrc->GetDescriptor().dimensions;
    } else {
        TF_CODING_ERROR("Could not determine the backbuffer dimensions");
    }

// todo move this into HgiGL::SubmitCmds

    // XXX Not everything is using Hgi yet, so we have inconsistent state
    // management in opengl. Remove when Hgi transition is complete.
    HgiGLPipeline* glPipeline = dynamic_cast<HgiGLPipeline*>(_pipeline.Get());
    if (glPipeline) {
        glPipeline->CaptureOpenGlState();
    }

    // Prepare graphics cmds.
    HgiGraphicsCmdsDesc gfxDesc;
    gfxDesc.width = dimensions[0];
    gfxDesc.height = dimensions[1];

    if (colorDst) {
        gfxDesc.colorAttachmentDescs.emplace_back(_attachment0);
        gfxDesc.colorTextures.emplace_back(colorDst);
    }

    if (depthDst) {
        gfxDesc.depthAttachmentDesc = _depthAttachment;
        gfxDesc.depthTexture = depthDst;
    }

    // Begin rendering
    HgiGraphicsCmdsUniquePtr gfxCmds = _hgi->CreateGraphicsCmds(gfxDesc);
    gfxCmds->PushDebugGroup(_debugName.c_str());
    gfxCmds->BindResources(_resourceBindings);
    gfxCmds->BindPipeline(_pipeline);
    gfxCmds->BindVertexBuffers(0, {_vertexBuffer}, {0});
    GfVec4i vp = GfVec4i(0, 0, dimensions[0], dimensions[1]);
    gfxCmds->SetViewport(vp);
    gfxCmds->DrawIndexed(_indexBuffer, 3, 0, 0, 1, 0);
    gfxCmds->PopDebugGroup();

    // Done recording commands, submit work.
    _hgi->SubmitCmds(gfxCmds.get(), 1);

    // XXX Not everything is using Hgi yet, so we have inconsistent state
    // management in opengl. Remove when Hgi transition is complete.
    if (glPipeline) {
        glPipeline->RestoreOpenGlState();
    }
}

void
HdxFullscreenShader::_PrintCompileErrors()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        std::cout << fn->GetCompileErrors() << std::endl;
    }
    std::cout << _shaderProgram->GetCompileErrors() << std::endl;
}


PXR_NAMESPACE_CLOSE_SCOPE
