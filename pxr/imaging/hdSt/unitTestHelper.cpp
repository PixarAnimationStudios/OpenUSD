//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hdSt/hioConversions.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/resourceBinder.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
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

HdSt_TestDriverBase::HdSt_TestDriverBase()
 : _hgi(Hgi::CreatePlatformDefaultHgi())
 , _hgiDriver{HgiTokens->renderDriver, VtValue(_hgi.get())}
 , _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _collection(_tokens->testCollection, HdReprSelector())
 , _clearColor(GfVec4f(0, 0, 0, 1))
 , _clearDepth(1)
{
}

HdSt_TestDriverBase::~HdSt_TestDriverBase()
{
    delete _sceneDelegate;
    delete _renderIndex;
}

void
HdSt_TestDriverBase::_Init()
{
    if (TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "CPU" ||
        TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "GPU") {
        _Init(HdReprSelector(HdReprTokens->smoothHull));
    } else {
        _Init(HdReprSelector(HdReprTokens->hull));
    }
}

void
HdSt_TestDriverBase::_Init(HdReprSelector const &reprSelector)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate, {&_hgiDriver});
    TF_VERIFY(_renderIndex != nullptr);

    _sceneDelegate = new HdUnitTestDelegate(_renderIndex,
                                            SdfPath::AbsoluteRootPath());

    _cameraId = SdfPath("/testCam");
    _sceneDelegate->AddCamera(_cameraId);
    _reprSelector = reprSelector;

    GfMatrix4d viewMatrix = GfMatrix4d().SetIdentity();
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(0.0, 1000.0, 0.0));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0));

    GfFrustum frustum;
    frustum.SetPerspective(45, true, 1, 1.0, 10000.0);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

    SetCamera(viewMatrix,
              projMatrix,
              CameraUtilFraming(
                  GfRect2i(GfVec2i(0, 0), 512, 512)));

    // Update collection with repr and add collection to change tracker.
    _collection.SetReprSelector(reprSelector);
    HdChangeTracker &tracker = _renderIndex->GetChangeTracker();
    tracker.AddCollection(_collection.GetName());
}


static
HdCamera::Projection
_ToHd(const GfCamera::Projection projection)
{
    switch(projection) {
    case GfCamera::Perspective:
        return HdCamera::Perspective;
    case GfCamera::Orthographic:
        return HdCamera::Orthographic;
    }
    TF_CODING_ERROR("Bad GfCamera::Projection value");
    return HdCamera::Perspective;
}

void
HdSt_TestDriverBase::SetCamera(GfMatrix4d const &viewMatrix,
                           GfMatrix4d const &projectionMatrix,
                           CameraUtilFraming const &framing)
{
    GfCamera cam;
    cam.SetFromViewAndProjectionMatrix(viewMatrix,
                                       projectionMatrix);
    
    _sceneDelegate->UpdateTransform(
        _cameraId,
        GfMatrix4f(cam.GetTransform()));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->projection,
        VtValue(_ToHd(cam.GetProjection())));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->focalLength,
        VtValue(cam.GetFocalLength() *
                float(GfCamera::FOCAL_LENGTH_UNIT)));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->horizontalAperture,
        VtValue(cam.GetHorizontalAperture() *
                float(GfCamera::APERTURE_UNIT)));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->verticalAperture,
        VtValue(cam.GetVerticalAperture() *
                float(GfCamera::APERTURE_UNIT)));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->horizontalApertureOffset,
        VtValue(cam.GetHorizontalApertureOffset() *
                float(GfCamera::APERTURE_UNIT)));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->verticalApertureOffset,
        VtValue(cam.GetVerticalApertureOffset() *
                float(GfCamera::APERTURE_UNIT)));
    _sceneDelegate->UpdateCamera(
        _cameraId,
        HdCameraTokens->clippingRange,
        VtValue(cam.GetClippingRange()));

    // Baselines for tests were generated without constraining the view
    // frustum based on the viewport aspect ratio.
    _sceneDelegate->UpdateCamera(
        _cameraId, HdCameraTokens->windowPolicy,
        VtValue(CameraUtilDontConform));
    
    const HdCamera * const camera =
        dynamic_cast<HdCamera const *>(
            _renderIndex->GetSprim(
                HdPrimTypeTokens->camera,
                _cameraId));
    TF_VERIFY(camera);

    for (const HdRenderPassStateSharedPtr &renderPassState: _renderPassStates) {
        renderPassState->SetCameraAndFraming(
            camera, framing, { false, CameraUtilFit });
    }
}
    
