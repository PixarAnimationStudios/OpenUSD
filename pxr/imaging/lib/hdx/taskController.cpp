//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdx/taskController.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hdx/colorizeTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdxIntersectionModeTokens, \
    HDX_INTERSECTION_MODE_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderTask)
    (translucencyRenderTask)
    (selectionTask)
    (simpleLightTask)
    (shadowTask)
    (colorizeTask)
    (colorCorrectionTask)
    (camera)
    (renderBufferDescriptor)
);

// ---------------------------------------------------------------------------
// Delegate implementation.

/* virtual */
VtValue
HdxTaskController::_Delegate::Get(SdfPath const& id, TfToken const& key)
{
    _ValueCache *vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if (vcache && TfMapLookup(*vcache, key, &ret)) {
        return ret;
    }
    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
            id.GetText(), key.GetText());
    return VtValue();
}

/* virtual */
bool
HdxTaskController::_Delegate::IsEnabled(TfToken const& option) const
{
    // The client using this task controller is responsible for setting
    // GL_SAMPLE_ALPHA_TO_COVERAGE.
    if (option == HdxOptionTokens->taskSetAlphaToCoverage) {
        return true;
    }
    return HdSceneDelegate::IsEnabled(option);
}

/* virtual */
std::vector<GfVec4d>
HdxTaskController::_Delegate::GetClipPlanes(SdfPath const& cameraId)
{
    return GetParameter<std::vector<GfVec4d>>(cameraId,
                HdCameraTokens->clipPlanes);
}

/* virtual */
HdRenderBufferDescriptor
HdxTaskController::_Delegate::GetRenderBufferDescriptor(SdfPath const& id)
{
    return GetParameter<HdRenderBufferDescriptor>(id,
                _tokens->renderBufferDescriptor);
}

// ---------------------------------------------------------------------------
// Task controller implementation.

HdxTaskController::HdxTaskController(HdRenderIndex *renderIndex,
                                     SdfPath const& controllerId)
    : _index(renderIndex)
    , _controllerId(controllerId)
    , _intersector(new HdxIntersector(renderIndex))
    , _delegate(renderIndex, controllerId)
{
    // We create camera and tasks here, but lights are created lazily by
    // SetLightingState. Camera needs to be created first, since it's a
    // parameter of most tasks.

    _CreateCamera();
    _CreateRenderTask();
    _CreateSelectionTask();
    _CreateLightingTask();
    _CreateShadowTask();
    _CreateColorizeTask();
    _CreateColorCorrectionTask();
}

void
HdxTaskController::_CreateCamera()
{
    // Create a default camera, driven by SetCameraMatrices.
    _cameraId = GetControllerId().AppendChild(_tokens->camera);
    GetRenderIndex()->InsertSprim(HdPrimTypeTokens->camera,
        &_delegate, _cameraId);

    _delegate.SetParameter(_cameraId, HdCameraTokens->windowPolicy,
        VtValue(CameraUtilFit));
    _delegate.SetParameter(_cameraId, HdCameraTokens->worldToViewMatrix,
        VtValue(GfMatrix4d(1.0)));
    _delegate.SetParameter(_cameraId, HdCameraTokens->projectionMatrix,
        VtValue(GfMatrix4d(1.0)));
    _delegate.SetParameter(_cameraId, HdCameraTokens->clipPlanes,
        VtValue(std::vector<GfVec4d>()));
}

