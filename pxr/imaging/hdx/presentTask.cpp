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
 : HdTask(id)
 , _hgi(nullptr)
 , _compositor()
{
}

HdxPresentTask::~HdxPresentTask()
{
}

void
HdxPresentTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Find Hgi driver in task context.
    if (!_hgi) {
        _hgi = HdTask::_GetDriver<Hgi*>(ctx, HgiTokens->renderDriver);
        if (!TF_VERIFY(_hgi, "Hgi driver missing from TaskContext")) {
            return;
        }
        _compositor.reset(new HdxFullscreenShader(_hgi, "Present"));
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxPresentTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            // PresentTask has currently not params to sync.
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
    // framebuffer. Depth is optional. When we are previewing a custom aov we
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

    // XXX TODO The below GL blit code needs to be replaced by HgiInterop.
    // HgiInterop should take the aov color and depth results, which are 
    // hgi textures of one specific backend (HgiGL, HgiMetal, etc), and blit 
    // those results into the viewer's framebuffer.
    // The viewer's framebuffer may be of a different graphics api, likely
    // opengl. So HgiInterop should do the necessary conversions.
    // For example, it should convert from HgiMetalTexture to HgiGLTexture and
    // then blit those HgiGLTextures to the viewer.

    { // XXX HgiInterop begin
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

        HdxFullscreenShader::TextureMap textures;

        HdxFullscreenShader::HdxFullscreenShaderTex colorShaderTex;
        colorShaderTex.handle = aovTexture;
        textures[TfToken("color")] = colorShaderTex;

        if (depthTexture) {
            HdxFullscreenShader::HdxFullscreenShaderTex depthShaderTex;
            depthShaderTex.handle = depthTexture;
            textures[TfToken("depth")] = depthShaderTex;
        }

        // Draw aov textures to framebuffer
        _compositor->DrawToFramebuffer(textures);

        if (blendEnabled) {
            glEnable(GL_BLEND);
        }

        glDepthFunc(restoreDepthFunc);
        if (!restoreDepthEnabled) {
            glDisable(GL_DEPTH_TEST);
        }
    } // XXX HgiInterop end
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv)
{
    out << "PresentTask Params: (...) ";
    return out;
}

bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    // PresentTask currently has no params to compare
    return true;
}

bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
