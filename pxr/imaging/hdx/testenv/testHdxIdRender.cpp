//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

struct PickParam {
    GfVec2d location;
    GfVec4d viewport;
};

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver(TfToken const &reprName);
    
    void SetupPickableAovs(int width, int height);
    void UpdatePickableAovDimensions(int width, int height);

    void Draw(GfVec4d const &viewport, PickParam const * pickParam);

    template<typename T>
    HdStTextureUtils::AlignedBuffer<T>
    ReadAovBuffer(TfToken const &aovName);

protected:
    void _Init(HdReprSelector const &reprSelector) override;
    HdRenderBuffer const * _FindAovBuffer(TfToken const &aovName) const;

private:
    HdRenderPassAovBindingVector _pickableAovBindings;
    SdfPathVector _pickableAovBufferIds;
};

Hdx_TestDriver::Hdx_TestDriver(TfToken const &reprName)
{
    _Init(HdReprSelector(reprName));
}

void
Hdx_TestDriver::_Init(HdReprSelector const &reprSelector)
{   
    _SetupSceneDelegate();
    
    Hdx_UnitTestDelegate &delegate = GetDelegate();

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);

    // render task parameters.
    HdxRenderTaskParams param = delegate.GetTaskParam(
        renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    delegate.SetTaskParam(renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, reprSelector)));
}

void
Hdx_TestDriver::Draw(GfVec4d const &viewport, PickParam const * pickParam)
{
    Hdx_UnitTestDelegate &delegate = GetDelegate();

    HdTaskSharedPtrVector tasks;
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    tasks.push_back(delegate.GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(delegate.GetRenderIndex().GetTask(renderTask));

    HdxRenderTaskParams param = delegate.GetTaskParam(
        renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.viewport = viewport;
    param.aovBindings = pickParam ? _pickableAovBindings : _aovBindings;
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    _GetEngine()->Execute(&delegate.GetRenderIndex(), &tasks);
}

static TfTokenVector _pickableAovOutputs = {
    HdAovTokens->primId,
    HdAovTokens->instanceId,
    HdAovTokens->depth
};

void
Hdx_TestDriver::SetupPickableAovs(int width, int height)
{
    if (_pickableAovBindings.empty()) {    
        // Delete old render buffers.
        for (auto const &id : _pickableAovBufferIds) {
            GetDelegate().GetRenderIndex().RemoveBprim(
                HdPrimTypeTokens->renderBuffer, id);
        }

        _pickableAovBufferIds.clear();
        _pickableAovBindings.clear();
        _pickableAovBindings.resize(_pickableAovOutputs.size());
        
        GfVec3i dimensions(width, height, 1);

        // Create aov bindings and render buffers.
        for (size_t i = 0; i < _pickableAovOutputs.size(); i++) {
            SdfPath aovId = _GetAovPath(_pickableAovOutputs[i]);

            _pickableAovBufferIds.push_back(aovId);

            HdAovDescriptor aovDesc = 
                _GetRenderDelegate()->GetDefaultAovDescriptor(
                    _pickableAovOutputs[i]); 

            HdRenderBufferDescriptor desc = { dimensions, aovDesc.format, 
                /*multiSampled*/false};
            GetDelegate().AddRenderBuffer(aovId, desc);

            HdRenderPassAovBinding &binding = _pickableAovBindings[i];
            binding.aovName = _pickableAovOutputs[i];
            binding.aovSettings = aovDesc.aovSettings;
            binding.renderBufferId = aovId;
            binding.renderBuffer = dynamic_cast<HdRenderBuffer*>(
                GetDelegate().GetRenderIndex().GetBprim(
                    HdPrimTypeTokens->renderBuffer, aovId));
            binding.clearValue = VtValue(GfVec4f(1));
        }
    }
}

HdRenderBuffer const *
Hdx_TestDriver::_FindAovBuffer(TfToken const &aovName) const
{
    HdRenderPassAovBindingVector::const_iterator bindingIt =
        std::find_if(_pickableAovBindings.begin(), _pickableAovBindings.end(),
            [&aovName](HdRenderPassAovBinding const & binding) {
                return binding.aovName == aovName;
            });

    if (!TF_VERIFY(bindingIt != _pickableAovBindings.end())) {
        return nullptr;
    }

    return bindingIt->renderBuffer;
}

template<typename T>
HdStTextureUtils::AlignedBuffer<T>
Hdx_TestDriver::ReadAovBuffer(TfToken const &aovName)
{
    HdRenderBuffer const * renderBuffer = _FindAovBuffer(aovName);
    
    VtValue aov = renderBuffer->GetResource(false);
    if (aov.IsHolding<HgiTextureHandle>()) {
        HgiTextureHandle texture = aov.UncheckedGet<HgiTextureHandle>();

        if (texture) {
            size_t bufferSize = 0;
            return HdStTextureUtils::HgiTextureReadback<T>(
                                        _GetHgi(), texture, &bufferSize);
        }
    }

    return HdStTextureUtils::AlignedBuffer<T>();
}

void
Hdx_TestDriver::UpdatePickableAovDimensions(int width, int height)
{
    const GfVec3i dimensions(width, height, 1);

    for (auto const& id : _pickableAovBufferIds) {
        HdRenderBufferDescriptor desc =
            GetDelegate().GetRenderBufferDescriptor(id);
        if (desc.dimensions != dimensions) {
            desc.dimensions = dimensions;
            GetDelegate().UpdateRenderBuffer(id, desc);
        }
    }
}

// --------------------------------------------------------------------------

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
    }

    void DrawScene(PickParam const * pickParam = nullptr);

    SdfPath PickScene(int pickX, int pickY, int * outInstanceIndex = nullptr);

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    void MousePress(int button, int x, int y, int modKeys) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    std::unique_ptr<Hdx_TestDriver> _driver;

    TfToken _reprName;
    int _refineLevel;
};

