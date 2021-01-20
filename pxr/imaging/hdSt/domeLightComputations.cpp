//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hdSt/domeLightComputations.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/samplerObject.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"

#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_DomeLightComputationGPU::HdSt_DomeLightComputationGPU(
    const TfToken &shaderToken,
    HdStSimpleLightingShaderPtr const &lightingShader,
    const unsigned int numLevels,
    const unsigned int level, 
    const float roughness) 
  : _shaderToken(shaderToken), 
    _lightingShader(lightingShader),
    _numLevels(numLevels), 
    _level(level), 
    _roughness(roughness)
{
}

static
void
_FillPixelsByteSize(HgiTextureDesc * const desc)
{
    const size_t s = HgiGetDataSizeOfFormat(desc->format);
    desc->pixelsByteSize =
        s * desc->dimensions[0] * desc->dimensions[1] * desc->dimensions[2];
}

static
bool
_GetSrcTextureDimensionsAndName(
    HdStSimpleLightingShaderSharedPtr const &shader,
    GfVec3i * srcDim,
    HgiTextureHandle * srcTextureName,
    HgiSamplerHandle * srcSamplerName)
{
    // Get source texture, the dome light environment map
    HdStTextureHandleSharedPtr const &srcTextureHandle =
        shader->GetDomeLightEnvironmentTextureHandle();
    if (!TF_VERIFY(srcTextureHandle)) {
        return false;
    }

    const HdStUvTextureObject * const srcTextureObject =
        dynamic_cast<HdStUvTextureObject*>(
            srcTextureHandle->GetTextureObject().get());
    if (!TF_VERIFY(srcTextureObject)) {
        return false;
    }

    const HdStUvSamplerObject * const srcSamplerObject =
        dynamic_cast<HdStUvSamplerObject*>(
            srcTextureHandle->GetSamplerObject().get());
    if (!TF_VERIFY(srcSamplerObject)) {
        return false;
    }

    if (!srcTextureObject->IsValid()) {
        const std::string &filePath =
            srcTextureObject->GetTextureIdentifier().GetFilePath();
        TF_WARN("Could not open dome light texture file at %s.",
                filePath.c_str());
        return false;
    }

    const HgiTexture * const srcTexture = srcTextureObject->GetTexture().Get();
    if (!TF_VERIFY(srcTexture)) {
        return false;
    }

    *srcDim = srcTexture->GetDescriptor().dimensions;
    *srcTextureName = srcTextureObject->GetTexture();
    *srcSamplerName = srcSamplerObject->GetSampler();

    return true;
}

