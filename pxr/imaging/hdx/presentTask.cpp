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
#include "pxr/imaging/hdx/presentTask.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


HdxPresentTask::HdxPresentTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxTask(id)
    , _interopDst(HgiTokens->OpenGL)
{
}

HdxPresentTask::~HdxPresentTask()
{
}

void
HdxPresentTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxPresentTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _interopDst = params.interopDst;
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxPresentTask::Prepare(HdTaskContext* ctx, HdRenderIndex *renderIndex)
{
}

void
HdxPresentTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The color and depth aovs have the results we want to blit to the
    // application. Depth is optional. When we are previewing a custom aov we
    // may not have a depth buffer.
    if (!_HasTaskContextData(ctx, HdAovTokens->color)) {
        return;
    }

    HgiTextureHandle aovTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);

    HgiTextureHandle depthTexture;
    if (_HasTaskContextData(ctx, HdAovTokens->depth)) {
        _GetTaskContextData(ctx, HdAovTokens->depth, &depthTexture);
    }

    // Use HgiInterop to composite the Hgi textures over the application's
    // framebuffer contents.
    // Eg. This allows us to render with HgiMetal and present the images
    // into a opengl based application (such as usdview).
    _interop.TransferToApp(_hgi, _interopDst, aovTexture, depthTexture);
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv)
{
    out << "PresentTask Params: (...) "
        << pv.interopDst;
    return out;
}

bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return lhs.interopDst == rhs.interopDst;
}

bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
