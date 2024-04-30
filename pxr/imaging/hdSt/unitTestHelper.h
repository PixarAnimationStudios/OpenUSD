//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HD_ST_UNIT_TEST_HELPER_H
#define PXR_IMAGING_HD_ST_UNIT_TEST_HELPER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/hioConversions.h"
#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestDelegate.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hgiInterop/hgiInterop.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/scoped.h"

#include <memory>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HdSt_ResourceBinder;

using HgiUniquePtr = std::unique_ptr<class Hgi>;

/// \class HdSt_TestDriverBase
///
/// A base class for unit test drivers that creates all core components but no
/// render passes.
///
/// \note This test driver does NOT assume OpenGL is available; in the event
/// that is is not available, all OpenGL calls become no-ops, but all other work
/// is performed as usual.
///
template<typename SceneDelegate>
class HdSt_TestDriverBase
{
public:
    HdSt_TestDriverBase();
    virtual ~HdSt_TestDriverBase();

    /// Set camera to renderpass
    void SetCamera(GfMatrix4d const &viewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   CameraUtilFraming const &framing);

    void SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes);
    
    /// Set cull style
    void SetCullStyle(HdCullStyle cullStyle);

    /// Returns the UnitTest delegate
    SceneDelegate& GetDelegate() { return *_sceneDelegate; }

    /// Switch repr
    void SetRepr(HdReprSelector const &reprSelector);

    void SetupAovs(int width, int height);

    void UpdateAovDimensions(int width, int height);

    bool WriteToFile(std::string const & attachment,
                     std::string const & filename);

    void Present(int width, int height, uint32_t framebuffer);

    void SetClearColor(GfVec4f const &clearColor) {
        _clearColor = clearColor;
    }

    void SetClearDepth(float clearDepth) {
        _clearDepth = clearDepth;
    }

protected:
    void _SetupSceneDelegate();
    virtual void _Init();
    virtual void _Init(HdReprSelector const &reprSelector);

    SdfPath _GetAovPath(TfToken const &aov) const;

    const HdRprimCollection &_GetCollection() const { return _collection; }
    HdStRenderDelegate * _GetRenderDelegate() { return &_renderDelegate; }
    HdEngine * _GetEngine() { return &_engine; }
    Hgi * _GetHgi() { return _hgi.get(); }

    std::vector<HdRenderPassSharedPtr> _renderPasses;
    std::vector<HdStRenderPassStateSharedPtr> _renderPassStates;

    HdRenderPassAovBindingVector _aovBindings;
    SdfPathVector _aovBufferIds;

    SdfPath _cameraId;
    HdRprimCollection _collection;

private:
    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr _hgi;
    HdDriver _hgiDriver;

    HdStRenderDelegate _renderDelegate;
    HdEngine _engine;
    HdRenderIndex *_renderIndex;
    SceneDelegate *_sceneDelegate;

    HgiInterop _interop;

    GfVec4f _clearColor;
    float _clearDepth;
};

template<typename SceneDelegate>
HdSt_TestDriverBase<SceneDelegate>::HdSt_TestDriverBase()
 : _collection(TfToken("testCollection"), HdReprSelector())
 , _hgi(Hgi::CreatePlatformDefaultHgi())
 , _hgiDriver{HgiTokens->renderDriver, VtValue(_hgi.get())}
 , _renderDelegate()
 , _engine()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _clearColor(GfVec4f(0, 0, 0, 1))
 , _clearDepth(1)
{
}

