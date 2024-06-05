//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/graphicsPipeline.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/shaderProgram.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"

#include "pxr/base/gf/half.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalGraphicsPipeline::HgiMetalGraphicsPipeline(
    HgiMetal *hgi,
    HgiGraphicsPipelineDesc const& desc)
    : HgiGraphicsPipeline(desc)
    , _vertexDescriptor(nil)
    , _depthStencilState(nil)
    , _renderPipelineState(nil)
    , _constantTessFactors(nil)
{
    _CreateVertexDescriptor();
    _CreateDepthStencilState(hgi);
    _CreateRenderPipelineState(hgi);
}

HgiMetalGraphicsPipeline::~HgiMetalGraphicsPipeline()
{
    if (_renderPipelineState) {
        [_renderPipelineState release];
    }
    if (_depthStencilState) {
        [_depthStencilState release];
    }
    if (_vertexDescriptor) {
        [_vertexDescriptor release];
    }
    if (_constantTessFactors) {
        [_constantTessFactors release];
    }
}

void
HgiMetalGraphicsPipeline::_CreateVertexDescriptor()
{
    _vertexDescriptor = [[MTLVertexDescriptor alloc] init];

    int index = 0;
    for (HgiVertexBufferDesc const& vbo : _descriptor.vertexBuffers) {

        HgiVertexAttributeDescVector const& vas = vbo.vertexAttributes;
        _vertexDescriptor.layouts[index].stride = vbo.vertexStride;

        // Set the vertex step rate such that the attribute index
        // will advance only according to the base instance at the
        // start of each draw command of a multi-draw. To do this
        // we set the vertex attribute to be constant and advance
        // the vertex buffer offset appropriately when encoding
        // draw commands.
        if (vbo.vertexStepFunction ==
                HgiVertexBufferStepFunctionConstant ||
            vbo.vertexStepFunction ==
                HgiVertexBufferStepFunctionPerDrawCommand) {
            _vertexDescriptor.layouts[index].stepFunction =
                MTLVertexStepFunctionConstant;
            _vertexDescriptor.layouts[index].stepRate = 0;
        } else if (vbo.vertexStepFunction ==
                HgiVertexBufferStepFunctionPerPatchControlPoint) {
            _vertexDescriptor.layouts[index].stepFunction =
                MTLVertexStepFunctionPerPatchControlPoint;
            _vertexDescriptor.layouts[index].stepRate = 1;
        }
        else {
            _vertexDescriptor.layouts[index].stepFunction =
                MTLVertexStepFunctionPerVertex;
            _vertexDescriptor.layouts[index].stepRate = 1;
        }

        // Describe each vertex attribute in the vertex buffer
        for (HgiVertexAttributeDesc const& va : vas) {
            uint32_t idx = va.shaderBindLocation;
            _vertexDescriptor.attributes[idx].format =
                HgiMetalConversions::GetVertexFormat(va.format);
            _vertexDescriptor.attributes[idx].bufferIndex = vbo.bindingIndex;
            _vertexDescriptor.attributes[idx].offset = va.offset;
        }

        index++;
    }
}

static
void
_SetTessellationStateFromShaderFunctions(
    MTLRenderPipelineDescriptor *stateDesc,
    HgiShaderFunctionHandleVector const & shaderFuncs)
{
    const HgiShaderFunctionHandle *postTessVertexFunc = nullptr;
    for (const HgiShaderFunctionHandle &handle : shaderFuncs) {
        if (handle->GetDescriptor().shaderStage ==
                HgiShaderStagePostTessellationVertex) {
            postTessVertexFunc = &handle;
        }
    }
    if (postTessVertexFunc == nullptr) {
        TF_CODING_ERROR("Did not find a post tess vertex function");
        return;
    }

    switch (postTessVertexFunc->Get()->GetDescriptor()
            .tessellationDescriptor.spacing) {
        case HgiShaderFunctionTessellationDesc::Spacing::Equal:
            stateDesc.tessellationPartitionMode =
                    MTLTessellationPartitionModeInteger;
            break;
        case HgiShaderFunctionTessellationDesc::Spacing::FractionalOdd:
            stateDesc.tessellationPartitionMode =
                    MTLTessellationPartitionModeFractionalOdd;
            break;
        case HgiShaderFunctionTessellationDesc::Spacing::FractionalEven:
            stateDesc.tessellationPartitionMode =
                    MTLTessellationPartitionModeFractionalEven;
            break;
        default:
            stateDesc.tessellationPartitionMode =
                    MTLTessellationPartitionModeInteger;
            break;
    }

    switch (postTessVertexFunc->Get()->GetDescriptor()
            .tessellationDescriptor.ordering) {
        case HgiShaderFunctionTessellationDesc::Ordering::CW:
            stateDesc.tessellationOutputWindingOrder =
                    MTLWindingClockwise;
            break;
        case HgiShaderFunctionTessellationDesc::Ordering::CCW:
            stateDesc.tessellationOutputWindingOrder =
                    MTLWindingCounterClockwise;
            break;
    }
}

