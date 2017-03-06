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
#include "pxr/imaging/hdx/simpleLightBypassTask.h"

#include "pxr/imaging/hdx/camera.h"
#include "pxr/imaging/hdx/simpleLightingShader.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //

HdxSimpleLightBypassTask::HdxSimpleLightBypassTask(HdSceneDelegate* delegate,
                                                   SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _lightingShader(new HdxSimpleLightingShader())
    , _simpleLightingContext()
{
}

void
HdxSimpleLightBypassTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
}

void
HdxSimpleLightBypassTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();

    HdDirtyBits bits = _GetTaskDirtyBits();

    if (bits & HdChangeTracker::DirtyParams) {
        HdxSimpleLightBypassTaskParams params;
        if (!_GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        _simpleLightingContext = params.simpleLightingContext;
        const HdRenderIndex &renderIndex = GetDelegate()->GetRenderIndex();
        _camera = static_cast<const HdxCamera *>(
                    renderIndex.GetSprim(HdPrimTypeTokens->camera,
                                         params.cameraPath));
    }

    if (_simpleLightingContext) {
        if (!TF_VERIFY(_camera)) {
            return;
        }

        VtValue modelViewMatrix = _camera->Get(HdShaderTokens->worldToViewMatrix);
        if (!TF_VERIFY(modelViewMatrix.IsHolding<GfMatrix4d>())) return;
        VtValue projectionMatrix = _camera->Get(HdShaderTokens->projectionMatrix);
        if (!TF_VERIFY(projectionMatrix.IsHolding<GfMatrix4d>())) return;

        // need camera matrices to compute lighting paramters in the eye-space.
        //
        // you should be a bit careful here...
        //
        // _simpleLightingContext->SetCamera() is useless, since
        // _lightingShader->SetLightingState() actually only copies the lighting
        // paramters, not the camera matrices.
        // _lightingShader->SetCamera() is the right one.
        //
        _lightingShader->SetLightingState(_simpleLightingContext);
        _lightingShader->SetCamera(modelViewMatrix.Get<GfMatrix4d>(),
                                   projectionMatrix.Get<GfMatrix4d>());
    }

    // Done at end, because the lighting context can be changed above.
    // Also we want the context in the shader as it's only a partial copy
    // of the context we own.
    (*ctx)[HdxTokens->lightingShader]  = boost::dynamic_pointer_cast<HdLightingShader>(_lightingShader);
    (*ctx)[HdxTokens->lightingContext] = _lightingShader->GetLightingContext();

}

// -------------------------------------------------------------------------- //
// VtValue requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out,
                         const HdxSimpleLightBypassTaskParams& pv)
{
    return out;
}

bool operator==(const HdxSimpleLightBypassTaskParams& lhs,
                const HdxSimpleLightBypassTaskParams& rhs) {
    return lhs.cameraPath == rhs.cameraPath 
        && lhs.simpleLightingContext == rhs.simpleLightingContext;
}

bool operator!=(const HdxSimpleLightBypassTaskParams& lhs,
                const HdxSimpleLightBypassTaskParams& rhs) {
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

