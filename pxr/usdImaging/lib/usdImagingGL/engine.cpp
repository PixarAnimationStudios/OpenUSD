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
#include "pxr/imaging/glf/glew.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#include "pxr/usdImaging/usdImagingGL/legacyEngine.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/info.h"

#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/rendererPluginRegistry.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static
bool
_IsHydraEnabled()
{
    // Make sure there is an OpenGL context when 
    // trying to initialize Hydra/Reference
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!context) {
        TF_CODING_ERROR("OpenGL context required, using reference renderer");
        return false;
    }

    if (TfGetenv("HD_ENABLED", "1") != "1") {
        return false;
    }
    
    // Check to see if we have a default plugin for the renderer
    TfToken defaultPlugin = 
        HdxRendererPluginRegistry::GetInstance().GetDefaultPluginId();

    return !defaultPlugin.IsEmpty();
}

} // anonymous namespace

//----------------------------------------------------------------------------
// Global State
//----------------------------------------------------------------------------

/*static*/
bool
UsdImagingGLEngine::IsHydraEnabled()
{
    GlfGlewInit();

    static bool isHydraEnabled = _IsHydraEnabled();
    return isHydraEnabled;
}

//----------------------------------------------------------------------------
// Construction
//----------------------------------------------------------------------------

UsdImagingGLEngine::UsdImagingGLEngine()
{
    SdfPathVector excluded, invised;
    if (IsHydraEnabled()) {
        _hdImpl.reset(new UsdImagingGLHdEngine(
            SdfPath::AbsoluteRootPath(), excluded, invised));
    } else {
        _legacyImpl.reset(new UsdImagingGLLegacyEngine(excluded));
    }
}

UsdImagingGLEngine::UsdImagingGLEngine(
    const SdfPath& rootPath,
    const SdfPathVector& excludedPaths,
    const SdfPathVector& invisedPaths,
    const SdfPath& delegateID)
{
    if (IsHydraEnabled()) {
        _hdImpl.reset(new UsdImagingGLHdEngine(rootPath, excludedPaths,
                                                 invisedPaths, delegateID));
    } else {
        // In the legacy engine, both excluded paths and invised paths are 
        // treated the same way.
        SdfPathVector pathsToExclude = excludedPaths;
        pathsToExclude.insert(pathsToExclude.end(), 
            invisedPaths.begin(), invisedPaths.end());
        _legacyImpl.reset(new UsdImagingGLLegacyEngine(pathsToExclude));
    }
}

UsdImagingGLEngine::~UsdImagingGLEngine()
{ 
}

//----------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::PrepareBatch(
    const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->PrepareBatch(root, params);
}

void
UsdImagingGLEngine::RenderBatch(
    const SdfPathVector& paths, 
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->RenderBatch(paths, params);
}

void 
UsdImagingGLEngine::Render(
    const UsdPrim& root, 
    const UsdImagingGLRenderParams &params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return _legacyImpl->Render(root, params);
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->Render(root, params);
}

void
UsdImagingGLEngine::InvalidateBuffers()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return _legacyImpl->InvalidateBuffers();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->InvalidateBuffers();
}

bool
UsdImagingGLEngine::IsConverged() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return true;
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->IsConverged();
}

//----------------------------------------------------------------------------
// Root and Transform Visibility
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetRootTransform(xf);
}

void
UsdImagingGLEngine::SetRootVisibility(bool isVisible)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetRootVisibility(isVisible);
}

//----------------------------------------------------------------------------
// Camera and Light State
//----------------------------------------------------------------------------

void 
UsdImagingGLEngine::SetCameraState(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const GfVec4d& viewport)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetCameraState(viewMatrix, projectionMatrix, viewport);
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetCameraState(viewMatrix, projectionMatrix, viewport);
}

void
UsdImagingGLEngine::SetCameraStateFromOpenGL()
{
    GfMatrix4d viewMatrix, projectionMatrix;
    GfVec4d viewport;
    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix.GetArray());
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix.GetArray());
    glGetDoublev(GL_VIEWPORT, &viewport[0]);

    SetCameraState(viewMatrix, projectionMatrix, viewport);
}

void
UsdImagingGLEngine::SetLightingStateFromOpenGL()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetLightingStateFromOpenGL();
}

void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetLightingState(src);
}

void
UsdImagingGLEngine::SetLightingState(
    GlfSimpleLightVector const &lights,
    GlfSimpleMaterial const &material,
    GfVec4f const &sceneAmbient)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetLightingState(lights, material, sceneAmbient);
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetLightingState(lights, material, sceneAmbient);
}

//----------------------------------------------------------------------------
// Selection Highlighting
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetSelected(paths);
}

void
UsdImagingGLEngine::ClearSelected()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->ClearSelected();
}

void
UsdImagingGLEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->AddSelected(path, instanceIndex);
}

void
UsdImagingGLEngine::SetSelectionColor(GfVec4f const& color)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetSelectionColor(color);
}

//----------------------------------------------------------------------------
// Picking
//----------------------------------------------------------------------------

bool 
UsdImagingGLEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outInstancerPath,
    int *outHitInstanceIndex,
    int *outHitElementIndex)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return _legacyImpl->TestIntersection(
            viewMatrix,
            projectionMatrix,
            worldToLocalSpace,
            root,
            params,
            outHitPoint,
            outHitPrimPath,
            outInstancerPath,
            outHitInstanceIndex,
            outHitElementIndex);
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->TestIntersection(
        viewMatrix,
        projectionMatrix,
        worldToLocalSpace,
        root,
        params,
        outHitPoint,
        outHitPrimPath,
        outInstancerPath,
        outHitInstanceIndex,
        outHitElementIndex);
}

SdfPath
UsdImagingGLEngine::GetRprimPathFromPrimId(int primId) const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return _legacyImpl->GetRprimPathFromPrimId(primId);
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRprimPathFromPrimId(primId);
}

SdfPath
UsdImagingGLEngine::GetPrimPathFromPrimIdColor(
    GfVec4i const &primIdColor,
    GfVec4i const &instanceIdColor,
    int * instanceIndexOut)
{
    unsigned char primIdColorBytes[] =  {
        uint8_t(primIdColor[0]),
        uint8_t(primIdColor[1]),
        uint8_t(primIdColor[2]),
        uint8_t(primIdColor[3])
    };

    int primId = HdxIntersector::DecodeIDRenderColor(primIdColorBytes);
    SdfPath result = GetRprimPathFromPrimId(primId);
    if (!result.IsEmpty()) {
        if (instanceIndexOut) {
            unsigned char instanceIdColorBytes[] =  {
                uint8_t(instanceIdColor[0]),
                uint8_t(instanceIdColor[1]),
                uint8_t(instanceIdColor[2]),
                uint8_t(instanceIdColor[3])
            };
            *instanceIndexOut = HdxIntersector::DecodeIDRenderColor(
                    instanceIdColorBytes);
        }
    }
    return result;
}

SdfPath 
UsdImagingGLEngine::GetPrimPathFromInstanceIndex(
    SdfPath const& protoPrimPath,
    int instanceIndex,
    int *absoluteInstanceIndex,
    SdfPath *rprimPath,
    SdfPathVector *instanceContext)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return SdfPath();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetPrimPathFromInstanceIndex(
        protoPrimPath,
        instanceIndex,
        absoluteInstanceIndex,
        rprimPath,
        instanceContext);
}

//----------------------------------------------------------------------------
// Renderer Plugin Management
//----------------------------------------------------------------------------

TfTokenVector
UsdImagingGLEngine::GetRendererPlugins() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return std::vector<TfToken>();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRendererPlugins();
}

std::string
UsdImagingGLEngine::GetRendererDisplayName(TfToken const &id) const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return std::string();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRendererDisplayName(id);
}

TfToken
UsdImagingGLEngine::GetCurrentRendererId() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return TfToken();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetCurrentRendererId();
}

bool
UsdImagingGLEngine::SetRendererPlugin(TfToken const &id)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->SetRendererPlugin(id);
}

//----------------------------------------------------------------------------
// AOVs and Renderer Settings
//----------------------------------------------------------------------------

TfTokenVector
UsdImagingGLEngine::GetRendererAovs() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return std::vector<TfToken>();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRendererAovs();
}

bool
UsdImagingGLEngine::SetRendererAov(TfToken const &id)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->SetRendererAov(id);
}

UsdImagingGLRendererSettingsList
UsdImagingGLEngine::GetRendererSettingsList() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return UsdImagingGLRendererSettingsList();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRendererSettingsList();
}

VtValue
UsdImagingGLEngine::GetRendererSetting(TfToken const& id) const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return VtValue();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRendererSetting(id);
}

void
UsdImagingGLEngine::SetRendererSetting(TfToken const& id, VtValue const& value)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->SetRendererSetting(id, value);
}

//----------------------------------------------------------------------------
// Resource Information
//----------------------------------------------------------------------------

VtDictionary
UsdImagingGLEngine::GetResourceAllocation() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return VtDictionary();
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetResourceAllocation();
}

//----------------------------------------------------------------------------
// Private/Protected
//----------------------------------------------------------------------------

HdRenderIndex *
UsdImagingGLEngine::_GetRenderIndex() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return nullptr;
    }

    TF_VERIFY(_hdImpl);
    return _hdImpl->GetRenderIndex();
}

void 
UsdImagingGLEngine::_Render(const UsdImagingGLRenderParams &params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_hdImpl);
    _hdImpl->Render(params);
}

PXR_NAMESPACE_CLOSE_SCOPE

