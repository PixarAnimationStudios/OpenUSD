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
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <iostream>

#ifdef PXR_OCIO_PLUGIN_ENABLED
    #include <OpenColorIO/OpenColorIO.h>
    namespace OCIO = OCIO_NAMESPACE;
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((colorCorrectionVertex,    "ColorCorrectionVertex"))
    ((colorCorrectionFragment,  "ColorCorrectionFragment"))
    (colorCorrectionShader)
);

static const int HDX_DEFAULT_LUT3D_SIZE_OCIO = 65;

HdxColorCorrectionTaskParams::HdxColorCorrectionTaskParams()
  : colorCorrectionMode(HdxColorCorrectionTokens->disabled)
  , lut3dSizeOCIO(HDX_DEFAULT_LUT3D_SIZE_OCIO)
{
}

HdxColorCorrectionTask::HdxColorCorrectionTask(
    HdSceneDelegate* delegate,
    SdfPath const& id)
  : HdxTask(id)
  , _indexBuffer()
  , _vertexBuffer()
  , _texture3dLUT()
  , _sampler()
  , _shaderProgram()
  , _resourceBindings()
  , _pipeline()
  , _lut3dSizeOCIO(HDX_DEFAULT_LUT3D_SIZE_OCIO)
  , _screenSize{}
{
}

