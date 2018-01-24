//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_SCENE_DELEGATE_H
#define PXRUSDMAYAGL_SCENE_DELEGATE_H

/// \file sceneDelegate.h

#include "pxr/pxr.h"

#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/renderParams.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/usd/sdf/path.h"

#include <maya/MDrawContext.h>

#include <memory>
#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE


class PxrMayaHdSceneDelegate : public HdSceneDelegate
{
    public:
        PXRUSDMAYAGL_API
        PxrMayaHdSceneDelegate(
                HdRenderIndex* renderIndex,
                const SdfPath& delegateID);

        // HdSceneDelegate interface
        PXRUSDMAYAGL_API
        virtual VtValue Get(const SdfPath& id, const TfToken& key) override;

        PXRUSDMAYAGL_API
        void SetCameraState(
                const GfMatrix4d& worldToViewMatrix,
                const GfMatrix4d& projectionMatrix,
                const GfVec4d& viewport);

        // VP 1.0 only.
        PXRUSDMAYAGL_API
        void SetLightingStateFromVP1(
                const GfMatrix4d& worldToViewMatrix,
                const GfMatrix4d& projectionMatrix);

        // VP 2.0 only.
        PXRUSDMAYAGL_API
        void SetLightingStateFromMayaDrawContext(
                const MHWRender::MDrawContext& context);

        PXRUSDMAYAGL_API
        HdTaskSharedPtrVector GetSetupTasks();

        PXRUSDMAYAGL_API
        HdTaskSharedPtr GetRenderTask(
                const size_t hash,
                const PxrMayaHdRenderParams& params,
                const SdfPathVector& roots);

    protected:
        PXRUSDMAYAGL_API
        void _SetLightingStateFromLightingContext();

        template <typename T>
        const T& _GetValue(const SdfPath& id, const TfToken& key) {
            VtValue vParams = _valueCacheMap[id][key];
            TF_VERIFY(vParams.IsHolding<T>());
            return vParams.Get<T>();
        }

        template <typename T>
        void _SetValue(const SdfPath& id, const TfToken& key, const T& value) {
            _valueCacheMap[id][key] = value;
        }

    private:
        SdfPath _rootId;

        SdfPath _cameraId;
        GfVec4d _viewport;

        SdfPath _simpleLightTaskId;
        SdfPathVector _lightIds;
        GlfSimpleLightingContextRefPtr _lightingContext;

        typedef std::unordered_map<size_t, SdfPath> _RenderTaskIdMap;
        _RenderTaskIdMap _renderTaskIdMap;

        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
};

typedef std::shared_ptr<PxrMayaHdSceneDelegate> PxrMayaHdSceneDelegateSharedPtr;


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_SCENE_DELEGATE_H
