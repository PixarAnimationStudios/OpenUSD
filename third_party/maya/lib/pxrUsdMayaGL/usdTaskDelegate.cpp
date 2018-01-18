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
// Some header #define's Bool as int, which breaks stuff in sdf/types.h.
// Include it first to sidestep the problem. :-/
#include "pxr/imaging/hdSt/camera.h"
#include "pxr/imaging/hdSt/light.h"

#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/shadowMatrixComputation.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/staticTokens.h"

#include "usdTaskDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
	_tokens,
	(idRenderTask)
	(renderTask)
	(shadowTask)
	(selectionTask)
	(simpleLightTask)
	(camera)
);

namespace {
	class ShadowMatrix : public HdxShadowMatrixComputation
	{
	public:
		ShadowMatrix(GlfSimpleLight const &light)
		{
			GfVec4d pos = light.GetPosition();
			GfVec3d dir = light.GetSpotDirection();

			GfFrustum frustum;
			if (light.GetSpotCutoff() < 180.0)
			{
				// Spot light
				frustum.SetProjectionType(GfFrustum::Perspective);
				frustum.SetPerspective(45, 1, 1, 100);
				frustum.SetPosition(GfVec3d(pos[0], pos[1], pos[2]));
				frustum.SetRotation(GfRotation(GfVec3d(0, 0, -1), dir));
			}
			else
			{
				frustum.SetProjectionType(GfFrustum::Orthographic);
				frustum.SetWindow(GfRange2d(GfVec2d(-10, -10), GfVec2d(10, 10)));
				frustum.SetNearFar(GfRange1d(-10, 100));
				frustum.SetPosition(GfVec3d(pos[0], pos[1], pos[2]));
				frustum.SetRotation(GfRotation(GfVec3d(0, 0, 1),
					GfVec3d(pos[0], pos[1], pos[2])));
			}

			_shadowMatrix =
				frustum.ComputeViewMatrix() * frustum.ComputeProjectionMatrix();
		}

		virtual GfMatrix4d Compute(const GfVec4f &viewport,
			CameraUtilConformWindowPolicy policy) {
			return _shadowMatrix;
		}
	private:
		GfMatrix4d _shadowMatrix;
	};
}

UsdTaskDelegate::UsdTaskDelegate(
    HdRenderIndex *renderIndex, SdfPath const& delegateID)
    : HdSceneDelegate(renderIndex, delegateID)
{
    // populate tasks in renderindex

    // create an unique namespace
    _rootId = delegateID.AppendChild(
        TfToken(TfStringPrintf("_UsdImaging_%p", this)));

    _simpleLightTaskId          = _rootId.AppendChild(_tokens->simpleLightTask);
    _cameraId                   = _rootId.AppendChild(_tokens->camera);
	_shadowTaskId				= _rootId.AppendChild(_tokens->shadowTask);
	_selectionTaskId			= _rootId.AppendChild(_tokens->selectionTask);

    // camera
    {
        // Since we're hardcoded to use HdStRenderDelegate, we expect to
        // have camera Sprims.
        TF_VERIFY(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->camera));

        renderIndex->InsertSprim(HdPrimTypeTokens->camera, this, _cameraId);
        _ValueCache &cache = _valueCacheMap[_cameraId];
        cache[HdStCameraTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdStCameraTokens->projectionMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdStCameraTokens->windowPolicy] = VtValue();  // no window policy.
    }

	// shadow
	{
		renderIndex->InsertTask<HdxShadowTask>(this, _shadowTaskId);
		_ValueCache &cache = _valueCacheMap[_shadowTaskId];
		HdxShadowTaskParams params;
		params.camera = _cameraId;
		cache[HdTokens->children] = VtValue(SdfPathVector());
		cache[HdTokens->params] = VtValue(params);
	}

	// selection task
	{
		renderIndex->InsertTask<HdxSelectionTask>(this, _selectionTaskId);
		_ValueCache &cache = _valueCacheMap[_selectionTaskId];
		HdxSelectionTaskParams params;
		params.enableSelection = true;
		params.selectionColor = GfVec4f(1, 1, 0, 1);
		params.locateColor = GfVec4f(0, 0, 1, 1);
		cache[HdTokens->params] = VtValue(params);
		cache[HdTokens->children] = VtValue(SdfPathVector());
	}

    // simple lighting task (for Hydra native)
    {
        renderIndex->InsertTask<HdxSimpleLightTask>(this, _simpleLightTaskId);
        _ValueCache &cache = _valueCacheMap[_simpleLightTaskId];
        HdxSimpleLightTaskParams taskParams;
        taskParams.cameraPath = _cameraId;
        cache[HdTokens->params] = VtValue(taskParams);
        cache[HdTokens->children] = VtValue(SdfPathVector());
    }
}

