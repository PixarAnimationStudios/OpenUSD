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

/// \file pxrUsdMayaGL/sceneDelegate.h

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
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/usd/sdf/path.h"

#include <maya/MDrawContext.h>

#include <memory>
#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE

struct PxrMayaHdPrimFilter {
    HdRprimCollection collection;
    TfTokenVector     renderTags;
};

using PxrMayaHdPrimFilterVector = std::vector<PxrMayaHdPrimFilter>;

class PxrMayaHdSceneDelegate : public HdSceneDelegate
{
    public:
        PXRUSDMAYAGL_API
        PxrMayaHdSceneDelegate(
                HdRenderIndex* renderIndex,
                const SdfPath& delegateID);

        // HdSceneDelegate interface
        PXRUSDMAYAGL_API
        VtValue Get(const SdfPath& id, const TfToken& key) override;

        PXRUSDMAYAGL_API
        VtValue GetCameraParamValue(
                SdfPath const& cameraId,
                TfToken const& paramName) override;

        PXRUSDMAYAGL_API
        TfTokenVector GetTaskRenderTags(SdfPath const& taskId) override;

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
        HdTaskSharedPtrVector GetRenderTasks(
                const size_t hash,
                const PxrMayaHdRenderParams& renderParams,
                const PxrMayaHdPrimFilterVector& primFilters);

        PXRUSDMAYAGL_API
        HdTaskSharedPtrVector GetPickingTasks(
                const TfTokenVector& renderTags);

    protected:
        PXRUSDMAYAGL_API
        void _SetLightingStateFromLightingContext();

        template <typename T>
        const T& _GetValue(const SdfPath& id, const TfToken& key) {
            VtValue vParams = _valueCacheMap[id][key];
            if (!TF_VERIFY(vParams.IsHolding<T>(),
                           "For Id = %s, Key = %s",
                           id.GetText(), key.GetText())) {
                static T ERROR_VALUE;
                return ERROR_VALUE;
            }
            return vParams.UncheckedGet<T>();
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

        SdfPath _shadowTaskId;

        // XXX: While this is correct, that we are using
        // hash in forming the task id, so the map is valid.
        // It is possible for the hash to collide, so the id
        // formed from the combination of hash and collection name is not
        // necessarily unique.
        struct _RenderTaskIdMapKey
        {
            size_t                hash;
            TfToken               collectionName;

            struct HashFunctor {
                size_t operator()(const  _RenderTaskIdMapKey& value) const;
            };

            bool operator==(const  _RenderTaskIdMapKey& other) const;
        };

        typedef std::unordered_map<
                _RenderTaskIdMapKey,
                SdfPath,
                _RenderTaskIdMapKey::HashFunctor> _RenderTaskIdMap;

        typedef std::unordered_map<size_t, SdfPath> _RenderParamTaskIdMap;


       
        _RenderParamTaskIdMap _renderSetupTaskIdMap;
        _RenderTaskIdMap      _renderTaskIdMap;
        _RenderParamTaskIdMap _selectionTaskIdMap;

		SdfPath _pickingTaskId;

        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
};

typedef std::shared_ptr<PxrMayaHdSceneDelegate> PxrMayaHdSceneDelegateSharedPtr;


PXR_NAMESPACE_CLOSE_SCOPE


#endif
