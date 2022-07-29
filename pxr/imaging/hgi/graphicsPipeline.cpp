//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hgi/graphicsPipeline.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiVertexAttributeDesc::HgiVertexAttributeDesc()
    : format(HgiFormatFloat32Vec4)
    , offset(0)
    , shaderBindLocation(0)
{
}

bool operator==(
    const HgiVertexAttributeDesc& lhs,
    const HgiVertexAttributeDesc& rhs)
{
    return lhs.format == rhs.format &&
           lhs.offset == rhs.offset &&
           lhs.shaderBindLocation == rhs.shaderBindLocation;
}

bool operator!=(
    const HgiVertexAttributeDesc& lhs,
    const HgiVertexAttributeDesc& rhs)
{
    return !(lhs == rhs);
}

HgiVertexBufferDesc::HgiVertexBufferDesc()
    : bindingIndex(0)
    , vertexStepFunction(HgiVertexBufferStepFunctionPerVertex)
    , vertexStride(0)
{
}

bool operator==(
    const HgiVertexBufferDesc& lhs,
    const HgiVertexBufferDesc& rhs)
{
    return lhs.bindingIndex == rhs.bindingIndex &&
           lhs.vertexAttributes == rhs.vertexAttributes &&
           lhs.vertexStepFunction == rhs.vertexStepFunction &&
           lhs.vertexStride == rhs.vertexStride;
}

bool operator!=(
    const HgiVertexBufferDesc& lhs,
    const HgiVertexBufferDesc& rhs)
{
    return !(lhs == rhs);
}

HgiMultiSampleState::HgiMultiSampleState()
    : multiSampleEnable(true)
    , alphaToCoverageEnable(false)
    , alphaToOneEnable(false)
    , sampleCount(HgiSampleCount1)
{
}

bool operator==(
    const HgiMultiSampleState& lhs,
    const HgiMultiSampleState& rhs)
{
    return lhs.multiSampleEnable == rhs.multiSampleEnable &&
           lhs.alphaToCoverageEnable == rhs.alphaToCoverageEnable &&
           lhs.alphaToOneEnable == rhs.alphaToOneEnable &&
           lhs.sampleCount == rhs.sampleCount;
}

bool operator!=(
    const HgiMultiSampleState& lhs,
    const HgiMultiSampleState& rhs)
{
    return !(lhs == rhs);
}

HgiRasterizationState::HgiRasterizationState()
    : polygonMode(HgiPolygonModeFill)
    , lineWidth(1.0f)
    , cullMode(HgiCullModeBack)
    , winding(HgiWindingCounterClockwise)
    , rasterizerEnabled(true)
    , depthClampEnabled(false)
    , depthRange(0.f, 1.f)
    , conservativeRaster(false)
    , numClipDistances(0)
{
}

bool operator==(
    const HgiRasterizationState& lhs,
    const HgiRasterizationState& rhs)
{
    return lhs.polygonMode == rhs.polygonMode &&
           lhs.lineWidth == rhs.lineWidth &&
           lhs.cullMode == rhs.cullMode &&
           lhs.winding == rhs.winding &&
           lhs.rasterizerEnabled == rhs.rasterizerEnabled &&
           lhs.depthClampEnabled == rhs.depthClampEnabled &&
           lhs.depthRange == rhs.depthRange &&
           lhs.conservativeRaster == rhs.conservativeRaster &&
           lhs.numClipDistances == rhs.numClipDistances;
}

bool operator!=(
    const HgiRasterizationState& lhs,
    const HgiRasterizationState& rhs)
{
    return !(lhs == rhs);
}

HgiDepthStencilState::HgiDepthStencilState()
    : depthTestEnabled(true)
    , depthWriteEnabled(true)
    , depthCompareFn(HgiCompareFunctionLess)
    , depthBiasEnabled(false)
    , depthBiasConstantFactor(0.0f)
    , depthBiasSlopeFactor(0.0f)
    , stencilTestEnabled(false)
    , stencilFront()
    , stencilBack()
{
}

