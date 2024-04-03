//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hdx/visualizeAovTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/presentTask.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hdSt/textureUtils.h"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // texture identifiers
    (aovIn)
    (depthIn)
    (idIn)
    (normalIn)

    // shader mixins
    ((visualizeAovVertex,           "VisualizeVertex"))
    ((visualizeAovFragmentDepth,    "VisualizeFragmentDepth"))
    ((visualizeAovFragmentFallback, "VisualizeFragmentFallback"))
    ((visualizeAovFragmentId,       "VisualizeFragmentId"))
    ((visualizeAovFragmentNormal,   "VisualizeFragmentNormal"))

    ((empty, ""))
);

HdxVisualizeAovTaskParams::HdxVisualizeAovTaskParams() = default;

HdxVisualizeAovTask::HdxVisualizeAovTask(
    HdSceneDelegate* delegate,
    SdfPath const& id)
  : HdxTask(id)
  , _outputTextureDimensions(0)
  , _screenSize{}
  , _minMaxDepth{}
  , _vizKernel(VizKernelNone)
{
}

HdxVisualizeAovTask::~HdxVisualizeAovTask()
{
    // Kernel independent resources
    {
        if (_vertexBuffer) {
            _GetHgi()->DestroyBuffer(&_vertexBuffer);
        }
        if (_indexBuffer) {
            _GetHgi()->DestroyBuffer(&_indexBuffer);
        }
        if (_sampler) {
            _GetHgi()->DestroySampler(&_sampler);
        }
    }

    // Kernel dependent resources
    {
        if (_outputTexture) {
            _GetHgi()->DestroyTexture(&_outputTexture);
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
}

static bool
_IsIdAov(TfToken const &aovName)
{
    return aovName == HdAovTokens->primId ||
           aovName == HdAovTokens->instanceId ||
           aovName == HdAovTokens->elementId ||
           aovName == HdAovTokens->edgeId ||
           aovName == HdAovTokens->pointId;
}

bool
HdxVisualizeAovTask::_UpdateVizKernel(TfToken const &aovName)
{
    VizKernel vk = VizKernelFallback;

    if (aovName == HdAovTokens->color) {
        vk = VizKernelNone;
    } else if (HdAovHasDepthSemantic(aovName) ||
               HdAovHasDepthStencilSemantic(aovName)) {
        vk = VizKernelDepth;
    } else if (_IsIdAov(aovName)) {
        vk = VizKernelId;
    } else if (aovName == HdAovTokens->normal) {
        vk = VizKernelNormal;
    }

    if (vk != _vizKernel) {
        _vizKernel = vk;
        return true;
    }
    return false;
}

TfToken const&
HdxVisualizeAovTask::_GetTextureIdentifierForShader() const
{
    switch(_vizKernel) {
        case VizKernelDepth:
            return _tokens->depthIn;
        case VizKernelId:
            return _tokens->idIn;
        case VizKernelNormal:
            return _tokens->normalIn;
        case VizKernelFallback:
            return _tokens->aovIn;
        default:
            TF_CODING_ERROR("Unhandled kernel viz enumeration");
            return _tokens->empty;
    }
}

TfToken const&
HdxVisualizeAovTask::_GetFragmentMixin() const
{
    switch(_vizKernel) {
        case VizKernelDepth:
            return _tokens->visualizeAovFragmentDepth;
        case VizKernelId:
            return _tokens->visualizeAovFragmentId;
        case VizKernelNormal:
            return _tokens->visualizeAovFragmentNormal;
        case VizKernelFallback:
            return _tokens->visualizeAovFragmentFallback;
        default:
            TF_CODING_ERROR("Unhandled kernel viz enumeration");
            return _tokens->empty;
    }
}

bool
HdxVisualizeAovTask::_CreateShaderResources(
    HgiTextureDesc const& inputAovTextureDesc)   
{
    if (_shaderProgram) {
        return true;
    }

    const HioGlslfx glslfx(
            HdxPackageVisualizeAovShader(), HioGlslfxTokens->defVal);

    // Setup the vertex shader (same for all kernels)
    HgiShaderFunctionHandle vertFn;
    {
        std::string vsCode;
        HgiShaderFunctionDesc vertDesc;
        vertDesc.debugName = _tokens->visualizeAovVertex.GetString();
        vertDesc.shaderStage = HgiShaderStageVertex;
        HgiShaderFunctionAddStageInput(
            &vertDesc, "position", "vec4");
        HgiShaderFunctionAddStageInput(
            &vertDesc, "uvIn", "vec2");
        HgiShaderFunctionAddStageOutput(
            &vertDesc, "gl_Position", "vec4", "position");
        HgiShaderFunctionAddStageOutput(
            &vertDesc, "uvOut", "vec2");
        vsCode += glslfx.GetSource(_tokens->visualizeAovVertex);
        vertDesc.shaderCode = vsCode.c_str();

        vertFn = _GetHgi()->CreateShaderFunction(vertDesc);
    }

    // Setup the fragment shader based on the kernel used.
    HgiShaderFunctionHandle fragFn;
    TfToken const &mixin = _GetFragmentMixin();
    {
        std::string fsCode;
        HgiShaderFunctionDesc fragDesc;
        HgiShaderFunctionAddStageInput(
            &fragDesc, "uvOut", "vec2");

        HgiShaderFunctionAddTexture(
            &fragDesc, _GetTextureIdentifierForShader().GetString(),
            /*bindIndex = */0, /*dimensions = */2, inputAovTextureDesc.format);

        HgiShaderFunctionAddStageOutput(
            &fragDesc, "hd_FragColor", "vec4", "color");
        HgiShaderFunctionAddConstantParam(
            &fragDesc, "screenSize", "vec2");
        
        if (_vizKernel == VizKernelDepth) {
            HgiShaderFunctionAddConstantParam(
                &fragDesc, "minMaxDepth", "vec2");
        }
        TfToken const &mixin = _GetFragmentMixin();
        fragDesc.debugName = mixin.GetString();
        fragDesc.shaderStage = HgiShaderStageFragment;
        fsCode += glslfx.GetSource(mixin);
        fragDesc.shaderCode = fsCode.c_str();

        fragFn = _GetHgi()->CreateShaderFunction(fragDesc);
    }


    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName = mixin.GetString();
    programDesc.shaderFunctions.push_back(std::move(vertFn));
    programDesc.shaderFunctions.push_back(std::move(fragFn));
    _shaderProgram = _GetHgi()->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create AOV visualization shader %s",
                         mixin.GetText());
        _PrintCompileErrors();
        _DestroyShaderProgram();
        return false;
    }

    return true;
}

