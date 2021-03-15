//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HDX_FREE_CAMERA_SCENE_DELEGATE_H
#define PXR_IMAGING_HDX_FREE_CAMERA_SCENE_DELEGATE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/camera.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxFreeCameraSceneDelegate
///
/// A simple scene delegate adding a camera prim to the given
/// render index.
///
class HdxFreeCameraSceneDelegate : public HdSceneDelegate
{
public:
    /// Constructs delegate and adds camera to render index if
    /// cameras are supported by render delegate.
    HDX_API
    HdxFreeCameraSceneDelegate(HdRenderIndex *renderIndex,
                               SdfPath const &delegateId);
    
    HDX_API
    ~HdxFreeCameraSceneDelegate() override;

    /// Path of added camera (in render index). Empty if cameras are not
    /// supported by render delegate.
    const SdfPath &GetCameraId() const {
        return _cameraId;
    }

    /// Set state of camera from GfCamera.
    HDX_API
    void SetCamera(const GfCamera &camera);
    /// Set window policy of camera.
    HDX_API
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy);
    
    /// For transition, set camera state from OpenGL-style
    /// view and projection matrix. GfCamera is preferred.
    HDX_API
    void SetMatrices(GfMatrix4d const &viewMatrix,
                     GfMatrix4d const &projMatrix);

    /// For transition, set clip planes for camera. GfCamera is preferred.
    HDX_API
    void SetClipPlanes(std::vector<GfVec4f> const &clipPlanes);

    // HdSceneDelegate interface
    HDX_API
    GfMatrix4d GetTransform(SdfPath const &id) override;
    HDX_API
    VtValue GetCameraParamValue(SdfPath const &id, 
                                TfToken const &key) override;

private:
    // Mark camera dirty in render index.
    void _MarkDirty(HdDirtyBits bits);

    // Path of camera in render index.
    const SdfPath _cameraId;

    // State of camera
    GfCamera _camera;
    // Window policy of camera.
    CameraUtilConformWindowPolicy _policy;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_FREE_CAMERA_SCENE_DELEGATE_H