bool operator==(
    const HgiDepthStencilState& lhs,
    const HgiDepthStencilState& rhs)
{
    return lhs.depthTestEnabled == rhs.depthTestEnabled &&
           lhs.depthWriteEnabled == rhs.depthWriteEnabled &&
           lhs.depthCompareFn == rhs.depthCompareFn &&
           lhs.depthBiasEnabled == rhs.depthBiasEnabled &&
           lhs.depthBiasConstantFactor == rhs.depthBiasConstantFactor &&
           lhs.depthBiasSlopeFactor == rhs.depthBiasSlopeFactor &&
           lhs.stencilTestEnabled == rhs.stencilTestEnabled &&
           lhs.stencilFront == rhs.stencilFront &&
           lhs.stencilBack == rhs.stencilBack;
}

bool operator!=(
    const HgiDepthStencilState& lhs,
    const HgiDepthStencilState& rhs)
{
    return !(lhs == rhs);
}

HgiStencilState::HgiStencilState()
    : compareFn(HgiCompareFunctionAlways)
    , referenceValue(0)
    , stencilFailOp(HgiStencilOpKeep)
    , depthFailOp(HgiStencilOpKeep)
    , depthStencilPassOp(HgiStencilOpKeep)
    , readMask(0xffffffff)
    , writeMask(0xffffffff)
{
}

bool operator==(
    const HgiStencilState& lhs,
    const HgiStencilState& rhs)
{
    return lhs.compareFn == rhs.compareFn &&
           lhs.referenceValue == rhs.referenceValue &&
           lhs.stencilFailOp == rhs.stencilFailOp &&
           lhs.depthFailOp == rhs.depthFailOp &&
           lhs.depthStencilPassOp == rhs.depthStencilPassOp &&
           lhs.readMask == rhs.readMask &&
           lhs.writeMask == rhs.writeMask;
}

bool operator!=(
    const HgiStencilState& lhs,
    const HgiStencilState& rhs)
{
    return !(lhs == rhs);
}

HgiGraphicsShaderConstantsDesc::HgiGraphicsShaderConstantsDesc()
    : byteSize(0)
    , stageUsage(HgiShaderStageFragment)
{
}

bool operator==(
    const HgiGraphicsShaderConstantsDesc& lhs,
    const HgiGraphicsShaderConstantsDesc& rhs)
{
    return lhs.byteSize == rhs.byteSize &&
           lhs.stageUsage == rhs.stageUsage;
}

bool operator!=(
    const HgiGraphicsShaderConstantsDesc& lhs,
    const HgiGraphicsShaderConstantsDesc& rhs)
{
    return !(lhs == rhs);
}

HgiTessellationLevel::HgiTessellationLevel()
    : innerTessLevel{0, 0}
    , outerTessLevel{0, 0, 0, 0}
{
}

HgiTessellationState::HgiTessellationState()
    : patchType(Triangle)
    , primitiveIndexSize(0)
    , tessellationLevel()
{
}

HgiGraphicsPipelineDesc::HgiGraphicsPipelineDesc()
    : primitiveType(HgiPrimitiveTypeTriangleList)
{
}

HgiGraphicsPipelineDesc const&
HgiGraphicsPipeline::GetDescriptor() const
{
    return _descriptor;
}

bool operator==(
    const HgiGraphicsPipelineDesc& lhs,
    const HgiGraphicsPipelineDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.primitiveType == rhs.primitiveType &&
           lhs.shaderProgram == rhs.shaderProgram &&
           lhs.depthState == rhs.depthState &&
           lhs.multiSampleState == rhs.multiSampleState &&
           lhs.rasterizationState == rhs.rasterizationState &&
           lhs.vertexBuffers == rhs.vertexBuffers &&
           lhs.colorAttachmentDescs == rhs.colorAttachmentDescs &&
           lhs.colorResolveAttachmentDescs == rhs.colorResolveAttachmentDescs &&
           lhs.depthAttachmentDesc == rhs.depthAttachmentDesc &&
           lhs.depthResolveAttachmentDesc == rhs.depthResolveAttachmentDesc &&
           lhs.shaderConstantsDesc == rhs.shaderConstantsDesc;
}

bool operator!=(
    const HgiGraphicsPipelineDesc& lhs,
    const HgiGraphicsPipelineDesc& rhs)
{
    return !(lhs == rhs);
}

HgiGraphicsPipeline::HgiGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
    : _descriptor(desc)
{
}

HgiGraphicsPipeline::~HgiGraphicsPipeline() = default;

PXR_NAMESPACE_CLOSE_SCOPE
