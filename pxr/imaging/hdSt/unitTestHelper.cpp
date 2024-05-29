//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/tf/staticTokens.h"

#include <iostream>
#include <string>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (lighting)
    (l0dir)
    (l0color)
    (l1dir)
    (l1color)
    (sceneAmbient)
    (vec3)

    // Collection names
    (testCollection)
);

HdSt_DrawTask::HdSt_DrawTask(
    HdRenderPassSharedPtr const &renderPass,
    HdStRenderPassStateSharedPtr const &renderPassState,
    const TfTokenVector &renderTags)
  : HdTask(SdfPath::EmptyPath())
  , _renderPass(renderPass)
  , _renderPassState(renderPassState)
  , _renderTags(renderTags)
{
}

void
HdSt_DrawTask::Sync(
    HdSceneDelegate*,
    HdTaskContext*,
    HdDirtyBits*)
{
    _renderPass->Sync();
}

void
HdSt_DrawTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    _renderPassState->Prepare(
        renderIndex->GetResourceRegistry());
}

void
HdSt_DrawTask::Execute(HdTaskContext* ctx)
{
    _renderPass->Execute(_renderPassState, GetRenderTags());
}

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

// --------------------------------------------------------------------------

HdSt_TestDriver::HdSt_TestDriver()
{
    _CreateRenderPassState();

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init();
}

HdSt_TestDriver::HdSt_TestDriver(TfToken const &reprName)
{
    _CreateRenderPassState();

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init(HdReprSelector(reprName));
}

HdSt_TestDriver::HdSt_TestDriver(HdReprSelector const &reprSelector)
{
    _CreateRenderPassState();

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init(reprSelector);
}

void
HdSt_TestDriver::_CreateRenderPassState()
{
    _renderPassStates = {
        std::dynamic_pointer_cast<HdStRenderPassState>(
            _GetRenderDelegate()->CreateRenderPassState()) };
    // set depthfunc to GL default
    _renderPassStates[0]->SetDepthFunc(HdCmpFuncLess);
}

HdRenderPassSharedPtr const &
HdSt_TestDriver::GetRenderPass()
{
    if (_renderPasses.empty()) {
        std::shared_ptr<HdSt_RenderPass> renderPass =
            std::make_shared<HdSt_RenderPass>(
                &GetDelegate().GetRenderIndex(),
                _GetCollection());

        _renderPasses.push_back(std::move(renderPass));
    }
    return _renderPasses[0];
}

void
HdSt_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(), withGuides);
}

void
HdSt_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides)
{
    static const TfTokenVector geometryTags {
        HdRenderTagTokens->geometry };
    static const TfTokenVector geometryAndGuideTags {
        HdRenderTagTokens->geometry,
        HdRenderTagTokens->guide };

    HdTaskSharedPtrVector tasks = {
        std::make_shared<HdSt_DrawTask>(
            renderPass,
            _renderPassStates[0],
            withGuides ? geometryAndGuideTags : geometryTags) };
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
}

// --------------------------------------------------------------------------

HdSt_TestLightingShader::HdSt_TestLightingShader(HdRenderIndex * renderIndex)
    : _renderIndex(renderIndex)
{
    const char *lightingShader =
        "-- glslfx version 0.1                                              \n"
        "-- configuration                                                   \n"
        "{\"techniques\": {\"default\": {\"fragmentShader\" : {             \n"
        " \"source\": [\"TestLighting.Lighting\"]                           \n"
        "}}}}                                                               \n"
        "-- glsl TestLighting.Lighting                                      \n"
        "vec3 FallbackLighting(vec3 Peye, vec3 Neye, vec3 color) {          \n"
        "    vec3 n = normalize(Neye);                                      \n"
        "    return HdGet_lighting_sceneAmbient()                           \n"
        "      + color * HdGet_lighting_l0color()                           \n"
        "              * max(0.0, dot(n, HdGet_lighting_l0dir()))           \n"
        "      + color * HdGet_lighting_l1color()                           \n"
        "              * max(0.0, dot(n, HdGet_lighting_l1dir()));          \n"
        "}                                                                  \n";

    _lights[0].dir   = GfVec3f(0, 0, 1);
    _lights[0].color = GfVec3f(1, 1, 1);
    _lights[1].dir   = GfVec3f(0, 0, 1);
    _lights[1].color = GfVec3f(0, 0, 0);
    _sceneAmbient    = GfVec3f(0.04, 0.04, 0.04);

    std::stringstream ss(lightingShader);
    _glslfx.reset(new HioGlslfx(ss));
}