template<typename SceneDelegate>
HdSt_TestDriverBase<SceneDelegate>::~HdSt_TestDriverBase()
{
    for (size_t i = 0; i < _renderPassStates.size(); i++) {
        _renderPassStates[i].reset();
    }

    for (size_t i = 0; i < _renderPasses.size(); i++) {
        _renderPasses[i].reset();
    }

    delete _sceneDelegate;
    delete _renderIndex;
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::_SetupSceneDelegate()
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate, {&_hgiDriver});
    TF_VERIFY(_renderIndex != nullptr);

    _sceneDelegate = new SceneDelegate(
        _renderIndex, SdfPath::AbsoluteRootPath());
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::_Init()
{
    _Init(HdReprSelector(HdReprTokens->smoothHull));
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::_Init(HdReprSelector const &reprSelector)
{
    _SetupSceneDelegate();

    _cameraId = SdfPath("/testCam");
    _sceneDelegate->AddCamera(_cameraId);

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

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::SetCamera(
    GfMatrix4d const &viewMatrix,
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

    for (const HdStRenderPassStateSharedPtr &renderPassState: _renderPassStates) {
        renderPassState->SetCamera(camera);
        renderPassState->SetFraming(framing);
        renderPassState->SetOverrideWindowPolicy(std::nullopt);
    }
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::SetCameraClipPlanes(
    std::vector<GfVec4d> const& clipPlanes)
{
    _sceneDelegate->UpdateCamera(_cameraId, HdCameraTokens->clipPlanes, 
        VtValue(clipPlanes));
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::SetCullStyle(HdCullStyle cullStyle)
{
    for (const HdStRenderPassStateSharedPtr &renderPassState: _renderPassStates) {
        renderPassState->SetCullStyle(cullStyle);
    }
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::SetRepr(HdReprSelector const &reprSelector)
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

template<typename SceneDelegate>
SdfPath
HdSt_TestDriverBase<SceneDelegate>::_GetAovPath(TfToken const &aov) const
{
    std::string identifier = std::string("aov_") +
        TfMakeValidIdentifier(aov.GetString());
    return SdfPath("/testDriver").AppendChild(TfToken(identifier));
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::SetupAovs(int width, int height)
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
            GetDelegate().AddRenderBuffer(aovId, desc);

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

    for (const HdStRenderPassStateSharedPtr &renderPassState: _renderPassStates) {
        renderPassState->SetAovBindings(_aovBindings);
    }
}

template<typename SceneDelegate>
bool
HdSt_TestDriverBase<SceneDelegate>::WriteToFile(
    std::string const & attachment, std::string const & filename)
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

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::UpdateAovDimensions(int width, int height)
{
    const GfVec3i dimensions(width, height, 1);

    for (auto const& id : _aovBufferIds) {
        HdRenderBufferDescriptor desc =
            GetDelegate().GetRenderBufferDescriptor(id);
        if (desc.dimensions != dimensions) {
            desc.dimensions = dimensions;
            GetDelegate().UpdateRenderBuffer(id, desc);
        }
    }
}

template<typename SceneDelegate>
void
HdSt_TestDriverBase<SceneDelegate>::Present(
    int width, int height, uint32_t framebuffer)
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

/// \class HdSt_DrawTask
///
/// A simple task to execute a render pass.
///
class HdSt_DrawTask final : public HdTask
{
public:
    HDST_API
    HdSt_DrawTask(HdRenderPassSharedPtr const &renderPass,
                  HdStRenderPassStateSharedPtr const &renderPassState,
                  const TfTokenVector &renderTags);
    
    HDST_API
    void Sync(HdSceneDelegate*,
              HdTaskContext*,
              HdDirtyBits*) override;

    HDST_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    HDST_API
    void Execute(HdTaskContext* ctx) override;

    HDST_API
    const TfTokenVector &GetRenderTags() const override
    {
        return _renderTags;
    }

private:
    HdRenderPassSharedPtr const _renderPass;
    HdStRenderPassStateSharedPtr const _renderPassState;
    TfTokenVector const _renderTags;

    HdSt_DrawTask() = delete;
    HdSt_DrawTask(const HdSt_DrawTask &) = delete;
    HdSt_DrawTask &operator =(const HdSt_DrawTask &) = delete;
};

// --------------------------------------------------------------------------

/// \class HdSt_TestDriver
///
/// A unit test driver that exercises the core engine.
///
class HdSt_TestDriver final : public HdSt_TestDriverBase<HdUnitTestDelegate>
{
public:
    HDST_API
    HdSt_TestDriver();
    HDST_API
    HdSt_TestDriver(TfToken const &reprName);
    HDST_API
    HdSt_TestDriver(HdReprSelector const &reprSelector);

    /// Draw
    HDST_API
    void Draw(bool withGuides=false);

    /// Draw with external renderPass
    HDST_API
    void Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides);

    HDST_API
    const HdStRenderPassStateSharedPtr &GetRenderPassState() const {
        return _renderPassStates[0];
    }

    HDST_API
    const HdRenderPassSharedPtr &GetRenderPass();

private:
    void _CreateRenderPassState();
};

// --------------------------------------------------------------------------

/// \class HdSt_TestLightingShader
///
/// A custom lighting shader for unit tests.
///
using HdSt_TestLightingShaderSharedPtr =
    std::shared_ptr<class HdSt_TestLightingShader>;

class HdSt_TestLightingShader : public HdStLightingShader
{
public:
    HDST_API
    HdSt_TestLightingShader(HdRenderIndex * renderIndex);
    HDST_API
    ~HdSt_TestLightingShader() override;

    /// HdStShaderCode overrides
    HDST_API
    ID ComputeHash() const override;
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;
    void AddBindings(HdStBindingRequestVector *customBindings) override;

    /// HdStLightingShader overrides
    void SetCamera(GfMatrix4d const &worldToViewMatrix,
                   GfMatrix4d const &projectionMatrix) override;

    void SetSceneAmbient(GfVec3f const &color);
    void SetLight(int light, GfVec3f const &dir, GfVec3f const &color);

    /// Prepare lighting resource buffers
    void Prepare();

private:
    struct Light {
        GfVec3f dir;
        GfVec3f eyeDir;
        GfVec3f color;
    };
    Light _lights[2];
    GfVec3f _sceneAmbient;
    std::unique_ptr<HioGlslfx> _glslfx;

    HdRenderIndex * _renderIndex;
    HdBufferArrayRangeSharedPtr _lightingBar;
};

// --------------------------------------------------------------------------

class HdSt_TextureTestDriver
{
public:
    HDST_API
    HdSt_TextureTestDriver();
    HDST_API
    ~HdSt_TextureTestDriver();

    HDST_API
    void Draw(HgiTextureHandle const &colorDst,
              HgiTextureHandle const &inputTexture,
              HgiSamplerHandle const &inputSampler);

    HDST_API
    bool WriteToFile(HgiTextureHandle const &dstTexture,
                     std::string filename) const;

    HDST_API
    Hgi * GetHgi() { return _hgi.get(); }

private:
    void _CreateShaderProgram();
    void _DestroyShaderProgram();
    void _CreateBufferResources();
    bool _CreateTextureBindings(HgiTextureHandle const &textureHandle, 
                                HgiSamplerHandle const &samplerHandle);
    void _CreateVertexBufferDescriptor();
    bool _CreatePipeline(HgiTextureHandle const &colorDst);
    void _PrintCompileErrors();

    HgiUniquePtr _hgi;
    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;
    HgiShaderProgramHandle _shaderProgram;
    HgiResourceBindingsHandle _resourceBindings;
    HgiGraphicsPipelineHandle _pipeline;
    HgiVertexBufferDesc _vboDesc;
    HgiAttachmentDesc _attachment0;
    std::vector<uint8_t> _constantsData;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_UNIT_TEST_HELPER_H