void
UsdTaskDelegate::_InsertRenderTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxRenderTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    HdxRenderTaskParams taskParams;
    taskParams.camera = _cameraId;
    // Initialize viewport to the latest value since render tasks can be lazily
    // instantiated, potentially even after SetCameraState.  All other
    // parameters will be updated by _UpdateRenderParams.
    taskParams.viewport = _viewport;
    cache[HdTokens->params] = VtValue(taskParams);
    cache[HdTokens->children] = VtValue(SdfPathVector());
    cache[HdTokens->collection] = VtValue();
}

/*virtual*/
VtValue
UsdTaskDelegate::Get(
    SdfPath const& id,
    TfToken const &key)
{
    _ValueCache *vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if( vcache && TfMapLookup(*vcache, key, &ret) )
        return ret;

    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
                    id.GetText(), key.GetText());
    return VtValue();
}

void
UsdTaskDelegate::SetCameraState(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const GfVec4d& viewport)
{
    // cache the camera matrices
    _ValueCache &cache = _valueCacheMap[_cameraId];
    cache[HdStCameraTokens->worldToViewMatrix] = VtValue(viewMatrix);
    cache[HdStCameraTokens->projectionMatrix] = VtValue(projectionMatrix);
    cache[HdStCameraTokens->windowPolicy] = VtValue(); // no window policy.

    // invalidate the camera to be synced
    GetRenderIndex().GetChangeTracker().MarkSprimDirty(_cameraId,
                                                       HdStCamera::AllDirty);

    if( _viewport != viewport )
    {
        // viewport is also read by HdxRenderTaskParam. invalidate it.
        _viewport = viewport;

        // update all render tasks
        for( const auto &it : _renderTaskIdMap )
        {
            SdfPath const &taskId = it.second;
            HdxRenderTaskParams taskParams
                = _GetValue<HdxRenderTaskParams>(taskId, HdTokens->params);

            // update viewport in HdxRenderTaskParams
            taskParams.viewport = viewport;
            _SetValue(taskId, HdTokens->params, taskParams);

            // invalidate
            GetRenderIndex().GetChangeTracker().MarkTaskDirty(
                taskId, HdChangeTracker::DirtyParams);
        }
    }
}

