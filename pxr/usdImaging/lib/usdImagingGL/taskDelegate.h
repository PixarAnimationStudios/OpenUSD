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
#ifndef USDIMAGINGGL_TASK_DELEGATE_H
#define USDIMAGINGGL_TASK_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/base/tf/type.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

//
// Render-graph delegate base
//

typedef boost::shared_ptr<class UsdImagingGLTaskDelegate> UsdImagingGLTaskDelegateSharedPtr;

class UsdImagingGLTaskDelegate : public HdSceneDelegate {
public:
    USDIMAGINGGL_API
    UsdImagingGLTaskDelegate(HdRenderIndexSharedPtr const& renderIndex,
                           SdfPath const& delegateID);

    USDIMAGINGGL_API
    ~UsdImagingGLTaskDelegate();

    // HdSceneDelegate interface
    virtual VtValue Get(SdfPath const& id, TfToken const& key) = 0;

    // UsdImagingGLTaskDelegate interface
    // returns tasks in the render graph for the given params
    virtual HdTaskSharedPtrVector GetRenderTasks(
        UsdImagingGLEngine::RenderParams const &params) = 0;

    // update roots and RenderParam
    virtual void SetCollectionAndRenderParams(
        const SdfPathVector &roots,
        const UsdImagingGLEngine::RenderParams &params) = 0;

    // Return the current active RprimCollection
    USDIMAGINGGL_API
    virtual HdRprimCollection const& GetRprimCollection() const;

    // set the lighting state using GlfSimpleLightingContext
    // HdLights are extracted from the lighting context and injected into
    // render index
    virtual void SetLightingState(const GlfSimpleLightingContextPtr &src) = 0;

    // set the camera matrices for the HdCamera injected in the render graph
    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport) = 0;

    // returns true if the task delegate can handle \p params.
    // if it's false, the default task will be used instead.
    // (for example, a plugin task may not support enableIdRender)
    virtual bool CanRender(const UsdImagingGLEngine::RenderParams &params) = 0;

    // returns true if the image is converged.
    virtual bool IsConverged() const = 0;
};

class UsdImagingGLTaskDelegateFactoryBase : public TfType::FactoryBase {
public:
    virtual UsdImagingGLTaskDelegateSharedPtr
    New(HdRenderIndexSharedPtr const& renderIndex,
        SdfPath const& delegateID) const = 0;
};

template <class T>
class UsdImagingGLTaskDelegateFactory : public UsdImagingGLTaskDelegateFactoryBase {
public:
    virtual UsdImagingGLTaskDelegateSharedPtr
    New(HdRenderIndexSharedPtr const& renderIndex,
        SdfPath const& delegateID) const
    {
        return UsdImagingGLTaskDelegateSharedPtr(new T(renderIndex, delegateID));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_TASK_DELEGATE_H