HdSt_TestLightingShader::~HdSt_TestLightingShader()
{
}

/* virtual */
HdSt_TestLightingShader::ID
HdSt_TestLightingShader::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    size_t hash = _glslfx->GetHash();
    return (ID)hash;
}

/* virtual */
std::string
HdSt_TestLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    std::string source = _glslfx->GetSource(shaderStageKey);
    return source;
}

/* virtual */
void
HdSt_TestLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                 GfMatrix4d const &projectionMatrix)
{
    // Update the lighting resource buffers only when necessary
    bool lightsChanged = false;

    for (int i = 0; i < 2; ++i) {
        GfVec3f eyeDir =
            worldToViewMatrix.TransformDir(_lights[i].dir).GetNormalized();

        if (_lights[i].eyeDir != eyeDir) {
            lightsChanged = true;
            _lights[i].eyeDir = eyeDir;
        }
    }

    if (lightsChanged) {
        Prepare();
    }
}

void
HdSt_TestLightingShader::Prepare()
{
    HdStResourceRegistrySharedPtr const & hdStResourceRegistry =
        std::dynamic_pointer_cast<HdStResourceRegistry>(
            _renderIndex->GetResourceRegistry());

    HdBufferSpecVector const bufferSpecs = {
        HdBufferSpec(_tokens->l0dir, HdTupleType{HdTypeFloatVec3, 1}),
        HdBufferSpec(_tokens->l0color, HdTupleType{HdTypeFloatVec3, 1}),
        HdBufferSpec(_tokens->l1dir, HdTupleType{HdTypeFloatVec3, 1}),
        HdBufferSpec(_tokens->l1color, HdTupleType{HdTypeFloatVec3, 1}),
        HdBufferSpec(_tokens->sceneAmbient, HdTupleType{HdTypeFloatVec3, 1}),
    };

    _lightingBar =
        hdStResourceRegistry->AllocateUniformBufferArrayRange(
            _tokens->lighting,
            bufferSpecs,
            HdBufferArrayUsageHintBitsUniform);

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            _tokens->l0dir, VtValue(VtVec3fArray(1, _lights[0].eyeDir))),
        std::make_shared<HdVtBufferSource>(
            _tokens->l0color, VtValue(VtVec3fArray(1, _lights[0].color))),
        std::make_shared<HdVtBufferSource>(
            _tokens->l1dir, VtValue(VtVec3fArray(1, _lights[1].eyeDir))),
        std::make_shared<HdVtBufferSource>(
            _tokens->l1color, VtValue(VtVec3fArray(1, _lights[1].color))),
        std::make_shared<HdVtBufferSource>(
            _tokens->sceneAmbient, VtValue(VtVec3fArray(1, _sceneAmbient))),
    };

    hdStResourceRegistry->AddSources(_lightingBar, std::move(sources));
}

static
HdStBindingRequest
_GetBindingRequest(HdBufferArrayRangeSharedPtr lightingBar)
{
    return HdStBindingRequest(HdStBinding::UBO,
                              _tokens->lighting,
                              lightingBar,
                              /*interleaved=*/true,
                              /*writable=*/false,
                              /*arraySize=*/1,
                              /*concatenateNames=*/true);
}

/* virtual */
void
HdSt_TestLightingShader::BindResources(const int program,
                                       HdSt_ResourceBinder const &binder)
{
    binder.Bind(_GetBindingRequest(_lightingBar));
}

/* virtual */
void
HdSt_TestLightingShader::UnbindResources(const int program,
                                         HdSt_ResourceBinder const &binder)
{
    binder.Unbind(_GetBindingRequest(_lightingBar));
}

/*virtual*/
void
HdSt_TestLightingShader::AddBindings(HdStBindingRequestVector *customBindings)
{
    customBindings->push_back(_GetBindingRequest(_lightingBar));
}

void
HdSt_TestLightingShader::SetSceneAmbient(GfVec3f const &color)
{
    _sceneAmbient = color;
}

void
HdSt_TestLightingShader::SetLight(int light,
                                GfVec3f const &dir, GfVec3f const &color)
{
    if (light < 2) {
        _lights[light].dir = dir;
        _lights[light].eyeDir = dir;
        _lights[light].color = color;
    }
}

