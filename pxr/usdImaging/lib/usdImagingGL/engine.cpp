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

UsdImagingGLEngine::~UsdImagingGLEngine()
{ 
    /*nothing*/ 
}

//----------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------

/* virtual */
void
UsdImagingGLEngine::PrepareBatch(const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::RenderBatch(const SdfPathVector& paths, 
    const UsdImagingGLRenderParams& params)
{
    // By default, do nothing.
}

/* virtual */
bool
UsdImagingGLEngine::IsConverged() const
{
    // always converges by default.
    return true;
}


//----------------------------------------------------------------------------
// Root and Transform Visibility
//----------------------------------------------------------------------------

/* virtual */
void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetRootVisibility(bool isVisible)
{
    // By default, do nothing.
}

//----------------------------------------------------------------------------
// Camera and Light State
//----------------------------------------------------------------------------

/*virtual*/
void 
UsdImagingGLEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport)
{
    // By default, do nothing.
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

/* virtual */
void
UsdImagingGLEngine::SetLightingStateFromOpenGL()
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightVector const &lights,
                                     GlfSimpleMaterial const &material,
                                     GfVec4f const &sceneAmbient)
{
    // By default, do nothing.
}

//----------------------------------------------------------------------------
// Selection Highlighting
//----------------------------------------------------------------------------

/* virtual */
void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::ClearSelected()
{
    // By default, do nothing.
}

/* virtual */
void
UsdImagingGLEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    // By default, do nothing.
}

/*virtual*/
void
UsdImagingGLEngine::SetSelectionColor(GfVec4f const& color)
{
    // By default, do nothing.
}

//----------------------------------------------------------------------------
// Picking
//----------------------------------------------------------------------------

/* virtual */
SdfPath
UsdImagingGLEngine::GetRprimPathFromPrimId(int primId) const
{
    return SdfPath();
}

/* virtual */
SdfPath
UsdImagingGLEngine::GetPrimPathFromPrimIdColor(GfVec4i const &primIdColor,
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

/* virtual */
SdfPath 
UsdImagingGLEngine::GetPrimPathFromInstanceIndex(
    SdfPath const& protoPrimPath,
    int instanceIndex,
    int *absoluteInstanceIndex,
    SdfPath *rprimPath,
    SdfPathVector *instanceContext)
{
    return SdfPath();
}

//----------------------------------------------------------------------------
// Renderer Plugin Management
//----------------------------------------------------------------------------

/* virtual */
TfTokenVector
UsdImagingGLEngine::GetRendererPlugins() const
{
    return std::vector<TfToken>();
}

/* virtual */
std::string
UsdImagingGLEngine::GetRendererDisplayName(TfToken const &id) const
{
    return std::string();
}

/* virtual */
TfToken
UsdImagingGLEngine::GetCurrentRendererId() const
{
    return TfToken();
}

/* virtual */
bool
UsdImagingGLEngine::SetRendererPlugin(TfToken const &id)
{
    return false;
}

//----------------------------------------------------------------------------
// AOVs and Renderer Settings
//----------------------------------------------------------------------------

/* virtual */
TfTokenVector
UsdImagingGLEngine::GetRendererAovs() const
{
    return std::vector<TfToken>();
}

/* virtual */
bool
UsdImagingGLEngine::SetRendererAov(TfToken const &id)
{
    return false;
}

/* virtual */
UsdImagingGLRendererSettingsList
UsdImagingGLEngine::GetRendererSettingsList() const
{
    return UsdImagingGLRendererSettingsList();
}

/* virtual */
VtValue
UsdImagingGLEngine::GetRendererSetting(TfToken const& id) const
{
    return VtValue();
}

/* virtual */
void
UsdImagingGLEngine::SetRendererSetting(TfToken const& id,
                                       VtValue const& value)
{
}

//----------------------------------------------------------------------------
// Resource Information
//----------------------------------------------------------------------------

/* virtual */
VtDictionary
UsdImagingGLEngine::GetResourceAllocation() const
{
    return VtDictionary();
}

//----------------------------------------------------------------------------
// Private/Protected
//----------------------------------------------------------------------------

/* virtual */
HdRenderIndex *
UsdImagingGLEngine::_GetRenderIndex() const
{
    return nullptr;
}

/* virtual */
void 
UsdImagingGLEngine::_Render(const UsdImagingGLRenderParams &params)
{
    // nothing here
}

PXR_NAMESPACE_CLOSE_SCOPE

