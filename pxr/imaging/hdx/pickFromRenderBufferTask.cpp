//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hdx/pickFromRenderBufferTask.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxPickFromRenderBufferTask::HdxPickFromRenderBufferTask(
        HdSceneDelegate* delegate, SdfPath const& id)
    : HdxTask(id)
    , _index(nullptr)
    , _primId(nullptr)
    , _instanceId(nullptr)
    , _elementId(nullptr)
    , _normal(nullptr)
    , _depth(nullptr)
    , _camera(nullptr)
    , _converged(false)
{
}

HdxPickFromRenderBufferTask::~HdxPickFromRenderBufferTask()
{
}

bool
HdxPickFromRenderBufferTask::IsConverged() const
{
    return _converged;
}

void
HdxPickFromRenderBufferTask::_Sync(HdSceneDelegate* delegate,
                                   HdTaskContext* ctx,
                                   HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _GetTaskContextData(ctx, HdxPickTokens->pickParams, &_contextParams);
    _index = &(delegate->GetRenderIndex());

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        _GetTaskParams(delegate, &_params);
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxPickFromRenderBufferTask::Prepare(HdTaskContext* ctx,
                                     HdRenderIndex *renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _primId = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.primIdBufferPath));
    _instanceId = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.instanceIdBufferPath));
    _elementId = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.elementIdBufferPath));
    _normal = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.normalBufferPath));
    _depth = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                              _params.depthBufferPath));

    _camera = static_cast<const HdCamera *>(
        renderIndex->GetSprim(HdPrimTypeTokens->camera,
                              _params.cameraId));
}

GfMatrix4d
HdxPickFromRenderBufferTask::_ComputeProjectionMatrix() const
{
    // Same logic as in HdRenderPassState::GetProjectionMatrix().

    if (_params.framing.IsValid()) {
        const CameraUtilConformWindowPolicy policy =
            _params.overrideWindowPolicy.first
                ?_params.overrideWindowPolicy.second
                : _camera->GetWindowPolicy();
        return
            _params.framing.ApplyToProjectionMatrix(
                _camera->GetProjectionMatrix(), policy);
    } else {
        const double aspect =
            _params.viewport[3] != 0.0
                ? _params.viewport[2] / _params.viewport[3]
                : 1.0;
        return
            CameraUtilConformedWindow(
                _camera->GetProjectionMatrix(),
                _camera->GetWindowPolicy(),
                aspect);
    }
}