// --------------------------------------------------------------------------

HdSt_TextureTestDriver::HdSt_TextureTestDriver() :
    _hgi(Hgi::CreatePlatformDefaultHgi())
  , _indexBuffer()
  , _vertexBuffer()
  , _shaderProgram()
  , _resourceBindings()
  , _pipeline()
{
    _CreateVertexBufferDescriptor();
}

HdSt_TextureTestDriver::~HdSt_TextureTestDriver()
{
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
}

void
HdSt_TextureTestDriver::Draw(HgiTextureHandle const &colorDst, 
                             HgiTextureHandle const &inputTexture,
                             HgiSamplerHandle const &inputSampler)
{
    const HgiTextureDesc &textureDesc = colorDst->GetDescriptor();

    const GfVec4i viewport(0, 0, textureDesc.dimensions[0], 
        textureDesc.dimensions[1]);
    float screenSize[2] = { static_cast<float>(viewport[2]), 
                            static_cast<float>(viewport[3]) };
    _constantsData.resize(sizeof(screenSize));
    memcpy(&_constantsData[0], &screenSize, sizeof(screenSize));

    _CreateShaderProgram();
    _CreateBufferResources();
    _CreateTextureBindings(inputTexture, inputSampler);
    _CreatePipeline(colorDst);

    // Create graphics commands
    HgiGraphicsCmdsDesc gfxDesc;
    if (colorDst) {
        gfxDesc.colorAttachmentDescs.push_back(_attachment0);
        gfxDesc.colorTextures.push_back(colorDst);
    }

    HgiGraphicsCmdsUniquePtr gfxCmds = _hgi->CreateGraphicsCmds(gfxDesc);
    gfxCmds->PushDebugGroup("Debug HdSt_TextureTestDriver");
    gfxCmds->BindResources(_resourceBindings);
    gfxCmds->BindPipeline(_pipeline);
    gfxCmds->BindVertexBuffers({{_vertexBuffer, 0, 0}});
    gfxCmds->SetViewport(viewport);
    gfxCmds->SetConstantValues(_pipeline, HgiShaderStageFragment, 0, 
        _constantsData.size(), _constantsData.data());
    gfxCmds->DrawIndexed(_indexBuffer, 3, 0, 0, 1, 0);
    gfxCmds->PopDebugGroup();

    _hgi->SubmitCmds(gfxCmds.get());
}

bool
HdSt_TextureTestDriver::WriteToFile(HgiTextureHandle const &texture, 
                                    std::string filename) const
{
    const HgiTextureDesc &textureDesc = texture->GetDescriptor();

    HioImage::StorageSpec storage;
    storage.width = textureDesc.dimensions[0];
    storage.height = textureDesc.dimensions[1];
    storage.format = HioFormatFloat32Vec4;
    storage.flipped = true;

    size_t size = 0;
    HdStTextureUtils::AlignedBuffer<uint8_t> buffer =
        HdStTextureUtils::HgiTextureReadback(_hgi.get(), texture, &size);
    storage.data = buffer.get();

    if (storage.format == HioFormatInvalid) {
        TF_CODING_ERROR("Hgi texture has format not corresponding to a"
                        "HioFormat");
        return false;
    }

    if (!storage.data) {
        TF_CODING_ERROR("No data for texture");
        return false;
    }
        
    HioImageSharedPtr const image = HioImage::OpenForWriting(filename);
    if (!image) {
        TF_RUNTIME_ERROR("Failed to open image for writing %s",
            filename.c_str());
        return false;
    }

    if (!image->Write(storage)) {
        TF_RUNTIME_ERROR("Failed to write image to %s", filename.c_str());
        return false;
    }

    return true;
}

static const std::string vertShaderStr =
"-- glslfx version 0.1\n"
"-- configuration\n"
"{\n"
"    \"techniques\": {\n"
"        \"default\": {\n"
"            \"VertexPassthrough\": {\n"
"                \"source\": [ \"Vertex.Main\" ]\n"
"            }\n"
"        }\n"
"    }\n"
"}\n"
"-- glsl Vertex.Main\n"
"void main(void)\n"
"{\n"
"    gl_Position = position;\n"
"    uvOut = uvIn;\n"
"}\n";