HdxColorCorrectionTask::~HdxColorCorrectionTask()
{
    if (_texture3dLUT) {
        _GetHgi()->DestroyTexture(&_texture3dLUT);
    }

    if (_sampler) {
        _GetHgi()->DestroySampler(&_sampler);
    }

    if (_vertexBuffer) {
        _GetHgi()->DestroyBuffer(&_vertexBuffer);
    }

    if (_indexBuffer) {
        _GetHgi()->DestroyBuffer(&_indexBuffer);
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
HdxColorCorrectionTask::_GetUseOcio() const
{
    // Client can choose to use Hydra's build-in sRGB color correction or use
    // OpenColorIO for color correction in which case we insert extra OCIO code.
    #ifdef PXR_OCIO_PLUGIN_ENABLED
        // Only use if $OCIO environment variable is set.
        // (Otherwise this option should be disabled.)
        if (TfGetenv("OCIO") == "") {
            return false;
        }

        return _colorCorrectionMode == HdxColorCorrectionTokens->openColorIO;
    #else
        return false;
    #endif
}

std::string
HdxColorCorrectionTask::_CreateOpenColorIOResources()
{
    #ifdef PXR_OCIO_PLUGIN_ENABLED
        // Use client provided OCIO values, or use default fallback values
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

        const char* display = _displayOCIO.empty() ?
                              config->getDefaultDisplay() :
                              _displayOCIO.c_str();

        const char* view = _viewOCIO.empty() ?
                           config->getDefaultView(display) :
                           _viewOCIO.c_str();

        std::string inputColorSpace = _colorspaceOCIO;
        if (inputColorSpace.empty()) {
            OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace("default");
            if (cs) {
                inputColorSpace = cs->getName();
            } else {
                inputColorSpace = OCIO::ROLE_SCENE_LINEAR;
            }
        }

        // Setup the transformation we need to apply
        OCIO::DisplayTransformRcPtr transform =
            OCIO::DisplayTransform::Create();
        transform->setDisplay(display);
        transform->setView(view);
        transform->setInputColorSpaceName(inputColorSpace.c_str());
        if (!_looksOCIO.empty()) {
            transform->setLooksOverride(_looksOCIO.c_str());
            transform->setLooksOverrideEnabled(true);
        } else {
            transform->setLooksOverrideEnabled(false);
        }

        OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);

        // Create a GPU Shader Description
        OCIO::GpuShaderDesc shaderDesc;
        shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
        shaderDesc.setFunctionName("OCIODisplay");
        shaderDesc.setLut3DEdgeLen(_lut3dSizeOCIO);

        // Compute and the 3D LUT
        const int num3Dentries =
            3 * _lut3dSizeOCIO * _lut3dSizeOCIO * _lut3dSizeOCIO;
        std::vector<float> lut3d(num3Dentries);
        processor->getGpuLut3D(lut3d.data(), shaderDesc);

        // Load the data into an OpenGL 3D Texture
        if (_texture3dLUT) {
            _GetHgi()->DestroyTexture(&_texture3dLUT);
        }

        HgiTextureDesc lutDesc;
        lutDesc.debugName = "OCIO 3d LUT";
        lutDesc.type = HgiTextureType3D;
        lutDesc.dimensions = GfVec3i(_lut3dSizeOCIO);
        lutDesc.format = HgiFormatFloat32Vec3;
        lutDesc.initialData = lut3d.data();
        lutDesc.layerCount = 1;
        lutDesc.mipLevels = 1;
        lutDesc.pixelsByteSize = lut3d.size() * sizeof(lut3d[0]);
        lutDesc.sampleCount = HgiSampleCount1;
        lutDesc.usage = HgiTextureUsageBitsShaderRead;
        _texture3dLUT = _GetHgi()->CreateTexture(lutDesc);

        const char* gpuShaderText = processor->getGpuShaderText(shaderDesc);

        return std::string(gpuShaderText);
    #else
        return std::string();
    #endif
}

bool
HdxColorCorrectionTask::_CreateShaderResources()
{
    if (_shaderProgram) {
        return true;
    }

    const bool useOCIO =_GetUseOcio();
    const HioGlslfx glslfx(
            HdxPackageColorCorrectionShader(), HioGlslfxTokens->defVal);

    // Setup the vertex shader
    std::string vsCode;
    HgiShaderFunctionDesc vertDesc;
    vertDesc.debugName = _tokens->colorCorrectionVertex.GetString();
    vertDesc.shaderStage = HgiShaderStageVertex;
    HgiShaderFunctionAddStageInput(
        &vertDesc, "position", "vec4");
    HgiShaderFunctionAddStageInput(
        &vertDesc, "uvIn", "vec2");
    if(_hgi->GetAPIName() == HgiTokens->OpenGL ||
       _hgi->GetAPIName() == HgiTokens->Vulkan) {
        vsCode = "#version 450 \n";
    }
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "gl_Position", "vec4", "position");
    HgiShaderFunctionAddStageOutput(
        &vertDesc, "uvOut", "vec2");
    vsCode += glslfx.GetSource(_tokens->colorCorrectionVertex);
    vertDesc.shaderCode = vsCode.c_str();
    HgiShaderFunctionHandle vertFn = _GetHgi()->CreateShaderFunction(vertDesc);

    // Setup the fragment shader
    std::string fsCode;
    HgiShaderFunctionDesc fragDesc;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn");
    if (useOCIO) {
        HgiShaderFunctionAddTexture(
            &fragDesc, "Lut3DIn", 3);
    }
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "screenSize", "vec2");
    fragDesc.debugName = _tokens->colorCorrectionFragment.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    if(_hgi->GetAPIName() == HgiTokens->OpenGL ||
       _hgi->GetAPIName() == HgiTokens->Vulkan) {
        fsCode = "#version 450 \n";
    }
    if (useOCIO) {
        fsCode += "#define GLSLFX_USE_OCIO\n";
        // Our current version of OCIO outputs 130 glsl and texture3D is
        // removed from glsl in 140.
        fsCode += "#define texture3D texture\n";
    }
    if (useOCIO) {
        std::string ocioGpuShaderText = _CreateOpenColorIOResources();
        fsCode = fsCode + ocioGpuShaderText;
    }
    fsCode += glslfx.GetSource(_tokens->colorCorrectionFragment);

    fragDesc.shaderCode = fsCode.c_str();
    HgiShaderFunctionHandle fragFn = _GetHgi()->CreateShaderFunction(fragDesc);

    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName =_tokens->colorCorrectionShader.GetString();
    programDesc.shaderFunctions.push_back(std::move(vertFn));
    programDesc.shaderFunctions.push_back(std::move(fragFn));
    _shaderProgram = _GetHgi()->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create color correction shader");
        _PrintCompileErrors();
        _DestroyShaderProgram();
        return false;
    }

    return true;
}

