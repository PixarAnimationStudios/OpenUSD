//
// Copyright 2017 Autodesk
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
///
/// \file usdTaskDelegate.h
///

#ifndef PXRUSDMAYAGL_USDTASKDELEGATE_H
#define PXRUSDMAYAGL_USDTASKDELEGATE_H

#include "pxrUsdMayaGL/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);

/// \brief hd task
class UsdTaskDelegate : public HdSceneDelegate {
public:
	PXRUSDMAYAGL_API
	UsdTaskDelegate(HdRenderIndex *renderIndex,
		SdfPath const& delegateID);

	// HdSceneDelegate interface
	PXRUSDMAYAGL_API
	virtual VtValue Get(SdfPath const& id, TfToken const& key);

	PXRUSDMAYAGL_API
	void SetCameraState(const GfMatrix4d& viewMatrix,
		const GfMatrix4d& projectionMatrix,
		const GfVec4d& viewport);

	// set the color for selection highlighting
	PXRUSDMAYAGL_API
	void SetSelectionColor(GfVec4f const& color);
	PXRUSDMAYAGL_API
	void SetSelectionEnable(bool enable);

	PXRUSDMAYAGL_API
	HdTaskSharedPtrVector GetSetupTasks(GlfSimpleLightingContextRefPtr lightingContext);

	PXRUSDMAYAGL_API
	HdTaskSharedPtr GetRenderTask(size_t hash,
		TfTokenVector const & renderTags,
		TfToken drawRepr,
		GfVec4f const & overrideColor,
		HdCullStyle cullStyle,
		SdfPathVector const &roots);

protected:
	void _UpdateLightingTask(GlfSimpleLightingContextRefPtr lightingContext);
	void _InsertRenderTask(SdfPath const &id);

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
	typedef std::unordered_map<size_t, SdfPath> _RenderTaskIdMap;
	_RenderTaskIdMap _renderTaskIdMap;
	SdfPath _rootId;

	SdfPath _simpleLightTaskId;

	SdfPathVector _lightIds;
	SdfPath _cameraId;
	SdfPath _shadowTaskId;
	SdfPath _selectionTaskId;
	GfVec4d _viewport;

	typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
	typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
	_ValueCacheMap _valueCacheMap;
};

typedef std::shared_ptr<UsdTaskDelegate> UsdTaskDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_USDTASKDELEGATE_H