void
HgiMetalGraphicsPipeline::_CreateRenderPipelineState(HgiMetal *hgi)
{
    MTLRenderPipelineDescriptor *stateDesc =
        [[MTLRenderPipelineDescriptor alloc] init];

    // Create a new render pipeline state object
    HGIMETAL_DEBUG_LABEL(stateDesc, _descriptor.debugName.c_str());
    stateDesc.rasterSampleCount = _descriptor.multiSampleState.sampleCount;
    
    bool const icbSupport = hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsIndirectCommandBuffers);
    stateDesc.supportIndirectCommandBuffers = icbSupport;

    stateDesc.inputPrimitiveTopology =
        HgiMetalConversions::GetPrimitiveClass(_descriptor.primitiveType);

    HgiMetalShaderProgram const *metalProgram =
        static_cast<HgiMetalShaderProgram*>(_descriptor.shaderProgram.Get());

    const bool usePTVSPath = _descriptor.primitiveType == HgiPrimitiveTypePatchList;
    if (usePTVSPath) {
        switch (_descriptor.tessellationState.tessFactorMode) {
            case HgiTessellationState::Constant:
                stateDesc.vertexFunction =
                        metalProgram->GetPostTessVertexFunction();
                stateDesc.tessellationFactorStepFunction =      
                        MTLTessellationFactorStepFunctionConstant;
                break;
            case HgiTessellationState::TessControl:
                stateDesc.vertexFunction =
                        metalProgram->GetPostTessControlFunction();
                stateDesc.tessellationFactorStepFunction =
                        MTLTessellationFactorStepFunctionConstant;
                stateDesc.tessellationPartitionMode =
                        MTLTessellationPartitionModePow2;
                break;
            case HgiTessellationState::TessVertex:
            {
                stateDesc.vertexFunction =
                        metalProgram->GetPostTessVertexFunction();
                stateDesc.tessellationFactorStepFunction =
                        MTLTessellationFactorStepFunctionPerPatch;

                _SetTessellationStateFromShaderFunctions(
                    stateDesc, metalProgram->GetShaderFunctions());
                break;
            }
            default:
                TF_CODING_ERROR("Unexpected tess factor mode");
        }

        // Basis curves should be treated as tris w/ polygonMode line
        // since Metal tessellation does not support isoline mode.
        if (stateDesc.inputPrimitiveTopology ==
                                MTLPrimitiveTopologyClassLine) {
            stateDesc.inputPrimitiveTopology =
                MTLPrimitiveTopologyClassTriangle;
        }

        if (_descriptor.tessellationState.patchType ==
                                HgiTessellationState::Isoline) {
             _descriptor.rasterizationState.polygonMode = HgiPolygonModeLine;
        }

        MTLWinding winding = HgiMetalConversions::GetWinding(
            _descriptor.rasterizationState.winding);
        //flip the tess winding
        winding = winding == MTLWindingClockwise ?
            MTLWindingCounterClockwise : MTLWindingClockwise;
        stateDesc.tessellationOutputWindingOrder = winding;

        stateDesc.tessellationControlPointIndexType =
            MTLTessellationControlPointIndexTypeUInt32;

        // Set maxTessellationFactor to be consistent with GL etc.
        stateDesc.maxTessellationFactor = 64;
    } else {
        stateDesc.vertexFunction = metalProgram->GetVertexFunction();
    }
    
    id<MTLFunction> fragFunction = metalProgram->GetFragmentFunction();
    if (fragFunction && _descriptor.rasterizationState.rasterizerEnabled) {
        stateDesc.fragmentFunction = fragFunction;
        stateDesc.rasterizationEnabled = YES;
    }
    else {
        stateDesc.rasterizationEnabled = NO;
    }

    // Color attachments
    for (size_t i=0; i<_descriptor.colorAttachmentDescs.size(); i++) {
        HgiAttachmentDesc const &hgiColorAttachment =
            _descriptor.colorAttachmentDescs[i];
        MTLRenderPipelineColorAttachmentDescriptor *metalColorAttachment =
            stateDesc.colorAttachments[i];
        
        metalColorAttachment.pixelFormat = HgiMetalConversions::GetPixelFormat(
            hgiColorAttachment.format, hgiColorAttachment.usage);

        metalColorAttachment.writeMask = HgiMetalConversions::GetColorWriteMask(
            hgiColorAttachment.colorMask);

        if (hgiColorAttachment.blendEnabled) {
            metalColorAttachment.blendingEnabled = YES;
            
            metalColorAttachment.sourceRGBBlendFactor =
                HgiMetalConversions::GetBlendFactor(
                    hgiColorAttachment.srcColorBlendFactor);
            metalColorAttachment.destinationRGBBlendFactor =
                HgiMetalConversions::GetBlendFactor(
                    hgiColorAttachment.dstColorBlendFactor);
            
            metalColorAttachment.sourceAlphaBlendFactor =
                HgiMetalConversions::GetBlendFactor(
                    hgiColorAttachment.srcAlphaBlendFactor);
            metalColorAttachment.destinationAlphaBlendFactor =
                HgiMetalConversions::GetBlendFactor(
                    hgiColorAttachment.dstAlphaBlendFactor);

            metalColorAttachment.rgbBlendOperation =
                HgiMetalConversions::GetBlendEquation(
                    hgiColorAttachment.colorBlendOp);
            metalColorAttachment.alphaBlendOperation =
                HgiMetalConversions::GetBlendEquation(
                    hgiColorAttachment.alphaBlendOp);
        }
        else {
            metalColorAttachment.blendingEnabled = NO;
        }
    }
    
    HgiAttachmentDesc const &hgiDepthAttachment =
        _descriptor.depthAttachmentDesc;

    MTLPixelFormat depthPixelFormat = HgiMetalConversions::GetPixelFormat(
        hgiDepthAttachment.format, hgiDepthAttachment.usage);

    stateDesc.depthAttachmentPixelFormat = depthPixelFormat;
    
    if (_descriptor.depthAttachmentDesc.usage & 
        HgiTextureUsageBitsStencilTarget) {
        stateDesc.stencilAttachmentPixelFormat = depthPixelFormat;
    }

    stateDesc.sampleCount = _descriptor.multiSampleState.sampleCount;
    if (_descriptor.multiSampleState.alphaToCoverageEnable) {
        stateDesc.alphaToCoverageEnabled = YES;
    } else {
        stateDesc.alphaToCoverageEnabled = NO;
    }
    if (_descriptor.multiSampleState.alphaToOneEnable) {
        stateDesc.alphaToOneEnabled = YES;
    } else {
        stateDesc.alphaToOneEnabled = NO;
    }

    stateDesc.vertexDescriptor = _vertexDescriptor;

    NSError *error = NULL;
    id<MTLDevice> device = hgi->GetPrimaryDevice();
    _renderPipelineState = [device
        newRenderPipelineStateWithDescriptor:stateDesc
        error:&error];
    [stateDesc release];
    
    if (!_renderPipelineState) {
        NSString *err = [error localizedDescription];
        TF_WARN("Failed to created pipeline state, error %s",
            [err UTF8String]);
    }
}

