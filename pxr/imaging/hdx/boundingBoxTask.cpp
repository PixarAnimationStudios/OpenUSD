//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/boundingBoxTask.h"

#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/capabilities.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/gf/transform.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((boundingBoxVertex,    "BoundingBoxVertex"))
    ((boundingBoxFragment,  "BoundingBoxFragment"))
    (boundingBoxShader)
);

namespace {

// Constants struct that has a layout matching what is expected by the GPU.
// This includes constant data for both vertex and fragment stages.
struct _ShaderConstants
{
    GfVec4f color;
    GfVec4f viewport;
    float dashSize;
};

}

HdxBoundingBoxTask::HdxBoundingBoxTask(
    HdSceneDelegate* delegate,
    const SdfPath& id)
  : HdxTask(id)
  , _vertexBuffer()
  , _maxTransforms(2)
  , _transformsBuffer()
  , _shaderProgram()
  , _resourceBindings()
  , _pipeline()
  , _params()
{
}

HdxBoundingBoxTask::~HdxBoundingBoxTask()
{
    if (_vertexBuffer) {
        _GetHgi()->DestroyBuffer(&_vertexBuffer);
    }

    if (_transformsBuffer) {
        _GetHgi()->DestroyBuffer(&_transformsBuffer);
    }

    if (_shaderProgram) {
        _DestroyShaderProgram();
    }

    if (_resourceBindings) {
        _GetHgi()->DestroyResourceBindings(&_resourceBindings);
    }

    if (_pipeline) {
        _GetHgi()->DestroyGraphicsPipeline(&_pipeline);
    }
}

bool
HdxBoundingBoxTask::_CreateShaderResources()
{
    if (_shaderProgram) {
        return true;
    }

    const HioGlslfx glslfx(
        HdxPackageBoundingBoxShader(), HioGlslfxTokens->defVal);

    // Using a constant buffer that contains data for both vertex and
    // fragment stages for simplicity.
    auto addConstantParams = [](HgiShaderFunctionDesc * stageDesc)
    {
        HgiShaderFunctionAddConstantParam(
            stageDesc, "color", "vec4");
        HgiShaderFunctionAddConstantParam(
            stageDesc, "viewport", "vec4");
        HgiShaderFunctionAddConstantParam(
            stageDesc, "dashSize", "float");
    };

    // Setup the vertex shader
    std::string vsCode;
    HgiShaderFunctionDesc vertDesc;
    vertDesc.debugName = _tokens->boundingBoxVertex.GetString();
    vertDesc.shaderStage = HgiShaderStageVertex;
    HgiShaderFunctionAddStageInput(
        &vertDesc, "position", "vec3");
    HgiShaderFunctionAddStageInput(
        &vertDesc, "hd_InstanceID", "uint",
        HgiShaderKeywordTokens->hdInstanceID);
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "gl_Position", "vec4", "position");
    HgiShaderFunctionParamDesc dashStartParam;
    dashStartParam.nameInShader = "dashStart";
    dashStartParam.type = "vec2";
    dashStartParam.interpolation = HgiInterpolationFlat;
    HgiShaderFunctionAddStageOutput(
        &vertDesc, dashStartParam);
    addConstantParams(&vertDesc);
    HgiShaderFunctionAddBuffer(
        &vertDesc, "worldViewProj", "mat4", 1, HgiBindingTypeUniformArray,
        static_cast<uint32_t>(_maxTransforms));
    vsCode += glslfx.GetSource(_tokens->boundingBoxVertex);
    vertDesc.shaderCode = vsCode.c_str();
    HgiShaderFunctionHandle vertFn = _GetHgi()->CreateShaderFunction(vertDesc);

    // Setup the fragment shader
    std::string fsCode;
    HgiShaderFunctionDesc fragDesc;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "gl_FragCoord", "vec4", HgiShaderKeywordTokens->hdFragCoord);
    HgiShaderFunctionAddStageInput(
        &fragDesc, dashStartParam);
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");
    addConstantParams(&fragDesc);
    fragDesc.debugName = _tokens->boundingBoxFragment.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    fsCode += glslfx.GetSource(_tokens->boundingBoxFragment);

    fragDesc.shaderCode = fsCode.c_str();
    HgiShaderFunctionHandle fragFn = _GetHgi()->CreateShaderFunction(fragDesc);

    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName =_tokens->boundingBoxShader.GetString();
    programDesc.shaderFunctions.push_back(std::move(vertFn));
    programDesc.shaderFunctions.push_back(std::move(fragFn));
    _shaderProgram = _GetHgi()->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create bounding box shader");
        _PrintCompileErrors();
        _DestroyShaderProgram();
        return false;
    }

    return true;
}

