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
    
    if (bits & DirtyMatrices) {
        GfMatrix4d worldToViewMatrix(1.0);
        GfMatrix4d worldToViewInverseMatrix(1.0);
        GfMatrix4d projectionMatrix(1.0);

        // extract view/projection matrices
        VtValue vMatrices = sceneDelegate->Get(id, HdStCameraTokens->matrices);
        if (!vMatrices.IsEmpty()) {
            const HdStCameraMatrices matrices =
                vMatrices.Get<HdStCameraMatrices>();
            worldToViewMatrix                = matrices.viewMatrix;
            worldToViewInverseMatrix         = worldToViewMatrix.GetInverse();
            projectionMatrix                 = matrices.projMatrix;
        } else {
            TF_CODING_ERROR("No camera matrices passed to HdStCamera.");
        }

        _cameraValues[HdStCameraTokens->worldToViewMatrix] =
            VtValue(worldToViewMatrix);
        _cameraValues[HdStCameraTokens->worldToViewInverseMatrix] =
            VtValue(worldToViewInverseMatrix);
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

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdStCameraMatrices& pv)
{
    out << "HdStCameraMatrices Params: (...) "
        << pv.viewMatrix << " " 
        << pv.projMatrix
        ;
    return out;
}

bool operator==(const HdStCameraMatrices& lhs, const HdStCameraMatrices& rhs)
{
    return lhs.viewMatrix           == rhs.viewMatrix &&
           lhs.projMatrix           == rhs.projMatrix;
}

bool operator!=(const HdStCameraMatrices& lhs, const HdStCameraMatrices& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

