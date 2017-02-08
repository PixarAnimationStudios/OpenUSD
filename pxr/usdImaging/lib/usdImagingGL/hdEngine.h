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
#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdx/selectionTracker.h"

#include "pxr/base/tf/declarePtrs.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);

class HdRenderIndex;
typedef boost::shared_ptr<class UsdImagingGLTaskDelegate>
                                        UsdImagingGLTaskDelegateSharedPtr;
typedef boost::shared_ptr<class UsdImagingGL_DefaultTaskDelegate>
                                        UsdImagingGL_DefaultTaskDelegateSharedPtr;
typedef boost::shared_ptr<class UsdImagingGLHdEngine> UsdImagingGLHdEngineSharedPtr;
typedef std::vector<UsdImagingGLHdEngineSharedPtr> 
                                        UsdImagingGLHdEngineSharedPtrVector;
typedef std::vector<UsdPrim> UsdPrimVector;
typedef boost::shared_ptr<class HdxIntersector> HdxIntersectorSharedPtr;

class UsdImagingGLHdEngine : public UsdImagingGLEngine
{
public:
    UsdImagingGLHdEngine(const SdfPath& rootPath,
                       const SdfPathVector& excludedPaths,
                       const SdfPathVector& invisedPaths=SdfPathVector(),
                       const SdfPath& sharedId = SdfPath::AbsoluteRootPath(),
                       const UsdImagingGLHdEngineSharedPtr& sharedImaging =
                           UsdImagingGLHdEngineSharedPtr());

    virtual ~UsdImagingGLHdEngine();

    HdRenderIndexSharedPtr GetRenderIndex() const;

    virtual void InvalidateBuffers();

    static void PrepareBatch(
        const UsdImagingGLHdEngineSharedPtrVector& engines,
        const UsdPrimVector& rootPrims,
        const std::vector<UsdTimeCode>& times,
        RenderParams params);

    virtual void PrepareBatch(const UsdPrim& root, RenderParams params);
    virtual void RenderBatch(const SdfPathVector& paths, RenderParams params);

    virtual void Render(const UsdPrim& root, RenderParams params);

    // A custom render override for hdEngine.
    // note: external RenderIndex may not be needed anymore.
    void Render(HdRenderIndex& index, RenderParams params);

    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);

    virtual void SetLightingStateFromOpenGL();

    virtual void SetLightingState(GlfSimpleLightingContextPtr const &src);

    virtual void SetLightingState(GlfSimpleLightVector const &lights,
                                  GlfSimpleMaterial const &material,
                                  GfVec4f const &sceneAmbient);

    virtual void SetRootTransform(GfMatrix4d const& xf);

    virtual void SetRootVisibility(bool isVisible);

    virtual void SetSelected(SdfPathVector const& paths);

    virtual void ClearSelected();
    virtual void AddSelected(SdfPath const &path, int instanceIndex);

    virtual void SetSelectionColor(GfVec4f const& color);

    virtual SdfPath GetPrimPathFromPrimIdColor(GfVec4i const& primIdColor,
                                               GfVec4i const& instanceIdColor,
                                               int* instanceIndexOut = NULL);

    virtual SdfPath GetPrimPathFromInstanceIndex(
        SdfPath const& protoPrimPath,
        int instanceIndex,
        int *absoluteInstanceIndex=NULL,
        SdfPath * rprimPath=NULL,
        SdfPathVector *instanceContext=NULL);

    virtual bool IsConverged() const;

    virtual std::vector<TfType> GetRenderGraphPlugins();

    virtual bool SetRenderGraphPlugin(TfType const &type);

    virtual bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const UsdPrim& root, 
        RenderParams params,
        GfVec3d *outHitPoint,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outHitInstancerPath = NULL,
        int *outHitInstanceIndex = NULL);

    virtual bool TestIntersectionBatch(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const SdfPathVector& paths, 
        RenderParams params,
        unsigned int pickResolution,
        PathTranslatorCallback pathTranslator,
        HitBatch *outHit);

    virtual VtDictionary GetResourceAllocation() const;

private:
    // Helper functions for preparing multiple engines for
    // batched drawing.
    static void _PrepareBatch(const UsdImagingGLHdEngineSharedPtrVector& engines,
                              const UsdPrimVector& rootPrims,
                              const std::vector<UsdTimeCode>& times,
                              const RenderParams& params);

    static void _Populate(const UsdImagingGLHdEngineSharedPtrVector& engines,
                          const UsdPrimVector& rootPrims);
    static void _SetTimes(const UsdImagingGLHdEngineSharedPtrVector& engines,
                          const UsdPrimVector& rootPrims,
                          const std::vector<UsdTimeCode>& times,
                          const RenderParams& params);

    // These functions factor batch preparation into separate steps so they
    // can be reused by both the vectorized and non-vectorized API.
    bool _CanPrepareBatch(const UsdPrim& root, const RenderParams& params);
    void _PreSetTime(const UsdPrim& root, const RenderParams& params);
    void _PostSetTime(const UsdPrim& root, const RenderParams& params);

    // returns the active task delegate for \p param. param is used to fallback
    // to the default task delegate when enableIdRender is true for picking.
    UsdImagingGLTaskDelegateSharedPtr _GetTaskDelegate(
        const RenderParams &params) const ;

    HdEngine _engine;
    HdRenderIndexSharedPtr _renderIndex;
    HdxSelectionTrackerSharedPtr _selTracker;
    HdxIntersectorSharedPtr _intersector;
    UsdImagingDelegate _delegate;

    // built-in render graph delegate
    UsdImagingGL_DefaultTaskDelegateSharedPtr _defaultTaskDelegate;

    // plug-in render graphs delegate
    bool _pluginDiscovered;
    typedef std::map<TfType, UsdImagingGLTaskDelegateSharedPtr>
        _PluginTaskDelegateMap;
    _PluginTaskDelegateMap _pluginTaskDelegates;
    UsdImagingGLTaskDelegateSharedPtr _currentPluginTaskDelegate;

    GlfSimpleLightingContextRefPtr _lightingContextForOpenGLState;

    SdfPath _rootPath;
    SdfPathVector _excludedPrimPaths;
    SdfPathVector _invisedPrimPaths;
    bool _isPopulated;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_HDENGINE_H