static const std::string fragShaderStr = 
"-- glslfx version 0.1\n"
"-- configuration\n"
"{\n"
"    \"techniques\": {\n"
"        \"default\": {\n"
"            \"FullscreenTexture\": {\n"
"                \"source\": [ \"Fragment.Main\" ]\n"
"            }\n"
"        }\n"
"    }\n"
"}\n"
"-- glsl Fragment.Main\n"
"void main(void)\n"
"{\n"
"    vec2 coord = (uvOut * screenSize) / 100.f;\n"
"    vec4 color = vec4(HgiGet_colorIn(coord).xyz, 1.0);\n"
"    hd_FragColor = color;\n"
"}\n";

void
HdSt_TextureTestDriver::_CreateShaderProgram()
{
    if (_pipeline) {
        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }
    if (_shaderProgram) {
        _DestroyShaderProgram();
    }

    HgiShaderFunctionDesc vertDesc;
    vertDesc.debugName = TfToken("Vertex");
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
    fragDesc.debugName = TfToken("Fragment");
    fragDesc.shaderStage = HgiShaderStageFragment;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn");
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "screenSize", "vec2");

    std::istringstream vertShaderStream(vertShaderStr);
    std::istringstream fragShaderStream(fragShaderStr);
    const HioGlslfx vsGlslfx = HioGlslfx(vertShaderStream);
    const HioGlslfx fsGlslfx = HioGlslfx(fragShaderStream);

    // Setup the vertex shader
    std::string vsCode;
    vsCode += vsGlslfx.GetSource(TfToken("VertexPassthrough"));
    TF_VERIFY(!vsCode.empty());
    vertDesc.shaderCode = vsCode.c_str();
    HgiShaderFunctionHandle vertFn = _hgi->CreateShaderFunction(vertDesc);

    // Setup the fragment shader
    std::string fsCode;
    fsCode += fsGlslfx.GetSource(TfToken("FullscreenTexture"));
    TF_VERIFY(!fsCode.empty());
    fragDesc.shaderCode = fsCode.c_str();
    HgiShaderFunctionHandle fragFn = _hgi->CreateShaderFunction(fragDesc);

    // Setup the shader program
    HgiShaderProgramDesc programDesc;
    programDesc.debugName = TfToken("FullscreenTriangle").GetString();
    programDesc.shaderFunctions.push_back(std::move(vertFn));
    programDesc.shaderFunctions.push_back(std::move(fragFn));
    _shaderProgram = _hgi->CreateShaderProgram(programDesc);

    if (!_shaderProgram->IsValid() || !vertFn->IsValid() || !fragFn->IsValid()){
        TF_CODING_ERROR("Failed to create shader program");
        _PrintCompileErrors();
        _DestroyShaderProgram();
        return;
    }
}

void
HdSt_TextureTestDriver::_CreateBufferResources()
{
    if (_vertexBuffer) {
        return;
    }

    constexpr size_t elementsPerVertex = 6;
    constexpr size_t vertDataCount = elementsPerVertex * 3;
    constexpr float vertData[vertDataCount] =
            { -1,  1, 0, 1,     0, 1,
              -1, -1, 0, 1,     0, 0,
               1, -1, 0, 1,     1, 0};

    HgiBufferDesc vboDesc;
    vboDesc.debugName = "HdSt_TextureTestDriver VertexBuffer";
    vboDesc.usage = HgiBufferUsageVertex;
    vboDesc.initialData = vertData;
    vboDesc.byteSize = sizeof(vertData);
    vboDesc.vertexStride = elementsPerVertex * sizeof(vertData[0]);
    _vertexBuffer = _hgi->CreateBuffer(vboDesc);

    static const int32_t indices[3] = { 0, 1, 2 };

    HgiBufferDesc iboDesc;
    iboDesc.debugName = "HdSt_TextureTestDriver IndexBuffer";
    iboDesc.usage = HgiBufferUsageIndex32;
    iboDesc.initialData = indices;
    iboDesc.byteSize = sizeof(indices) * sizeof(indices[0]);
    _indexBuffer = _hgi->CreateBuffer(iboDesc);
}