void
HdxTaskController::_CreateRenderTask()
{
    // Avoid triggering double-rendering in render backends by only pushing
    // the translucent task when the renderer is Stream.
    // XXX This dependecy on HdSt can be removed once we have a better system
    //     in-place to have per renderDelegate tasks.
    HdStRenderDelegate* hdStRenderDelegate = dynamic_cast<HdStRenderDelegate*>(
        GetRenderIndex()->GetRenderDelegate());

    TfTokenVector renderTasks;
    renderTasks.push_back(_tokens->renderTask);
    if (hdStRenderDelegate) {
        renderTasks.push_back(_tokens->translucencyRenderTask);
    }

    TfToken const materialTags[2] = {
        HdMaterialTagTokens->defaultMaterialTag,
        HdxMaterialTagTokens->translucent
    };

    for (unsigned i=0; i<renderTasks.size(); i++) {
        SdfPath renderTaskId = GetControllerId().AppendChild(renderTasks[i]);
        _renderTaskIds.push_back(renderTaskId);
        TfToken const& materialTag = materialTags[i];

        HdxRenderTaskParams renderParams;
        renderParams.camera = _cameraId;
        renderParams.viewport = GfVec4d(0,0,1,1);

        if (materialTag == HdxMaterialTagTokens->translucent) {
            // Additive blend -- so no sorting of drawItems is needed
            renderParams.blendEnable = true;
            renderParams.blendColorOp = HdBlendOpAdd;
            renderParams.blendAlphaOp = HdBlendOpAdd;
            renderParams.blendColorSrcFactor = HdBlendFactorOne;
            renderParams.blendColorDstFactor = HdBlendFactorOne;
            renderParams.blendAlphaSrcFactor = HdBlendFactorOne;
            renderParams.blendAlphaDstFactor = HdBlendFactorOne;

            // Translucent objects should not block each other in depth buffer
            renderParams.depthMaskEnable = false;

            // Since we are using alpha blending, we disable screen door
            // transparency. However screen door transparency remains a valid
            // transparency method for the opaque task so we only disable it
            // during the translucent task.
            renderParams.enableAlphaToCoverage = false;
        }

        HdRprimCollection collection(HdTokens->geometry,
                                    HdReprSelector(HdReprTokens->smoothHull),
                                    /*forcedRepr*/ false,
                                    materialTag);
        collection.SetRootPath(SdfPath::AbsoluteRootPath());

        GetRenderIndex()->InsertTask<HdxRenderTask>(&_delegate,
            renderTaskId);

        _delegate.SetParameter(renderTaskId, HdTokens->params,
            renderParams);
        _delegate.SetParameter(renderTaskId, HdTokens->collection,
            collection);
    }
}

void
HdxTaskController::_CreateSelectionTask()
{
    // Create a selection highlighting task.
    _selectionTaskId = GetControllerId().AppendChild(_tokens->selectionTask);

    HdxSelectionTaskParams selectionParams;
    selectionParams.enableSelection = true;
    selectionParams.selectionColor = GfVec4f(1,1,0,1);
    selectionParams.locateColor = GfVec4f(0,0,1,1);

    GetRenderIndex()->InsertTask<HdxSelectionTask>(&_delegate,
        _selectionTaskId);

    _delegate.SetParameter(_selectionTaskId, HdTokens->params,
        selectionParams);
}

void
HdxTaskController::_CreateLightingTask()
{
    // Simple lighting task uses lighting state from Sprims.
    _simpleLightTaskId = GetControllerId().AppendChild(
        _tokens->simpleLightTask);

    HdxSimpleLightTaskParams simpleLightParams;
    simpleLightParams.cameraPath = _cameraId;

    GetRenderIndex()->InsertTask<HdxSimpleLightTask>(&_delegate,
        _simpleLightTaskId);

    _delegate.SetParameter(_simpleLightTaskId, HdTokens->params,
        simpleLightParams);
}

void
HdxTaskController::_CreateShadowTask() 
{
    _shadowTaskId = GetControllerId().AppendChild(_tokens->shadowTask);

    HdxShadowTaskParams shadowParams;
    shadowParams.camera = _cameraId;

    GetRenderIndex()->InsertTask<HdxShadowTask>(&_delegate, _shadowTaskId);

    _delegate.SetParameter(_shadowTaskId, HdTokens->params, shadowParams);
}

void
HdxTaskController::_CreateColorizeTask()
{
    // create a colorize task, for use with the SetRenderOutputs API.
    _colorizeTaskId = GetControllerId().AppendChild(
        _tokens->colorizeTask);

    HdxColorizeTaskParams taskParams;

    GetRenderIndex()->InsertTask<HdxColorizeTask>(&_delegate,
        _colorizeTaskId);

    _delegate.SetParameter(_colorizeTaskId, HdTokens->params,
        taskParams);
}

void
HdxTaskController::_CreateColorCorrectionTask()
{
    _colorCorrectionTaskId = GetControllerId().AppendChild(
        _tokens->colorCorrectionTask);

    HdxColorCorrectionTaskParams taskParams;

    GetRenderIndex()->InsertTask<HdxColorCorrectionTask>(&_delegate,
        _colorCorrectionTaskId);

    _delegate.SetParameter(_colorCorrectionTaskId, HdTokens->params,
        taskParams);
}

