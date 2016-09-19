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

#include "pxr/usdImaging/usdImaging/gl.h"
#include "pxr/usdImaging/usdImaging/hdEngine.h"
#include "pxr/usdImaging/usdImaging/refEngine.h"

#include "pxr/imaging/hd/renderContextCaps.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/textureRegistry.h"

#include <boost/foreach.hpp>

namespace {

static
bool
_IsEnabledHydra()
{
    // Make sure there is an OpenGL context when 
    // trying to initialize Hydra/Reference
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!context) {
        TF_CODING_ERROR("OpenGL context required, using reference renderer");
        return false;
    }

    return HdRenderContextCaps::GetInstance().SupportsHydra()
        && (TfGetenv("HD_ENABLED", "1") == "1");
}

}

/*static*/
bool
UsdImagingGL::IsEnabledHydra()
{
    GlfGlewInit();

    static bool isEnabledHydra = _IsEnabledHydra();
    return isEnabledHydra;
}

static
UsdImagingEngine* _InitEngine(const SdfPath& rootPath,
                              const SdfPathVector& excludedPaths,
                              const SdfPathVector& invisedPaths,
                              const SdfPath& sharedId =
                                        SdfPath::AbsoluteRootPath(),
                              const UsdImagingEngineSharedPtr& sharedEngine =
                                        UsdImagingEngineSharedPtr())
{
    if (UsdImagingGL::IsEnabledHydra()) {
        return new UsdImagingHdEngine(rootPath, excludedPaths, invisedPaths, 
            sharedId, 
            boost::dynamic_pointer_cast<UsdImagingHdEngine>(sharedEngine));
    } else {
        // In the refEngine, both excluded paths and invised paths are treated
        // the same way.
        SdfPathVector pathsToExclude = excludedPaths;
        pathsToExclude.insert(pathsToExclude.end(), 
            invisedPaths.begin(), invisedPaths.end());
        return new UsdImagingRefEngine(pathsToExclude);
    }
}

UsdImagingGL::UsdImagingGL()
{
    SdfPathVector excluded, invised;
    _engine.reset(_InitEngine(SdfPath::AbsoluteRootPath(), excluded, invised));
}

UsdImagingGL::UsdImagingGL(const SdfPath& rootPath,
                           const SdfPathVector& excludedPaths,
                           const SdfPathVector& invisedPaths,
                           const SdfPath& sharedId,
                           const UsdImagingGLSharedPtr& sharedImaging)
{
    _engine.reset(_InitEngine(rootPath, excludedPaths, invisedPaths, sharedId,
        (sharedImaging ? sharedImaging->_engine : UsdImagingEngineSharedPtr())));
}

UsdImagingGL::~UsdImagingGL()
{
    _engine->InvalidateBuffers();
}

void
UsdImagingGL::InvalidateBuffers()
{
    _engine->InvalidateBuffers();
}

/* static */
bool
UsdImagingGL::IsBatchingSupported()
{
    // Currently, batch drawing is supported only by the Hydra engine.
    return IsEnabledHydra();
}

/* static */
void
UsdImagingGL::PrepareBatch(
    const UsdImagingGLSharedPtrVector& renderers,
    const UsdPrimVector& rootPrims,
    const std::vector<UsdTimeCode>& times,
    RenderParams params)
{
    if (not IsBatchingSupported()) {
        return;
    }

    // Batching is only supported if the Hydra engine is enabled, and if
    // it is then all of the UsdImagingGL instances we've been given
    // must use a UsdImagingHdEngine engine. So we explicitly call the
    // the static method on that class.
    UsdImagingHdEngineSharedPtrVector hdEngines;
    hdEngines.reserve(renderers.size());
    TF_FOR_ALL(it, renderers) {
        hdEngines.push_back(
            boost::dynamic_pointer_cast<UsdImagingHdEngine>(
                (*it)->_engine));
    }

    UsdImagingHdEngine::PrepareBatch(hdEngines, rootPrims, times, params);
}

/*virtual*/
void
UsdImagingGL::PrepareBatch(const UsdPrim& root, RenderParams params)
{
    _engine->PrepareBatch(root, params);
}

