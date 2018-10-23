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

/// \file hdEngine.h

#ifndef USDIMAGINGGL_HDENGINE_H
#define USDIMAGINGGL_HDENGINE_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/engine.h"

#include "pxr/imaging/hdx/rendererPlugin.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/taskController.h"

#include "pxr/base/tf/declarePtrs.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);

class HdRenderIndex;
typedef boost::shared_ptr<class UsdImagingGLHdEngine> 
                                        UsdImagingGLHdEngineSharedPtr;
typedef std::vector<UsdImagingGLHdEngineSharedPtr> 
                                        UsdImagingGLHdEngineSharedPtrVector;
typedef std::vector<UsdPrim> UsdPrimVector;

class UsdImagingGLHdEngine : public UsdImagingGLEngine
{
public:
    USDIMAGINGGL_API
    UsdImagingGLHdEngine(const SdfPath& rootPath,
                       const SdfPathVector& excludedPaths,
                       const SdfPathVector& invisedPaths=SdfPathVector(),
                       const SdfPath& delegateID = SdfPath::AbsoluteRootPath());

    USDIMAGINGGL_API
    virtual ~UsdImagingGLHdEngine();

    USDIMAGINGGL_API
    HdRenderIndex *GetRenderIndex() const;

    USDIMAGINGGL_API
    virtual void InvalidateBuffers();

    USDIMAGINGGL_API
    virtual void PrepareBatch(const UsdPrim& root, 
        const UsdImagingGLRenderParams& params) override;

    USDIMAGINGGL_API
    virtual void RenderBatch(const SdfPathVector& paths, 
        const UsdImagingGLRenderParams& params) override;

    USDIMAGINGGL_API
    virtual void Render(const UsdPrim& root, 
        const UsdImagingGLRenderParams& params) override;

    // Core rendering function: just draw, don't update anything.
    USDIMAGINGGL_API
    void Render(const UsdImagingGLRenderParams& params);

    USDIMAGINGGL_API
    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);

    USDIMAGINGGL_API
    virtual void SetLightingStateFromOpenGL();

    USDIMAGINGGL_API
    virtual void SetLightingState(GlfSimpleLightingContextPtr const &src);

    USDIMAGINGGL_API
    virtual void SetLightingState(GlfSimpleLightVector const &lights,
                                  GlfSimpleMaterial const &material,
                                  GfVec4f const &sceneAmbient);

    USDIMAGINGGL_API
    virtual void SetRootTransform(GfMatrix4d const& xf);

    USDIMAGINGGL_API
    virtual void SetRootVisibility(bool isVisible);

    USDIMAGINGGL_API
    virtual void SetSelected(SdfPathVector const& paths);

    USDIMAGINGGL_API
    virtual void ClearSelected();
    USDIMAGINGGL_API
    virtual void AddSelected(SdfPath const &path, int instanceIndex);

    USDIMAGINGGL_API
    virtual void SetSelectionColor(GfVec4f const& color);

    USDIMAGINGGL_API
    virtual SdfPath GetRprimPathFromPrimId(int primId) const;

    USDIMAGINGGL_API
    virtual SdfPath GetPrimPathFromInstanceIndex(
        SdfPath const& protoPrimPath,
        int instanceIndex,
        int *absoluteInstanceIndex=NULL,
        SdfPath * rprimPath=NULL,
        SdfPathVector *instanceContext=NULL);

    USDIMAGINGGL_API
    virtual bool IsConverged() const;

    USDIMAGINGGL_API
    virtual TfTokenVector GetRendererPlugins() const;

    USDIMAGINGGL_API
    virtual std::string GetRendererDisplayName(TfToken const &id) const 
        override;

    USDIMAGINGGL_API
    virtual TfToken GetCurrentRendererId() const override;

    USDIMAGINGGL_API
    virtual bool SetRendererPlugin(TfToken const &id);

    USDIMAGINGGL_API
    virtual TfTokenVector GetRendererAovs() const;

    USDIMAGINGGL_API
    virtual bool SetRendererAov(TfToken const& id);

    USDIMAGINGGL_API
    virtual bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const UsdPrim& root, 
        const UsdImagingGLRenderParams& params,
        GfVec3d *outHitPoint,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outHitInstancerPath = NULL,
        int *outHitInstanceIndex = NULL,
        int *outHitElementIndex = NULL);

    USDIMAGINGGL_API
    virtual bool TestIntersectionBatch(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const SdfPathVector& paths, 
        const UsdImagingGLRenderParams& params,
        unsigned int pickResolution,
        PathTranslatorCallback pathTranslator,
        HitBatch *outHit);

    USDIMAGINGGL_API
    virtual VtDictionary GetResourceAllocation() const;

    USDIMAGINGGL_API
    virtual UsdImagingGLRendererSettingsList GetRendererSettingsList() const;

    USDIMAGINGGL_API
    virtual VtValue GetRendererSetting(TfToken const& id) const;

    USDIMAGINGGL_API
    virtual void SetRendererSetting(TfToken const& id,
                                    VtValue const& value);

private:

    // These functions factor batch preparation into separate steps so they
    // can be reused by both the vectorized and non-vectorized API.
    bool _CanPrepareBatch(const UsdPrim& root, 
        const UsdImagingGLRenderParams& params);
    void _PreSetTime(const UsdPrim& root, 
        const UsdImagingGLRenderParams& params);
    void _PostSetTime(const UsdPrim& root, 
        const UsdImagingGLRenderParams& params);

    // Create a hydra collection given root paths and render params.
    // Returns true if the collection was updated.
    static bool _UpdateHydraCollection(HdRprimCollection *collection,
                          SdfPathVector const& roots,
                          UsdImagingGLRenderParams const& params,
                          TfTokenVector *renderTags);
    static HdxRenderTaskParams _MakeHydraUsdImagingGLRenderParams(
                          UsdImagingGLRenderParams const& params);

    // This function disposes of: the render index, the render plugin,
    // the task controller, and the usd imaging delegate.
    void _DeleteHydraResources();

    static TfToken _GetDefaultRendererPluginId();

    HdEngine _engine;

    HdRenderIndex *_renderIndex;

    HdxSelectionTrackerSharedPtr _selTracker;
    HdRprimCollection _renderCollection;
    HdRprimCollection _intersectCollection;

    SdfPath const _delegateID;
    UsdImagingDelegate *_delegate;

    HdxRendererPlugin *_rendererPlugin;
    TfToken _rendererId;
    HdxTaskController *_taskController;

    GlfSimpleLightingContextRefPtr _lightingContextForOpenGLState;

    // Data we want to live across render plugin switches:
    GfVec4f _selectionColor;

    SdfPath _rootPath;
    SdfPathVector _excludedPrimPaths;
    SdfPathVector _invisedPrimPaths;
    bool _isPopulated;

    TfTokenVector _renderTags;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_HDENGINE_H