bool
HdSt_TextureTestDriver::_CreateTextureBindings(
    HgiTextureHandle const &textureHandle, 
    HgiSamplerHandle const &samplerHandle)
{
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "HdSt_TextureTestDriver";

    if (textureHandle) {
        HgiTextureBindDesc texBindDesc;
        texBindDesc.bindingIndex = 0;
        texBindDesc.stageUsage = HgiShaderStageFragment;
        texBindDesc.writable = false;
        texBindDesc.textures.push_back(textureHandle);
        if (samplerHandle) {
            texBindDesc.samplers.push_back(samplerHandle);
        }
        resourceDesc.textures.push_back(std::move(texBindDesc));
    }

    // If nothing has changed in the descriptor we avoid re-creating the
    // resource bindings object.
    if (_resourceBindings) {
        HgiResourceBindingsDesc const& desc= _resourceBindings->GetDescriptor();
        if (desc == resourceDesc) {
            return true;
        }
        _hgi->DestroyResourceBindings(&_resourceBindings);
    }

    _resourceBindings = _hgi->CreateResourceBindings(resourceDesc);

    return true;
}

void
HdSt_TextureTestDriver::_CreateVertexBufferDescriptor()
{
    HgiVertexAttributeDesc posAttr;
    posAttr.format = HgiFormatFloat32Vec3;
    posAttr.offset = 0;
    posAttr.shaderBindLocation = 0;

    HgiVertexAttributeDesc uvAttr;
    uvAttr.format = HgiFormatFloat32Vec2;
    uvAttr.offset = sizeof(float) * 4; // after posAttr
    uvAttr.shaderBindLocation = 1;

    _vboDesc.bindingIndex = 0;
    _vboDesc.vertexStride = sizeof(float) * 6; // pos, uv
    _vboDesc.vertexAttributes = { posAttr, uvAttr };
}

bool
HdSt_TextureTestDriver::_CreatePipeline(HgiTextureHandle const& colorDst)
{   
    if (_pipeline) {
        _hgi->DestroyGraphicsPipeline(&_pipeline);
    }

    // Setup attachments
    _attachment0.blendEnabled = false;
    _attachment0.loadOp = HgiAttachmentLoadOpDontCare;
    _attachment0.storeOp = HgiAttachmentStoreOpStore;
    _attachment0.srcColorBlendFactor = HgiBlendFactorZero;
    _attachment0.dstColorBlendFactor = HgiBlendFactorZero;
    _attachment0.colorBlendOp = HgiBlendOpAdd;
    _attachment0.srcAlphaBlendFactor = HgiBlendFactorZero;
    _attachment0.dstAlphaBlendFactor = HgiBlendFactorZero;
    _attachment0.alphaBlendOp = HgiBlendOpAdd;

    if (colorDst) {
        _attachment0.format = colorDst->GetDescriptor().format;
        _attachment0.usage = colorDst->GetDescriptor().usage;
    } else {
        _attachment0.format = HgiFormatInvalid;
    }

    HgiGraphicsPipelineDesc desc;
    desc.debugName = "TestPipeline";
    desc.shaderProgram = _shaderProgram;
    if (_attachment0.format != HgiFormatInvalid) {
        desc.colorAttachmentDescs.push_back(_attachment0);
    }

    HgiDepthStencilState depthState;
    depthState.depthTestEnabled = true;
    depthState.depthCompareFn = HgiCompareFunctionAlways;
    depthState.stencilTestEnabled = false;
    desc.depthState = depthState;

    desc.vertexBuffers = { _vboDesc };
    desc.depthState.depthWriteEnabled = false;
    desc.multiSampleState.alphaToCoverageEnable = false;
    desc.rasterizationState.cullMode = HgiCullModeBack;
    desc.rasterizationState.polygonMode = HgiPolygonModeFill;
    desc.rasterizationState.winding = HgiWindingCounterClockwise;
    desc.shaderProgram = _shaderProgram;
    desc.shaderConstantsDesc.byteSize = _constantsData.size();
    desc.shaderConstantsDesc.stageUsage = HgiShaderStageFragment;

    _pipeline = _hgi->CreateGraphicsPipeline(desc);

    return true;
}

void
HdSt_TextureTestDriver::_DestroyShaderProgram()
{
    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        _hgi->DestroyShaderFunction(&fn);
    }
    _hgi->DestroyShaderProgram(&_shaderProgram);
}

void
HdSt_TextureTestDriver::_PrintCompileErrors()
{
    for (HgiShaderFunctionHandle fn : _shaderProgram->GetShaderFunctions()) {
        std::cout << fn->GetCompileErrors() << std::endl;
    }
    std::cout << _shaderProgram->GetCompileErrors() << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE

