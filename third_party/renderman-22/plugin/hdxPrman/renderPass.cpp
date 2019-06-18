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
#include "pxr/imaging/glf/glew.h"
#include "hdxPrman/renderPass.h"
#include "hdxPrman/context.h"
#include "hdxPrman/renderBuffer.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/rixStrings.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/rotation.h"

#include "Riley.h"
#include "RixParamList.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdxPrman_RenderPass::HdxPrman_RenderPass(HdRenderIndex *index,
                                     HdRprimCollection const &collection,
                                     std::shared_ptr<HdPrman_Context> context)
    : HdRenderPass(index, collection)
    , _width(0)
    , _height(0)
    , _converged(false)
    , _context(context)
    , _lastRenderedVersion(0)
    , _lastSettingsVersion(0)
{
    // Check if this is an interactive context.
    _interactiveContext =
        std::dynamic_pointer_cast<HdxPrman_InteractiveContext>(context);
}

HdxPrman_RenderPass::~HdxPrman_RenderPass()
{
}

bool
HdxPrman_RenderPass::IsConverged() const
{
    if (!_interactiveContext) {
        return true;
    }
    return _converged;
}

void
HdxPrman_RenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                            TfTokenVector const &renderTags)
{
    static const RtUString us_PxrPerspective("PxrPerspective");
    static const RtUString us_PxrOrthographic("PxrOrthographic");
    static const RtUString us_PathTracer("PathTracer");

    if (!_interactiveContext) {
        // If this is not an interactive context, don't use Hydra to drive
        // rendering and presentation of the framebuffer.  Instead, assume
        // we are just using Hydra to sync the scene contents to Riley.
        return;
    }

    RixRileyManager *mgr = _interactiveContext->mgr;
    riley::Riley *riley = _interactiveContext->riley;

    bool needStartRender = false;
    int currentSceneVersion = _interactiveContext->sceneVersion.load();
    if (currentSceneVersion != _lastRenderedVersion) {
        needStartRender = true;
        _lastRenderedVersion = currentSceneVersion;
    }

    // Enable/disable the fallback light when the scene provides no lights.
    _interactiveContext->SetFallbackLightsEnabled(
        _interactiveContext->sceneLightCount == 0);

    // Check if any camera update needed
    // TODO: This should be part of a Camera sprim; then we wouldn't
    // need to sync anything here.  Note that we'll need to solve
    // thread coordination for sprim sync/finalize first.
    GfMatrix4d proj = renderPassState->GetProjectionMatrix();
    GfMatrix4d viewToWorldMatrix =
        renderPassState->GetWorldToViewMatrix().GetInverse();
    GfVec4f vp = renderPassState->GetViewport();
    if (proj != _lastProj || viewToWorldMatrix != _lastViewToWorldMatrix ||
        _width != vp[2] || _height != vp[3]) {

        _width = vp[2];
        _height = vp[3];
        _lastProj = proj;
        _lastViewToWorldMatrix = viewToWorldMatrix;

        _interactiveContext->StopRender();

        RixParamList *camParams = mgr->CreateRixParamList();
        RixParamList *projParams = mgr->CreateRixParamList();

        // XXX: Renderman doesn't support resizing the viewport, so instead
        // we allocate a large image buffer (in context.cpp) and modify the
        // crop window here so we don't do extra work for small viewports.
        // We additionally need to alter the camera and the blit to
        // compensate for the size of the crop window vs the full image.
        RixParamList *options = mgr->CreateRixParamList();

        // Center the CropWindow on the center of the image.
        float fracWidth = _width / float(_interactiveContext->framebuffer.w);
        float fracHeight = _height / float(_interactiveContext->framebuffer.h);
        // Clamp fracWidth and fracHeight to (0,1) or we'll clip things weirdly.
        // If fracWidth or fracHeight are greater than 1, it means the viewport
        // size exceeds image dimensions, and we'll be stretching the image
        // to cover the viewport in that dimension.
        fracWidth = std::max(0.0f, std::min(1.0f, fracWidth));
        fracHeight = std::max(0.0f, std::min(1.0f, fracHeight));
        float cropWindow[4] = {
            0.5f - fracWidth * 0.5f, 0.5f + fracWidth * 0.5f,
            0.5f - fracHeight * 0.5f, 0.5f + fracHeight * 0.5f };
        options->SetFloatArray(RixStr.k_Ri_CropWindow, cropWindow, 4);
        _interactiveContext->riley->SetOptions(*options);

        mgr->DestroyRixParamList(options);

        // Coordinate system notes.
        //
        // # Hydra & USD are right-handed
        // - Camera space is always Y-up, looking along -Z.
        // - World space may be either Y-up or Z-up, based on stage metadata.
        // - Individual prims may be marked to be left-handed, which
        //   does not affect spatial coordinates, it only flips the
        //   winding order of polygons.
        //
        // # Prman is left-handed
        // - World is Y-up
        // - Camera looks along +Z.

        // Check if camera has switched between perspective or orthographic
        riley::ShadingNode* cameraNode = &_interactiveContext->cameraNode;
        bool isPerspective = cameraNode->name == us_PxrPerspective;
        bool wantsOrtho = round(proj[3][3]) == 1 && proj != GfMatrix4d(1);

        if (wantsOrtho && isPerspective) {
            cameraNode->name = us_PxrOrthographic;
        } else if (!wantsOrtho && !isPerspective) {
            cameraNode->name = us_PxrPerspective;
        }

        // XXX Normally we would update RenderMan option 'ScreenWindow' to
        // account for an orthographic camera,
        //     options->SetFloatArray(RixStr.k_Ri_ScreenWindow, window, 4);
        // But we cannot update this option in Renderman once it is running.
        // We apply the orthographic-width to the viewMatrix scale instead.
        // Inverse computation of GfFrustum::ComputeProjectionMatrix()
        if (wantsOrtho) {
            double left   = -(1 + proj[3][0]) / proj[0][0];
            double right  =  (1 - proj[3][0]) / proj[0][0];
            double bottom = -(1 - proj[3][1]) / proj[1][1];
            double top    =  (1 + proj[3][1]) / proj[1][1];
            double w = (right-left)/(2 * fracWidth);
            double h = (top-bottom)/(2 * fracHeight);
            GfMatrix4d scaleMatrix;
            scaleMatrix.SetScale(GfVec3d(w,h,1));
            viewToWorldMatrix = scaleMatrix * viewToWorldMatrix;
        } else {
            // Extract vertical FOV from hydra projection matrix.
            const float fov_rad = atan(1.0f / (fracHeight * proj[1][1]))*2.0;
            const float fov_deg = fov_rad / M_PI * 180.0;

            projParams->SetFloat(RixStr.k_fov, fov_deg);
            _interactiveContext->cameraNode.params = projParams;

            // Aspect ratio correction: modify the camera so the image aspect
            // ratio matches the viewport (the image dimensions here being the
            // crop dimensions).
            const float fbAspect =
                (fracWidth * _interactiveContext->framebuffer.w) /
                (fracHeight * _interactiveContext->framebuffer.h);
            const float vpAspect = _width / float(_height);
            GfMatrix4d aspectCorrection(1.0);
            aspectCorrection[0][0] = vpAspect / fbAspect;
            viewToWorldMatrix = aspectCorrection * viewToWorldMatrix;
        }

        // Riley camera xform is "move the camera", aka viewToWorld.
        // Convert left-handed Y-up camera space (USD, Hydra) to
        // right-handed Y-up (Prman) coordinates.  This just amounts to
        // flipping the Z axis.
        GfMatrix4d flipZ(1.0);
        flipZ[2][2] = -1.0;
        viewToWorldMatrix = flipZ * viewToWorldMatrix;

        // Convert from Gf to Rt.
        RtMatrix4x4 matrix = HdPrman_GfMatrixToRtMatrix(viewToWorldMatrix);

        // Commit new camera.
        float const zerotime = 0.0f;
        riley::Transform xform = { 1, &matrix, &zerotime };
        riley->ModifyCamera(_interactiveContext->cameraId, cameraNode,
                            &xform, nullptr);
        mgr->DestroyRixParamList(camParams);
        mgr->DestroyRixParamList(projParams);

        // Update the framebuffer Z scaling
        _interactiveContext->framebuffer.proj = proj;

        needStartRender = true;
    }

    // Likewise the render settings.
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion) {
        const std::string PxrPathTracer("PxrPathTracer");
        const std::string integrator = renderDelegate->GetRenderSetting(
            HdPrmanRenderSettingsTokens->integrator,
            PxrPathTracer);
        RixParamList *params = mgr->CreateRixParamList();
        riley::ShadingNode integratorNode {
            riley::ShadingNode::k_Integrator,
            RtUString(integrator.c_str()),
            us_PathTracer,
            params
        };

        _interactiveContext->StopRender();
        riley->CreateIntegrator(integratorNode);
        mgr->DestroyRixParamList(params);

        _lastSettingsVersion = currentSettingsVersion;
        needStartRender = true;
    }

    // Start (or restart) concurrent rendering.
    if (needStartRender) {
        _interactiveContext->renderThread.StartRender();
    }

    _converged = !_interactiveContext->renderThread.IsRendering();

    HdRenderPassAovBindingVector aovBindings =
        renderPassState->GetAovBindings();

    // Determine the blit sub-region.  We only want to blit out of the region
    // specified by the crop window.
    float fracWidth = _width /
        float(_interactiveContext->framebuffer.w);
    float fracHeight = _height /
        float(_interactiveContext->framebuffer.h);
    fracWidth = std::max(0.0f, std::min(1.0f, fracWidth));
    fracHeight = std::max(0.0f, std::min(1.0f, fracHeight));
    int width = _interactiveContext->framebuffer.w * fracWidth;
    int height = _interactiveContext->framebuffer.h * fracHeight;
    int skipPixels = (0.5f - 0.5f * fracWidth) *
        _interactiveContext->framebuffer.w;
    int skipRows = (0.5f - 0.5f * fracHeight) *
        _interactiveContext->framebuffer.h;
    int offset = skipPixels + skipRows * _interactiveContext->framebuffer.w;
    int stride = _interactiveContext->framebuffer.w;

    if (aovBindings.size() == 0) {
        // No AOV bindings means blit current framebuffer contents.
        // We don't bother to synchronize -- but we could, if presenting
        // partial updates becomes objectionable.

        // Adjust GL blending for compositing.
        //
        // As configured, the framebuffer coming from Renderman will be
        // effectively premultiplied.  The transition from a foreground
        // object that is blurred (ex: due to motion or lens defocus)
        // to the background will have alpha go from 1..0 and color
        // channels likewise to go zero.  To composite this correctly
        // we want to use GL_ONE for the foreground element.
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, _interactiveContext->framebuffer.w);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        glPixelStorei(GL_UNPACK_SKIP_ROWS,  skipRows);
        _compositor.UpdateColor(_interactiveContext->framebuffer.w * fracWidth,
                                _interactiveContext->framebuffer.h * fracHeight,
                                &_interactiveContext->framebuffer.color[0]);
        _compositor.UpdateDepth(_interactiveContext->framebuffer.w * fracWidth,
                                _interactiveContext->framebuffer.h * fracHeight,
                                reinterpret_cast<uint8_t*>(
                                   &_interactiveContext->framebuffer.depth[0]));
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        // Blit
        _compositor.Draw();

        glBlendFunc(GL_ONE, GL_ZERO);
        glDisable(GL_BLEND);
    } else {

        // Blit from the framebuffer to the currently selected AOVs.
        for (size_t aov = 0; aov < aovBindings.size(); ++aov) {
            if(!TF_VERIFY(aovBindings[aov].renderBuffer)) {
                continue;
            }
            HdxPrmanRenderBuffer *rb = static_cast<HdxPrmanRenderBuffer*>(
                aovBindings[aov].renderBuffer);

            // Forward convergence state to the render buffers...
            rb->SetConverged(_converged);

            if (aovBindings[aov].aovName == HdAovTokens->color) {
                rb->Blit(HdFormatUNorm8Vec4, width, height, offset, stride,
                    reinterpret_cast<uint8_t*>(
                        &_interactiveContext->framebuffer.color[0]));
            } else if (aovBindings[aov].aovName == HdAovTokens->depth) {
                rb->Blit(HdFormatFloat32, width, height, offset, stride,
                    reinterpret_cast<uint8_t*>(
                        &_interactiveContext->framebuffer.depth[0]));
            } else if (aovBindings[aov].aovName == HdAovTokens->primId) {
                rb->Blit(HdFormatInt32, width, height, offset, stride,
                    reinterpret_cast<uint8_t*>(
                        &_interactiveContext->framebuffer.primId[0]));
            } else if (aovBindings[aov].aovName == HdAovTokens->instanceId) {
                rb->Blit(HdFormatInt32, width, height, offset, stride,
                    reinterpret_cast<uint8_t*>(
                        &_interactiveContext->framebuffer.instanceId[0]));
            } else if (aovBindings[aov].aovName == HdAovTokens->elementId) {
                rb->Blit(HdFormatInt32, width, height, offset, stride,
                    reinterpret_cast<uint8_t*>(
                        &_interactiveContext->framebuffer.elementId[0]));
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
