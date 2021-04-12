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
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <iostream>

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
  , _sampler()
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

    // Create descriptor for vertex pos and uvs
    _CreateVertexBufferDescriptor();

    // Depth test and write must be on since we may want to transfer depth.
    // Depth test must be on because when off it also disables depth writes.
    // Instead we set the compare function to always.
    _depthState.depthTestEnabled = true;
    _depthState.depthCompareFn = HgiCompareFunctionAlways;

    // We don't use the stencil mask in this task.
    _depthState.stencilTestEnabled = false;
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

    if (_shaderProgram) {
        _DestroyShaderProgram();
    }

    if (_resourceBindings) {
        _hgi->DestroyResourceBindings(&_resourceBindings);
    }

    if (_pipeline) {
        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }

    if (_sampler) {
        _hgi->DestroySampler(&_sampler);
    }
}

void
HdxFullscreenShader::SetProgram(
    TfToken const& glslfx, 
    TfToken const& shaderName,
    HgiShaderFunctionDesc &fragDesc,
    HgiShaderFunctionDesc vertDesc
    )
{
    if (_glslfx == glslfx && _shaderName == shaderName) {
        return;
    }
    _glslfx = glslfx;
    _shaderName = shaderName;

    if (_shaderProgram) {
        _DestroyShaderProgram();
    }
    TfToken const& technique = HioGlslfxTokens->defVal;

    const HioGlslfx vsGlslfx(HdxPackageFullscreenShader(), technique);
    const HioGlslfx fsGlslfx(glslfx, technique);

    // Setup the vertex shader
    std::string vsCode;
    //pass this guy in as a reference -->
    
    if(_hgi->GetAPIName() == HgiTokens->OpenGL) {
        vsCode = "#version 450 \n";
    }
    vsCode +=vsGlslfx.GetSource(_tokens->fullscreenVertex);
    TF_VERIFY(!vsCode.empty());

    vertDesc.shaderCode = vsCode.c_str();
    HgiShaderFunctionHandle vertFn = _hgi->CreateShaderFunction(vertDesc);

    // Setup the fragment shader
    std::string fsCode;
    
    if(_hgi->GetAPIName() == HgiTokens->OpenGL) {
        fsCode = "#version 450 \n";
    }
    fsCode += fsGlslfx.GetSource(_shaderName);
    TF_VERIFY(!fsCode.empty());
    fragDesc.shaderCode = fsCode.c_str();
    HgiShaderFunctionHandle fragFn = _hgi->CreateShaderFunction(fragDesc);

    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName = _tokens->fullscreenShader.GetString();
    programDesc.shaderFunctions.push_back(std::move(vertFn));
    programDesc.shaderFunctions.push_back(std::move(fragFn));
    _shaderProgram = _hgi->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create HdxFullscreenShader shader program");
        _PrintCompileErrors();
        _DestroyShaderProgram();
        return;
    }
}

HgiShaderFunctionDesc
HdxFullscreenShader::GetFullScreenVertexDesc()
{
    HgiShaderFunctionDesc vertDesc;
    vertDesc.debugName = _tokens->fullscreenVertex;
    vertDesc.shaderStage = HgiShaderStageVertex;
    
    HgiShaderFunctionAddStageInput(
        &vertDesc, "position", "vec4", "position");
    HgiShaderFunctionAddStageInput(
        &vertDesc, "uvIn", "vec2");
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "gl_Position", "vec4", "position");
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "uvOut", "vec2");
    return vertDesc;
}

void
HdxFullscreenShader::BindBuffer(
    HgiBufferHandle const& buffer,
    uint32_t bindingIndex)
{
    _buffers[bindingIndex] = buffer;
}

void
HdxFullscreenShader::SetDepthState(HgiDepthStencilState const& state)
{
    if (_depthState == state) {
        return;
    }

    if (_pipeline) {
        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }

    _depthState = state;
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
    if (_blendingEnabled == enableBlending &&
        _srcColorBlendFactor == srcColorBlendFactor &&
        _dstColorBlendFactor == dstColorBlendFactor &&
        _colorBlendOp == colorBlendOp &&
        _srcAlphaBlendFactor == srcAlphaBlendFactor &&
        _dstAlphaBlendFactor == dstAlphaBlendFactor &&
        _alphaBlendOp == alphaBlendOp) 
    {
        return;
    }

    if (_pipeline) {
        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }

    _blendingEnabled = enableBlending;
    _srcColorBlendFactor = srcColorBlendFactor;
    _dstColorBlendFactor = dstColorBlendFactor;
    _colorBlendOp = colorBlendOp;
    _srcAlphaBlendFactor = srcAlphaBlendFactor;
    _dstAlphaBlendFactor = dstAlphaBlendFactor;
    _alphaBlendOp = alphaBlendOp;
}