bool
HdxColorCorrectionTask::_CreateBufferResources()
{
    if (_vertexBuffer) {
        return true;
    }

    // A larger-than screen triangle made to fit the screen.
    constexpr float vertDataGL[][6] =
            { { -1,  3, 0, 1,     0, 2 },
              { -1, -1, 0, 1,     0, 0 },
              {  3, -1, 0, 1,     2, 0 } };

    constexpr float vertDataOther[][6] =
            { { -1,  3, 0, 1,     0, -1 },
              { -1, -1, 0, 1,     0,  1 },
              {  3, -1, 0, 1,     2,  1 } };

    HgiBufferDesc vboDesc;
    vboDesc.debugName = "HdxColorCorrectionTask VertexBuffer";
    vboDesc.usage = HgiBufferUsageVertex;
    vboDesc.initialData = _hgi->GetAPIName() != HgiTokens->OpenGL
        ? vertDataOther : vertDataGL;
    vboDesc.byteSize = sizeof(vertDataOther);
    vboDesc.vertexStride = sizeof(vertDataOther[0]);
    _vertexBuffer = _GetHgi()->CreateBuffer(vboDesc);

    static const int32_t indices[3] = {0,1,2};

    HgiBufferDesc iboDesc;
    iboDesc.debugName = "HdxColorCorrectionTask IndexBuffer";
    iboDesc.usage = HgiBufferUsageIndex32;
    iboDesc.initialData = indices;
    iboDesc.byteSize = sizeof(indices);
    _indexBuffer = _GetHgi()->CreateBuffer(iboDesc);
    return true;
}

bool
HdxColorCorrectionTask::_CreateResourceBindings(
    HgiTextureHandle const &aovTexture)
{
    // The color aov has the rendered results and we wish to color correct it.
    bool useOCIO =_GetUseOcio();

    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "ColorCorrection";

    HgiTextureBindDesc texBind0;
    texBind0.bindingIndex = 0;
    texBind0.stageUsage = HgiShaderStageFragment;
    texBind0.textures.push_back(aovTexture);
    texBind0.samplers.push_back(_sampler);
    resourceDesc.textures.push_back(std::move(texBind0));

    if (useOCIO && _texture3dLUT) {
        HgiTextureBindDesc texBind1;
        texBind1.bindingIndex = 1;
        texBind1.stageUsage = HgiShaderStageFragment;
        texBind1.textures.push_back(_texture3dLUT);
        texBind1.samplers.push_back(_sampler);
        resourceDesc.textures.push_back(std::move(texBind1));
    }

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
HdxColorCorrectionTask::_CreatePipeline(HgiTextureHandle const& aovTexture)
{
    if (_pipeline) {
        if (_attachment0.format == aovTexture->GetDescriptor().format) {
            return true;
        }

        _GetHgi()->DestroyGraphicsPipeline(&_pipeline);
    }

    HgiGraphicsPipelineDesc desc;
    desc.debugName = "ColorCorrection Pipeline";
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

    // Setup raserization state
    desc.rasterizationState.cullMode = HgiCullModeBack;
    desc.rasterizationState.polygonMode = HgiPolygonModeFill;
    desc.rasterizationState.winding = HgiWindingCounterClockwise;

    // Setup attachment descriptor
    _attachment0.blendEnabled = false;
    _attachment0.loadOp = HgiAttachmentLoadOpDontCare;
    _attachment0.storeOp = HgiAttachmentStoreOpStore;
    _attachment0.format = aovTexture->GetDescriptor().format;
    _attachment0.usage = aovTexture->GetDescriptor().usage;
    desc.colorAttachmentDescs.push_back(_attachment0);

    desc.shaderConstantsDesc.stageUsage = HgiShaderStageFragment;
    desc.shaderConstantsDesc.byteSize = sizeof(_screenSize);

    _pipeline = _GetHgi()->CreateGraphicsPipeline(desc);

    return true;
}

bool
HdxColorCorrectionTask::_CreateSampler()
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

void
HdxColorCorrectionTask::_ApplyColorCorrection(
    HgiTextureHandle const& aovTexture)
{
    GfVec3i const& dimensions = aovTexture->GetDescriptor().dimensions;

    // Prepare graphics cmds.
    HgiGraphicsCmdsDesc gfxDesc;
    gfxDesc.colorAttachmentDescs.push_back(_attachment0);
    gfxDesc.colorTextures.push_back(aovTexture);

    // Begin rendering
    HgiGraphicsCmdsUniquePtr gfxCmds = _GetHgi()->CreateGraphicsCmds(gfxDesc);
    gfxCmds->PushDebugGroup("ColorCorrection");
    gfxCmds->BindResources(_resourceBindings);
    gfxCmds->BindPipeline(_pipeline);
    gfxCmds->BindVertexBuffers(0, {_vertexBuffer}, {0});
    const GfVec4i vp(0, 0, dimensions[0], dimensions[1]);
    _screenSize[0] = static_cast<float>(dimensions[0]);
    _screenSize[1] = static_cast<float>(dimensions[1]);
    gfxCmds->SetConstantValues(
        _pipeline,
        HgiShaderStageFragment,
        0,
        sizeof(_screenSize),
        &_screenSize);
    gfxCmds->SetViewport(vp);
    gfxCmds->DrawIndexed(_indexBuffer, 3, 0, 0, 1);
    gfxCmds->PopDebugGroup();

    // Done recording commands, submit work.
    _GetHgi()->SubmitCmds(gfxCmds.get());
}

void
HdxColorCorrectionTask::_Sync(HdSceneDelegate* delegate,
                              HdTaskContext* ctx,
                              HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxColorCorrectionTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _colorCorrectionMode = params.colorCorrectionMode;
            _displayOCIO = params.displayOCIO;
            _viewOCIO = params.viewOCIO;
            _colorspaceOCIO = params.colorspaceOCIO;
            _looksOCIO = params.looksOCIO;
            _lut3dSizeOCIO = params.lut3dSizeOCIO;
            _aovName = params.aovName;

            if (_lut3dSizeOCIO <= 0) {
                TF_CODING_ERROR("Invalid OCIO LUT size.");
                _lut3dSizeOCIO = 65;
            }

            // Rebuild Hgi objects when ColorCorrection params change
            _DestroyShaderProgram();
            if (_resourceBindings) {
                _GetHgi()->DestroyResourceBindings(&_resourceBindings);
            }
            if (_pipeline) {
                _GetHgi()->DestroyGraphicsPipeline(&_pipeline);
            }
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxColorCorrectionTask::Prepare(HdTaskContext* ctx,
                                HdRenderIndex* renderIndex)
{
}

void
HdxColorCorrectionTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // We currently only color correct the color aov.
    if (_aovName != HdAovTokens->color) {
        return;
    }

    // The color aov has the rendered results and we wish to
    // color correct it into colorIntermediate aov to ensure we do not
    // read from the same color target that we write into.
    if (!_HasTaskContextData(ctx, HdAovTokens->color) ||
        !_HasTaskContextData(ctx, HdxAovTokens->colorIntermediate)) {
        return;
    }

    HgiTextureHandle aovTexture, aovTextureIntermediate;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);
    _GetTaskContextData(
        ctx, HdxAovTokens->colorIntermediate, &aovTextureIntermediate);

    if (!TF_VERIFY(_CreateBufferResources())) {
        return;
    }
    if (!TF_VERIFY(_CreateSampler())) {
        return;
    }
    if (!TF_VERIFY(_CreateShaderResources())) {
        return;
    }
    if (!TF_VERIFY(_CreateResourceBindings(aovTexture))) {
        return;
    }
    if (!TF_VERIFY(_CreatePipeline(aovTextureIntermediate))) {
        return;
    }

    _ApplyColorCorrection(aovTextureIntermediate);

    // Toggle color and colorIntermediate
    _ToggleRenderTarget(ctx);
}