static MTLStencilDescriptor *
_CreateStencilDescriptor(HgiStencilState const & stencilState)
{
    MTLStencilDescriptor *stencilDescriptor =
        [[MTLStencilDescriptor alloc] init];

    stencilDescriptor.stencilCompareFunction =
        HgiMetalConversions::GetCompareFunction(stencilState.compareFn);
    stencilDescriptor.stencilFailureOperation =
        HgiMetalConversions::GetStencilOp(stencilState.stencilFailOp);
    stencilDescriptor.depthFailureOperation =
        HgiMetalConversions::GetStencilOp(stencilState.depthFailOp);
    stencilDescriptor.depthStencilPassOperation =
        HgiMetalConversions::GetStencilOp(stencilState.depthStencilPassOp);
    stencilDescriptor.readMask = stencilState.readMask;
    stencilDescriptor.writeMask = stencilState.writeMask;

    return stencilDescriptor;
}

void
HgiMetalGraphicsPipeline::_CreateDepthStencilState(HgiMetal *hgi)
{
    MTLDepthStencilDescriptor *depthStencilStateDescriptor =
        [[MTLDepthStencilDescriptor alloc] init];
    
    HGIMETAL_DEBUG_LABEL(
        depthStencilStateDescriptor, _descriptor.debugName.c_str());

    if (_descriptor.depthState.depthTestEnabled) {
        MTLCompareFunction depthFn = HgiMetalConversions::GetCompareFunction(
            _descriptor.depthState.depthCompareFn);
        depthStencilStateDescriptor.depthCompareFunction = depthFn;
        if (_descriptor.depthState.depthWriteEnabled) {
            depthStencilStateDescriptor.depthWriteEnabled = YES;
        }
        else {
            depthStencilStateDescriptor.depthWriteEnabled = NO;
        }
    }
    else {
        // Even if there is no depth attachment, some drivers may still perform
        // the depth test. So we pick Always over Never.
        depthStencilStateDescriptor.depthCompareFunction =
            MTLCompareFunctionAlways;
        depthStencilStateDescriptor.depthWriteEnabled = NO;
    }
    
    if (_descriptor.depthState.stencilTestEnabled) {
        depthStencilStateDescriptor.backFaceStencil =
            _CreateStencilDescriptor(_descriptor.depthState.stencilFront);
        depthStencilStateDescriptor.frontFaceStencil =
            _CreateStencilDescriptor(_descriptor.depthState.stencilBack);
    }
    
    id<MTLDevice> device = hgi->GetPrimaryDevice();
    _depthStencilState = [device
        newDepthStencilStateWithDescriptor:depthStencilStateDescriptor];
    [depthStencilStateDescriptor release];

    TF_VERIFY(_depthStencilState,
        "Failed to created depth stencil state");
}