////////////////////////////////////////////////////////////

static GfMatrix4d
_GetTranslate(float tx, float ty, float tz)
{
    GfMatrix4d m(1.0f);
    m.SetRow(3, GfVec4f(tx, ty, tz, 1.0));
    return m;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<Hdx_TestDriver>(_reprName);

    Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();

    delegate.SetRefineLevel(_refineLevel);

    // prepare scene
    // To ensure that the non-aggregated element index returned via picking, 
    // we need to have at least two cubes with uniform colors.
    GfVec4f red(1,0,0,1), green(0,1,0,1), blue(0,0,1,1),
            yellow(1,1,0,1), magenta(1,0,1,1), cyan(0,1,1,1),
            white(1,1,1,1), black(0,0,0,1);

    GfVec4f faceColors[] = { red, green, blue, yellow, magenta, cyan};
    VtValue faceColor = VtValue(_BuildArray(&faceColors[0],
                                 sizeof(faceColors)/sizeof(faceColors[0])));

    GfVec4f vertColors[] = { white, blue, green, yellow,
                             black, blue, magenta, red};
    VtValue vertColor = VtValue(_BuildArray(&vertColors[0],
                                 sizeof(vertColors)/sizeof(vertColors[0])));

    delegate.AddCube(SdfPath("/cube0"), _GetTranslate( 5, 0, 5),
                       /*guide=*/false, /*instancerId=*/SdfPath(),
                       /*scheme=*/PxOsdOpenSubdivTokens->catmullClark,
                       /*color=*/faceColor,
                       /*colorInterpolation=*/HdInterpolationUniform);
    delegate.AddCube(SdfPath("/cube1"), _GetTranslate(-5, 0, 5),
                       /*guide=*/false, /*instancerId=*/SdfPath(),
                       /*scheme=*/PxOsdOpenSubdivTokens->catmullClark,
                       /*color=*/faceColor,
                       /*colorInterpolation=*/HdInterpolationUniform);
    delegate.AddCube(SdfPath("/cube2"), _GetTranslate(-5, 0,-5));
    delegate.AddCube(SdfPath("/cube3"), _GetTranslate( 5, 0,-5),
                        /*guide=*/false, /*instancerId=*/SdfPath(),
                       /*scheme=*/PxOsdOpenSubdivTokens->catmullClark,
                       /*color=*/vertColor,
                       /*colorInterpolation=*/HdInterpolationVertex);

    {
        delegate.AddInstancer(SdfPath("/instancerTop"));
        delegate.AddCube(SdfPath("/protoTop"),
                         GfMatrix4d(1), false, SdfPath("/instancerTop"));

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoTop"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, 2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, 2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, 2);
        prototypeIndex[2] = 0;

        delegate.SetInstancerProperties(SdfPath("/instancerTop"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    {
        delegate.AddInstancer(SdfPath("/instancerBottom"));
        delegate.AddCube(SdfPath("/protoBottom"),
                         GfMatrix4d(1), false, SdfPath("/instancerBottom"));

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoBottom"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, -2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, -2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, -2);
        prototypeIndex[2] = 0;

        delegate.SetInstancerProperties(SdfPath("/instancerBottom"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    SetCameraTranslate(GfVec3f(0, 0, -20));

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
    _driver->SetupPickableAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    DrawScene();
}

void
My_TestGLDrawing::OffscreenTest()
{
    SdfPath primId;
    int instanceIndex = -1;

    primId = PickScene(175, 90, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/cube1") && instanceIndex == 0);

    primId = PickScene(470, 90, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/cube0") && instanceIndex == 0);

    primId = PickScene(470, 364, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/cube3") && instanceIndex == 0);
    
    primId = PickScene(250, 190, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/protoTop") && instanceIndex == 2);

    primId = PickScene(320, 290, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/protoBottom") && instanceIndex == 1);
}

void
My_TestGLDrawing::DrawScene(PickParam const * pickParam)
{
    int width = GetWidth(), height = GetHeight();

    GfMatrix4d viewMatrix = GetViewMatrix();

    GfFrustum frustum = GetFrustum();
    GfVec4d viewport(0, 0, width, height);

    if (pickParam) {
        frustum = frustum.ComputeNarrowedFrustum(
            GfVec2d((2.0 * pickParam->location[0]) / width - 1.0,
                    (2.0 * (height-pickParam->location[1])) / height - 1.0),
            GfVec2d(1.0 / width, 1.0 / height));
        viewport = pickParam->viewport;
    }

    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    _driver->GetDelegate().SetCamera(viewMatrix, projMatrix);

    _driver->UpdateAovDimensions(width, height);
    _driver->UpdatePickableAovDimensions(width, height);

    _driver->Draw(viewport, pickParam);
}

SdfPath
My_TestGLDrawing::PickScene(int pickX, int pickY, int * outInstanceIndex)
{
    int width = 128;
    int height = 128;

    PickParam pickParam;
    pickParam.location = GfVec2d(pickX, pickY);
    pickParam.viewport = GfVec4d(0, 0, width, height);

    DrawScene(&pickParam);

    HdStTextureUtils::AlignedBuffer<unsigned char> primId =
        _driver->ReadAovBuffer<unsigned char>(HdAovTokens->primId);

    HdStTextureUtils::AlignedBuffer<unsigned char> instanceId =
        _driver->ReadAovBuffer<unsigned char>(HdAovTokens->instanceId);

    HdStTextureUtils::AlignedBuffer<float> depths =
        _driver->ReadAovBuffer<float>(HdAovTokens->depth);

    double zMin = 1.0;
    int zMinIndex = -1;
    for (int y=0, i=0; y<height; y++) {
        for (int x=0; x<width; x++, i++) {
            if (depths.get()[i] < zMin) {
                zMin = depths.get()[i];
                zMinIndex = i;
            }
        }
    }

    bool didHit = (zMin < 1.0);

    SdfPath result;
    if (didHit) {
        int idIndex = zMinIndex*4;

        result = _driver->GetDelegate().GetRenderIndex().GetRprimPathFromPrimId(
                HdxPickTask::DecodeIDRenderColor(primId.get() + idIndex));
        if (outInstanceIndex) {
            *outInstanceIndex = HdxPickTask::DecodeIDRenderColor(
                instanceId.get() + idIndex);
        }
    }

    return result;
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MousePress(button, x, y, modKeys);
    int instanceIndex = 0;
    SdfPath primId = PickScene(x, y, &instanceIndex);

    if (!primId.IsEmpty()) {
        std::cout << "pick(" << x << ", " << y << "): "
                  << "primId == " << primId << " "
                  << "instance == " << instanceIndex << "\n";
    }
}

void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        }
    }
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;

    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