void
HdxFullscreenShader::SetShaderConstants(
    uint32_t byteSize,
    const void* data)
{
    _constantsData.resize(byteSize);
    if (byteSize > 0) {
        memcpy(&_constantsData[0], data, byteSize);
    }
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
    constexpr size_t elementsPerVertex = 6;
    constexpr size_t vertDataCount = elementsPerVertex * 3;
    constexpr float vertDataGL[vertDataCount] = 
            { -1,  3, 0, 1,     0, 2,
              -1, -1, 0, 1,     0, 0,
               3, -1, 0, 1,     2, 0};

    constexpr float vertDataOther[vertDataCount] =
            { -1,  3, 0, 1,     0, -1,
              -1, -1, 0, 1,     0, 1,
               3, -1, 0, 1,     2, 1};

    HgiBufferDesc vboDesc;
    vboDesc.debugName = "HdxFullscreenShader VertexBuffer";
    vboDesc.usage = HgiBufferUsageVertex;
    vboDesc.initialData = _hgi->GetAPIName() != HgiTokens->OpenGL 
        ? vertDataOther : vertDataGL;
    vboDesc.byteSize = sizeof(vertDataGL);
    vboDesc.vertexStride = elementsPerVertex * sizeof(vertDataGL[0]);
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
HdxFullscreenShader::BindTextures(
    TfTokenVector const& names,
    HgiTextureHandleVector const& textures)
{
    if (!TF_VERIFY(names.size() == textures.size())) {
        return;
    }

    for (size_t i=0; i<names.size(); i++) {
        TfToken const& name = names[i];
        HgiTextureHandle const& tex = textures[i];
        if (tex) {
            _textures[name] = tex;
        } else {
            _textures.erase(name);
        }
    }
}

bool
HdxFullscreenShader::_CreateResourceBindings(TextureMap const& textures)
{
    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "HdxFullscreenShader";

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
        texBind.stageUsage = HgiShaderStageFragment;
        texBind.textures.push_back(texHandle);
        texBind.samplers.push_back(_sampler);
        resourceDesc.textures.push_back(std::move(texBind));
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
        resourceDesc.buffers.push_back(std::move(bufBind));
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

void
HdxFullscreenShader::_CreateVertexBufferDescriptor()
{
    // Describe the vertex buffer
    HgiVertexAttributeDesc posAttr;
    posAttr.format = HgiFormatFloat32Vec3;
    posAttr.offset = 0;
    posAttr.shaderBindLocation = 0;

    HgiVertexAttributeDesc uvAttr;
    uvAttr.format = HgiFormatFloat32Vec2;
    uvAttr.offset = sizeof(float) * 4; // after posAttr
    uvAttr.shaderBindLocation = 1;

    size_t bindSlots = 0;

    _vboDesc.bindingIndex = bindSlots++;
    _vboDesc.vertexStride = sizeof(float) * 6; // pos, uv
    _vboDesc.vertexAttributes.clear();
    _vboDesc.vertexAttributes.push_back(posAttr);
    _vboDesc.vertexAttributes.push_back(uvAttr);
}

bool
HdxFullscreenShader::_CreatePipeline(
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

        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }

    // Setup attachments
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
        _attachment0.usage = colorDst.Get()->GetDescriptor().usage;
    }

    _depthAttachment.loadOp = HgiAttachmentLoadOpDontCare;
    _depthAttachment.storeOp = HgiAttachmentStoreOpStore;
    if (depthDst) {
        _depthAttachment.format = depthDst.Get()->GetDescriptor().format;
        _depthAttachment.usage = depthDst.Get()->GetDescriptor().usage;
    }

    HgiGraphicsPipelineDesc desc;
    desc.debugName = _debugName + " Pipeline";
    desc.shaderProgram = _shaderProgram;
    desc.colorAttachmentDescs.push_back(_attachment0);
    desc.depthAttachmentDesc = _depthAttachment;

    desc.vertexBuffers.push_back(_vboDesc);

    // User can provide custom depth state, but DepthWrite is controlled by the
    // presence of the depth attachment.
    desc.depthState = _depthState;
    desc.depthState.depthWriteEnabled = depthWrite;

    // Alpha to coverage would prevent any pixels that have an alpha of 0.0 from
    // being written. We want to transfer all pixels. Even background
    // pixels that were set with a clearColor alpha of 0.0.
    desc.multiSampleState.alphaToCoverageEnable = false;

    // Setup raserization state
    desc.rasterizationState.cullMode = HgiCullModeBack;
    desc.rasterizationState.polygonMode = HgiPolygonModeFill;
    desc.rasterizationState.winding = HgiWindingCounterClockwise;

    // Set the shaders
    desc.shaderProgram = _shaderProgram;

    // Ignore user provided vertex buffers. The VBO must always match the
    // vertex attributes we setup for the fullscreen triangle.
    desc.vertexBuffers.clear();
    desc.vertexBuffers.push_back(_vboDesc);

    // shader constants
    if (!_constantsData.empty()) {
        desc.shaderConstantsDesc.byteSize = _constantsData.size();
        desc.shaderConstantsDesc.stageUsage = HgiShaderStageFragment;
    }

    _pipeline = _hgi->CreateGraphicsPipeline(desc);

    return true;
}