/*virtual*/
void
UsdImagingGL::RenderBatch(const SdfPathVector& paths, RenderParams params) {
    _engine->RenderBatch(paths, params);
}

/*virtual*/
void
UsdImagingGL::Render(const UsdPrim& root, RenderParams params)
{
    _engine->Render(root, params);
}

/*virtual*/
void
UsdImagingGL::SetSelectionColor(GfVec4f const& color)
{
    _engine->SetSelectionColor(color);
}

/*virtual*/
void 
UsdImagingGL::SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport)
{
    _engine->SetCameraState(
        viewMatrix, projectionMatrix,
        viewport);
}

/*virtual*/
SdfPath 
UsdImagingGL::GetPrimPathFromPrimIdColor(
        GfVec4i const& primIdColor,
        GfVec4i const& instanceIdColor,
        int* instanceIndexOut)
{
    return _engine->GetPrimPathFromPrimIdColor(primIdColor,
                                               instanceIdColor,
                                               instanceIndexOut);
}

/* virtual */
SdfPath 
UsdImagingGL::GetPrimPathFromInstanceIndex(const SdfPath& protoPrimPath,
                                           int instanceIndex,
                                           int *absoluteInstanceIndex)
{
    return _engine->GetPrimPathFromInstanceIndex(protoPrimPath, instanceIndex,
                                                 absoluteInstanceIndex);
}

/* virtual */
void
UsdImagingGL::SetLightingStateFromOpenGL()
{
    _engine->SetLightingStateFromOpenGL();
}

/* virtual */
void
UsdImagingGL::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    _engine->SetLightingState(src);
}

/* virtual */
void
UsdImagingGL::SetRootTransform(GfMatrix4d const& xf)
{
    _engine->SetRootTransform(xf);
}

/* virtual */
void
UsdImagingGL::SetRootVisibility(bool isVisible)
{
    _engine->SetRootVisibility(isVisible);
}

/* virtual */
void
UsdImagingGL::SetSelected(SdfPathVector const& paths)
{
    _engine->SetSelected(paths);
}

/* virtual */
void
UsdImagingGL::ClearSelected()
{
    _engine->ClearSelected();
}

/* virtual */
void
UsdImagingGL::AddSelected(SdfPath const &path, int instanceIndex)
{
    _engine->AddSelected(path, instanceIndex);
}

/* virtual */
bool
UsdImagingGL::IsConverged() const
{
    return _engine->IsConverged();
}

/* virtual */
std::vector<TfType>
UsdImagingGL::GetRenderGraphPlugins()
{
    return _engine->GetRenderGraphPlugins();
}

/* virtual */
bool
UsdImagingGL::SetRenderGraphPlugin(TfType const &type)
{
    return _engine->SetRenderGraphPlugin(type);
}

bool
UsdImagingGL::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root, 
    RenderParams params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex)
{
    return _engine->TestIntersection(viewMatrix, projectionMatrix,
                worldToLocalSpace, root, params, outHitPoint,
                outHitPrimPath, outHitInstancerPath, outHitInstanceIndex);
}

bool
UsdImagingGL::TestIntersectionBatch(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const SdfPathVector& paths, 
    RenderParams params,
    unsigned int pickResolution,
    PathTranslatorCallback pathTranslator,
    HitBatch *outHit)
{
    return _engine->TestIntersectionBatch(viewMatrix, projectionMatrix,
                worldToLocalSpace, paths, params, pickResolution,
                pathTranslator, outHit);
}

/* virtual */
VtDictionary
UsdImagingGL::GetResourceAllocation() const
{
    VtDictionary dict;
    dict = _engine->GetResourceAllocation();

    // append texture usage
    size_t texMem = 0;
    for (auto const &texInfo :
             GlfTextureRegistry::GetInstance().GetTextureInfos()) {
        VtDictionary::const_iterator it = texInfo.find("memoryUsed");
        if (it != texInfo.end()) {
            VtValue mem = it->second;
            if (mem.IsHolding<size_t>()) {
                texMem += mem.Get<size_t>();
            }
        }
    }
    dict["textureMemoryUsed"] = texMem;
    return dict;
}