HdxTaskController::~HdxTaskController()
{
    GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->camera, _cameraId);
    for (SdfPath const& renderTaskId : _renderTaskIds) {
        GetRenderIndex()->RemoveTask(renderTaskId);
    }
    SdfPath const tasks[] = {
        _selectionTaskId,
        _simpleLightTaskId,
        _shadowTaskId,
        _colorizeTaskId,
        _colorCorrectionTaskId
    };
    for (size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); ++i) {
        GetRenderIndex()->RemoveTask(tasks[i]);
    }
    for (auto const& id : _lightIds) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->simpleLight, id);
    }
    for (auto const& id : _renderBufferIds) {
        GetRenderIndex()->RemoveBprim(HdPrimTypeTokens->renderBuffer, id);
    }
}

HdTaskSharedPtrVector const
HdxTaskController::GetTasks()
{
    HdTaskSharedPtrVector tasks;

    // Light - Only run simpleLightTask if the backend supports simpleLight...
    if (GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)) {
        tasks.push_back(GetRenderIndex()->GetTask(_simpleLightTaskId));

        // If shadows are enabled then we add the task to generate the 
        // shadow maps.
        const HdxSimpleLightTaskParams& simpleLightParams =
            _delegate.GetParameter<HdxSimpleLightTaskParams>(
                _simpleLightTaskId, HdTokens->params);

        if (simpleLightParams.enableShadows) {
            tasks.push_back(GetRenderIndex()->GetTask(_shadowTaskId));
        }
    }

    // Render
    for (SdfPath const& renderTaskId : _renderTaskIds) {
        tasks.push_back(GetRenderIndex()->GetTask(renderTaskId));
    }

    // Selection highlighting (overlay as long as this isn't an id render).
    const HdxRenderTaskParams& renderTaskParams =
        _delegate.GetParameter<HdxRenderTaskParams>(
            _renderTaskIds.front(), HdTokens->params);

    if (!renderTaskParams.enableIdRender) {
        tasks.push_back(GetRenderIndex()->GetTask(_selectionTaskId));
    }

    if (_renderBufferIds.size() > 0) {
        HdxColorizeTaskParams colorizeParams =
            _delegate.GetParameter<HdxColorizeTaskParams>(
                    _colorizeTaskId, HdTokens->params);
        if (!colorizeParams.aovName.IsEmpty()) {
            tasks.push_back(GetRenderIndex()->GetTask(_colorizeTaskId));
        }
    }

    // -------------------------------------------------------------------------
    // Color-Correction - should be LAST task in the list
    const HdxColorCorrectionTaskParams& colorCorrectionParams =
        _delegate.GetParameter<HdxColorCorrectionTaskParams>(
            _colorCorrectionTaskId, HdTokens->params);

    bool useColorCorrect = colorCorrectionParams.colorCorrectionMode != 
                           HdxColorCorrectionTokens->disabled &&
                           !colorCorrectionParams.colorCorrectionMode.IsEmpty();

    if (useColorCorrect) {
        tasks.push_back(GetRenderIndex()->GetTask(_colorCorrectionTaskId));
    }

    return tasks;
}

SdfPath
HdxTaskController::_GetAovPath(TfToken const& aov)
{
    std::string str = TfStringPrintf("aov_%s", aov.GetText());
    std::replace(str.begin(), str.end(), ':', '_');
    return GetControllerId().AppendChild(TfToken(str));
}