void
UsdTaskDelegate::_UpdateLightingTask(GlfSimpleLightingContextRefPtr lightingContext)
{
	if (! lightingContext)
		return;

    // cache the GlfSimpleLight vector
    const GlfSimpleLightVector& lights
        = lightingContext->GetLights();

    bool hasNumLightsChanged = false;

    // Insert the light Ids into HdRenderIndex for those not yet exist.
    while( _lightIds.size() < lights.size() )
    {
        SdfPath lightId(
            TfStringPrintf("%s/light%d",
							_rootId.GetText(),
                           (int)_lightIds.size()));
        _lightIds.push_back(lightId);
		
		// Since we're hardcoded to use HdStRenderDelegate, we expect to have
        // light Sprims.
        TF_VERIFY(GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->light));

		GetRenderIndex().InsertSprim(HdPrimTypeTokens->light, this, lightId);
        hasNumLightsChanged = true;
    }
    // Remove unused light Ids from HdRenderIndex
    while( _lightIds.size() > lights.size() )
    {
        GetRenderIndex().RemoveSprim(HdPrimTypeTokens->light, _lightIds.back());
        _lightIds.pop_back();
        hasNumLightsChanged = true;
    }

    // invalidate HdLights
    for( size_t i = 0; i < lights.size(); ++i )
    {
        _ValueCache &cache = _valueCacheMap[_lightIds[i]];
        // store GlfSimpleLight directly.
        cache[HdStLightTokens->params] = VtValue(lights[i]);
        cache[HdStLightTokens->transform] = VtValue();
		// store shadow params
		HdxShadowParams shadowParams = HdxShadowParams();
		shadowParams.enabled = lights[i].HasShadow();
		if (shadowParams.enabled)
		{
			shadowParams.resolution = (_viewport[3] + _viewport[2] - _viewport[1] - _viewport[0]) / 2; // Dynamic shadow resolution
			shadowParams.shadowMatrix = HdxShadowMatrixComputationSharedPtr(new ShadowMatrix(lights[i]));
			shadowParams.bias = -0.001;
			shadowParams.blur = 0.1;
			cache[HdStLightTokens->shadowParams] = VtValue(shadowParams);
			cache[HdStLightTokens->shadowCollection] = HdRprimCollection(HdTokens->geometry, HdTokens->refined);
		
			GetRenderIndex().GetChangeTracker().MarkSprimDirty(
				_lightIds[i], HdStLight::DirtyShadowParams);
		}
		else
		{
			cache[HdStLightTokens->shadowParams] = VtValue(HdxShadowParams());
			cache[HdStLightTokens->shadowCollection] = VtValue();
		}

        // Only mark as dirty the parameters to avoid unnecessary invalidation
        // specially marking as dirty lightShadowCollection will trigger
        // a collection dirty on geometry and we don't want that to happen
        // always
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            _lightIds[i], HdStLight::AllDirty);
    }

    // sadly the material also comes from lighting context right now...
    HdxSimpleLightTaskParams lightTaskParams
        = _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                              HdTokens->params);
	lightTaskParams.sceneAmbient = lightingContext->GetSceneAmbient();
	lightTaskParams.material = lightingContext->GetMaterial();
	lightTaskParams.viewport = GfVec4f(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);

    // invalidate HdxSimpleLightTask too
    if( hasNumLightsChanged ||
		lightingContext->GetUseShadows() != lightTaskParams.enableShadows
		)
    {
		lightTaskParams.enableShadows = lightingContext->GetUseShadows();

        _SetValue(_simpleLightTaskId, HdTokens->params, lightTaskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }

	// Shadow task params
	_ValueCache &cache = _valueCacheMap[_shadowTaskId];
	HdxShadowTaskParams shadowTaskParams =
		cache[HdTokens->params].Get<HdxShadowTaskParams>();
	if (lightingContext->GetUseShadows() != shadowTaskParams.enableLighting ||
		_viewport != shadowTaskParams.viewport
		)
	{
		shadowTaskParams.viewport = _viewport;
		shadowTaskParams.enableLighting = lightingContext->GetUseShadows();
		cache[HdTokens->params] = VtValue(shadowTaskParams);

		GetRenderIndex().GetChangeTracker().MarkTaskDirty(
			_shadowTaskId, HdChangeTracker::DirtyParams);
	}
}

HdTaskSharedPtrVector
UsdTaskDelegate::GetSetupTasks(GlfSimpleLightingContextRefPtr lightingContext)
{
    HdTaskSharedPtrVector tasks;

    // lighting
	if (lightingContext)
	{
		if (lightingContext->GetUseLighting())
		{
			_UpdateLightingTask(lightingContext);
			tasks.push_back(GetRenderIndex().GetTask(_simpleLightTaskId));
			// Shadow
			if (lightingContext->GetUseShadows())
			{
				tasks.push_back(GetRenderIndex().GetTask(_shadowTaskId));
			}
		}
	}
	// selection highlighting (selectionTask comes after renderTask)
	tasks.push_back(GetRenderIndex().GetTask(_selectionTaskId));
    return tasks;
}

