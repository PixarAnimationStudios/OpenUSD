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
#include "pxr/imaging/hdSt/camera.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(HdStCameraTokens, HDST_CAMERA_TOKENS);

HdStCamera::HdStCamera(SdfPath const &id)
 : HdSprim(id)
{
}

HdStCamera::~HdStCamera()
{
}

void
HdStCamera::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetID();
    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    // HdStCamera communicates to the scene graph and caches all interesting
    // values within this class.
    // Later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.
    HdDirtyBits bits = *dirtyBits;
    
    if (bits & DirtyViewMatrix) {
        GfMatrix4d worldToViewMatrix(1.0);
        GfMatrix4d worldToViewInverseMatrix(1.0);

        // extract view matrix
        VtValue vMatrix = sceneDelegate->Get(id,
            HdStCameraTokens->worldToViewMatrix);
        worldToViewMatrix = vMatrix.Get<GfMatrix4d>();
        worldToViewInverseMatrix = worldToViewMatrix.GetInverse();

        // store view matrix
        _cameraValues[HdStCameraTokens->worldToViewMatrix] =
            VtValue(worldToViewMatrix);
        _cameraValues[HdStCameraTokens->worldToViewInverseMatrix] =
            VtValue(worldToViewInverseMatrix);
    }

    if (bits & DirtyProjMatrix) {
        GfMatrix4d projectionMatrix(1.0);

        // extract projection matrix
        VtValue vMatrix = sceneDelegate->Get(id,
            HdStCameraTokens->projectionMatrix);
        projectionMatrix = vMatrix.Get<GfMatrix4d>();

        // store projection matrix
        _cameraValues[HdStCameraTokens->projectionMatrix] =
            VtValue(projectionMatrix);
    }

    if (bits & DirtyWindowPolicy) {
        _cameraValues[HdStCameraTokens->windowPolicy] =
                sceneDelegate->Get(id, HdStCameraTokens->windowPolicy);
    }

    if (bits & DirtyClipPlanes) {
        _cameraValues[HdStCameraTokens->clipPlanes] =
                sceneDelegate->GetClipPlanes(id);
    }

    *dirtyBits = Clean;
}

/* virtual */
VtValue
HdStCamera::Get(TfToken const &name) const
{
    VtValue r;

    TF_VERIFY(TfMapLookup(_cameraValues, name, &r),
            "HdStCamera - Unknown %s\n",
            name.GetText());

    return r;
}

/* virtual */
HdDirtyBits
HdStCamera::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE

