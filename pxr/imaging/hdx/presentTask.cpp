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
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxPresentTask::HdxPresentTask(HdSceneDelegate* delegate, SdfPath const& id)
 : HdTask(id)
 , _aovBufferPath()
 , _depthBufferPath()
 , _aovBuffer(nullptr)
 , _depthBuffer(nullptr)
 , _compositor()
{
}

HdxPresentTask::~HdxPresentTask()
{
}

void
HdxPresentTask::Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxPresentTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _aovBufferPath = params.aovBufferPath;
            _depthBufferPath = params.depthBufferPath;
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxPresentTask::Prepare(HdTaskContext* ctx, HdRenderIndex *renderIndex)
{
    _aovBuffer = nullptr;
    _depthBuffer = nullptr;

    // An empty _aovBufferPath disables the task
    if (!_aovBufferPath.IsEmpty()) {
        _aovBuffer = static_cast<HdRenderBuffer*>(
            renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer, _aovBufferPath));
    }

    if (!_depthBufferPath.IsEmpty()) {
        _depthBuffer = static_cast<HdRenderBuffer*>(
            renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer, _depthBufferPath));
    }
}

void
HdxPresentTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    const bool mulSmp = false;

    HgiGLTexture* colorTex = _aovBuffer ? 
        static_cast<HgiGLTexture*>(_aovBuffer->GetHgiTextureHandle(mulSmp)) :
        nullptr;

    HgiGLTexture* depthTex = _depthBuffer ? 
        static_cast<HgiGLTexture*>(_depthBuffer->GetHgiTextureHandle(mulSmp)):
        nullptr;

    uint32_t colorId = colorTex ? colorTex->GetTextureId() : 0;
    uint32_t depthId = depthTex ? depthTex->GetTextureId() : 0;

    if (colorId == 0 && depthId == 0) {
        return;
    }

    // Depth test must be ALWAYS instead of disabling the depth_test because
    // we want to transfer the depth pixels. Disabling depth_test 
    // disables depth writes and we need to copy depth to screen FB.
    GLboolean restoreDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    GLint restoreDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &restoreDepthFunc);
    glDepthFunc(GL_ALWAYS);

    // Any alpha blending the client wanted should have happened into the AOV. 
    // When copying back to client buffer disable blending.
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glDisable(GL_BLEND);

    _compositor.Draw(colorId, depthId);

    if (blendEnabled) {
        glEnable(GL_BLEND);
    }

    glDepthFunc(restoreDepthFunc);
    if (!restoreDepthEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv)
{
    out << "PresentTask Params: (...) "
        << pv.aovBufferPath << " "
        << pv.depthBufferPath;
    return out;
}

bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return lhs.aovBufferPath   == rhs.aovBufferPath    &&
           lhs.depthBufferPath == rhs.depthBufferPath;
}

bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