HdTaskSharedPtr
UsdTaskDelegate::GetRenderTask(
	size_t hash,
	TfTokenVector const & renderTags,
	TfToken drawRepr,
	GfVec4f const & overrideColor,
	HdCullStyle cullStyle,
    SdfPathVector const &roots)
{
    // select bucket
    SdfPath renderTaskId;
    if( ! TfMapLookup(_renderTaskIdMap, hash, &renderTaskId) )
    {
        // create new render task if not exists
        renderTaskId = _rootId.AppendChild(
            TfToken(TfStringPrintf("renderTask%zx", hash)));
        _InsertRenderTask(renderTaskId);
        _renderTaskIdMap[hash] = renderTaskId;
    }

    // Update collection in the value cache
    TfToken colName = HdTokens->geometry;
    HdRprimCollection rprims(colName, drawRepr);
    rprims.SetRootPaths(roots);
	rprims.SetRenderTags(renderTags);

    // update value cache
    _SetValue(renderTaskId, HdTokens->collection, rprims);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId, HdChangeTracker::DirtyCollection);

    // update render params in the value cache
    HdxRenderTaskParams taskParams =
                _GetValue<HdxRenderTaskParams>(renderTaskId, HdTokens->params);

    // update params
    taskParams.overrideColor         = overrideColor;
    taskParams.wireframeColor        = GfVec4f(0, 0.001238, 0.11666, 1.0);
    taskParams.enableLighting        = ! _lightIds.empty();
    taskParams.enableIdRender        = false;
    taskParams.alphaThreshold        = 0.1;
    taskParams.tessLevel             = 32.0;
    const float tinyThreshold        = 0.9f;
    taskParams.drawingRange          = GfVec2f(tinyThreshold, -1.0f);
    taskParams.depthBiasUseDefault   = true;
    taskParams.depthFunc             = HdCmpFuncLess;
    taskParams.cullStyle             = cullStyle;
    taskParams.geomStyle             = HdGeomStylePolygons;
    taskParams.enableHardwareShading = true;

    // note that taskParams.rprims and taskParams.viewport are not updated
    // in this function, and needed to be preserved.

    // store into cache
    _SetValue(renderTaskId, HdTokens->params, taskParams);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId,  HdChangeTracker::DirtyParams);

    return GetRenderIndex().GetTask(renderTaskId);
}

void
UsdTaskDelegate::SetSelectionEnable(bool enable)
{
	_ValueCache &cache = _valueCacheMap[_selectionTaskId];
	HdxSelectionTaskParams params =
		cache[HdTokens->params].Get<HdxSelectionTaskParams>();
	if (params.enableSelection != enable)
	{
		GetRenderIndex().GetChangeTracker().MarkTaskDirty(
			_selectionTaskId, HdChangeTracker::DirtyParams);

		params.enableSelection = enable;
		cache[HdTokens->params] = VtValue(params);
	}
}

void
UsdTaskDelegate::SetSelectionColor(GfVec4f const& color)
{
	_ValueCache &cache = _valueCacheMap[_selectionTaskId];
	HdxSelectionTaskParams params =
		cache[HdTokens->params].Get<HdxSelectionTaskParams>();
	if (params.selectionColor != color)
	{
		GetRenderIndex().GetChangeTracker().MarkTaskDirty(
			_selectionTaskId, HdChangeTracker::DirtyParams);

		params.selectionColor = color;
		cache[HdTokens->params] = VtValue(params);
	}
}

PXR_NAMESPACE_CLOSE_SCOPE