bool
HdxBoundingBoxTask::_CreateBufferResources()
{
    if (_vertexBuffer && _transformsBuffer) {
        if (_params.bboxes.size() <= _maxTransforms) {
            return true;
        }

        // Must re-create any objects that depend on the transform buffer size
        // directly and any objects that depend on those re-created objects.
        _GetHgi()->DestroyGraphicsPipeline(&_pipeline);
        _DestroyShaderProgram();
        _GetHgi()->DestroyResourceBindings(&_resourceBindings);
        _GetHgi()->DestroyBuffer(&_transformsBuffer);
    }

    if (!_vertexBuffer) {
        // 12 edges of a cube with sides of length 2, centered at the origin
        constexpr float vertData[24][3] = {
            { -1, -1, -1 }, { -1, -1, +1 },
            { -1, +1, -1 }, { -1, +1, +1 },
            { +1, -1, -1 }, { +1, -1, +1 },
            { +1, +1, -1 }, { +1, +1, +1 },

            { -1, -1, -1 }, { -1, +1, -1 },
            { +1, -1, -1 }, { +1, +1, -1 },
            { -1, -1, +1 }, { -1, +1, +1 },
            { +1, -1, +1 }, { +1, +1, +1 },

            { -1, -1, -1 }, { +1, -1, -1 },
            { -1, +1, -1 }, { +1, +1, -1 },
            { -1, -1, +1 }, { +1, -1, +1 },
            { -1, +1, +1 }, { +1, +1, +1 },
        };

        HgiBufferDesc vboDesc;
        vboDesc.debugName = "HdxBoundingBoxTask VertexBuffer";
        vboDesc.usage = HgiBufferUsageVertex;
        vboDesc.initialData = vertData;
        vboDesc.byteSize = sizeof(vertData);
        vboDesc.vertexStride = sizeof(vertData[0]);
        _vertexBuffer = _GetHgi()->CreateBuffer(vboDesc);
    }

    // Uniform array of transforms for the bboxes
    _maxTransforms = _params.bboxes.size();

    HgiBufferDesc transformsDesc;
    transformsDesc.debugName = "HdxBoundingBoxTask TransformsBuffer";
    transformsDesc.usage = HgiBufferUsageUniform;
    transformsDesc.byteSize = 16 * sizeof(float) * _maxTransforms;
    _transformsBuffer = _GetHgi()->CreateBuffer(transformsDesc);

    return true;
}

bool
HdxBoundingBoxTask::_CreateResourceBindings()
{
    if (_resourceBindings) {
        return true;
    }

    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "BoundingBox";

    // Transform array used only in the vertex shader.
    // Note this binds at index 1 since shader constants are also used, which
    // will bind at index 0 on some backends.
    HgiBufferBindDesc bufBind1;
    bufBind1.bindingIndex = 1;
    bufBind1.resourceType = HgiBindResourceTypeUniformBuffer;
    bufBind1.stageUsage = HgiShaderStageVertex;
    bufBind1.offsets.push_back(0);
    bufBind1.sizes.push_back(0);
    bufBind1.buffers.push_back(_transformsBuffer);
    bufBind1.writable = false;
    resourceDesc.buffers.push_back(std::move(bufBind1));

    _resourceBindings = _GetHgi()->CreateResourceBindings(resourceDesc);

    return true;
}

static
bool
_MatchesFormatAndSampleCount(
    const HgiTextureHandle& texture,
    const HgiFormat format,
    const HgiSampleCount sampleCount)
{
    if (texture) {
        const HgiTextureDesc& desc = texture->GetDescriptor();
        return format == desc.format && sampleCount == desc.sampleCount;
    }
    return false;
}

bool
HdxBoundingBoxTask::_CreatePipeline(
    const HgiTextureHandle& colorTexture,
    const HgiTextureHandle& depthTexture)
{
    if (_pipeline) {
        const HgiSampleCount sampleCount =
            _pipeline->GetDescriptor().multiSampleState.sampleCount;

        if ( _MatchesFormatAndSampleCount(
                 colorTexture, _colorAttachment.format, sampleCount) &&
             _MatchesFormatAndSampleCount(
                 depthTexture, _depthAttachment.format, sampleCount)) {
            return true;
        }

        _GetHgi()->DestroyGraphicsPipeline(&_pipeline);
    }

    HgiGraphicsPipelineDesc desc;
    desc.debugName = "BoundingBox Pipeline";
    desc.primitiveType = HgiPrimitiveTypeLineList;
    desc.shaderProgram = _shaderProgram;

    // Describe the vertex buffer
    HgiVertexAttributeDesc posAttr;
    posAttr.format = HgiFormatFloat32Vec3;
    posAttr.offset = 0;
    posAttr.shaderBindLocation = 0;

    HgiVertexBufferDesc vboDesc;
    vboDesc.bindingIndex = 0;
    vboDesc.vertexStride = 3 * sizeof(float); // pos
    vboDesc.vertexAttributes.push_back(posAttr);

    desc.vertexBuffers.push_back(std::move(vboDesc));

    // The MSAA on renderPipelineState has to match the render target.
    const HgiSampleCount sampleCount =
        colorTexture->GetDescriptor().sampleCount;
    desc.multiSampleState.multiSampleEnable = sampleCount != HgiSampleCount1;
    desc.multiSampleState.sampleCount = sampleCount;

    // Setup color attachment descriptor
    _colorAttachment.format = colorTexture->GetDescriptor().format;
    _colorAttachment.usage = colorTexture->GetDescriptor().usage;
    desc.colorAttachmentDescs.push_back(_colorAttachment);

    // Setup depth attachment descriptor
    _depthAttachment.format = depthTexture->GetDescriptor().format;
    _depthAttachment.usage = depthTexture->GetDescriptor().usage;
    desc.depthAttachmentDesc = _depthAttachment;

    // Shared constants used in both vertex and fragment stages.
    desc.shaderConstantsDesc.stageUsage =
       HgiShaderStageVertex|HgiShaderStageFragment;
    desc.shaderConstantsDesc.byteSize = sizeof(_ShaderConstants);

    _pipeline = _GetHgi()->CreateGraphicsPipeline(desc);

    return true;
}