void
HdSt_TestDriverBase::SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes)
{
    _sceneDelegate->UpdateCamera(
        _cameraId, HdCameraTokens->clipPlanes, VtValue(clipPlanes));
}

void
HdSt_TestDriverBase::SetCullStyle(HdCullStyle cullStyle)
{
    for (const HdRenderPassStateSharedPtr &renderPassState: _renderPassStates) {
        renderPassState->SetCullStyle(cullStyle);
    }
}

void
HdSt_TestDriverBase::SetRepr(HdReprSelector const &reprSelector)
{
    _collection.SetReprSelector(reprSelector);

    // Mark changes.
    HdChangeTracker &tracker = _renderIndex->GetChangeTracker();
    tracker.MarkCollectionDirty(_collection.GetName());

    for (const HdRenderPassSharedPtr &renderPass : _renderPasses) {
        renderPass->SetRprimCollection(_collection);
    }
}

static TfTokenVector _aovOutputs {
    HdAovTokens->color,
    HdAovTokens->depth
};

SdfPath
HdSt_TestDriverBase::_GetAovPath(TfToken const &aov) const
{
    std::string identifier = std::string("aov_") +
        TfMakeValidIdentifier(aov.GetString());
    return SdfPath("/testDriver").AppendChild(TfToken(identifier));
}

void
HdSt_TestDriverBase::SetupAovs(int width, int height)
{
    if (_aovBindings.empty()) {  
        // Delete old render buffers.
        for (auto const &id : _aovBufferIds) {
            _renderIndex->RemoveBprim(HdPrimTypeTokens->renderBuffer, id);
        }

        _aovBufferIds.clear();
        _aovBindings.clear();
        _aovBindings.resize(_aovOutputs.size());
        
        GfVec3i dimensions(width, height, 1);

        // Create aov bindings and render buffers.
        for (size_t i = 0; i < _aovOutputs.size(); i++) {
            SdfPath aovId = _GetAovPath(_aovOutputs[i]);

            _aovBufferIds.push_back(aovId);

            HdAovDescriptor aovDesc = _renderDelegate.GetDefaultAovDescriptor(
                _aovOutputs[i]);

            HdRenderBufferDescriptor desc = { dimensions, aovDesc.format,
                /*multiSampled*/false};
            _AddRenderBuffer(aovId, desc);

            HdRenderPassAovBinding &binding = _aovBindings[i];
            binding.aovName = _aovOutputs[i];
            binding.aovSettings = aovDesc.aovSettings;
            binding.renderBufferId = aovId;
            binding.renderBuffer = dynamic_cast<HdRenderBuffer*>(
                _renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer, aovId));

            if (_aovOutputs[i] == HdAovTokens->color) {
                binding.clearValue = VtValue(_clearColor);
            } else if (_aovOutputs[i] == HdAovTokens->depth) {
                binding.clearValue = VtValue(_clearDepth); 
            }
        }
    }

    for (const HdRenderPassStateSharedPtr &renderPassState: _renderPassStates) {
        renderPassState->SetAovBindings(_aovBindings);
    }
}