bool
HdxVisualizeAovTask::_CreateBufferResources()
{
    if (_vertexBuffer && _indexBuffer) {
        return true;
    }

    // A larger-than screen triangle made to fit the screen.
    constexpr float vertData[][6] =
            { { -1,  3, 0, 1,     0, 2 },
              { -1, -1, 0, 1,     0, 0 },
              {  3, -1, 0, 1,     2, 0 } };

    HgiBufferDesc vboDesc;
    vboDesc.debugName = "HdxVisualizeAovTask VertexBuffer";
    vboDesc.usage = HgiBufferUsageVertex;
    vboDesc.initialData = vertData;
    vboDesc.byteSize = sizeof(vertData);
    vboDesc.vertexStride = sizeof(vertData[0]);
    _vertexBuffer = _GetHgi()->CreateBuffer(vboDesc);

    static const int32_t indices[3] = {0,1,2};

    HgiBufferDesc iboDesc;
    iboDesc.debugName = "HdxVisualizeAovTask IndexBuffer";
    iboDesc.usage = HgiBufferUsageIndex32;
    iboDesc.initialData = indices;
    iboDesc.byteSize = sizeof(indices);
    _indexBuffer = _GetHgi()->CreateBuffer(iboDesc);
    return true;
}

bool
HdxVisualizeAovTask::_CreateResourceBindings(
    HgiTextureHandle const &inputAovTexture)
{
    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "HdxVisualizeAovTask resourceDesc";

    HgiTextureBindDesc texBind0;
    texBind0.bindingIndex = 0;
    texBind0.stageUsage = HgiShaderStageFragment;
    texBind0.writable = false;
    texBind0.textures.push_back(inputAovTexture);
    texBind0.samplers.push_back(_sampler);
    resourceDesc.textures.push_back(std::move(texBind0));

    // If nothing has changed in the descriptor we avoid re-creating the
    // resource bindings object.
    if (_resourceBindings) {
        HgiResourceBindingsDesc const& desc= _resourceBindings->GetDescriptor();
        if (desc == resourceDesc) {
            return true;
        } else {
            _GetHgi()->DestroyResourceBindings(&_resourceBindings);
        }
    }

    _resourceBindings = _GetHgi()->CreateResourceBindings(resourceDesc);

    return true;
}

