//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/imaging/hdSt/dynamicCubemapTextureObject.h"
#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"

#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

int
_MakeMultipleOf(int dim, int localSize)
{
    return ((dim + localSize - 1) / localSize) * localSize;
}

void
_FillPixelsByteSize(HgiTextureDesc * const desc)
{
    const size_t s = HgiGetDataSizeOfFormat(desc->format);
    desc->pixelsByteSize =
        s * desc->dimensions[0] * desc->dimensions[1] * desc->dimensions[2];
}

template<typename Texture, typename Sampler>
bool
_GetSrcDimensionsTextureAndSampler(
    HdStTextureHandleSharedPtr const &srcTextureHandle,
    GfVec3i * srcDim,
    HgiTextureHandle * srcTexture,
    HgiSamplerHandle * srcSampler)
{
    if (!TF_VERIFY(srcTextureHandle)) {
        return false;
    }

    const auto * const srcTextureObject =
        dynamic_cast<Texture*>(
            srcTextureHandle->GetTextureObject().get());
    if (!TF_VERIFY(srcTextureObject)) {
        return false;
    }

    const auto * const srcSamplerObject =
        dynamic_cast<Sampler*>(
            srcTextureHandle->GetSamplerObject().get());
    if (!TF_VERIFY(srcSamplerObject)) {
        return false;
    }

    if (!srcTextureObject->IsValid()) {
        const std::string &filePath =
            srcTextureObject->GetTextureIdentifier().GetFilePath();
        TF_WARN("Invalid texture %s.", filePath.c_str());
        return false;
    }

    const HgiTexture * const hgiTexture = srcTextureObject->GetTexture().Get();
    if (!TF_VERIFY(hgiTexture)) {
        return false;
    }

    *srcDim = hgiTexture->GetDescriptor().dimensions;
    *srcTexture = srcTextureObject->GetTexture();
    *srcSampler = srcSamplerObject->GetSampler();

    return true;
}

bool
_ValidTextureObject(
    const HdStTextureObjectSharedPtr& textureObject)
{
    if (dynamic_cast<HdStDynamicCubemapTextureObject*>(textureObject.get())) {
        return true;
    }

    if (dynamic_cast<HdStDynamicUvTextureObject*>(textureObject.get())) {
        return true;
    }

    return false;
}

HgiShaderTextureType
_GetHgiShaderTextureType(
    const HdStTextureObjectSharedPtr& textureObject)
{
    if (dynamic_cast<HdStDynamicCubemapTextureObject*>(textureObject.get())) {
        return HgiShaderTextureTypeCubemapTexture;
    }

    return HgiShaderTextureTypeTexture;
}

int
_GetLayerCount(
    const HdStTextureObjectSharedPtr& textureObject)
{
    if (dynamic_cast<HdStDynamicCubemapTextureObject*>(textureObject.get())) {
        return 6;
    }

    return 1;
}

HgiTextureType
_GetHgiTextureType(
    const HdStTextureObjectSharedPtr& textureObject)
{
    if (dynamic_cast<HdStDynamicCubemapTextureObject*>(textureObject.get())) {
        return HgiTextureTypeCubemap;
    }

    return HgiTextureType2D;
}

void
_CreateTexture(
    HdStTextureObjectSharedPtr& textureObject,
    const HgiTextureDesc& desc)
{
    if (auto * const cubemapTextureObject =
            dynamic_cast<HdStDynamicCubemapTextureObject*>(
                textureObject.get())) {
        cubemapTextureObject->CreateTexture(desc);
    }

    if (auto * const uvTextureObject =
            dynamic_cast<HdStDynamicUvTextureObject*>(
                textureObject.get())) {
        uvTextureObject->CreateTexture(desc);
    }
}

HgiTextureHandle
_GetTextureHandle(
    const HdStTextureObjectSharedPtr& textureObject)
{
    if (const auto * const cubemapTextureObject =
            dynamic_cast<HdStDynamicCubemapTextureObject*>(
                textureObject.get())) {
        return cubemapTextureObject->GetTexture();
    }

    if (const auto * const uvTextureObject =
            dynamic_cast<HdStDynamicUvTextureObject*>(
                textureObject.get())) {
        return uvTextureObject->GetTexture();
    }

    return {};
}

HgiSamplerHandle
_GetSampler(
    const HdStSamplerObjectSharedPtr& samplerObject)
{
    if (const auto * const cubemapSamplerObject =
            dynamic_cast<HdStCubemapSamplerObject*>(
                samplerObject.get())) {
        return cubemapSamplerObject->GetSampler();
    }
    
    if (const auto * const uvSamplerObject =
            dynamic_cast<HdStUvSamplerObject*>(
                samplerObject.get())) {
        return uvSamplerObject->GetSampler();
    }

    return {};
}

}

// ----------------------------------------------------------------------------

