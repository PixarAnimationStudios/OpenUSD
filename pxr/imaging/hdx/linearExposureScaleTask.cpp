//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/linearExposureScaleTask.h"
#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((linearExposureScaleFrag, "LinearExposureScaleFragment"))
);

HdxLinearExposureScaleTask::HdxLinearExposureScaleTask(
    HdSceneDelegate* delegate,
    SdfPath const& id)
  : HdxTask(id)
{
}

HdxLinearExposureScaleTask::~HdxLinearExposureScaleTask() = default;

void
HdxLinearExposureScaleTask::_Sync(HdSceneDelegate* delegate,
                           HdTaskContext* ctx,
                           HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_compositor) {
        _compositor = std::make_unique<HdxFullscreenShader>(
            _GetHgi(), "LinearExposureScale");
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxLinearExposureScaleTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _cameraPath = params.cameraPath;
        }
    }

    // Currently, we're querying GetLinearExposureScale() every frame -
    // if we wanted to be more selective:
    //
    // - target camera should get a HdCamera::DirtyParams update whenever
    //   any input attrs to linearExposureScale change
    // - we could store a version counter on the HdCamera
    // - ...and a `_lastCamVersion` on this task, and then only update
    //   our _linearExposureScale if it didn't == cam's version
    //
    // However, this shouldn't be a problem unless
    // _compositor->SetShaderConstants is expensive, so not going to
    // optimize unless we have evidence of that.
    if (_cameraPath.IsEmpty()) {
        _linearExposureScale = 1.0f;
    } else {
        HdRenderIndex &renderIndex = delegate->GetRenderIndex();
        const HdCamera *camera = static_cast<const HdCamera *>(
            renderIndex.GetSprim(HdPrimTypeTokens->camera, _cameraPath));
        if (!TF_VERIFY(camera)) {
            return;
        }

        _linearExposureScale = camera->GetLinearExposureScale();
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxLinearExposureScaleTask::Prepare(HdTaskContext* ctx,
                             HdRenderIndex* renderIndex)
{
}

void
HdxLinearExposureScaleTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HgiTextureHandle aovTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);

    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = _tokens->linearExposureScaleFrag.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn");
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");

    // The order of the constant parameters has to match the order in the
    // _ParameterBuffer struct
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "screenSize", "vec2");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "linearExposureScale", "float");

    _compositor->SetProgram(
        HdxPackageLinearExposureScaleShader(),
        _tokens->linearExposureScaleFrag,
        fragDesc);
    const auto &aovDesc = aovTexture->GetDescriptor();
    if (_UpdateParameterBuffer(
            static_cast<float>(aovDesc.dimensions[0]),
            static_cast<float>(aovDesc.dimensions[1]))) {
        size_t byteSize = sizeof(_ParameterBuffer);
        _compositor->SetShaderConstants(byteSize, &_parameterData);
    }

    _compositor->BindTextures({aovTexture});

    _compositor->Draw(aovTexture, /*no depth*/HgiTextureHandle());
}

bool
HdxLinearExposureScaleTask::_UpdateParameterBuffer(
    float screenSizeX, float screenSizeY)
{
    _ParameterBuffer pb;

    pb.linearExposureScale = _linearExposureScale;
    pb.screenSize[0] = screenSizeX;
    pb.screenSize[1] = screenSizeY;

    // All data is still the same, no need to update the storage buffer
    if (pb == _parameterData) {
        return false;
    }

    _parameterData = pb;

    return true;
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(
    std::ostream& out,
    const HdxLinearExposureScaleTaskParams& pv)
{
    out << "LinearExposureScaleTask Params: (...) "
        << pv.cameraPath << " "
    ;
    return out;
}

bool operator==(const HdxLinearExposureScaleTaskParams& lhs,
                const HdxLinearExposureScaleTaskParams& rhs)
{
    return lhs.cameraPath == rhs.cameraPath;
}

bool operator!=(const HdxLinearExposureScaleTaskParams& lhs,
                const HdxLinearExposureScaleTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