bool
HdxFullscreenShader::_CreateSampler()
{
    if (_sampler) {
        return true;
    }

    HgiSamplerDesc sampDesc;
    
    sampDesc.magFilter = HgiSamplerFilterLinear;
    sampDesc.minFilter = HgiSamplerFilterLinear;

    sampDesc.addressModeU = HgiSamplerAddressModeClampToEdge;
    sampDesc.addressModeV = HgiSamplerAddressModeClampToEdge;
    
    _sampler = _hgi->CreateSampler(sampDesc);

    return true;
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
        auto const& it = textures.find(HdAovTokens->depth);
        const bool depthAware = it != textures.end();
        HgiShaderFunctionDesc vertDesc;
        
        vertDesc.debugName = _tokens->fullscreenVertex.GetString();
        vertDesc.shaderStage = HgiShaderStageVertex;
        HgiShaderFunctionAddStageInput(
            &vertDesc, "position", "vec4", "position");
        HgiShaderFunctionAddStageInput(
            &vertDesc, "uvIn", "vec2");
        HgiShaderFunctionAddStageOutput(
            &vertDesc, "gl_Position", "vec4", "position");
        HgiShaderFunctionAddStageOutput(
            &vertDesc, "uvOut", "vec2");
        
        HgiShaderFunctionDesc fragDesc;
        fragDesc.debugName = _shaderName.GetString();
        fragDesc.shaderStage = HgiShaderStageFragment;
        HgiShaderFunctionAddStageInput(
            &fragDesc, "uvOut", "vec2");
        HgiShaderFunctionAddStageOutput(
            &fragDesc, "hd_FragColor", "vec4", "color");
        HgiShaderFunctionAddStageOutput(
            &fragDesc, "hd_FragDepth", "float", "depth(any)");
        HgiShaderFunctionAddTexture(
            &fragDesc, "colorIn");

        if(depthAware) {
            HgiShaderFunctionAddTexture(
                &fragDesc, "depth");
        }
        
        SetProgram(HdxPackageFullscreenShader(),
            depthAware ? _tokens->compositeFragmentWithDepth :
                         _tokens->compositeFragmentNoDepth,
            fragDesc,
            vertDesc);
    }

    // Create draw buffers if they haven't been created yet.
    if (!_vertexBuffer) {
        _CreateBufferResources();
    }

    // create a default texture sampler (first time)
    _CreateSampler();

    // Create or update the resource bindings (textures may have changed)
    _CreateResourceBindings(textures);

    // create pipeline (first time)
    _CreatePipeline(colorDst, depthDst, writeDepth);
    
    // If a destination color target is provided we can use it as the
    // dimensions of the backbuffer. If not destination textures are provided
    // it means we are rendering to the framebuffer.
    // In that case we use one of the provided input texture dimensions.
    // XXX Remove this once HgiInterop is in place in PresentTask. We should
    // error out if 'colorDst' is not provided.
    HgiTextureHandle dimensionSrc = colorDst;
    if (!dimensionSrc) {
        auto const& it = textures.find(HdAovTokens->color);
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

    // Prepare graphics cmds.
    HgiGraphicsCmdsDesc gfxDesc;

    if (colorDst) {
        gfxDesc.colorAttachmentDescs.push_back(_attachment0);
        gfxDesc.colorTextures.push_back(colorDst);
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
    const GfVec4i vp(0, 0, dimensions[0], dimensions[1]);
    gfxCmds->SetViewport(vp);

    if (!_constantsData.empty()) {
        gfxCmds->SetConstantValues(
            _pipeline, HgiShaderStageFragment, 0,
            _constantsData.size(), _constantsData.data());
    }

    gfxCmds->DrawIndexed(_indexBuffer, 3, 0, 0, 1);
    gfxCmds->PopDebugGroup();

    // Done recording commands, submit work.
    _hgi->SubmitCmds(gfxCmds.get());
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