void
HdxTaskController::SetRenderOutputs(TfTokenVector const& outputs)
{
    if (!GetRenderIndex()->IsBprimTypeSupported(
            HdPrimTypeTokens->renderBuffer)) {
        return;
    }

    // We currently assume all renderTasks use the same viewport dimensions and
    // query only the first renderTask for the value.
    HdxRenderTaskParams firstRenderParams =
        _delegate.GetParameter<HdxRenderTaskParams>(_renderTaskIds.front(),
            HdTokens->params);

    GfVec3i dimensions = GfVec3i(firstRenderParams.viewport[2],
            firstRenderParams.viewport[3], 1);

    SdfPathVector oldRenderBufferIds;
    std::swap(_renderBufferIds, oldRenderBufferIds);

    // Get default AOV descriptors from the render delegate.
    HdAovDescriptorList outputDescs;
    outputDescs.resize(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        outputDescs[i] = GetRenderIndex()->GetRenderDelegate()->
            GetDefaultAovDescriptor(outputs[i]);
    }

    // Insert renderbuffers for the list of outputs, with the name
    // {controller_id}/aov_{name}.
    //
    // To minimize churn, if the renderbuffer already exists just reuse it.
    // If it doesn't exist, insert it.  If any of the previously existing
    // renderbuffers aren't in the new output list, reclaim them.
    for (size_t i = 0; i < outputs.size(); ++i) {
        SdfPath id = _GetAovPath(outputs[i]);
        SdfPathVector::iterator it =
            std::find(oldRenderBufferIds.begin(), oldRenderBufferIds.end(), id);
        if (it != oldRenderBufferIds.end()) {
            // If the AOV already exists, delete it from the old list so we
            // don't reclaim it.
            oldRenderBufferIds.erase(it);
        } else {
            // Otherwise add it to the render index.
            GetRenderIndex()->InsertBprim(HdPrimTypeTokens->renderBuffer,
                &_delegate, id);
            _delegate.SetParameter(id, _tokens->renderBufferDescriptor,
                HdRenderBufferDescriptor());
            GetRenderIndex()->GetChangeTracker().MarkBprimDirty(id,
                HdRenderBuffer::AllDirty);
        }
        _renderBufferIds.push_back(id);

        // Insert the render buffer descriptor based on the aov descriptor.
        HdRenderBufferDescriptor desc =
            _delegate.GetParameter<HdRenderBufferDescriptor>(id,
                _tokens->renderBufferDescriptor);

        if (desc.dimensions != dimensions ||
            desc.format != outputDescs[i].format ||
            desc.multiSampled != outputDescs[i].multiSampled) {

            desc.dimensions = dimensions;
            desc.format = outputDescs[i].format;
            desc.multiSampled = outputDescs[i].multiSampled;

            _delegate.SetParameter(id, _tokens->renderBufferDescriptor, desc);
            GetRenderIndex()->GetChangeTracker().MarkBprimDirty(
                id, HdRenderBuffer::DirtyDescription);
        }
    }

    // Clean up the old (no longer used) renderbuffers.
    for (size_t i = 0; i < oldRenderBufferIds.size(); ++i) {
        GetRenderIndex()->RemoveBprim(HdPrimTypeTokens->renderBuffer,
            oldRenderBufferIds[i]);
    }
    oldRenderBufferIds.clear();

    // Create the aov binding list and set it on the render task.
    HdRenderPassAovBindingVector aovBindings;
    aovBindings.resize(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        SdfPath renderBufferId = _GetAovPath(outputs[i]);

        aovBindings[i].aovName = outputs[i];
        aovBindings[i].clearValue = outputDescs[i].clearValue;
        aovBindings[i].renderBufferId = renderBufferId;
        aovBindings[i].aovSettings = outputDescs[i].aovSettings;
    }

    for (SdfPath const& renderTaskId : _renderTaskIds) {
        HdxRenderTaskParams renderParams =
            _delegate.GetParameter<HdxRenderTaskParams>(renderTaskId,
                HdTokens->params);

        if (renderParams.aovBindings != aovBindings) {
            renderParams.aovBindings = aovBindings;
            _delegate.SetParameter(renderTaskId, HdTokens->params,renderParams);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                renderTaskId, HdChangeTracker::DirtyParams);
        }
    }

    // If only one output was specified, send it to the viewer;
    // otherwise, disable colorization.
    if (outputs.size() == 1) {
        SetViewportRenderOutput(outputs[0]);
    } else {
        SetViewportRenderOutput(TfToken());
    }
}

void
HdxTaskController::SetViewportRenderOutput(TfToken const& name)
{
    if (!GetRenderIndex()->IsBprimTypeSupported(
            HdPrimTypeTokens->renderBuffer)) {
        return;
    }

    HdxColorizeTaskParams params;
    if (name.IsEmpty()) {
        params.aovName = name;
        params.renderBuffer = SdfPath::EmptyPath();
    } else {
        params.aovName = name;
        params.renderBuffer = _GetAovPath(name);
    }

    HdxColorizeTaskParams oldParams =
        _delegate.GetParameter<HdxColorizeTaskParams>(
            _colorizeTaskId, HdTokens->params);

    if (oldParams != params) {
        _delegate.SetParameter(_colorizeTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorizeTaskId, HdChangeTracker::DirtyParams);
    }
}