bool
HdSt_TestDriverBase::WriteToFile(std::string const & attachment,
                                 std::string const & filename)
{
    const SdfPath aovId = _GetAovPath(TfToken(attachment));

    HdRenderBuffer * const renderBuffer = dynamic_cast<HdRenderBuffer*>(
        GetDelegate().GetRenderIndex().GetBprim(HdPrimTypeTokens->renderBuffer,
            aovId));
    if (!renderBuffer) {
        TF_CODING_ERROR("No HdRenderBuffer prim at path %s", aovId.GetText());
        return false;
    }

    HioImage::StorageSpec storage;
    storage.width = renderBuffer->GetWidth();
    storage.height = renderBuffer->GetHeight();
    storage.format = 
        HdStHioConversions::GetHioFormat(renderBuffer->GetFormat());
    storage.flipped = true;
    storage.data = renderBuffer->Map();
    TfScoped<> scopedUnmap([renderBuffer](){ renderBuffer->Unmap(); });

    if (storage.format == HioFormatInvalid) {
        TF_CODING_ERROR("Render buffer %s has format not corresponding to a"
                        "HioFormat", aovId.GetText());
        return false;
    }

    if (!storage.data) {
        TF_CODING_ERROR("No data for render buffer %s", aovId.GetText());
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

void
HdSt_TestDriverBase::_AddRenderBuffer(SdfPath const &id, 
    HdRenderBufferDescriptor const &desc)
{
    GetDelegate().AddRenderBuffer(id, desc.dimensions, desc.format,
        desc.multiSampled);
}

void
HdSt_TestDriverBase::UpdateAovDimensions(int width, int height)
{
    const GfVec3i dimensions(width, height, 1);

    for (auto const& id : _aovBufferIds) {
        HdRenderBufferDescriptor desc =
            GetDelegate().GetRenderBufferDescriptor(id);
        if (desc.dimensions != dimensions) {
            GetDelegate().UpdateRenderBuffer(id, dimensions, desc.format, 
                desc.multiSampled);
        }
    }
}

void
HdSt_TestDriverBase::Present(int width, int height, uint32_t framebuffer)
{
    HgiTextureHandle colorTexture;
    {
        HdRenderPassAovBinding const &aovBinding = _aovBindings[0];
        VtValue aov = aovBinding.renderBuffer->GetResource(false);
        if (aov.IsHolding<HgiTextureHandle>()) {
            colorTexture = aov.UncheckedGet<HgiTextureHandle>();
        }
    }

    _interop.TransferToApp(
        _hgi.get(),
        colorTexture, 
        /*srcDepth*/HgiTextureHandle(),
        HgiTokens->OpenGL,
        VtValue(framebuffer), 
        GfVec4i(0, 0, width, height));
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

HdSt_TestLightingShader::HdSt_TestLightingShader()
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
        "    return HdGet_sceneAmbient()                                    \n"
        "      + color * HdGet_l0color() * max(0.0, dot(n, HdGet_l0dir()))  \n"
        "      + color * HdGet_l1color() * max(0.0, dot(n, HdGet_l1dir())); \n"
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
    for (int i = 0; i < 2; ++i) {
        _lights[i].eyeDir
            = worldToViewMatrix.TransformDir(_lights[i].dir).GetNormalized();
    }
}

/* virtual */
void
HdSt_TestLightingShader::BindResources(const int program,
                                       HdSt_ResourceBinder const &binder,
                                       HdRenderPassState const &state)
{
    binder.BindUniformf(_tokens->l0dir,   3, _lights[0].eyeDir.GetArray());
    binder.BindUniformf(_tokens->l0color, 3, _lights[0].color.GetArray());
    binder.BindUniformf(_tokens->l1dir,   3, _lights[1].eyeDir.GetArray());
    binder.BindUniformf(_tokens->l1color, 3, _lights[1].color.GetArray());
    binder.BindUniformf(_tokens->sceneAmbient, 3, _sceneAmbient.GetArray());
}

/* virtual */
void
HdSt_TestLightingShader::UnbindResources(const int program,
                                         HdSt_ResourceBinder const &binder,
                                         HdRenderPassState const &state)
{
}

/*virtual*/
void
HdSt_TestLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l0dir, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l0color, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l1dir, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l1color, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->sceneAmbient, HdTypeFloatVec3);
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


PXR_NAMESPACE_CLOSE_SCOPE