GfMatrix4d
HdxBoundingBoxTask::_ComputeViewProjectionMatrix(
    const HdStRenderPassState& hdStRenderPassState) const
{
    // Get the view and projection matrices.
    const GfMatrix4d view = hdStRenderPassState.GetWorldToViewMatrix();
    GfMatrix4d projection = hdStRenderPassState.GetProjectionMatrix();

    const HgiCapabilities* capabilities = _GetHgi()->GetCapabilities();
    if (!capabilities->IsSet(
        HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne)) {
        // Different backends use different clip space depth ranges. The
        // codebase generally assumes an OpenGL-style depth of [-1, 1] when
        // computing projection matrices, so we must add an additional
        // conversion when the Hgi backend expects a [0, 1] depth range.
        GfMatrix4d depthAdjustmentMat(
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.0, 0.0, 0.5, 1.0);
        projection = projection * depthAdjustmentMat;
    }

    return view * projection;
}

static
GfMatrix4d
_GetWorldMatrixFromBBox(const GfBBox3d& bbox)
{
    // Convert bbox to a matrix that can be applied to the cube line geometry.
    GfTransform worldTransform;
    const GfRange3d& range = bbox.GetRange();
    worldTransform.SetScale(0.5*(range.GetMax() - range.GetMin()));
    worldTransform.SetTranslation(range.GetMidpoint());
    return worldTransform.GetMatrix() * bbox.GetMatrix();
}

void
HdxBoundingBoxTask::_UpdateShaderConstants(
    HgiGraphicsCmds* gfxCmds,
    const GfVec4i& gfxViewport,
    const HdStRenderPassState& hdStRenderPassState)
{
    // View-Projection matrix is the same for either bbox
    const GfMatrix4d viewProj =
        _ComputeViewProjectionMatrix(hdStRenderPassState);

    // Populate the transforms based on the provided bboxes
    using TransformVector = std::vector<GfMatrix4f>;
    TransformVector transforms(_maxTransforms, GfMatrix4f(1.0f));
    for (size_t i = 0; i < _params.bboxes.size(); ++i) {
        const GfMatrix4d world = _GetWorldMatrixFromBBox(_params.bboxes[i]);
        transforms[i] = GfMatrix4f(world * viewProj);
    }

    // Upload the transform data to the GPU.
    void* transformsStaging = _transformsBuffer->GetCPUStagingAddress();
    memcpy(transformsStaging,
           transforms.data(),
           16 * sizeof(float) * _maxTransforms);

    HgiBufferCpuToGpuOp transformsBlit;
    transformsBlit.cpuSourceBuffer = transformsStaging;
    transformsBlit.sourceByteOffset = 0;
    transformsBlit.gpuDestinationBuffer = _transformsBuffer;
    transformsBlit.destinationByteOffset = 0;
    transformsBlit.byteSize = 16 * sizeof(float) * _maxTransforms;

    HgiBlitCmdsUniquePtr blitCmds = _GetHgi()->CreateBlitCmds();
    blitCmds->CopyBufferCpuToGpu(transformsBlit);
    _GetHgi()->SubmitCmds(blitCmds.get());

    // Update and upload the other constant data.
    const GfVec4f color(
        GfClamp(_params.color[0], 0.0f, 1.0f),
        GfClamp(_params.color[1], 0.0f, 1.0f),
        GfClamp(_params.color[2], 0.0f, 1.0f),
        GfClamp(_params.color[3], 0.0f, 1.0f));

    const GfVec4f viewport(gfxViewport);

    // dashSize smaller than 1 pixel disables any line pattern
    const float dashSize = _params.dashSize < 1.0f ? 0.0f : _params.dashSize;

    const _ShaderConstants constants = {
        color,
        viewport,
        dashSize
    };

    gfxCmds->SetConstantValues(
        _pipeline,
        HgiShaderStageVertex|HgiShaderStageFragment,
        0,
        sizeof(_ShaderConstants),
        &constants);
}