HdRenderBuffer*
HdxTaskController::GetRenderOutput(TfToken const& name)
{
    if (!GetRenderIndex()->IsBprimTypeSupported(
            HdPrimTypeTokens->renderBuffer)) {
        return nullptr;
    }

    SdfPath renderBufferId = _GetAovPath(name);
    return static_cast<HdRenderBuffer*>(
        GetRenderIndex()->GetBprim(HdPrimTypeTokens->renderBuffer,
            renderBufferId));
}

void
HdxTaskController::SetRenderOutputSettings(TfToken const& name,
                                           HdAovDescriptor const& desc)
{
    if (!GetRenderIndex()->IsBprimTypeSupported(
            HdPrimTypeTokens->renderBuffer)) {
        return;
    }

    // Check if we're setting a value for a nonexistent AOV.
    SdfPath renderBufferId = _GetAovPath(name);
    if (!_delegate.HasParameter(renderBufferId,
                                _tokens->renderBufferDescriptor)) {
        TF_WARN("Render output %s doesn't exist", name.GetText());
        return;
    }

    // HdAovDescriptor contains data for both the renderbuffer descriptor,
    // and the renderpass aov binding.  Update them both.
    HdRenderBufferDescriptor rbDesc =
        _delegate.GetParameter<HdRenderBufferDescriptor>(renderBufferId,
            _tokens->renderBufferDescriptor);

    if (rbDesc.format != desc.format ||
        rbDesc.multiSampled != desc.multiSampled) {

        rbDesc.format = desc.format;
        rbDesc.multiSampled = desc.multiSampled;
        _delegate.SetParameter(renderBufferId,
            _tokens->renderBufferDescriptor, rbDesc);
        GetRenderIndex()->GetChangeTracker().MarkBprimDirty(renderBufferId,
            HdRenderBuffer::DirtyDescription);
    }

    for (SdfPath const& renderTaskId : _renderTaskIds) {
        HdxRenderTaskParams renderParams =
            _delegate.GetParameter<HdxRenderTaskParams>(
                renderTaskId, HdTokens->params);

        for (size_t i = 0; i < renderParams.aovBindings.size(); ++i) {
            if (renderParams.aovBindings[i].renderBufferId == renderBufferId) {
                if (renderParams.aovBindings[i].clearValue != desc.clearValue ||
                    renderParams.aovBindings[i].aovSettings != desc.aovSettings) {
                    renderParams.aovBindings[i].clearValue = desc.clearValue;
                    renderParams.aovBindings[i].aovSettings = desc.aovSettings;
                    _delegate.SetParameter(renderTaskId, HdTokens->params,
                                        renderParams);
                    GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                        renderTaskId, HdChangeTracker::DirtyParams);
                }
                break;
            }
        }
    }
}

void
HdxTaskController::SetCollection(HdRprimCollection const& collection)
{
    // XXX For now we assume the application calling to set a new
    //     collection does not know or setup the material tags and does not
    //     split up the collection according to material tags.
    //     In order to ignore materialTags when comparing collections we need
    //     to copy the old tag into the new collection. Since the provided
    //     collection is const, we need to make a not-ideal copy.
    HdRprimCollection newCollection = collection;

    for (SdfPath const& renderTaskId : _renderTaskIds) {
        HdRprimCollection oldCollection =
            _delegate.GetParameter<HdRprimCollection>(
                renderTaskId, HdTokens->collection);

        TfToken const& oldMaterialTag = oldCollection.GetMaterialTag();
        newCollection.SetMaterialTag(oldMaterialTag);

        if (oldCollection == newCollection) {
            continue;
        }

        _delegate.SetParameter(renderTaskId, HdTokens->collection, 
                               newCollection);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            renderTaskId, HdChangeTracker::DirtyCollection);
    }
}