void
HgiMetalGraphicsPipeline::BindPipeline(id<MTLRenderCommandEncoder> renderEncoder)
{
    [renderEncoder setRenderPipelineState:_renderPipelineState];
    if (_descriptor.primitiveType == HgiPrimitiveTypePatchList) {
        if (_constantTessFactors == nullptr) {

            // tess factors are half floats encoded as uint16_t
            uint16_t const factorZero =
                    reinterpret_cast<uint16_t>(GfHalf(0.0f).bits());
            uint16_t const factorOne =
                    reinterpret_cast<uint16_t>(GfHalf(1.0f).bits());

            if (_descriptor.tessellationState.patchType ==
                        HgiTessellationState::PatchType::Triangle) {
                MTLTriangleTessellationFactorsHalf triangleFactors;
                triangleFactors.insideTessellationFactor = factorZero;
                triangleFactors.edgeTessellationFactor[0] = factorOne;
                triangleFactors.edgeTessellationFactor[1] = factorOne;
                triangleFactors.edgeTessellationFactor[2] = factorOne;
                _constantTessFactors =
                    [renderEncoder.device
                        newBufferWithBytes:&triangleFactors
                                    length:sizeof(triangleFactors)
                                    options:MTLResourceStorageModeShared];
            } else { // is Quad tess factor
                MTLQuadTessellationFactorsHalf quadFactors;
                quadFactors.insideTessellationFactor[0] = factorZero;
                quadFactors.insideTessellationFactor[1] = factorZero;
                quadFactors.edgeTessellationFactor[0] = factorOne;
                quadFactors.edgeTessellationFactor[1] = factorOne;
                quadFactors.edgeTessellationFactor[2] = factorOne;
                quadFactors.edgeTessellationFactor[3] = factorOne;
                _constantTessFactors =
                    [renderEncoder.device
                        newBufferWithBytes:&quadFactors
                                    length:sizeof(quadFactors)
                                   options:MTLResourceStorageModeShared];
            }
        }
        [renderEncoder setTessellationFactorBuffer:_constantTessFactors
                                            offset: 0
                                    instanceStride: 0];
    }

    //
    // DepthStencil state
    //
    HgiDepthStencilState const & dsState = _descriptor.depthState;
    if (_descriptor.depthState.depthBiasEnabled) {
        [renderEncoder
            setDepthBias: dsState.depthBiasConstantFactor
              slopeScale: dsState.depthBiasSlopeFactor
                   clamp: 0.0f];
    }

    if (_descriptor.depthState.stencilTestEnabled) {
        [renderEncoder
            setStencilFrontReferenceValue: dsState.stencilFront.referenceValue
                       backReferenceValue: dsState.stencilBack.referenceValue];
    }

    //
    // Rasterization state
    //
    [renderEncoder setCullMode:HgiMetalConversions::GetCullMode(
        _descriptor.rasterizationState.cullMode)];
    [renderEncoder setTriangleFillMode:HgiMetalConversions::GetPolygonMode(
        _descriptor.rasterizationState.polygonMode)];
    [renderEncoder setFrontFacingWinding:HgiMetalConversions::GetWinding(
        _descriptor.rasterizationState.winding)];
    [renderEncoder setDepthStencilState:_depthStencilState];

    if (_descriptor.rasterizationState.depthClampEnabled) {
        [renderEncoder
            setDepthClipMode: MTLDepthClipModeClamp];     
    }

    TF_VERIFY(_descriptor.rasterizationState.lineWidth == 1.0f,
        "Missing implementation buffers");
}

PXR_NAMESPACE_CLOSE_SCOPE
