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
#pragma once

#include "pxr/usdImaging/usdImaging/engine.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/base/tf/type.h"

#include <boost/shared_ptr.hpp>

//
// Render-graph delegate base
//

typedef boost::shared_ptr<class UsdImagingTaskDelegate> UsdImagingTaskDelegateSharedPtr;

class UsdImagingTaskDelegate : public HdSceneDelegate {
public:
    UsdImagingTaskDelegate(HdRenderIndexSharedPtr const& renderIndex,
                           SdfPath const& delegateID);

    ~UsdImagingTaskDelegate();

    // HdSceneDelegate interface
    virtual VtValue Get(SdfPath const& id, TfToken const& key) = 0;

    // UsdImagingTaskDelegate interface
    // returns tasks in the render graph for the given params
    virtual HdTaskSharedPtrVector GetRenderTasks(
        UsdImagingEngine::RenderParams const &params) = 0;

    // update roots and RenderParam
    virtual void SetCollectionAndRenderParams(
        const SdfPathVector &roots,
        const UsdImagingEngine::RenderParams &params) = 0;

    // Return the current active RprimCollection
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
    virtual bool CanRender(const UsdImagingEngine::RenderParams &params) = 0;

    // returns true if the image is converged.
    virtual bool IsConverged() const = 0;
};

class UsdImagingTaskDelegateFactoryBase : public TfType::FactoryBase {
public:
    virtual UsdImagingTaskDelegateSharedPtr
    New(HdRenderIndexSharedPtr const& renderIndex,
        SdfPath const& delegateID) const = 0;
};

template <class T>
class UsdImagingTaskDelegateFactory : public UsdImagingTaskDelegateFactoryBase {
public:
    virtual UsdImagingTaskDelegateSharedPtr
    New(HdRenderIndexSharedPtr const& renderIndex,
        SdfPath const& delegateID) const
    {
        return UsdImagingTaskDelegateSharedPtr(new T(renderIndex, delegateID));
    }
};
