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
#include "pxr/imaging/hdx/camera.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/gf/frustum.h"

TF_DEFINE_PUBLIC_TOKENS(HdxCameraTokens, HDX_CAMERA_TOKENS);

HdxCamera::HdxCamera(HdSceneDelegate *delegate, SdfPath const &id)
    : HdSprim(delegate, id)
{
}

HdxCamera::~HdxCamera()
{
}

void
HdxCamera::Sync()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    SdfPath const &id = GetID();
    HdSceneDelegate *delegate = GetDelegate();
    if (not TF_VERIFY(delegate)) {
        return;
    }

    // HdxCamera communicates to the scene graph and caches all interesting
    // values within this class.

    // later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.
    HdChangeTracker& changeTracker = 
                                delegate->GetRenderIndex().GetChangeTracker();
    HdChangeTracker::DirtyBits bits = changeTracker.GetSprimDirtyBits(id);
    
    if (bits & DirtyParams) {
        GfMatrix4d worldToViewMatrix;
        GfMatrix4d worldToViewInverseMatrix;
        GfMatrix4d projectionMatrix;

        // first we attempt to get GfFrustum, and if it's empty
        // we ask view/projection matrices.

        // XXX: we will soon remove the GfFrustum interface and
        // use view/projection matrices from Phd too.

        VtValue vfrustum = delegate->Get(id, HdxCameraTokens->cameraFrustum);
        if (not vfrustum.IsEmpty()) {
            // XXX This branch can be removed once we only support 
            //     camera matrices
            TF_VERIFY(vfrustum.IsHolding<GfFrustum>());
            GfFrustum frustum = vfrustum.Get<GfFrustum>();

            worldToViewMatrix        = frustum.ComputeViewMatrix();
            worldToViewInverseMatrix = worldToViewMatrix.GetInverse();
            projectionMatrix         = frustum.ComputeProjectionMatrix();

            // Storing the camera frustum in the render index so we can
            // recalculate it later to fit specific windows
            _cameraValues[HdxCameraTokens->cameraFrustum] = frustum;
        } else {
            // XXX Line below to be removed when we only support matrices
            _cameraValues[HdxCameraTokens->cameraFrustum] = VtValue();

            // view/projection matrices
            VtValue vViewMatrix
                = delegate->Get(id, HdxCameraTokens->worldToViewMatrix);
            VtValue vProjMatrix
                = delegate->Get(id, HdxCameraTokens->projectionMatrix);

            TF_VERIFY(vViewMatrix.IsHolding<GfMatrix4d>() and
                      vProjMatrix.IsHolding<GfMatrix4d>());

            worldToViewMatrix        = vViewMatrix.Get<GfMatrix4d>();
            worldToViewInverseMatrix = worldToViewMatrix.GetInverse();
            projectionMatrix         = vProjMatrix.Get<GfMatrix4d>();
        }

        _cameraValues[HdxCameraTokens->worldToViewMatrix] =
            VtValue(worldToViewMatrix);
        _cameraValues[HdxCameraTokens->worldToViewInverseMatrix] =
            VtValue(worldToViewInverseMatrix);
        _cameraValues[HdxCameraTokens->projectionMatrix] =
            VtValue(projectionMatrix);
    }

    if (bits & DirtyWindowPolicy) {
        VtValue vWindowPolicy = delegate->Get(id, HdxCameraTokens->windowPolicy);
        if (vWindowPolicy.IsHolding<CameraUtilConformWindowPolicy>()) {
            _cameraValues[HdxCameraTokens->windowPolicy] = 
                vWindowPolicy.UncheckedGet<CameraUtilConformWindowPolicy>();
        }
    }

    if (bits & DirtyClipPlanes) {
        _cameraValues[HdxCameraTokens->clipPlanes] = delegate->GetClipPlanes(id);
    }
}

/* virtual */
VtValue
HdxCamera::Get(TfToken const &name) const
{
    VtValue r;

    TF_VERIFY(TfMapLookup(_cameraValues, name, &r),
            "HdxCamera - Unknown %s\n",
            name.GetText());

    return r;
}