void
HdxPickFromRenderBufferTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // We need primId, depth, and a source camera to do anything. The other
    // inputs are optional...
    if (!_primId || !_depth || !_camera) {
        _converged = true;
        return;
    }

    // Resolve the render buffers.
    _primId->Resolve();
    _converged = _primId->IsConverged();

    _depth->Resolve();
    _converged = _converged && _depth->IsConverged();
    if (_depth->GetWidth() != _primId->GetWidth() ||
        _depth->GetHeight() != _primId->GetHeight()) {
        TF_WARN("Depth buffer %s has different dimensions "
                "than Prim Id buffer %s",
                _params.depthBufferPath.GetText(),
                _params.primIdBufferPath.GetText());
        return;
    }

    if (_normal) {
        _normal->Resolve();
        _converged = _converged && _normal->IsConverged();
        if (_normal->GetWidth() != _primId->GetWidth() ||
            _normal->GetHeight() != _primId->GetHeight()) {
            TF_WARN("Normal buffer %s has different dimensions "
                    "than Prim Id buffer %s",
                    _params.normalBufferPath.GetText(),
                    _params.primIdBufferPath.GetText());
            return;
        }
    }
    if (_elementId) {
        _elementId->Resolve();
        _converged = _converged && _elementId->IsConverged();
        if (_elementId->GetWidth() != _primId->GetWidth() ||
            _elementId->GetHeight() != _primId->GetHeight()) {
            TF_WARN("Element Id buffer %s has different dimensions "
                    "than Prim Id buffer %s",
                    _params.elementIdBufferPath.GetText(),
                    _params.primIdBufferPath.GetText());
            return;
        }
    }
    if (_instanceId) {
        _instanceId->Resolve();
        _converged = _converged && _instanceId->IsConverged();
        if (_instanceId->GetWidth() != _primId->GetWidth() ||
            _instanceId->GetHeight() != _primId->GetHeight()) {
            TF_WARN("Instance Id buffer %s has different dimensions "
                    "than Prim Id buffer %s",
                    _params.instanceIdBufferPath.GetText(),
                    _params.primIdBufferPath.GetText());
            return;
        }
    }

    uint8_t *primIdPtr = reinterpret_cast<uint8_t*>(_primId->Map());
    uint8_t *depthPtr = reinterpret_cast<uint8_t*>(_depth->Map());
    uint8_t *normalPtr = _normal ?
                         reinterpret_cast<uint8_t*>(_normal->Map()) :
                         nullptr;
    uint8_t *elementIdPtr = _elementId ? 
                            reinterpret_cast<uint8_t*>(_elementId->Map()) : 
                            nullptr;
    uint8_t *instanceIdPtr = _instanceId ? 
                             reinterpret_cast<uint8_t*>(_instanceId->Map()) : 
                             nullptr;

    const GfVec2i renderBufferSize(_primId->GetWidth(),
                                   _primId->GetHeight());

    // A bit of trickiness: instead of being given an (x,y,radius) tuple,
    // we're given a pick frustum with which to generate an id render.
    // Since we're re-using the id buffers from the main render, we need to
    // project the pick frustum near plane onto the main-render window
    // coordinate space, so that we can determine the subregion of the ID
    // buffer to look at.

    // Get the view, projection used to generate the ID buffers.
    const GfMatrix4d renderView = _camera->GetViewMatrix();
    const GfMatrix4d renderProj = _ComputeProjectionMatrix();

    // renderBufferXf transforms renderbuffer NDC to integer renderbuffer
    // indices, assuming (-1,-1) maps to 0,0 and (1,1) maps to w,h.
    GfMatrix4d renderBufferXf;
    renderBufferXf.SetScale(
            GfVec3d(0.5 * renderBufferSize[0], 0.5 * renderBufferSize[1], 1));
    renderBufferXf.SetTranslateOnly(
            GfVec3d(0.5 * renderBufferSize[0], 0.5 * renderBufferSize[1], 0));

    // Transform the corners of the pick frustum near plane from picking
    // NDC space to main render NDC space to render buffer indices.
    GfMatrix4d pickNdcToRenderBuffer =
        (_contextParams.viewMatrix *
         _contextParams.projectionMatrix).GetInverse() *
        renderView * renderProj * renderBufferXf;

    // Calculate the ID buffer area of interest: the indices of the pick
    // frustum near plane.

    // Take the min and max corners in NDC space as representatives.
    GfVec3d corner0 = pickNdcToRenderBuffer.Transform(GfVec3d(-1,-1,-1));
    GfVec3d corner1 = pickNdcToRenderBuffer.Transform(GfVec3d(1,1,-1));
    // Once transformed, find the minimum and maximum bounds of these points.
    GfVec2d pickMin = GfVec2d(std::min(corner0[0], corner1[0]),
                              std::min(corner0[1], corner1[1]));
    GfVec2d pickMax = GfVec2d(std::max(corner0[0], corner1[0]),
                              std::max(corner0[1], corner1[1]));
    // Since we're turning these into integer indices, round away from the
    // center; otherwise, we'll miss relevant pixels.
    pickMin = GfVec2d(floor(pickMin[0]), floor(pickMin[1]));
    pickMax = GfVec2d(ceil(pickMax[0]), ceil(pickMax[1]));
    GfVec4i subRect = GfVec4i(
            pickMin[0], pickMin[1],
            pickMax[0] - pickMin[0], pickMax[1] - pickMin[1]);

    // Depth range of the "depth" AOV is (0,1)
    GfVec2f depthRange(0, 1);

    // For un-projection in HdxPickResult, we need to provide "viewMatrix" and
    // "projectionMatrix", to be combined into ndcToWorld.
    // Since the id buffers were generated by the main render pass,
    // specify the transform in terms of the main render pass.
    HdxPickResult result(
            reinterpret_cast<int32_t*>(primIdPtr),
            reinterpret_cast<int32_t*>(instanceIdPtr),
            reinterpret_cast<int32_t*>(elementIdPtr),
            nullptr,
            nullptr,
            reinterpret_cast<int32_t*>(normalPtr),
            reinterpret_cast<float*>(depthPtr),
            _index, _contextParams.pickTarget,
            renderView, renderProj, depthRange,
            renderBufferSize, subRect);

    // Resolve!
    if (_contextParams.resolveMode ==
            HdxPickTokens->resolveNearestToCenter) {
        result.ResolveNearestToCenter(_contextParams.outHits);
    } else if (_contextParams.resolveMode ==
            HdxPickTokens->resolveNearestToCamera) {
        result.ResolveNearestToCamera(_contextParams.outHits);
    } else if (_contextParams.resolveMode ==
            HdxPickTokens->resolveUnique) {
        result.ResolveUnique(_contextParams.outHits);
    } else if (_contextParams.resolveMode ==
            HdxPickTokens->resolveAll) {
        result.ResolveAll(_contextParams.outHits);
    } else {
        TF_CODING_ERROR("Unrecognized interesection mode '%s'",
            _contextParams.resolveMode.GetText());
    }

    if (primIdPtr)
        _primId->Unmap();
    if (normalPtr)
        _normal->Unmap();
    if (elementIdPtr)
        _elementId->Unmap();
    if (instanceIdPtr)
        _instanceId->Unmap();
    if (depthPtr)
        _depth->Unmap();
}

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out,
                         const HdxPickFromRenderBufferTaskParams& pv)
{
    out << "PickFromRenderBufferTask Params: (...) "
        << pv.primIdBufferPath << " "
        << pv.instanceIdBufferPath << " "
        << pv.elementIdBufferPath << " "
        << pv.normalBufferPath << " "
        << pv.depthBufferPath << " "
        << pv.cameraId;
    return out;
}

bool operator==(const HdxPickFromRenderBufferTaskParams& lhs,
                const HdxPickFromRenderBufferTaskParams& rhs)
{
    return lhs.primIdBufferPath     == rhs.primIdBufferPath     &&
           lhs.instanceIdBufferPath == rhs.instanceIdBufferPath &&
           lhs.elementIdBufferPath  == rhs.elementIdBufferPath  &&
           lhs.normalBufferPath     == rhs.normalBufferPath     &&
           lhs.depthBufferPath      == rhs.depthBufferPath      &&
           lhs.cameraId             == rhs.cameraId;
}

bool operator!=(const HdxPickFromRenderBufferTaskParams& lhs,
                const HdxPickFromRenderBufferTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
