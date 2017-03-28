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
#ifndef USDIMAGINGGL_DEFAULT_TASK_DELEGATE_H
#define USDIMAGINGGL_DEFAULT_TASK_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/usdImaging/usdImagingGL/taskDelegate.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE


// ---------------------------------------------------------------------------
// Task Delegate for built-in render graph
class UsdImagingGL_DefaultTaskDelegate : public UsdImagingGLTaskDelegate {
public:
    UsdImagingGL_DefaultTaskDelegate(HdRenderIndex *renderIndex,
                  SdfPath const& delegateID);
    ~UsdImagingGL_DefaultTaskDelegate();

    // HdSceneDelegate interface
    virtual VtValue Get(SdfPath const& id, TfToken const& key);

    // returns tasks in the render graph for the given params
    virtual HdTaskSharedPtrVector GetRenderTasks(
        UsdImagingGLEngine::RenderParams const &params);

    // update roots and RenderParam
    virtual void SetCollectionAndRenderParams(
        const SdfPathVector &roots,
        const UsdImagingGLEngine::RenderParams &params);

    virtual HdRprimCollection const& GetRprimCollection() const;

    // set the lighting state using GlfSimpleLightingContext
    // HdLights are extracted from the lighting context and injected into
    // render index
    virtual void SetLightingState(const GlfSimpleLightingContextPtr &src);

    // bypasses the lighting context down to HdxRenderTask (transitional method)
    void SetBypassedLightingState(const GlfSimpleLightingContextPtr &src);

    // set the camera matrices for the HdxCamera injected in the render graph
    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);

    // set the color for selection highlighting
    void SetSelectionColor(GfVec4f const& color);

    // always returns true for the default task
    virtual bool CanRender(const UsdImagingGLEngine::RenderParams &params);

    // returns true if the image is converged.
    virtual bool IsConverged() const;

    /// Returns true if the named option is enabled by the delegate.
    virtual bool IsEnabled(TfToken const& option) const;

    // returns the root namespace scope which tasks, camera and lights
    // belong to.
    SdfPath const &GetRootID() const { return _rootId; }

    // returns clip planes for the camera
    virtual std::vector<GfVec4d> GetClipPlanes(SdfPath const& cameraId);


protected:
    void _InsertRenderTask(SdfPath const &id);
    void _UpdateCollection(HdRprimCollection *col,
                           TfToken const &colName, TfToken const &reprName,
                           SdfPathVector const &roots,
                           SdfPath const &renderTaskId,
                           SdfPath const &idRenderTaskId);
    void _UpdateRenderParams(UsdImagingGLEngine::RenderParams const &renderParams,
                         UsdImagingGLEngine::RenderParams const &oldRenderParams,
                         SdfPath const &renderTaskId);

    template <typename T>
    T const &_GetValue(SdfPath const &id, TfToken const &key) {
        VtValue vParams = _valueCacheMap[id][key];
        TF_VERIFY(vParams.IsHolding<T>());
        return vParams.Get<T>();
    }
    template <typename T>
    void _SetValue(SdfPath const &id, TfToken const &key, T const &value) {
        _valueCacheMap[id][key] = value;
    }

private:
    HdRprimCollection _rprims;
    UsdImagingGLEngine::RenderParams _renderParams;
    UsdImagingGLEngine::RenderParams _idRenderParams;
    GfVec4d _viewport;

    SdfPath _rootId;
    SdfPath _renderTaskId;
    SdfPath _idRenderTaskId;

    SdfPath _selectionTaskId;
    GfVec4f _selectionColor;

    SdfPath _simpleLightTaskId;
    SdfPath _simpleLightBypassTaskId;
    SdfPath _activeSimpleLightTaskId;

    SdfPath _cameraId;

    SdfPathVector _lightIds;

    typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
    typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
    _ValueCacheMap _valueCacheMap;

    std::vector<GfVec4d> _clipPlanes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_DEFAULT_TASK_DELEGATE_H