void
HdxColorCorrectionTask::_DestroyShaderProgram()
{
    if (!_shaderProgram) return;

    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        _GetHgi()->DestroyShaderFunction(&fn);
    }
    _GetHgi()->DestroyShaderProgram(&_shaderProgram);
}

void
HdxColorCorrectionTask::_PrintCompileErrors()
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
    const HdxColorCorrectionTaskParams& pv)
{
    out << "ColorCorrectionTask Params: (...) "
        << pv.colorCorrectionMode << " "
        << pv.displayOCIO << " "
        << pv.viewOCIO << " "
        << pv.colorspaceOCIO << " "
        << pv.looksOCIO << " "
        << pv.lut3dSizeOCIO << " "
        << pv.aovName
    ;
    return out;
}

bool operator==(const HdxColorCorrectionTaskParams& lhs,
                const HdxColorCorrectionTaskParams& rhs)
{
    return lhs.colorCorrectionMode == rhs.colorCorrectionMode &&
           lhs.displayOCIO == rhs.displayOCIO &&
           lhs.viewOCIO == rhs.viewOCIO &&
           lhs.colorspaceOCIO == rhs.colorspaceOCIO &&
           lhs.looksOCIO == rhs.looksOCIO &&
           lhs.lut3dSizeOCIO == rhs.lut3dSizeOCIO &&
           lhs.aovName == rhs.aovName;
}

bool operator!=(const HdxColorCorrectionTaskParams& lhs,
                const HdxColorCorrectionTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
