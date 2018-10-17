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
#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImagingGL/gl.h"
#include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#include "pxr/usdImaging/usdImagingGL/refEngine.h"

#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/glf/glContext.h"

PXR_NAMESPACE_OPEN_SCOPE

static
UsdImagingGLEngine* 
_InitEngine(const SdfPath& rootPath,
            const SdfPathVector& excludedPaths,
            const SdfPathVector& invisedPaths,
            const SdfPath& delegateID = SdfPath::AbsoluteRootPath())
{
    if (UsdImagingGLEngine::IsHydraEnabled()) {
        return new UsdImagingGLHdEngine(rootPath, excludedPaths,
                                        invisedPaths, delegateID);
    } else {
        // In the refEngine, both excluded paths and invised paths are treated
        // the same way.
        SdfPathVector pathsToExclude = excludedPaths;
        pathsToExclude.insert(pathsToExclude.end(), 
            invisedPaths.begin(), invisedPaths.end());
        return new UsdImagingGLRefEngine(pathsToExclude);
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
                           const SdfPath& delegateID)
{
    _engine.reset(_InitEngine(rootPath, excludedPaths,
                              invisedPaths, delegateID));
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

/*virtual*/
void
UsdImagingGL::PrepareBatch(const UsdPrim& root, 
                           const UsdImagingGLRenderParams& params)
{
    _engine->PrepareBatch(root, params);
}

/*virtual*/
void
UsdImagingGL::RenderBatch(const SdfPathVector& paths, 
                          const UsdImagingGLRenderParams& params)
{
    _engine->RenderBatch(paths, params);
}

/*virtual*/
void
UsdImagingGL::Render(const UsdPrim& root, 
                     const UsdImagingGLRenderParams& params)
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
UsdImagingGL::GetRprimPathFromPrimId(int primId) const
{
    return _engine->GetRprimPathFromPrimId(primId);
}

/* virtual */
SdfPath 
UsdImagingGL::GetPrimPathFromInstanceIndex(
    const SdfPath& protoPrimPath,
    int instanceIndex,
    int *absoluteInstanceIndex,
    SdfPath * rprimPath,
    SdfPathVector *instanceContext)
{
    return _engine->GetPrimPathFromInstanceIndex(protoPrimPath, instanceIndex,
                                                 absoluteInstanceIndex, 
                                                 rprimPath,
                                                 instanceContext);
}

/* virtual */
void
UsdImagingGL::SetLightingStateFromOpenGL()
{
    _engine->SetLightingStateFromOpenGL();
}

/* virtual */
void
UsdImagingGL::SetLightingState(GlfSimpleLightVector const &lights,
                               GlfSimpleMaterial const &material,
                               GfVec4f const &sceneAmbient)
{
    _engine->SetLightingState(lights, material, sceneAmbient);
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
TfTokenVector
UsdImagingGL::GetRendererPlugins() const
{
    return _engine->GetRendererPlugins();
}

/* virtual */
std::string
UsdImagingGL::GetRendererDisplayName(TfToken const &id) const
{
    return _engine->GetRendererDisplayName(id);
}

/* virtual */
TfToken
UsdImagingGL::GetCurrentRendererId() const
{
    return _engine->GetCurrentRendererId();
}

/* virtual */
bool
UsdImagingGL::SetRendererPlugin(TfToken const &id)
{
    return _engine->SetRendererPlugin(id);
}

/* virtual */
TfTokenVector
UsdImagingGL::GetRendererAovs() const
{
    return _engine->GetRendererAovs();
}

/* virtual */
bool
UsdImagingGL::SetRendererAov(TfToken const &id)
{
    return _engine->SetRendererAov(id);
}

bool
UsdImagingGL::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root, 
    const UsdImagingGLRenderParams& params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    int *outHitElementIndex)
{
    return _engine->TestIntersection(viewMatrix, projectionMatrix,
                worldToLocalSpace, root, params, outHitPoint,
                outHitPrimPath, outHitInstancerPath, outHitInstanceIndex,
                outHitElementIndex);
}

bool
UsdImagingGL::TestIntersectionBatch(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const SdfPathVector& paths, 
    const UsdImagingGLRenderParams& params,
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
    return _engine->GetResourceAllocation();
}

PXR_NAMESPACE_CLOSE_SCOPE

