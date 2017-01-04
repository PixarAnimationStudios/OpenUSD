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
    if (!TF_VERIFY(delegate)) {
        return;
    }

    // HdxCamera communicates to the scene graph and caches all interesting
    // values within this class.
    // Later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.
    HdChangeTracker& changeTracker = 
                                delegate->GetRenderIndex().GetChangeTracker();
    HdChangeTracker::DirtyBits bits = changeTracker.GetSprimDirtyBits(id);
    
    if (bits & DirtyMatrices) {
        GfMatrix4d worldToViewMatrix(1.0);
        GfMatrix4d worldToViewInverseMatrix(1.0);
        GfMatrix4d projectionMatrix(1.0);

        // extract view/projection matrices
        VtValue vMatrices = delegate->Get(id, HdxCameraTokens->matrices);
        if (not vMatrices.IsEmpty()) {
            const HdxCameraMatrices matrices = 
                vMatrices.Get<HdxCameraMatrices>();
            worldToViewMatrix                = matrices.viewMatrix;
            worldToViewInverseMatrix         = worldToViewMatrix.GetInverse();
            projectionMatrix                 = matrices.projMatrix;
        } else {
            TF_CODING_ERROR("No camera matrices passed to HdxCamera.");
        }

        _cameraValues[HdxCameraTokens->worldToViewMatrix] =
            VtValue(worldToViewMatrix);
        _cameraValues[HdxCameraTokens->worldToViewInverseMatrix] =
            VtValue(worldToViewInverseMatrix);
        _cameraValues[HdxCameraTokens->projectionMatrix] =
            VtValue(projectionMatrix);
    }

    if (bits & DirtyWindowPolicy) {
        _cameraValues[HdxCameraTokens->windowPolicy] = 
            delegate->Get(id, HdxCameraTokens->windowPolicy);
    }

    if (bits & DirtyClipPlanes) {
        _cameraValues[HdxCameraTokens->clipPlanes] = 
            delegate->GetClipPlanes(id);
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

/* virtual */
int
HdxCamera::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxCameraMatrices& pv)
{
    out << "HdxCameraMatrices Params: (...) " 
        << pv.viewMatrix << " " 
        << pv.projMatrix
        ;
    return out;
}

bool operator==(const HdxCameraMatrices& lhs, const HdxCameraMatrices& rhs) 
{
    return lhs.viewMatrix           == rhs.viewMatrix &&
           lhs.projMatrix           == rhs.projMatrix;
}

bool operator!=(const HdxCameraMatrices& lhs, const HdxCameraMatrices& rhs) 
{
    return !(lhs == rhs);
}