bool
HdxVisualizeAovTask::_CreatePipeline(HgiTextureDesc const& outputTextureDesc)
{
    if (_pipeline) {
        return true;
    }

    HgiGraphicsPipelineDesc desc;
    desc.debugName = "AOV Visualiztion Pipeline";
    desc.shaderProgram = _shaderProgram;

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

    HgiVertexBufferDesc vboDesc;

    vboDesc.bindingIndex = bindSlots++;
    vboDesc.vertexStride = sizeof(float) * 6; // pos, uv
    vboDesc.vertexAttributes.clear();
    vboDesc.vertexAttributes.push_back(posAttr);
    vboDesc.vertexAttributes.push_back(uvAttr);

    desc.vertexBuffers.push_back(std::move(vboDesc));

    // Depth test and write can be off since we only colorcorrect the color aov.
    desc.depthState.depthTestEnabled = false;
    desc.depthState.depthWriteEnabled = false;

    // We don't use the stencil mask in this task.
    desc.depthState.stencilTestEnabled = false;

    // Alpha to coverage would prevent any pixels that have an alpha of 0.0 from
    // being written. We want to color correct all pixels. Even background
    // pixels that were set with a clearColor alpha of 0.0.
    desc.multiSampleState.alphaToCoverageEnable = false;

    // Setup rasterization state
    desc.rasterizationState.cullMode = HgiCullModeBack;
    desc.rasterizationState.polygonMode = HgiPolygonModeFill;
    desc.rasterizationState.winding = HgiWindingCounterClockwise;

    // Setup attachment descriptor
    _outputAttachmentDesc.blendEnabled = false;
    _outputAttachmentDesc.loadOp = HgiAttachmentLoadOpDontCare;
    _outputAttachmentDesc.storeOp = HgiAttachmentStoreOpStore;
    _outputAttachmentDesc.format = outputTextureDesc.format;
    _outputAttachmentDesc.usage = outputTextureDesc.usage;
    desc.colorAttachmentDescs.push_back(_outputAttachmentDesc);

    desc.shaderConstantsDesc.stageUsage = HgiShaderStageFragment;
    desc.shaderConstantsDesc.byteSize = sizeof(_screenSize);
    if (_vizKernel == VizKernelDepth) {
        desc.shaderConstantsDesc.byteSize += sizeof(_minMaxDepth);
    }

    _pipeline = _GetHgi()->CreateGraphicsPipeline(desc);

    return true;
}

bool
HdxVisualizeAovTask::_CreateSampler()
{
    if (_sampler) {
        return true;
    }

    HgiSamplerDesc sampDesc;

    sampDesc.magFilter = HgiSamplerFilterLinear;
    sampDesc.minFilter = HgiSamplerFilterLinear;

    sampDesc.addressModeU = HgiSamplerAddressModeClampToEdge;
    sampDesc.addressModeV = HgiSamplerAddressModeClampToEdge;

    _sampler = _GetHgi()->CreateSampler(sampDesc);

    return true;
}