HdSt_DomeLightComputationGPU::HdSt_DomeLightComputationGPU(
    const TfToken & shaderToken,
    const bool useCubemapAsSourceTexture,
    HdStSimpleLightingShaderPtr const &lightingShader,
    const unsigned int numLevels,
    const unsigned int level,
    const float roughness)
  : _shaderToken(shaderToken),
    _useCubemapAsSourceTexture(useCubemapAsSourceTexture),
    _lightingShader(lightingShader),
    _numLevels(numLevels),
    _level(level),
    _roughness(roughness)
{
}

void
HdSt_DomeLightComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const & /*range*/,
    HdResourceRegistry * const resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStSimpleLightingShaderSharedPtr const shader = _lightingShader.lock();
    if (!TF_VERIFY(shader)) {
        return;
    }

    // Get texture object from lighting shader that this computation is supposed
    // to populate.
    HdStTextureHandleSharedPtr const &dstTextureHandle =
        shader->GetTextureHandle(_shaderToken);
    if (!TF_VERIFY(dstTextureHandle)) {
        return;
    }

    // Depending on the shader, we are either creating a cubemap texture or
    // a 2D UV texture.
    auto dstTextureObject = dstTextureHandle->GetTextureObject();
    if (!TF_VERIFY(_ValidTextureObject(dstTextureObject))) {
        return;
    }

    constexpr int localSize = 8;
    const bool hasUniforms = _roughness >= 0.0f;

    HdStGLSLProgramSharedPtr const computeProgram =
        HdStGLSLProgram::GetComputeProgram(
            HdStPackageDomeLightShader(), 
            _shaderToken,
            "",
            static_cast<HdStResourceRegistry*>(resourceRegistry), 
            [&] (HgiShaderFunctionDesc &computeDesc) {
                computeDesc.debugName = _shaderToken.GetString();
                computeDesc.shaderStage = HgiShaderStageCompute;
                computeDesc.computeDescriptor.localSize = 
                    GfVec3i(localSize, localSize, 1);

                const HgiShaderTextureType inTextureType =
                    _useCubemapAsSourceTexture
                        ? HgiShaderTextureTypeCubemapTexture
                        : HgiShaderTextureTypeTexture;
                HgiShaderFunctionAddTexture(&computeDesc, "inTexture",
                    /*bindIndex = */0, /*dimensions = */2,
                    /*format = */HgiFormatFloat16Vec4,
                    /*textureType = */inTextureType);

                const HgiShaderTextureType outTextureType =
                    _GetHgiShaderTextureType(dstTextureObject);
                HgiShaderFunctionAddWritableTexture(&computeDesc, "outTexture",
                    /*bindIndex = */1, /*dimensions = */2,
                    /*format = */HgiFormatFloat16Vec4,
                    /*textureType = */outTextureType);
                if (hasUniforms) {
                    HgiShaderFunctionAddConstantParam(
                        &computeDesc, "inRoughness", HdStTokens->_float);
                }
                HgiShaderFunctionAddStageInput(
                    &computeDesc, "hd_GlobalInvocationID", "uvec3",
                    HgiShaderKeywordTokens->hdGlobalInvocationID);
            }
        );
    if (!TF_VERIFY(computeProgram)) {
        return;
    }

    // Data from source 2D or cubemap texture.
    GfVec3i srcDim;
    HgiTextureHandle srcTexture;
    HgiSamplerHandle srcSampler;

    // Size of texture to be created.
    int width = 0;
    int height = 0;
    if (_useCubemapAsSourceTexture) {
        // Source texture is the cubemap texture generated from the
        // latlong dome light texture.

        if (!_GetSrcDimensionsTextureAndSampler<
                HdStDynamicCubemapTextureObject,
                HdStCubemapSamplerObject>(
                    shader->GetDomeLightEnvironmentCubemapTextureHandle(),
                    &srcDim,
                    &srcTexture,
                    &srcSampler)) {
            return;
        }

        // Width and height are half that of the source cubemap.
        width = std::max(srcDim[0] / 2, 1);
        height = width;
    } else {
        // Source texture is the latlong dome light texture.

        if (!_GetSrcDimensionsTextureAndSampler<
                HdStAssetUvTextureObject,
                HdStUvSamplerObject>(
                    shader->GetDomeLightEnvironmentTextureHandle(),
                    &srcDim,
                    &srcTexture,
                    &srcSampler)) {
            return;
        }

        // We are either generating a cubemap with 6 equal-sized faces from the
        // latlong texture, or a single 2D lookup texture that will
        // be the size of one face of the cubemap.
        width = HdSt_ComputeDomeLightCubemapDimensions(srcDim)[0];
        height = width;
    }

    // Make sure dimensions align with the local size used in the compute shader
    width = _MakeMultipleOf(width, localSize);
    height = _MakeMultipleOf(height, localSize);

    const int layerCount = _GetLayerCount(dstTextureObject);

    if (_level == 0) {
        // Level zero is in charge of actually creating the GPU resource.
        HgiTextureDesc desc;
        desc.debugName = _shaderToken.GetText();
        desc.type = _GetHgiTextureType(dstTextureObject);
        desc.format = HgiFormatFloat16Vec4;
        desc.dimensions = GfVec3i(width, height, 1);
        desc.layerCount = layerCount;
        desc.mipLevels = _numLevels;
        desc.usage =
            HgiTextureUsageBitsShaderRead | HgiTextureUsageBitsShaderWrite;
        _FillPixelsByteSize(&desc);
        _CreateTexture(dstTextureObject, desc);
    }

    // Create a texture view for the layer we want to write to
    HgiTextureViewDesc texViewDesc;
    texViewDesc.layerCount = layerCount;
    texViewDesc.mipLevels = 1;
    texViewDesc.format = HgiFormatFloat16Vec4;
    texViewDesc.sourceFirstLayer = 0;
    texViewDesc.sourceFirstMip = _level;
    texViewDesc.sourceTexture = _GetTextureHandle(dstTextureObject);

    auto* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);
    Hgi* hgi = hdStResourceRegistry->GetHgi();
    HgiTextureViewHandle dstTextureView = hgi->CreateTextureView(texViewDesc);

    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = _shaderToken.GetString();

    HgiTextureBindDesc texBind0;
    texBind0.bindingIndex = 0;
    texBind0.stageUsage = HgiShaderStageCompute;
    texBind0.writable = false;
    texBind0.textures.push_back(srcTexture);
    texBind0.samplers.push_back(srcSampler);
    texBind0.resourceType = HgiBindResourceTypeCombinedSamplerImage;
    resourceDesc.textures.push_back(std::move(texBind0));

    HgiSamplerHandle dstSampler =
        _GetSampler(dstTextureHandle->GetSamplerObject());

    HgiTextureBindDesc texBind1;
    texBind1.bindingIndex = 1;
    texBind1.stageUsage = HgiShaderStageCompute;
    texBind1.writable = true;
    texBind1.textures.push_back(dstTextureView->GetViewTexture());
    texBind1.samplers.push_back(dstSampler);
    texBind1.resourceType = HgiBindResourceTypeStorageImage;
    resourceDesc.textures.push_back(std::move(texBind1));

    HgiResourceBindingsHandle resourceBindings =
        hgi->CreateResourceBindings(resourceDesc);
    
    // Prepare uniform buffer for GPU computation if needed.
    struct Uniforms {
        float roughness;
    } uniform;

    uniform.roughness = _roughness;

    HgiComputePipelineDesc desc;
    desc.debugName = _shaderToken.GetString();
    desc.shaderProgram = computeProgram->GetProgram();
    if (hasUniforms) {
        desc.shaderConstantsDesc.byteSize = sizeof(uniform);
    }
    HgiComputePipelineHandle pipeline = hgi->CreateComputePipeline(desc);

    HgiComputeCmds* computeCmds = hdStResourceRegistry->GetGlobalComputeCmds();

    computeCmds->PushDebugGroup(_shaderToken.GetText());
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
    computeCmds->Dispatch(width * layerCount, height);

    computeCmds->PopDebugGroup();

    // Garbage collect the intermediate resources (destroyed at end of frame).
    hgi->DestroyTextureView(&dstTextureView);
    hgi->DestroyComputePipeline(&pipeline);
    hgi->DestroyResourceBindings(&resourceBindings);
}