void
HdSt_DomeLightComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const &range,
    HdResourceRegistry * const resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistry* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);
    HdStGLSLProgramSharedPtr const computeProgram =
        HdStGLSLProgram::GetComputeProgram(
            HdStPackageDomeLightShader(), 
            _shaderToken,
            static_cast<HdStResourceRegistry*>(resourceRegistry));
    if (!TF_VERIFY(computeProgram)) {
        return;
    }

    HdStSimpleLightingShaderSharedPtr const shader = _lightingShader.lock();
    if (!TF_VERIFY(shader)) {
        return;
    }

    // Size of source texture (the dome light environment map)
    GfVec3i srcDim;
    HgiTextureHandle srcTextureName;
    HgiSamplerHandle srcSamplerName;
    if (!_GetSrcTextureDimensionsAndName(
            shader, &srcDim, &srcTextureName, &srcSamplerName)) {
        return;
    }

    // Size of texture to be created.
    const int width  = srcDim[0] / 2;
    const int height = srcDim[1] / 2;

    // Get texture object from lighting shader that this
    // computation is supposed to populate
    HdStTextureHandleSharedPtr const &dstTextureHandle =
        shader->GetTextureHandle(_shaderToken);
    if (!TF_VERIFY(dstTextureHandle)) {
        return;
    }

    HdStDynamicUvTextureObject * const dstUvTextureObject =
        dynamic_cast<HdStDynamicUvTextureObject*>(
            dstTextureHandle->GetTextureObject().get());
    if (!TF_VERIFY(dstUvTextureObject)) {
        return;
    }

    if (_level == 0) {
        // Level zero is in charge of actually creating the
        // GPU resource.
        HgiTextureDesc desc;
        desc.debugName = _shaderToken.GetText();
        desc.format = HgiFormatFloat16Vec4;
        desc.dimensions = GfVec3i(width, height, 1);
        desc.layerCount = 1;
        desc.mipLevels = _numLevels;
        desc.usage =
            HgiTextureUsageBitsShaderRead | HgiTextureUsageBitsShaderWrite;
        _FillPixelsByteSize(&desc);
        dstUvTextureObject->CreateTexture(desc);
    }

    // Create a texture view for the layer we want to write to
    HgiTextureViewDesc texViewDesc;
    texViewDesc.layerCount = 1;
    texViewDesc.mipLevels = 1;
    texViewDesc.format = HgiFormatFloat16Vec4;
    texViewDesc.sourceFirstLayer = 0;
    texViewDesc.sourceFirstMip = _level;
    texViewDesc.sourceTexture = dstUvTextureObject->GetTexture();

    Hgi* hgi = hdStResourceRegistry->GetHgi();
    HgiTextureViewHandle dstTextureView = hgi->CreateTextureView(texViewDesc);

    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "DomeLightComputation";

    HgiTextureBindDesc texBind0;
    texBind0.bindingIndex = 0;
    texBind0.stageUsage = HgiShaderStageCompute;
    texBind0.textures.push_back(srcTextureName);
    texBind0.samplers.push_back(srcSamplerName);
    texBind0.resourceType = HgiBindResourceTypeCombinedSamplerImage;
    resourceDesc.textures.push_back(std::move(texBind0));

    HgiTextureBindDesc texBind1;
    texBind1.bindingIndex = 1;
    texBind1.stageUsage = HgiShaderStageCompute;
    texBind1.textures.push_back(dstTextureView->GetViewTexture());
    texBind1.samplers.push_back(srcSamplerName);
    texBind1.resourceType = HgiBindResourceTypeStorageImage;
    resourceDesc.textures.push_back(std::move(texBind1));

    HgiResourceBindingsHandle resourceBindings =
        hgi->CreateResourceBindings(resourceDesc);
    
    // prepare uniform buffer for GPU computation
    struct Uniforms {
        float roughness;
    } uniform;

    uniform.roughness = _roughness;

    bool hasUniforms = uniform.roughness >= 0.0f;

    HgiComputePipelineDesc desc;
    desc.debugName = "DomeLightComputation";
    desc.shaderProgram = computeProgram->GetProgram();
    if (hasUniforms) {
        desc.shaderConstantsDesc.byteSize = sizeof(uniform);
    }
    HgiComputePipelineHandle pipeline = hgi->CreateComputePipeline(desc);

    HgiComputeCmds* computeCmds = hdStResourceRegistry->GetGlobalComputeCmds();
    computeCmds->PushDebugGroup("DomeLightComputationCmds");
    computeCmds->BindResources(resourceBindings);
    computeCmds->BindPipeline(pipeline);

    // Queue transfer uniform buffer.
    // If we are calculating the irradiance map we do not need to send over
    // the roughness value to the shader
    // flagged this with a negative roughness value
    if (hasUniforms) {
        computeCmds->SetConstantValues(pipeline, 0, sizeof(uniform), &uniform);
    }

    // Queue compute work
    computeCmds->Dispatch(width / 32, height / 32);

    computeCmds->PopDebugGroup();

    // Garbage collect the intermediate resources (destroyed at end of frame).
    hgi->DestroyTextureView(&dstTextureView);
    hgi->DestroyComputePipeline(&pipeline);
    hgi->DestroyResourceBindings(&resourceBindings);
}

PXR_NAMESPACE_CLOSE_SCOPE