void
HdxTaskController::SetRenderParams(HdxRenderTaskParams const& params)
{
    for (SdfPath const& renderTaskId : _renderTaskIds) {
        HdxRenderTaskParams oldParams = 
            _delegate.GetParameter<HdxRenderTaskParams>(
                renderTaskId, HdTokens->params);

        // We explicitly ignore params.viewport and params.camera
        HdxRenderTaskParams mergedParams = params;
        mergedParams.camera = oldParams.camera;
        mergedParams.viewport = oldParams.viewport;
        mergedParams.aovBindings = oldParams.aovBindings;

        // XXX For now we assume the app does not provide blending params for
        //     the translucent pass since the app does not specifically add
        //     the translucent pass.
        mergedParams.blendEnable = oldParams.blendEnable;
        mergedParams.blendColorOp = oldParams.blendColorOp;
        mergedParams.blendAlphaOp = oldParams.blendAlphaOp;
        mergedParams.blendColorSrcFactor = oldParams.blendColorSrcFactor;
        mergedParams.blendColorDstFactor = oldParams.blendColorDstFactor;
        mergedParams.blendAlphaDstFactor = oldParams.blendAlphaDstFactor;
        mergedParams.blendAlphaSrcFactor = oldParams.blendAlphaSrcFactor;
        mergedParams.depthMaskEnable = oldParams.depthMaskEnable;

        if (mergedParams != oldParams) {
            _delegate.SetParameter(renderTaskId, HdTokens->params, mergedParams);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                    renderTaskId, HdChangeTracker::DirtyParams);

            // Update shadow task in case materials have been enabled/disabled
            if (GetRenderIndex()->IsSprimTypeSupported(
                HdPrimTypeTokens->simpleLight)) {

                HdxShadowTaskParams oldShParams = 
                    _delegate.GetParameter<HdxShadowTaskParams>(
                    _shadowTaskId, HdTokens->params);

                if (oldShParams.enableSceneMaterials != 
                    mergedParams.enableSceneMaterials) {

                    oldShParams.enableSceneMaterials = 
                        mergedParams.enableSceneMaterials;
                    _delegate.SetParameter(_shadowTaskId, 
                        HdTokens->params, oldShParams);

                    GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                        _shadowTaskId, HdChangeTracker::DirtyParams);
                }
            }
        }
    }
}