// ----------------------------------------------------------------------------

HdSt_DomeLightMipmapComputationGPU::HdSt_DomeLightMipmapComputationGPU(
    HdStSimpleLightingShaderPtr const &lightingShader)
  : _lightingShader(lightingShader)
{
}

void HdSt_DomeLightMipmapComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const & /*range*/,
    HdResourceRegistry * /*resourceRegistry*/)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStSimpleLightingShaderSharedPtr const shader = _lightingShader.lock();
    if (!TF_VERIFY(shader)) {
        return;
    }

    HdStTextureHandleSharedPtr const &srcTextureHandle =
        shader->GetDomeLightEnvironmentCubemapTextureHandle();
    if (!TF_VERIFY(srcTextureHandle)) {
        return;
    }

    auto * const srcTextureObject =
        dynamic_cast<HdStDynamicCubemapTextureObject*>(
            srcTextureHandle->GetTextureObject().get());
    if (!TF_VERIFY(srcTextureObject)) {
        return;
    }

    // We encapsulate this mipmap generation invocation into this computation
    // object so we can order it after the cubemap mip level 0 has been
    // populated, and prior to subsequent filtering commands that expect the
    // cubemap to have mipmaps.

    srcTextureObject->GenerateMipmaps();
}

GfVec3i HdSt_ComputeDomeLightCubemapDimensions(
    const GfVec3i& srcDim)
{
    const int width = std::max(srcDim[0] / 4, 1);
    return { width, width, 1 };
}

PXR_NAMESPACE_CLOSE_SCOPE