void
HdxBoundingBoxTask::_DrawBBoxes(
    const HgiTextureHandle& colorTexture,
    const HgiTextureHandle& depthTexture,
    const HdStRenderPassState& hdStRenderPassState)
{
    // Prepare graphics cmds.
    HgiGraphicsCmdsDesc gfxDesc;
    gfxDesc.colorAttachmentDescs.push_back(_colorAttachment);
    gfxDesc.colorTextures.push_back(colorTexture);
    gfxDesc.depthAttachmentDesc = _depthAttachment;
    gfxDesc.depthTexture = depthTexture;

    // Begin rendering
    HgiGraphicsCmdsUniquePtr gfxCmds = _GetHgi()->CreateGraphicsCmds(gfxDesc);
    gfxCmds->PushDebugGroup("BoundingBox");
    gfxCmds->BindPipeline(_pipeline);
    gfxCmds->BindVertexBuffers({{_vertexBuffer, 0, 0}});

    const GfVec4i viewport = hdStRenderPassState.ComputeViewport();
    gfxCmds->SetViewport(viewport);

    _UpdateShaderConstants(gfxCmds.get(), viewport, hdStRenderPassState);
    gfxCmds->BindResources(_resourceBindings);

    const uint32_t instanceCount = static_cast<uint32_t>(_params.bboxes.size());
    gfxCmds->Draw(24, 0, instanceCount, 0);

    gfxCmds->PopDebugGroup();

    // Done recording commands, submit work.
    _GetHgi()->SubmitCmds(gfxCmds.get());
}

void
HdxBoundingBoxTask::_Sync(HdSceneDelegate* delegate,
                          HdTaskContext* ctx,
                          HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        _GetTaskParams(delegate, &_params);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxBoundingBoxTask::Prepare(HdTaskContext* ctx,
                            HdRenderIndex* renderIndex)
{
}

void
HdxBoundingBoxTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Only draw the bounding boxes when rendering to the color aov
    if (_params.bboxes.empty() || _params.aovName != HdAovTokens->color) {
        return;
    }

    // We want to render the bounding boxes into the color aov
    // and have them respect the depth aov.
    if (!_HasTaskContextData(ctx, HdAovTokens->color) ||
        !_HasTaskContextData(ctx, HdAovTokens->depth)) {
        return;
    }

    HgiTextureHandle colorTexture, depthTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &colorTexture);
    _GetTaskContextData(ctx, HdAovTokens->depth, &depthTexture);

    if (!TF_VERIFY(_CreateBufferResources())) {
        return;
    }
    if (!TF_VERIFY(_CreateShaderResources())) {
        return;
    }
    if (!TF_VERIFY(_CreateResourceBindings())) {
        return;
    }
    if (!TF_VERIFY(_CreatePipeline(colorTexture, depthTexture))) {
        return;
    }

    HdRenderPassStateSharedPtr renderPassState;
    _GetTaskContextData(ctx, HdxTokens->renderPassState, &renderPassState);
    HdStRenderPassState * const hdStRenderPassState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!hdStRenderPassState) {
        return;
    }

    _DrawBBoxes(colorTexture, depthTexture, *hdStRenderPassState);
}

void
HdxBoundingBoxTask::_DestroyShaderProgram()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        _GetHgi()->DestroyShaderFunction(&fn);
    }
    _GetHgi()->DestroyShaderProgram(&_shaderProgram);
}

void
HdxBoundingBoxTask::_PrintCompileErrors()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        std::cout << fn->GetCompileErrors() << std::endl;
    }
    std::cout << _shaderProgram->GetCompileErrors() << std::endl;
}

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(
    std::ostream& out,
    const HdxBoundingBoxTaskParams& pv)
{
    out << "BoundingBoxTask Params: (...) { ";

    for (size_t i = 0; i < pv.bboxes.size(); ++i) {
        out << "BBox" << i << " " << pv.bboxes[i] << ", ";
    }

    out << pv.color << " "
        << pv.dashSize << " }";
    return out;
}

bool operator==(const HdxBoundingBoxTaskParams& lhs,
                const HdxBoundingBoxTaskParams& rhs)
{
    return lhs.aovName == rhs.aovName &&
           lhs.bboxes == rhs.bboxes &&
           lhs.color == rhs.color &&
           lhs.dashSize == rhs.dashSize;
}

bool operator!=(const HdxBoundingBoxTaskParams& lhs,
                const HdxBoundingBoxTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