void
HdxTaskController::SetShadowParams(HdxShadowTaskParams const& params)
{
    if (!GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)){
        return;
    }

    HdxShadowTaskParams oldParams = 
        _delegate.GetParameter<HdxShadowTaskParams>(
            _shadowTaskId, HdTokens->params);

    HdxShadowTaskParams mergedParams = params;
    mergedParams.camera = oldParams.camera;

    if (mergedParams != oldParams) {
        _delegate.SetParameter(_shadowTaskId, HdTokens->params, mergedParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _shadowTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetEnableShadows(bool enable)
{
    if (!GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)){
        return;
    }

    HdxSimpleLightTaskParams params =
        _delegate.GetParameter<HdxSimpleLightTaskParams>(
            _simpleLightTaskId, HdTokens->params);

    if (params.enableShadows != enable) {
        params.enableShadows = enable;
        _delegate.SetParameter(_simpleLightTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetEnableSelection(bool enable)
{
    HdxSelectionTaskParams params =
        _delegate.GetParameter<HdxSelectionTaskParams>(
            _selectionTaskId, HdTokens->params);

    if (params.enableSelection != enable) {
        params.enableSelection = enable;
        _delegate.SetParameter(_selectionTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _selectionTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetSelectionColor(GfVec4f const& color)
{
    HdxSelectionTaskParams params =
        _delegate.GetParameter<HdxSelectionTaskParams>(
            _selectionTaskId, HdTokens->params);

    if (params.selectionColor != color) {
        params.selectionColor = color;
        _delegate.SetParameter(_selectionTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _selectionTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetPickResolution(unsigned int size)
{
    _intersector->SetResolution(GfVec2i(size, size));
}

bool
HdxTaskController::TestIntersection(
        HdEngine* engine,
        HdRprimCollection const& collection,
        HdxIntersector::Params const& qparams,
        TfToken const& intersectionMode,
        HdxIntersector::HitVector *allHits)
{
    if (allHits == nullptr) {
        TF_CODING_ERROR("Null hit vector passed to TestIntersection");
        return false;
    }

    HdxIntersector::Result result;
    if (!_intersector->Query(qparams, collection, engine, &result)) {
        return false;
    }

    if (intersectionMode == HdxIntersectionModeTokens->nearestToCenter) {
        HdxIntersector::Hit hit;
        if (!result.ResolveNearestToCenter(&hit)) {
            return false;
        }
        allHits->push_back(hit);
    } else if (intersectionMode == HdxIntersectionModeTokens->nearestToCamera) {
        HdxIntersector::Hit hit;
        if (!result.ResolveNearestToCamera(&hit)) {
            return false;
        }
        allHits->push_back(hit);
    } else if (intersectionMode == HdxIntersectionModeTokens->unique) {
        HdxIntersector::HitSet hits;
        if (!result.ResolveUnique(&hits)) {
            return false;
        }
        allHits->assign(hits.begin(), hits.end());
    } else if (intersectionMode == HdxIntersectionModeTokens->all) {
        if (!result.ResolveAll(allHits)) {
            return false;
        }
    } else {
        TF_CODING_ERROR("Unrecognized interesection mode '%s'",
            intersectionMode.GetText());
        return false;
    }

    return true;
}

void
HdxTaskController::SetLightingState(GlfSimpleLightingContextPtr const& src)
{
    // If the backend doesn't support simpleLight, no need to set parameters
    // for simpleLightTask, or create simpleLight prims for lights in the
    // lighting context.
    if (!GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)){
        return;
    }

    if (!src) {
        TF_CODING_ERROR("Null lighting context");
        return;
    }

    GlfSimpleLightVector const& lights = src->GetLights();

    // HdxTaskController inserts a set of light prims to represent the lights
    // passed in through the simple lighting context. These are managed by
    // the task controller, and not by the scene; they represent transient
    // application state such as camera lights.
    //
    // The light pool can be re-used as lights change, but we need to make sure
    // we have the right number of light prims. Add them as necessary until
    // there are enough light prims to represent the light context.
    while (_lightIds.size() < lights.size()) {
        SdfPath lightId = GetControllerId().AppendChild(TfToken(
            TfStringPrintf("light%d", (int)_lightIds.size())));
        _lightIds.push_back(lightId);

        GetRenderIndex()->InsertSprim(HdPrimTypeTokens->simpleLight, 
            &_delegate,
            lightId);

        // After inserting a light, initialize its parameters and mark the light
        // as dirty.
        _delegate.SetParameter(lightId, HdLightTokens->transform,
            VtValue());
        _delegate.SetParameter(lightId, HdLightTokens->shadowParams,
            HdxShadowParams());
        _delegate.SetParameter(lightId, HdLightTokens->shadowCollection,
            VtValue());
        _delegate.SetParameter(lightId, HdLightTokens->params,
            GlfSimpleLight());

        // Note: Marking the shadowCollection as dirty (included in AllDirty)
        // will mark the geometry collection dirty.
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(lightId,
            HdLight::AllDirty);
    }

    // If the light pool is too big for the light context, remove the extra
    // sprims.
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->simpleLight,
            _lightIds.back());

        _lightIds.pop_back();
    }

    // Update light Sprims to match the lights passed in through the context;
    // hydra simpleLight prims store a GlfSimpleLight as their "params" field.
    for (size_t i = 0; i < lights.size(); ++i) {
        GlfSimpleLight lt = _delegate.GetParameter<GlfSimpleLight>(
            _lightIds[i], HdLightTokens->params);

        if (lt != lights[i]) {
            _delegate.SetParameter(_lightIds[i], HdLightTokens->params,
                lights[i]);

            GetRenderIndex()->GetChangeTracker().MarkSprimDirty(
                _lightIds[i], HdLight::DirtyParams);
        }
    }

    // In addition to lights, the lighting context contains material parameters.
    // These are passed in through the simple light task's "params" field, so
    // we need to update that field if the material parameters changed.
    //
    // It's unfortunate that the lighting context is split this way.
    HdxSimpleLightTaskParams lightParams =
        _delegate.GetParameter<HdxSimpleLightTaskParams>(_simpleLightTaskId,
            HdTokens->params);

    if (lightParams.sceneAmbient != src->GetSceneAmbient() ||
        lightParams.material != src->GetMaterial()) {

        lightParams.sceneAmbient = src->GetSceneAmbient();
        lightParams.material = src->GetMaterial();

        _delegate.SetParameter(
            _simpleLightTaskId, HdTokens->params, lightParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetCameraMatrices(GfMatrix4d const& viewMatrix,
                                     GfMatrix4d const& projMatrix)
{
    GfMatrix4d oldView = _delegate.GetParameter<GfMatrix4d>(_cameraId,
        HdCameraTokens->worldToViewMatrix);

    if (viewMatrix != oldView) {
        // Cache the new view matrix
        _delegate.SetParameter(_cameraId, HdCameraTokens->worldToViewMatrix,
            viewMatrix);
        // Invalidate the camera
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyViewMatrix);
    }

    GfMatrix4d oldProj = _delegate.GetParameter<GfMatrix4d>(_cameraId,
        HdCameraTokens->projectionMatrix);

    if (projMatrix != oldProj) {
        // Cache the new proj matrix
        _delegate.SetParameter(_cameraId, HdCameraTokens->projectionMatrix,
            projMatrix);
        // Invalidate the camera
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyProjMatrix);
    }
}

void
HdxTaskController::SetCameraViewport(GfVec4d const& viewport)
{
    bool viewportChanged = false;

    for (SdfPath const& renderTaskId : _renderTaskIds) {
        HdxRenderTaskParams params =
            _delegate.GetParameter<HdxRenderTaskParams>(
                renderTaskId, HdTokens->params);

        if (params.viewport == viewport) {
            continue;
        }

        viewportChanged = true;
        params.viewport = viewport;
        _delegate.SetParameter(renderTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            renderTaskId, HdChangeTracker::DirtyParams);
    }

    if (!viewportChanged) {
        return;
    }

    if (GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)) {
        // The shadow and camera viewport should be the same
        // so we don't have to double check what the shadow task has.
        HdxShadowTaskParams params =
            _delegate.GetParameter<HdxShadowTaskParams>(
                _shadowTaskId, HdTokens->params);
        params.viewport = viewport;
        _delegate.SetParameter(_shadowTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _shadowTaskId, HdChangeTracker::DirtyParams);
    }

    if (_renderBufferIds.size() > 0) {
        GfVec3i dimensions = GfVec3i(viewport[2], viewport[3], 1);

        for (auto const& id : _renderBufferIds) {
            HdRenderBufferDescriptor desc =
                _delegate.GetParameter<HdRenderBufferDescriptor>(id,
                    _tokens->renderBufferDescriptor);
            if (desc.dimensions != dimensions) {
                desc.dimensions = dimensions;
                _delegate.SetParameter(id, _tokens->renderBufferDescriptor,
                    desc);
                GetRenderIndex()->GetChangeTracker().MarkBprimDirty(id,
                    HdRenderBuffer::DirtyDescription);
            }
        }
    }
}

void
HdxTaskController::SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes)
{
    // Cache the clip planes
    std::vector<GfVec4d> oldClipPlanes =
        _delegate.GetParameter<std::vector<GfVec4d>>(_cameraId,
            HdCameraTokens->clipPlanes);

    if (oldClipPlanes != clipPlanes) {
        _delegate.SetParameter(_cameraId, HdCameraTokens->clipPlanes,
            clipPlanes);
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyClipPlanes);
    }
}

void
HdxTaskController::SetCameraWindowPolicy(
    CameraUtilConformWindowPolicy windowPolicy)
{
    // Cache the window policy, if needed
    const CameraUtilConformWindowPolicy oldPolicy =
        _delegate.GetParameter<CameraUtilConformWindowPolicy>(
            _cameraId, HdCameraTokens->windowPolicy);

    if (oldPolicy != windowPolicy) {
        _delegate.SetParameter(_cameraId, HdCameraTokens->windowPolicy,
            windowPolicy);
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyWindowPolicy);
    }
}

bool
HdxTaskController::IsConverged() const
{
    // Pass this call through to HdxRenderTask's IsConverged().
    if (_renderBufferIds.size() > 0) {
        HdxColorizeTaskParams colorizeParams =
            _delegate.GetParameter<HdxColorizeTaskParams>(
                _colorizeTaskId, HdTokens->params);
        if (!colorizeParams.aovName.IsEmpty()) {
            return static_cast<HdxColorizeTask*>(
                    GetRenderIndex()->GetTask(_colorizeTaskId).get())
                ->IsConverged();
        }
    }
    return static_cast<HdxRenderTask*>(
        GetRenderIndex()->GetTask(_renderTaskIds.front()).get())
        ->IsConverged();
}

void 
HdxTaskController::SetColorCorrectionParams(
    HdxColorCorrectionTaskParams const& params)
{
    HdxColorCorrectionTaskParams oldParams = 
        _delegate.GetParameter<HdxColorCorrectionTaskParams>(
            _colorCorrectionTaskId, HdTokens->params);

    if (params != oldParams) {
        _delegate.SetParameter(_colorCorrectionTaskId, HdTokens->params,params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorCorrectionTaskId, HdChangeTracker::DirtyParams);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