bool
HdxVisualizeAovTask::_CreateOutputTexture(GfVec3i const &dimensions)
{
    if (_outputTexture) {
        if (_outputTextureDimensions == dimensions) {
            return true;
        }
        _GetHgi()->DestroyTexture(&_outputTexture);
    }

    _outputTextureDimensions = dimensions;

    HgiTextureDesc texDesc;
    texDesc.debugName = "Visualize Aov Output Texture";
    texDesc.dimensions = dimensions;
    texDesc.format = HgiFormatFloat32Vec4;
    texDesc.layerCount = 1;
    texDesc.mipLevels = 1;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsColorTarget |
                    HgiTextureUsageBitsShaderRead;
    _outputTexture = _GetHgi()->CreateTexture(texDesc);

    return bool(_outputTexture);
}

void
HdxVisualizeAovTask::_DestroyShaderProgram()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        _GetHgi()->DestroyShaderFunction(&fn);
    }
    _GetHgi()->DestroyShaderProgram(&_shaderProgram);
}

void
HdxVisualizeAovTask::_PrintCompileErrors()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        std::cout << fn->GetCompileErrors() << std::endl;
    }
    std::cout << _shaderProgram->GetCompileErrors() << std::endl;
}

void
HdxVisualizeAovTask::_UpdateMinMaxDepth(HgiTextureHandle const &inputAovTexture)
{
    // XXX CPU readback to determine min, max depth
    // This should be rewritten to use a compute shader.
    const HgiTextureDesc& textureDesc = inputAovTexture.Get()->GetDescriptor();
    if (textureDesc.format != HgiFormatFloat32) {
         TF_WARN("Non-floating point depth AOVs aren't supported yet.");
         return;
    }

    size_t size = 0;
    HdStTextureUtils::AlignedBuffer<uint8_t> buffer =
        HdStTextureUtils::HgiTextureReadback(_GetHgi(), inputAovTexture, &size);

    {
        const HgiTextureDesc& textureDesc = inputAovTexture.Get()->GetDescriptor();
        const size_t width = textureDesc.dimensions[0];
        const size_t height = textureDesc.dimensions[1];
        float const *ptr = reinterpret_cast<float const *>(buffer.get());
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();
        for (size_t ii = 0; ii < width * height; ii++) {
            float const &val = ptr[ii];
            if (val < min) {
                min = val;
            }
            if (val > max) {
                max = val;
            }
        }

        _minMaxDepth[0] = min;
        _minMaxDepth[1] = max;
    }
}

void
HdxVisualizeAovTask::_ApplyVisualizationKernel(
    HgiTextureHandle const& outputTexture)
{
    GfVec3i const& dimensions = outputTexture->GetDescriptor().dimensions;

    // Prepare graphics cmds.
    HgiGraphicsCmdsDesc gfxDesc;
    gfxDesc.colorAttachmentDescs.push_back(_outputAttachmentDesc);
    gfxDesc.colorTextures.push_back(outputTexture);

    // Begin rendering
    HgiGraphicsCmdsUniquePtr gfxCmds = _GetHgi()->CreateGraphicsCmds(gfxDesc);
    gfxCmds->PushDebugGroup("Visualize AOV");
    gfxCmds->BindResources(_resourceBindings);
    gfxCmds->BindPipeline(_pipeline);
    gfxCmds->BindVertexBuffers({{_vertexBuffer, 0, 0}});
    const GfVec4i vp(0, 0, dimensions[0], dimensions[1]);
    _screenSize[0] = static_cast<float>(dimensions[0]);
    _screenSize[1] = static_cast<float>(dimensions[1]);
    
    if (_vizKernel == VizKernelDepth) {
        struct Uniform {
            float screenSize[2];
            float minMaxDepth[2];
        };
        Uniform data;
        data.screenSize[0] = _screenSize[0];
        data.screenSize[1] = _screenSize[1];
        data.minMaxDepth[0] = _minMaxDepth[0];
        data.minMaxDepth[1] = _minMaxDepth[1];

        gfxCmds->SetConstantValues(
            _pipeline,
            HgiShaderStageFragment,
            0,
            sizeof(data),
            &data);
    } else {
        gfxCmds->SetConstantValues(
            _pipeline,
            HgiShaderStageFragment,
            0,
            sizeof(_screenSize),
            &_screenSize);
    }

    gfxCmds->SetViewport(vp);
    gfxCmds->DrawIndexed(_indexBuffer, 3, 0, 0, 1, 0);
    gfxCmds->PopDebugGroup();

    // Done recording commands, submit work.
    _GetHgi()->SubmitCmds(gfxCmds.get());
}

