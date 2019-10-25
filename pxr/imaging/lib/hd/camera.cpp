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
#include "pxr/imaging/hd/camera.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/gf/frustum.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(HdCameraTokens, HD_CAMERA_TOKENS);

HdCamera::HdCamera(SdfPath const &id)
 :  HdSprim(id)
  , _windowPolicy(CameraUtilFit)
{
}

HdCamera::~HdCamera()
{
}

void
HdCamera::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetId();
    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    // HdCamera communicates to the scene graph and caches all interesting
    // values within this class.
    // Later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.
    HdDirtyBits bits = *dirtyBits;
    
    if (bits & DirtyViewMatrix) {
        // extract and store view matrix
        VtValue vMatrix = sceneDelegate->GetCameraParamValue(id,
            HdCameraTokens->worldToViewMatrix);
        
        _worldToViewMatrix = vMatrix.Get<GfMatrix4d>();
        _worldToViewInverseMatrix = _worldToViewMatrix.GetInverse();
    }

    if (bits & DirtyProjMatrix) {
        // extract and store projection matrix
        VtValue vMatrix = sceneDelegate->GetCameraParamValue(id,
            HdCameraTokens->projectionMatrix);
        _projectionMatrix = vMatrix.Get<GfMatrix4d>();
    }

    if (bits & DirtyWindowPolicy) {
        // treat window policy as an optional parameter
        VtValue vPolicy = sceneDelegate->GetCameraParamValue(id, 
            HdCameraTokens->windowPolicy);
        if (!vPolicy.IsEmpty())  {
            _windowPolicy = vPolicy.Get<CameraUtilConformWindowPolicy>();
        }
    }

    if (bits & DirtyClipPlanes) {
        // treat clip planes as an optional parameter
        VtValue vClipPlanes = 
            sceneDelegate->GetCameraParamValue(id, HdCameraTokens->clipPlanes);
        if (!vClipPlanes.IsEmpty()) {
            _clipPlanes = vClipPlanes.Get< std::vector<GfVec4d> >();
        }
    }

    // Clear all the dirty bits (i.e., including DirtyParams even though it
    // isn't handled above). This ensures that the sprim doesn't remain in the
    // dirty list always.
    *dirtyBits = Clean;
}

/* virtual */
HdDirtyBits
HdCamera::GetInitialDirtyBitsMask() const
{
    // Don't set DirtyParams. A renderer interested in consuming camera
    // parameters is expected to inherit from HdCamera and override this method
    // and Sync to handle DirtyParams.
    return (DirtyViewMatrix | DirtyProjMatrix | DirtyWindowPolicy |
            DirtyClipPlanes);
}

PXR_NAMESPACE_CLOSE_SCOPE