void
HdxVisualizeAovTask::_Sync(HdSceneDelegate* delegate,
                              HdTaskContext* ctx,
                              HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxVisualizeAovTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            // Rebuild necessary Hgi objects when aov to be visualized changes.
            if (_UpdateVizKernel(params.aovName)) {
                _DestroyShaderProgram();
                if (_resourceBindings) {
                    _GetHgi()->DestroyResourceBindings(&_resourceBindings);
                }
                if (_pipeline) {
                    _GetHgi()->DestroyGraphicsPipeline(&_pipeline);
                }
            }
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxVisualizeAovTask::Prepare(HdTaskContext* ctx,
                                HdRenderIndex* renderIndex)
{
}

void
HdxVisualizeAovTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_vizKernel == VizKernelNone) {
        return;
    }

    // XXX: HdxAovInputTask sets the 'color' and 'colorIntermediate' texture
    // handles for the "active" AOV on the task context.
    // The naming is misleading and may be improved to
    // 'aovTexture' and 'aovTextureIntermediate' instead.
    if (!_HasTaskContextData(ctx, HdAovTokens->color) ||
        !_HasTaskContextData(ctx, HdxAovTokens->colorIntermediate)) {
        return;
    }

    HgiTextureHandle aovTexture, aovTextureIntermediate;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);
    _GetTaskContextData(
        ctx, HdxAovTokens->colorIntermediate, &aovTextureIntermediate);
    HgiTextureDesc const& aovTexDesc = aovTexture->GetDescriptor();

    if (!TF_VERIFY(_CreateBufferResources())) {
        return;
    }
    if (!TF_VERIFY(_CreateSampler())) {
        return;
    }
    if (!TF_VERIFY(_CreateShaderResources(/*inputTextureDesc*/aovTexDesc))) {
        return;
    }
    if (!TF_VERIFY(_CreateResourceBindings(/*inputTexture*/aovTexture))) {
        return;
    }

    bool canUseIntermediateAovTexture = false;
    // Normal AOV typically uses a 3 channel float format in which case we can
    // reuse the intermediate AOV to write the colorized results into.
    // For single channel AOVs like id or depth, colorize such that all color
    // components (R,G,B) are used.
    canUseIntermediateAovTexture =
        HdxPresentTask::IsFormatSupported(aovTexDesc.format) &&
        HgiGetComponentCount(aovTexDesc.format) >= 3;

    if (!canUseIntermediateAovTexture &&
        !TF_VERIFY(_CreateOutputTexture(aovTexDesc.dimensions))) {
        return;
    }

    HgiTextureHandle const &outputTexture = canUseIntermediateAovTexture?
        aovTextureIntermediate : _outputTexture;
    if (!TF_VERIFY(_CreatePipeline(outputTexture->GetDescriptor()))) {
        return;
    }

    if (_vizKernel == VizKernelDepth) {
        _UpdateMinMaxDepth(/*inputTexture*/aovTexture);
    }

    _ApplyVisualizationKernel(outputTexture);

    if (canUseIntermediateAovTexture) {
        // Swap the handles on the task context so that future downstream tasks
        // can use HdxAovTokens->color to get the output of this task.
        _ToggleRenderTarget(ctx); 
    } else {
        (*ctx)[HdAovTokens->color] = VtValue(_outputTexture);
    }
}

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(
    std::ostream& out,
    const HdxVisualizeAovTaskParams& pv)
{
    out << "HdxVisualizeAovTaskParams Params: "
        << pv.aovName
    ;
    return out;
}

bool operator==(const HdxVisualizeAovTaskParams& lhs,
                const HdxVisualizeAovTaskParams& rhs)
{
    return lhs.aovName == rhs.aovName;
}

bool operator!=(const HdxVisualizeAovTaskParams& lhs,
                const HdxVisualizeAovTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
