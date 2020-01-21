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
#include "pxr/imaging/hdx/aovResolveTask.h"
#include "pxr/imaging/hdx/colorizeSelectionTask.h"
#include "pxr/imaging/hdx/colorizeTask.h"
#include "pxr/imaging/hdx/colorChannelTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/oitRenderTask.h"
#include "pxr/imaging/hdx/oitResolveTask.h"
#include "pxr/imaging/hdx/oitVolumeRenderTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/pickFromRenderBufferTask.h"
#include "pxr/imaging/hdx/presentTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/textureRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // tasks
    (simpleLightTask)
    (shadowTask)
    (selectionTask)
    (colorizeTask)
    (colorizeSelectionTask)
    (oitResolveTask)
    (colorCorrectionTask)
    (colorChannelTask)
    (pickTask)
    (pickFromRenderBufferTask)
    (aovColorResolveTask)
    (aovDepthResolveTask)
    (presentTask)

    // global camera
    (camera)

    // For the internal delegate...
    (renderBufferDescriptor)
    (renderTags)

    // for the stage orientation
    (StageOrientation)
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
GfMatrix4d
HdxTaskController::_Delegate::GetTransform(SdfPath const& id)
{
    // We expect this to be called only for the free cam.
    VtValue val = GetCameraParamValue(id, HdCameraTokens->worldToViewMatrix);
    GfMatrix4d xform(1.0);
    if (val.IsHolding<GfMatrix4d>()) {
        xform = val.Get<GfMatrix4d>().GetInverse(); // camera to world
    } else {
        TF_CODING_ERROR(
            "Unexpected call to GetTransform for %s in HdxTaskController's "
            "internal scene delegate.\n", id.GetText());
    }
    return xform;
}

/* virtual */
VtValue
HdxTaskController::_Delegate::GetCameraParamValue(SdfPath const& id, 
                                                  TfToken const& key)
{   
    if (key == HdCameraTokens->worldToViewMatrix ||
        key == HdCameraTokens->projectionMatrix ||
        key == HdCameraTokens->clipPlanes ||
        key == HdCameraTokens->windowPolicy) {

        return Get(id, key);
    } else {
        // XXX: For now, skip handling physical params on the free cam.
        return VtValue();
    }
}

/* virtual */
VtValue
HdxTaskController::_Delegate::GetLightParamValue(SdfPath const& id, 
                                                  TfToken const& paramName)
{   
    return Get(id, paramName);
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
HdRenderBufferDescriptor
HdxTaskController::_Delegate::GetRenderBufferDescriptor(SdfPath const& id)
{
    return GetParameter<HdRenderBufferDescriptor>(id,
                _tokens->renderBufferDescriptor);
}


/* virtual */
TfTokenVector
HdxTaskController::_Delegate::GetTaskRenderTags(SdfPath const& taskId)
{
    if (HasParameter(taskId, _tokens->renderTags)) {
        return GetParameter<TfTokenVector>(taskId, _tokens->renderTags);
    }
    return TfTokenVector();
}


// ---------------------------------------------------------------------------
// Task controller implementation.

static bool
_IsStormRenderingBackend(HdRenderIndex const *index)
{
    if(!dynamic_cast<HdStRenderDelegate*>(index->GetRenderDelegate())) {
        return false;
    }

    return true;
}

static GfVec3i
_ViewportToAovDimension(const GfVec4d& viewport)
{
    // Get the viewport dimensions (for renderbuffer allocation)
    // XXX Sometimes viewport is set very large to do cam-zooming tricks.
    // That would result in extra-large / incorrect Aov dimensions.
    // What we really want here is the window size, but non of the hydra 
    // clients currently pass in window size.
    // The viewport XY (lower left corner) may also have been modified for e.g.
    // a 'camera mask border'.
    int w = viewport[2] + viewport[0];
    int h = viewport[3] + viewport[1];
    return GfVec3i(w, h, 1);
}

HdxTaskController::HdxTaskController(HdRenderIndex *renderIndex,
                                     SdfPath const& controllerId)
    : _index(renderIndex)
    , _controllerId(controllerId)
    , _delegate(renderIndex, controllerId)
{
    _CreateRenderGraph();
    _LoadDefaultDomeLightTexture();
}

HdxTaskController::~HdxTaskController()
{
    GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->camera, _freeCamId);
    SdfPath const tasks[] = {
        _oitResolveTaskId,
        _selectionTaskId,
        _simpleLightTaskId,
        _shadowTaskId,
        _colorizeSelectionTaskId,
        _colorizeTaskId,
        _colorCorrectionTaskId,
        _colorChannelTaskId,
        _pickTaskId,
        _pickFromRenderBufferTaskId,
        _aovColorResolveTaskId,
        _aovDepthResolveTaskId,
        _presentTaskId
    };
    for (size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); ++i) {
        if (!tasks[i].IsEmpty()) {
            GetRenderIndex()->RemoveTask(tasks[i]);
        }
    }
    for (auto const& id : _renderTaskIds) {
        GetRenderIndex()->RemoveTask(id);
    }
    for (auto const& id : _lightIds) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->simpleLight, id);
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->domeLight, id);
    }
    for (auto const& id : _aovBufferIds) {
        GetRenderIndex()->RemoveBprim(HdPrimTypeTokens->renderBuffer, id);
    }
}

void
HdxTaskController::_CreateRenderGraph()
{
    // We create camera and tasks here, but lights are created lazily by
    // SetLightingState. Camera needs to be created first, since it's a
    // parameter of most tasks.
    _CreateCamera();

    // XXX: The general assumption is that we have "stream" backends which are
    // rasterization based and have their own rules, like multipass for
    // transparency; and other backends are more single-pass.  As render
    // delegate capabilities evolve, we'll need a more complicated switch
    // than this...
    if (_IsStormRenderingBackend(GetRenderIndex())) {
        // Rendering rendergraph
        _CreateLightingTask();
        _CreateShadowTask();
        _renderTaskIds.push_back(_CreateRenderTask(
            HdMaterialTagTokens->defaultMaterialTag));

        _renderTaskIds.push_back(_CreateRenderTask(
            HdxMaterialTagTokens->additive));
        _renderTaskIds.push_back(_CreateRenderTask(
            HdxMaterialTagTokens->translucent));
        _renderTaskIds.push_back(_CreateRenderTask(
            HdStMaterialTagTokens->volume));

        _CreateOitResolveTask();

        _aovColorResolveTaskId = 
            _CreateAovResolveTask(_tokens->aovColorResolveTask);
        _aovDepthResolveTaskId =
            _CreateAovResolveTask(_tokens->aovDepthResolveTask);

        _CreateSelectionTask();
        _CreateColorCorrectionTask();
        _CreateColorChannelTask();
        _CreatePresentTask();

        // Picking rendergraph
        _CreatePickTask();

        // XXX AOVs are OFF by default for Storm TaskController because hybrid
        // rendering in Presto spawns an ImagineGLEngine, which creates a task
        // controlller. But the Hydrid rendering setups are not yet AOV ready.
        // App (UsdView) for now calls engine->SetRendererAov(color) to enable.
        // SetRenderOutputs({HdAovTokens->color});
    } else {
        _renderTaskIds.push_back(_CreateRenderTask(TfToken()));

        if (_AovsSupported()) {
            _CreateColorizeTask();
            _CreateColorizeSelectionTask();
            _CreatePickFromRenderBufferTask();
        }

        _CreateColorCorrectionTask();
        _CreateColorChannelTask();

        // Initialize the AOV system to render color. Note:
        // SetRenderOutputs special-cases color to include support for
        // depth-compositing and selection highlighting/picking.
        if (_AovsSupported()) {
            SetRenderOutputs({HdAovTokens->color});
        }
    }
}

void
HdxTaskController::_CreateCamera()
{
    // Create a default camera, driven by SetFreeCameraMatrices.
    _freeCamId = GetControllerId().AppendChild(_tokens->camera);
    GetRenderIndex()->InsertSprim(HdPrimTypeTokens->camera,
        &_delegate, _freeCamId);

    _delegate.SetParameter(_freeCamId, HdCameraTokens->windowPolicy,
        VtValue(CameraUtilFit));
    _delegate.SetParameter(_freeCamId, HdCameraTokens->worldToViewMatrix,
        VtValue(GfMatrix4d(1.0)));
    _delegate.SetParameter(_freeCamId, HdCameraTokens->projectionMatrix,
        VtValue(GfMatrix4d(1.0)));
    _delegate.SetParameter(_freeCamId, HdCameraTokens->clipPlanes,
        VtValue(std::vector<GfVec4d>()));
}

SdfPath
HdxTaskController::_GetRenderTaskPath(TfToken const& materialTag) const
{
    std::string str = TfStringPrintf("renderTask_%s",
        materialTag.GetText());
    std::replace(str.begin(), str.end(), ':', '_');
    return GetControllerId().AppendChild(TfToken(str));
}

SdfPath
HdxTaskController::_CreateRenderTask(TfToken const& materialTag)
{
    SdfPath taskId = _GetRenderTaskPath(materialTag);

    HdxRenderTaskParams renderParams;
    renderParams.camera = _freeCamId;
    renderParams.viewport = GfVec4d(0,0,1,1);

    // Set the blend state based on material tag.
    _SetBlendStateForMaterialTag(materialTag, &renderParams);

    HdRprimCollection collection(HdTokens->geometry,
                                 HdReprSelector(HdReprTokens->smoothHull),
                                 /*forcedRepr*/ false,
                                 materialTag);
    collection.SetRootPath(SdfPath::AbsoluteRootPath());

    if (materialTag == HdMaterialTagTokens->defaultMaterialTag || 
        materialTag == HdxMaterialTagTokens->additive ||
        materialTag.IsEmpty()) {
        GetRenderIndex()->InsertTask<HdxRenderTask>(&_delegate, taskId);
    } else if (materialTag == HdxMaterialTagTokens->translucent) {
        GetRenderIndex()->InsertTask<HdxOitRenderTask>(&_delegate, taskId);
    } else if (materialTag == HdStMaterialTagTokens->volume) {
        GetRenderIndex()->InsertTask<HdxOitVolumeRenderTask>(&_delegate, taskId);
    }

    // Create an initial set of render tags in case the user doesn't set any
    TfTokenVector renderTags = { HdRenderTagTokens->geometry };

    _delegate.SetParameter(taskId, HdTokens->params, renderParams);
    _delegate.SetParameter(taskId, HdTokens->collection, collection);
    _delegate.SetParameter(taskId, HdTokens->renderTags, renderTags);

    return taskId;
}

void
HdxTaskController::_SetBlendStateForMaterialTag(TfToken const& materialTag,
                                        HdxRenderTaskParams *renderParams) const
{
    if (!TF_VERIFY(renderParams)) {
        return;
    }

    if (materialTag == HdxMaterialTagTokens->additive) {
        // Additive blend -- so no sorting of drawItems is needed
        renderParams->blendEnable = true;
        // We are setting all factors to ONE, This means we are expecting
        // pre-multiplied alpha coming out of the shader: vec4(rgb*a, a).
        // Setting ColorSrc to HdBlendFactorSourceAlpha would give less
        // control on the shader side, since it means we would force a
        // pre-multiplied alpha step on the color coming out of the shader.
        renderParams->blendColorOp = HdBlendOpAdd;
        renderParams->blendAlphaOp = HdBlendOpAdd;
        renderParams->blendColorSrcFactor = HdBlendFactorOne;
        renderParams->blendColorDstFactor = HdBlendFactorOne;
        renderParams->blendAlphaSrcFactor = HdBlendFactorOne;
        renderParams->blendAlphaDstFactor = HdBlendFactorOne;

        // Translucent objects should not block each other in depth buffer
        renderParams->depthMaskEnable = false;

        // Since we are using alpha blending, we disable screen door
        // transparency for this renderpass.
        renderParams->enableAlphaToCoverage = false;
    } else if (materialTag == HdxMaterialTagTokens->translucent ||
               materialTag == HdStMaterialTagTokens->volume) {
        // Order Independent Transparency blend state or its first render pass.
        renderParams->blendEnable = false;
        renderParams->enableAlphaToCoverage = false;
        renderParams->depthMaskEnable = false;
    } else {
        renderParams->blendEnable = false;
        renderParams->depthMaskEnable = true;
        renderParams->enableAlphaToCoverage = true;
    }
}

void
HdxTaskController::_CreateOitResolveTask()
{
    _oitResolveTaskId = GetControllerId().AppendChild(_tokens->oitResolveTask);

    GetRenderIndex()->InsertTask<HdxOitResolveTask>(&_delegate,
        _oitResolveTaskId);
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
HdxTaskController::_CreateColorizeSelectionTask()
{
    // Create a post-process selection highlighting task.
    _colorizeSelectionTaskId = GetControllerId().AppendChild(
        _tokens->colorizeSelectionTask);

    HdxColorizeSelectionTaskParams selectionParams;
    selectionParams.enableSelection = true;
    selectionParams.selectionColor = GfVec4f(1,1,0,1);
    selectionParams.locateColor = GfVec4f(0,0,1,1);

    GetRenderIndex()->InsertTask<HdxColorizeSelectionTask>(&_delegate,
        _colorizeSelectionTaskId);

    _delegate.SetParameter(_colorizeSelectionTaskId, HdTokens->params,
        selectionParams);
}

void
HdxTaskController::_CreateLightingTask()
{
    // Simple lighting task uses lighting state from Sprims.
    _simpleLightTaskId = GetControllerId().AppendChild(
        _tokens->simpleLightTask);

    HdxSimpleLightTaskParams simpleLightParams;
    simpleLightParams.cameraPath = _freeCamId;

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
    shadowParams.camera = _freeCamId;

    GetRenderIndex()->InsertTask<HdxShadowTask>(&_delegate, _shadowTaskId);

    TfTokenVector renderTags = { HdRenderTagTokens->geometry };

    _delegate.SetParameter(_shadowTaskId, HdTokens->params, shadowParams);
    _delegate.SetParameter(_shadowTaskId, _tokens->renderTags, renderTags);
}

void
HdxTaskController::_CreateColorizeTask()
{
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

void
HdxTaskController::_CreateColorChannelTask()
{
    _colorChannelTaskId = GetControllerId().AppendChild(
        _tokens->colorChannelTask);

    HdxColorChannelTaskParams taskParams;

    GetRenderIndex()->InsertTask<HdxColorChannelTask>(&_delegate,
        _colorChannelTaskId);

    _delegate.SetParameter(_colorChannelTaskId, HdTokens->params,
        taskParams);
}

void
HdxTaskController::_CreatePickTask()
{
    _pickTaskId = GetControllerId().AppendChild(
        _tokens->pickTask);

    HdxPickTaskParams taskParams;

    GetRenderIndex()->InsertTask<HdxPickTask>(&_delegate, _pickTaskId);

    _delegate.SetParameter(_pickTaskId, HdTokens->params, taskParams);
}

void
HdxTaskController::_CreatePickFromRenderBufferTask()
{
    _pickFromRenderBufferTaskId = GetControllerId().AppendChild(
        _tokens->pickFromRenderBufferTask);

    HdxPickFromRenderBufferTaskParams taskParams;
    taskParams.cameraId = _freeCamId;

    GetRenderIndex()->InsertTask<HdxPickFromRenderBufferTask>(&_delegate,
        _pickFromRenderBufferTaskId);

    _delegate.SetParameter(_pickFromRenderBufferTaskId, HdTokens->params,
        taskParams);
}

SdfPath 
HdxTaskController::_CreateAovResolveTask(TfToken const& aovName)
{
    SdfPath aovResolveTaskId = GetControllerId().AppendChild(aovName);

    HdxAovResolveTaskParams taskParams;

    GetRenderIndex()->InsertTask<HdxAovResolveTask>(&_delegate,
        aovResolveTaskId);

    _delegate.SetParameter(aovResolveTaskId, HdTokens->params,
        taskParams);

    return aovResolveTaskId;    
}

void
HdxTaskController::_CreatePresentTask()
{
    _presentTaskId = GetControllerId().AppendChild(
        _tokens->presentTask);

    HdxPresentTaskParams taskParams;

    GetRenderIndex()->InsertTask<HdxPresentTask>(&_delegate,
        _presentTaskId);

    _delegate.SetParameter(_presentTaskId, HdTokens->params,
        taskParams);
}

bool
HdxTaskController::_ShadowsEnabled() const
{
    if (_simpleLightTaskId.IsEmpty())
        return false;

    const HdxSimpleLightTaskParams& simpleLightParams =
        _delegate.GetParameter<HdxSimpleLightTaskParams>(
                _simpleLightTaskId, HdTokens->params);

    // Only enable the shadow task (which renders shadow maps) if shadows are
    // enabled.
    return simpleLightParams.enableShadows;
}

bool
HdxTaskController::_SelectionEnabled() const
{
    if (_renderTaskIds.empty())
        return false;

    const HdxRenderTaskParams& renderTaskParams =
        _delegate.GetParameter<HdxRenderTaskParams>(
            _renderTaskIds.front(), HdTokens->params);

    // Disable selection highlighting when we're rendering ID buffers.
    return !renderTaskParams.enableIdRender;
}

bool
HdxTaskController::_ColorizeSelectionEnabled() const
{
    if (_viewportAov == HdAovTokens->color) {
        return true;
    }
    return false;
}

bool
HdxTaskController::_ColorCorrectionEnabled() const
{
    if (_colorCorrectionTaskId.IsEmpty())
        return false;

    const HdxColorCorrectionTaskParams& colorCorrectionParams =
        _delegate.GetParameter<HdxColorCorrectionTaskParams>(
            _colorCorrectionTaskId, HdTokens->params);

    bool useColorCorrect = colorCorrectionParams.colorCorrectionMode != 
                           HdxColorCorrectionTokens->disabled &&
                           !colorCorrectionParams.colorCorrectionMode.IsEmpty();
    return useColorCorrect;
}

bool
HdxTaskController::_ColorChannelEnabled() const
{
    if (_colorChannelTaskId.IsEmpty()) {
        return false;
    }

    const HdxColorChannelTaskParams& colorChannelParams =
        _delegate.GetParameter<HdxColorChannelTaskParams>(
            _colorChannelTaskId, HdTokens->params);

    // Disable the task if the chosen channel is "color"
    bool useColorChannel = colorChannelParams.channel != 
                           HdxColorChannelTokens->color &&
                           !colorChannelParams.channel.IsEmpty();
    return useColorChannel;
}

bool
HdxTaskController::_ColorizeQuantizationEnabled() const
{
    if (_colorizeTaskId.IsEmpty())
        return false;

    const HdxColorizeTaskParams& params =
        _delegate.GetParameter<HdxColorizeTaskParams>(
            _colorizeTaskId, HdTokens->params);

    return params.applyColorQuantization;
}

bool
HdxTaskController::_AovsSupported() const
{
    return GetRenderIndex()->IsBprimTypeSupported(
        HdPrimTypeTokens->renderBuffer);
}

HdTaskSharedPtrVector const
HdxTaskController::GetRenderingTasks() const
{
    HdTaskSharedPtrVector tasks;

    /* The superset of tasks we can run, in order, is:
     * - simpleLightTaskId
     * - shadowTaskId
     * - renderTaskIds (There may be more than one)
     * - aovResolveTaskId (There may be more than one)
     * - selectionTaskId
     * - colorizeTaskId
     * - colorizeSelectionTaskId
     * - colorCorrectionTaskId
     * - colorChannelTaskId
     * - PresentTask
     *
     * Some of these won't be populated, based on the backend type.
     * Additionally, shadow, selection, color correction and color channel can
     * be conditionally disabled.
     *
     * See _CreateRenderGraph for more details.
     */

    if (!_simpleLightTaskId.IsEmpty())
        tasks.push_back(GetRenderIndex()->GetTask(_simpleLightTaskId));

    if (!_shadowTaskId.IsEmpty() && _ShadowsEnabled())
        tasks.push_back(GetRenderIndex()->GetTask(_shadowTaskId));

    // Perform draw calls
    if (!_renderTaskIds.empty()) {
        SdfPath volumeId = _GetRenderTaskPath(HdStMaterialTagTokens->volume);

        // Render opaque prims, additive and translucent blended prims.
        // Skip volume prims, because volume rendering reads from the depth
        // buffer so we must resolve depth first first.
        for (SdfPath const& id : _renderTaskIds) {
            if (id != volumeId) {
                tasks.push_back(GetRenderIndex()->GetTask(id));
            }
        }

        // Resolve color multi-sample Aov.
        if (!_aovColorResolveTaskId.IsEmpty()) {
            tasks.push_back(GetRenderIndex()->GetTask(_aovColorResolveTaskId));
        }
        // Resolve depth multi-sample Aov
        if (!_aovDepthResolveTaskId.IsEmpty()) {
            tasks.push_back(GetRenderIndex()->GetTask(_aovDepthResolveTaskId));
        }

        // Render volume prims
        if (std::find(_renderTaskIds.begin(), _renderTaskIds.end(), volumeId) 
                != _renderTaskIds.end()) {
            tasks.push_back(GetRenderIndex()->GetTask(volumeId));
        }
    }

    // Merge translucent and volume pixels into color target
    if (!_oitResolveTaskId.IsEmpty()) {
        tasks.push_back(GetRenderIndex()->GetTask(_oitResolveTaskId));
    }

    if (!_selectionTaskId.IsEmpty() && _SelectionEnabled())
        tasks.push_back(GetRenderIndex()->GetTask(_selectionTaskId));

    // Take path-tracer CPU pixels and render to screen
    if (!_colorizeTaskId.IsEmpty()) {
        // XXX Colorize already applies color quantization since it renders
        // directly to 8bit framebuffer. But it does not quantize the background
        // since it alpha-blend the aov on top of the background.
        // Therefor we must first color correct the background.
        // Future work is to stop Colorize from rendering to the 8bit FB.
        // Instead it should Colorize into a render target that color correction
        // can then quantize.
        if (_ColorizeQuantizationEnabled() && _ColorCorrectionEnabled()) {
            tasks.push_back(GetRenderIndex()->GetTask(_colorCorrectionTaskId));
        }

        tasks.push_back(GetRenderIndex()->GetTask(_colorizeTaskId));
    }

    if (!_colorizeSelectionTaskId.IsEmpty() && _ColorizeSelectionEnabled())
        tasks.push_back(GetRenderIndex()->GetTask(_colorizeSelectionTaskId));

    // Apply color correction / grading (convert to display colors)
    // XXX Skip is Colorize has already quantized the colors.
    if (_ColorCorrectionEnabled() && !_ColorizeQuantizationEnabled()) {
        tasks.push_back(GetRenderIndex()->GetTask(_colorCorrectionTaskId));
    }

    // Apply color channel filtering
    if (_ColorChannelEnabled()) {
        tasks.push_back(GetRenderIndex()->GetTask(_colorChannelTaskId));
    }

    // Render pixels to screen
    if (!_presentTaskId.IsEmpty())
        tasks.push_back(GetRenderIndex()->GetTask(_presentTaskId));

    return tasks;
}

HdTaskSharedPtrVector const
HdxTaskController::GetPickingTasks() const
{
    HdTaskSharedPtrVector tasks;
    if (!_pickTaskId.IsEmpty())
        tasks.push_back(GetRenderIndex()->GetTask(_pickTaskId));
    if (!_pickFromRenderBufferTaskId.IsEmpty())
        tasks.push_back(GetRenderIndex()->GetTask(_pickFromRenderBufferTaskId));

    return tasks;
}

SdfPath
HdxTaskController::_GetAovPath(TfToken const& aov) const
{
    std::string str = TfStringPrintf("aov_%s", aov.GetText());
    std::replace(str.begin(), str.end(), ':', '_');
    return GetControllerId().AppendChild(TfToken(str));
}

void
HdxTaskController::_LoadDefaultDomeLightTexture()
{
    // load and attach the texture for the Default DomeLight Texture
    GlfTextureHandleRefPtr texture = 
                GlfTextureRegistry::GetInstance().GetTextureHandle(
                HdxPackageDefaultDomeLightTexture(),
                GlfImage::ImageOriginLocation::OriginUpperLeft);

    _defaultDomeLightTextureResource = HdTextureResourceSharedPtr(
                new HdStSimpleTextureResource(texture, HdTextureType::Uv, 
                HdWrap::HdWrapRepeat, HdWrap::HdWrapRepeat, 
                HdWrap::HdWrapRepeat, HdMinFilter::HdMinFilterLinear, 
                HdMagFilter::HdMagFilterLinear, 
                0.0f));
}

void 
HdxTaskController::_SetParameters(SdfPath const& pathName, 
                                    GlfSimpleLight const& light)
{
    _delegate.SetParameter(pathName, HdLightTokens->transform,
        VtValue(light.GetTransform()));
    _delegate.SetParameter(pathName, HdLightTokens->shadowParams,
        HdxShadowParams());
    _delegate.SetParameter(pathName, HdLightTokens->shadowCollection,
        VtValue());
    _delegate.SetParameter(pathName, HdLightTokens->params,
        light);

    // if we are setting the parameters for the dome light we need to add the 
    // default dome light texture resource.
    if (light.IsDomeLight()) {
        _delegate.SetParameter(pathName, HdLightTokens->textureResource, 
            _defaultDomeLightTextureResource);
    }
}

GlfSimpleLight
HdxTaskController::_GetLightAtId(size_t const& pathIdx)
{
    GlfSimpleLight light = GlfSimpleLight();
    if (pathIdx < _lightIds.size()) {
        light = _delegate.GetParameter<GlfSimpleLight>(
            _lightIds[pathIdx], HdLightTokens->params);
    }
    return light;
}

void 
HdxTaskController::_RemoveLightSprim(size_t const& pathIdx)
{
    if (pathIdx < _lightIds.size()) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->simpleLight, 
                        _lightIds[pathIdx]);
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->domeLight, 
                        _lightIds[pathIdx]);
    }
}

void 
HdxTaskController::_ReplaceLightSprim(size_t const& pathIdx, 
                        GlfSimpleLight const& light, SdfPath const& pathName)
{                
    _RemoveLightSprim(pathIdx);
    if (light.IsDomeLight()) {
        GetRenderIndex()->InsertSprim(HdPrimTypeTokens->domeLight,
                        &_delegate, pathName);
    }
    else {
        GetRenderIndex()->InsertSprim(HdPrimTypeTokens->simpleLight,
                            &_delegate, pathName);
    }
    // set the parameters for lights[i] and mark as dirty
    _SetParameters(pathName, light);            
    GetRenderIndex()->GetChangeTracker().MarkSprimDirty(pathName, 
                                                HdLight::AllDirty);

}

void
HdxTaskController::SetRenderOutputs(TfTokenVector const& outputs)
{
    if (!_AovsSupported() || _renderTaskIds.empty()) {
        return;
    }

    if (_aovOutputs == outputs) {
        return;
    }
    _aovOutputs = outputs;

    TfTokenVector localOutputs = outputs;

    // When we're asked to render "color", we treat that as final color,
    // complete with depth-compositing and selection, so we in-line add
    // some extra buffers if they weren't already requested.
    if (_IsStormRenderingBackend(GetRenderIndex())) {
        if (std::find(localOutputs.begin(), 
                      localOutputs.end(),
                      HdAovTokens->depth) == localOutputs.end()) {
            localOutputs.push_back(HdAovTokens->depth);
        }
    } else {
        std::set<TfToken> mainRenderTokens;
        for (auto const& aov : outputs) {
            if (aov == HdAovTokens->color || aov == HdAovTokens->depth ||
                aov == HdAovTokens->primId || aov == HdAovTokens->instanceId ||
                aov == HdAovTokens->elementId) {
                mainRenderTokens.insert(aov);
            }
        }
        // For a backend like PrMan/Embree we fill not just the color buffer,
        // but also buffers that are used during picking.
        if (mainRenderTokens.count(HdAovTokens->color) > 0) {
            if (mainRenderTokens.count(HdAovTokens->depth) == 0) {
                localOutputs.push_back(HdAovTokens->depth);
            }
            if (mainRenderTokens.count(HdAovTokens->primId) == 0) {
                localOutputs.push_back(HdAovTokens->primId);
            }
            if (mainRenderTokens.count(HdAovTokens->elementId) == 0) {
                localOutputs.push_back(HdAovTokens->elementId);
            }
            if (mainRenderTokens.count(HdAovTokens->instanceId) == 0) {
                localOutputs.push_back(HdAovTokens->instanceId);
            }
        }
    }

    // Delete the old renderbuffers.
    for (size_t i = 0; i < _aovBufferIds.size(); ++i) {
        GetRenderIndex()->RemoveBprim(HdPrimTypeTokens->renderBuffer,
            _aovBufferIds[i]);
    }
    _aovBufferIds.clear();

    // All RenderTasks share the same AOV buffers, so we update the AOVs by
    // using the dimension of the first RenderTask. Then push the updated
    // aov bindings to all render tasks.
    const SdfPath& firstRenderTask = _renderTaskIds.front();
    HdxRenderTaskParams renderParams =
        _delegate.GetParameter<HdxRenderTaskParams>(firstRenderTask,
            HdTokens->params);

    // Get the viewport dimensions (for renderbuffer allocation)
    GfVec3i dimensions = _ViewportToAovDimension(renderParams.viewport);

    // Get default AOV descriptors from the render delegate.
    HdAovDescriptorList outputDescs;
    outputDescs.resize(localOutputs.size());
    for (size_t i = 0; i < localOutputs.size(); ++i) {
        outputDescs[i] = GetRenderIndex()->GetRenderDelegate()->
            GetDefaultAovDescriptor(localOutputs[i]);
    }

    // Add the new renderbuffers. _GetAovPath returns ids of the form
    // {controller_id}/aov_{name}.
    for (size_t i = 0; i < localOutputs.size(); ++i) {
        SdfPath aovId = _GetAovPath(localOutputs[i]);
        GetRenderIndex()->InsertBprim(HdPrimTypeTokens->renderBuffer,
            &_delegate, aovId);
        HdRenderBufferDescriptor desc;
        desc.dimensions = dimensions;
        desc.format = outputDescs[i].format;
        desc.multiSampled = outputDescs[i].multiSampled;
        _delegate.SetParameter(aovId, _tokens->renderBufferDescriptor,desc);
        GetRenderIndex()->GetChangeTracker().MarkBprimDirty(aovId,
            HdRenderBuffer::DirtyDescription);
        _aovBufferIds.push_back(aovId);
    }

    // Create the list of AOV bindings.
    // Only the first render task clears AOVs so we also have a bindings set
    // that specifies no clear color for the remaining render tasks.
    HdRenderPassAovBindingVector aovBindingsClear;
    HdRenderPassAovBindingVector aovBindingsNoClear;
    aovBindingsClear.resize(localOutputs.size());
    aovBindingsNoClear.resize(aovBindingsClear.size());

    for (size_t i = 0; i < localOutputs.size(); ++i) {
        aovBindingsClear[i].aovName = localOutputs[i];
        aovBindingsClear[i].clearValue = outputDescs[i].clearValue;
        aovBindingsClear[i].renderBufferId = _GetAovPath(localOutputs[i]);
        aovBindingsClear[i].aovSettings = outputDescs[i].aovSettings;

        aovBindingsNoClear[i] = aovBindingsClear[i];
        aovBindingsNoClear[i].clearValue = VtValue();
    }

    // Set AOV bindings on render tasks
    for (SdfPath const& renderTaskId : _renderTaskIds) {
        bool isFirstRenderTask = renderTaskId == _renderTaskIds.front();

        const HdRenderPassAovBindingVector& aovBindings = 
            isFirstRenderTask ? 
            aovBindingsClear : 
            aovBindingsNoClear;

        HdxRenderTaskParams rParams =
            _delegate.GetParameter<HdxRenderTaskParams>(renderTaskId,
                HdTokens->params);

        rParams.aovBindings = aovBindings;

        _delegate.SetParameter(renderTaskId, HdTokens->params, rParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            renderTaskId, HdChangeTracker::DirtyParams);
    }

    // For AOV visualization, if only one output was specified, send it
    // to the viewer; otherwise, disable colorization.
    if (outputs.size() == 1) {
        SetViewportRenderOutput(outputs[0]);
    } else {
        SetViewportRenderOutput(TfToken());
    }
}

void
HdxTaskController::SetViewportRenderOutput(TfToken const& name)
{
    if (!_AovsSupported()) {
        return;
    }

    if (_viewportAov == name) {
        return;
    }
    _viewportAov = name;

    if (!_aovColorResolveTaskId.IsEmpty()) {
        HdxAovResolveTaskParams params;
        // Only resolve color aov if we are visualizing color (+ depth)
        if (name == HdAovTokens->color) {
            params.aovBufferPath = _GetAovPath(HdAovTokens->color);
        } else {
            params.aovBufferPath = SdfPath::EmptyPath();
        }

        _delegate.SetParameter(_aovColorResolveTaskId, HdTokens->params,params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _aovColorResolveTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_aovDepthResolveTaskId.IsEmpty()) {
        HdxAovResolveTaskParams params;
        // Only resolve depth aov if we are visualizing depth or color, since
        // color implicitely includes depth.
        if (name == HdAovTokens->color ||
            name == HdAovTokens->depth) {
            params.aovBufferPath = _GetAovPath(HdAovTokens->depth);
        } else {
            params.aovBufferPath = SdfPath::EmptyPath();
        }

        _delegate.SetParameter(_aovDepthResolveTaskId, HdTokens->params,params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _aovDepthResolveTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_colorizeTaskId.IsEmpty()) {
        HdxColorizeTaskParams params =
            _delegate.GetParameter<HdxColorizeTaskParams>(
                _colorizeTaskId, HdTokens->params);

        if (name.IsEmpty()) {
            // Empty token means don't colorize anything.
            params.aovName = name;
            params.aovBufferPath = SdfPath::EmptyPath();
            params.depthBufferPath = SdfPath::EmptyPath();
        } else if (name == HdAovTokens->color) {
            // Color is depth-composited...
            params.aovName = name;
            params.aovBufferPath = _GetAovPath(name);
            params.depthBufferPath = _GetAovPath(HdAovTokens->depth);
        } else {
            // But AOV visualizations are not.
            params.aovName = name;
            params.aovBufferPath = _GetAovPath(name);
            params.depthBufferPath = SdfPath::EmptyPath();
        }

        _delegate.SetParameter(_colorizeTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorizeTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_colorizeSelectionTaskId.IsEmpty()) {
        HdxColorizeSelectionTaskParams selParams =
            _delegate.GetParameter<HdxColorizeSelectionTaskParams>(
                _colorizeSelectionTaskId, HdTokens->params);

        if (name == HdAovTokens->color) {
            // If we're rendering color, make sure the colorize selection task
            // has the proper id buffers...
            selParams.primIdBufferPath =
                _GetAovPath(HdAovTokens->primId);
            selParams.instanceIdBufferPath =
                _GetAovPath(HdAovTokens->instanceId);
            selParams.elementIdBufferPath =
                _GetAovPath(HdAovTokens->elementId);
        } else {
            // Otherwise, clear the colorize selection task out.
            selParams.primIdBufferPath = SdfPath::EmptyPath();
            selParams.instanceIdBufferPath = SdfPath::EmptyPath();
            selParams.elementIdBufferPath = SdfPath::EmptyPath();
        }

        _delegate.SetParameter(_colorizeSelectionTaskId, HdTokens->params,
            selParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorizeSelectionTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_pickFromRenderBufferTaskId.IsEmpty()) {
        HdxPickFromRenderBufferTaskParams pickParams =
            _delegate.GetParameter<HdxPickFromRenderBufferTaskParams>(
                _pickFromRenderBufferTaskId, HdTokens->params);

        if (name == HdAovTokens->color) {
            // If we're rendering color, make sure the pick task has the
            // proper id & depth buffers...
            pickParams.primIdBufferPath =
                _GetAovPath(HdAovTokens->primId);
            pickParams.instanceIdBufferPath =
                _GetAovPath(HdAovTokens->instanceId);
            pickParams.elementIdBufferPath =
                _GetAovPath(HdAovTokens->elementId);
            pickParams.depthBufferPath =
                _GetAovPath(HdAovTokens->depth);
        } else {
            pickParams.primIdBufferPath = SdfPath::EmptyPath();
            pickParams.instanceIdBufferPath = SdfPath::EmptyPath();
            pickParams.elementIdBufferPath = SdfPath::EmptyPath();
            pickParams.depthBufferPath = SdfPath::EmptyPath();
        }

        _delegate.SetParameter(_pickFromRenderBufferTaskId, HdTokens->params,
            pickParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _pickFromRenderBufferTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_colorCorrectionTaskId.IsEmpty()) {
        HdxColorCorrectionTaskParams colCorParams =
            _delegate.GetParameter<HdxColorCorrectionTaskParams>(
                _colorCorrectionTaskId, HdTokens->params);

        colCorParams.aovName = name;
        colCorParams.aovBufferPath = _GetAovPath(name);

        _delegate.SetParameter(_colorCorrectionTaskId, HdTokens->params,
            colCorParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorCorrectionTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_colorChannelTaskId.IsEmpty()) {
        HdxColorChannelTaskParams colChannelParams =
            _delegate.GetParameter<HdxColorChannelTaskParams>(
                _colorChannelTaskId, HdTokens->params);

        _delegate.SetParameter(_colorChannelTaskId, HdTokens->params,
            colChannelParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorChannelTaskId, HdChangeTracker::DirtyParams);
    }

    if (!_presentTaskId.IsEmpty()) {
        HdxPresentTaskParams params;
        if (name.IsEmpty()) {
            // Empty token means don't colorize anything.
            params.aovBufferPath = SdfPath::EmptyPath();
            params.depthBufferPath = SdfPath::EmptyPath();
        } else if (name == HdAovTokens->color) {
            // Color is depth-composited...
            params.aovBufferPath = _GetAovPath(name);
            params.depthBufferPath = _GetAovPath(HdAovTokens->depth);
        } else {
            // But AOV visualizations are not.
            params.aovBufferPath = _GetAovPath(name);
            params.depthBufferPath = SdfPath::EmptyPath();
        }

        _delegate.SetParameter(_presentTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _presentTaskId, HdChangeTracker::DirtyParams);
    }
}

HdRenderBuffer*
HdxTaskController::GetRenderOutput(TfToken const& name)
{
    if (!_AovsSupported()) {
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
    if (!_AovsSupported() || _renderTaskIds.empty()) {
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

        bool isFirstRenderTask = renderTaskId == _renderTaskIds.front();

        for (size_t i = 0; i < renderParams.aovBindings.size(); ++i) {
            if (renderParams.aovBindings[i].renderBufferId == renderBufferId) {
                if (renderParams.aovBindings[i].clearValue != desc.clearValue ||
                    renderParams.aovBindings[i].aovSettings != desc.aovSettings) 
                {
                    // Only the first RenderTask should clear the AOV
                    renderParams.aovBindings[i].clearValue = isFirstRenderTask ?
                        desc.clearValue : VtValue();

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

HdAovDescriptor
HdxTaskController::GetRenderOutputSettings(TfToken const& name) const
{
    if (!_AovsSupported() || _renderTaskIds.empty()) {
        return HdAovDescriptor();
    }

    // Check if we're getting a value for a nonexistent AOV.
    SdfPath renderBufferId = _GetAovPath(name);
    if (!_delegate.HasParameter(renderBufferId,
                                _tokens->renderBufferDescriptor)) {
        return HdAovDescriptor();
    }

    HdRenderBufferDescriptor rbDesc =
        _delegate.GetParameter<HdRenderBufferDescriptor>(renderBufferId,
            _tokens->renderBufferDescriptor);

    HdAovDescriptor desc;
    desc.format = rbDesc.format;
    desc.multiSampled = rbDesc.multiSampled;

    const SdfPath& renderTaskId = _renderTaskIds.front();

    HdxRenderTaskParams renderParams =
        _delegate.GetParameter<HdxRenderTaskParams>(
            renderTaskId, HdTokens->params);

    for (size_t i = 0; i < renderParams.aovBindings.size(); ++i) {
        if (renderParams.aovBindings[i].renderBufferId == renderBufferId) {
            desc.clearValue = renderParams.aovBindings[i].clearValue;
            desc.aovSettings = renderParams.aovBindings[i].aovSettings;
            break;
        }
    }

    return desc;
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
        HdRprimCollection collection =
            _delegate.GetParameter<HdRprimCollection>(
                renderTaskId, HdTokens->collection);

        HdxRenderTaskParams oldParams = 
            _delegate.GetParameter<HdxRenderTaskParams>(
                renderTaskId, HdTokens->params);

        // We explicitly ignore input camera, viewport, and aovBindings because
        // these are internally managed.
        HdxRenderTaskParams mergedParams = params;
        mergedParams.camera = oldParams.camera;
        mergedParams.viewport = oldParams.viewport;
        mergedParams.aovBindings = oldParams.aovBindings;

        // We also explicitly manage blend params, based on the render tag.
        // XXX: Note: if params.enableIdRender is set, we want to use default
        // blend params so that we don't try to additive blend ID buffers...
        _SetBlendStateForMaterialTag(
            params.enableIdRender ? TfToken() : collection.GetMaterialTag(),
            &mergedParams);

        if (mergedParams != oldParams) {
            _delegate.SetParameter(renderTaskId,
                HdTokens->params, mergedParams);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                renderTaskId, HdChangeTracker::DirtyParams);
        }
    }

    // Update shadow task in case materials have been enabled/disabled
    if (!_shadowTaskId.IsEmpty()) {
        HdxShadowTaskParams oldShParams = 
            _delegate.GetParameter<HdxShadowTaskParams>(
                _shadowTaskId, HdTokens->params);

        if (oldShParams.enableSceneMaterials != params.enableSceneMaterials) {
            oldShParams.enableSceneMaterials = params.enableSceneMaterials;
            _delegate.SetParameter(_shadowTaskId, 
                HdTokens->params, oldShParams);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _shadowTaskId, HdChangeTracker::DirtyParams);
        }
    }

    // Update pick task
    if (!_pickTaskId.IsEmpty()) {
        HdxPickTaskParams pickParams =
            _delegate.GetParameter<HdxPickTaskParams>(
                _pickTaskId, HdTokens->params);
        
        if (pickParams.cullStyle != params.cullStyle ||
            pickParams.enableSceneMaterials != params.enableSceneMaterials) {

            pickParams.cullStyle = params.cullStyle;
            pickParams.enableSceneMaterials = params.enableSceneMaterials;

            _delegate.SetParameter(_pickTaskId, HdTokens->params, pickParams);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _pickTaskId, HdChangeTracker::DirtyParams);
        }
    }
}


void
HdxTaskController::SetRenderTags(TfTokenVector const& renderTags)
{
    HdChangeTracker &tracker = GetRenderIndex()->GetChangeTracker();

    for (SdfPath const& renderTaskId : _renderTaskIds) {
        if (_delegate.GetTaskRenderTags(renderTaskId) != renderTags) {
            _delegate.SetParameter(renderTaskId,
                                   _tokens->renderTags,
                                   renderTags);
            tracker.MarkTaskDirty(renderTaskId,
                                  HdChangeTracker::DirtyRenderTags);
        }
    }

    if (!_pickTaskId.IsEmpty()) {
        if (_delegate.GetTaskRenderTags(_pickTaskId) != renderTags) {
            _delegate.SetParameter(_pickTaskId,
                                   _tokens->renderTags,
                                   renderTags);

            tracker.MarkTaskDirty(_pickTaskId,
                                  HdChangeTracker::DirtyRenderTags);
        }
    }
}


void
HdxTaskController::SetShadowParams(HdxShadowTaskParams const& params)
{
    if (_shadowTaskId.IsEmpty()) {
        return;
    }

    HdxShadowTaskParams oldParams = 
        _delegate.GetParameter<HdxShadowTaskParams>(
            _shadowTaskId, HdTokens->params);

    HdxShadowTaskParams mergedParams = params;
    mergedParams.camera = oldParams.camera;
    mergedParams.viewport = oldParams.viewport;
    mergedParams.enableSceneMaterials = oldParams.enableSceneMaterials;

    if (mergedParams != oldParams) {
        _delegate.SetParameter(_shadowTaskId, HdTokens->params, mergedParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _shadowTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetEnableShadows(bool enable)
{
    if (_simpleLightTaskId.IsEmpty()) {
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
    if (!_selectionTaskId.IsEmpty()) {
        HdxSelectionTaskParams params =
            _delegate.GetParameter<HdxSelectionTaskParams>(
                _selectionTaskId, HdTokens->params);

        if (params.enableSelection != enable) {
            params.enableSelection = enable;
            _delegate.SetParameter(_selectionTaskId,
                HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _selectionTaskId, HdChangeTracker::DirtyParams);
        }
    }

    if (!_colorizeSelectionTaskId.IsEmpty()) {
        HdxColorizeSelectionTaskParams params =
            _delegate.GetParameter<HdxColorizeSelectionTaskParams>(
                _colorizeSelectionTaskId, HdTokens->params);

        if (params.enableSelection != enable) {
            params.enableSelection = enable;
            _delegate.SetParameter(_colorizeSelectionTaskId,
                HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _colorizeSelectionTaskId, HdChangeTracker::DirtyParams);
        }
    }
}

void
HdxTaskController::SetSelectionColor(GfVec4f const& color)
{
    if (!_selectionTaskId.IsEmpty()) {
        HdxSelectionTaskParams params =
            _delegate.GetParameter<HdxSelectionTaskParams>(
                _selectionTaskId, HdTokens->params);

        if (params.selectionColor != color) {
            params.selectionColor = color;
            _delegate.SetParameter(_selectionTaskId,
                HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _selectionTaskId, HdChangeTracker::DirtyParams);
        }
    }

    if (!_colorizeSelectionTaskId.IsEmpty()) {
        HdxColorizeSelectionTaskParams params =
            _delegate.GetParameter<HdxColorizeSelectionTaskParams>(
                _colorizeSelectionTaskId, HdTokens->params);

        if (params.selectionColor != color) {
            params.selectionColor = color;
            _delegate.SetParameter(_colorizeSelectionTaskId,
                HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _colorizeSelectionTaskId, HdChangeTracker::DirtyParams);
        }
    }
}

void
HdxTaskController::SetSelectionEnableOutline(bool enableOutline)
{
    if (!_colorizeSelectionTaskId.IsEmpty()) {
        HdxColorizeSelectionTaskParams params =
            _delegate.GetParameter<HdxColorizeSelectionTaskParams>(
                _colorizeSelectionTaskId, HdTokens->params);

        if (params.enableOutline != enableOutline) {
            params.enableOutline = enableOutline;
            _delegate.SetParameter(_colorizeSelectionTaskId,
                HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _colorizeSelectionTaskId, HdChangeTracker::DirtyParams);
        }
    }
}

void
HdxTaskController::SetSelectionOutlineRadius(unsigned int radius)
{
    if (!_colorizeSelectionTaskId.IsEmpty()) {
        HdxColorizeSelectionTaskParams params =
            _delegate.GetParameter<HdxColorizeSelectionTaskParams>(
                _colorizeSelectionTaskId, HdTokens->params);

        if (params.outlineRadius != radius) {
            params.outlineRadius = radius;
            _delegate.SetParameter(_colorizeSelectionTaskId,
                HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _colorizeSelectionTaskId, HdChangeTracker::DirtyParams);
        }
    }
}


void
HdxTaskController::SetLightingState(GlfSimpleLightingContextPtr const& src)
{
    // If simpleLightTask doesn't exist, no need to process the lighting
    // context...
    if (_simpleLightTaskId.IsEmpty()) {
        return;
    }

    if (!src) {
        TF_CODING_ERROR("Null lighting context");
        return;
    }

    GlfSimpleLightVector const& lights = src->GetLights();

    // HdxTaskController inserts a set of light prims to represent the lights
    // passed in through the simple lighting context (lights vector). These are 
    // managed by the task controller, and not by the scene; they represent the
    // application state which
    //
    // if we need to add any lights to the _lightIds vector 
    if (_lightIds.size() < lights.size()) {

        // cycle through the lights, add the new light and make sure the Sprims
        // at _lightIds[i] match with what is in lights[i]
        for (size_t i = 0; i < lights.size(); ++i) {
            
            // Get or create the light path for lights[i]
            bool needToAddLightPath = false;
            SdfPath lightPath = SdfPath();            
            if (i >= _lightIds.size()) {
                lightPath = GetControllerId().AppendChild(TfToken(
                            TfStringPrintf("light%d", (int)_lightIds.size())));
                needToAddLightPath = true;
            }
            else {
                lightPath = _lightIds[i];
            }
            // make sure that light at _lightIds[i] matches with lights[i]
            GlfSimpleLight currLight = _GetLightAtId(i);
            if (currLight != lights[i]) {

                // replace _lightIds[i] with the appropriate light 
                _ReplaceLightSprim(i, lights[i], lightPath);
            }
            if (needToAddLightPath){
                _lightIds.push_back(lightPath);
            }
        }
    }

    // if we need to remove Ids from the _lightIds vector 
    else if (_lightIds.size() > lights.size()) {

        // cycle through the lights making sure the Sprims at _lightIds[i] 
        // match with what is in lights[i]
        for (size_t i = 0; i < lights.size(); ++i) {
            
            // Get the light path for lights[i]
            SdfPath lightPath = _lightIds[i];
            
            // make sure that light at _lightIds[i] matches with lights[i]
            GlfSimpleLight currLight = _GetLightAtId(i);
            if (currLight != lights[i]) {

                // replace _lightIds[i] with the appropriate light 
                _ReplaceLightSprim(i, lights[i], lightPath);
            }
        }
        // now that everything matches just remove the last item in _lightIds
        _RemoveLightSprim(_lightIds.size()-1);
        _lightIds.pop_back();
    }

    // if there has been no change in the number of lights we still may need to 
    // update the light parameters eg. if the free camera has moved 
    for (size_t i = 0; i < lights.size(); ++i) {
        GlfSimpleLight light = _GetLightAtId(i);
        if (light != lights[i]) {
            _delegate.SetParameter(_lightIds[i], 
                                    HdLightTokens->params, lights[i]);

            if (light.IsDomeLight()) {
                _delegate.SetParameter(_lightIds[i], 
                                    HdLightTokens->textureResource, 
                                    _defaultDomeLightTextureResource);
            }
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
HdxTaskController::SetRenderViewport(GfVec4d const& viewport)
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

    if (!_shadowTaskId.IsEmpty()) {
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

    if (!_pickFromRenderBufferTaskId.IsEmpty()) {
        HdxPickFromRenderBufferTaskParams params =
            _delegate.GetParameter<HdxPickFromRenderBufferTaskParams>(
                _pickFromRenderBufferTaskId, HdTokens->params);
        params.viewport = viewport;
        _delegate.SetParameter(
            _pickFromRenderBufferTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _pickFromRenderBufferTaskId, HdChangeTracker::DirtyParams);
    }

    // Update all of the render buffer sizes as well.
    GfVec3i dimensions = _ViewportToAovDimension(viewport);
    for (auto const& id : _aovBufferIds) {
        HdRenderBufferDescriptor desc =
            _delegate.GetParameter<HdRenderBufferDescriptor>(id,
                _tokens->renderBufferDescriptor);
        if (desc.dimensions != dimensions) {
            desc.dimensions = dimensions;
            _delegate.SetParameter(id, _tokens->renderBufferDescriptor, desc);
            GetRenderIndex()->GetChangeTracker().MarkBprimDirty(id,
                HdRenderBuffer::DirtyDescription);
        }
    }
}

void
HdxTaskController::SetCameraPath(SdfPath const& id)
{
    _SetCameraParamForTasks(id);
}

void
HdxTaskController::SetFreeCameraMatrices(GfMatrix4d const& viewMatrix,
                                         GfMatrix4d const& projMatrix)
{
    _SetCameraParamForTasks(_freeCamId);

    GfMatrix4d oldView = _delegate.GetParameter<GfMatrix4d>(_freeCamId,
        HdCameraTokens->worldToViewMatrix);

    if (viewMatrix != oldView) {
        // Cache the new view matrix
        _delegate.SetParameter(_freeCamId, HdCameraTokens->worldToViewMatrix,
            viewMatrix);
        // Invalidate the camera
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_freeCamId,
            HdCamera::DirtyViewMatrix);
    }

    GfMatrix4d oldProj = _delegate.GetParameter<GfMatrix4d>(_freeCamId,
        HdCameraTokens->projectionMatrix);

    if (projMatrix != oldProj) {
        // Cache the new proj matrix
        _delegate.SetParameter(_freeCamId, HdCameraTokens->projectionMatrix,
            projMatrix);
        // Invalidate the camera
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_freeCamId,
            HdCamera::DirtyProjMatrix);
    }
}

void
HdxTaskController::
SetFreeCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes)
{
    // Cache the clip planes
    std::vector<GfVec4d> oldClipPlanes =
        _delegate.GetParameter<std::vector<GfVec4d>>(_freeCamId,
            HdCameraTokens->clipPlanes);

    if (oldClipPlanes != clipPlanes) {
        _delegate.SetParameter(_freeCamId, HdCameraTokens->clipPlanes,
            clipPlanes);
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_freeCamId,
            HdCamera::DirtyClipPlanes);
    }
}

bool
HdxTaskController::IsConverged() const
{
    bool converged = true;

    HdTaskSharedPtrVector tasks = GetRenderingTasks();
    for (auto const& task : tasks) {
        boost::shared_ptr<HdxProgressiveTask> progressiveTask =
            boost::dynamic_pointer_cast<HdxProgressiveTask>(task);
        if (progressiveTask) {
            converged = converged && progressiveTask->IsConverged();
            if (!converged) {
                break;
            }
        }
    }

    return converged;
}

void 
HdxTaskController::SetColorCorrectionParams(
    HdxColorCorrectionTaskParams const& params)
{
    if (_colorCorrectionTaskId.IsEmpty()) {
        return;
    }

    HdxColorCorrectionTaskParams oldParams = 
        _delegate.GetParameter<HdxColorCorrectionTaskParams>(
            _colorCorrectionTaskId, HdTokens->params);

    // We assume the caller for SetColorCorrectionParams wants to set the
    // OCIO settings, but does not want to override the AOV used to do color-
    // correction on. (Currently this AOV is controlled via TaskController)
    HdxColorCorrectionTaskParams newParams = params;
    newParams.aovName = oldParams.aovName;
    newParams.aovBufferPath = oldParams.aovBufferPath;

    if (newParams != oldParams) {
        _delegate.SetParameter(
            _colorCorrectionTaskId, HdTokens->params, newParams);

        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorCorrectionTaskId, HdChangeTracker::DirtyParams);

        // XXX Disable Colorize 'color quantization' when ColorCorrection is
        // disabled. We need to retire Colorize writing to the framebuffer so we
        // can just rely on ColorCorrection.
        SetColorizeQuantizationEnabled(
            !params.colorCorrectionMode.IsEmpty() &&
            params.colorCorrectionMode != HdxColorCorrectionTokens->disabled);
    }
}

void 
HdxTaskController::SetColorChannelParams(
    HdxColorChannelTaskParams const& params)
{
    if (_colorChannelTaskId.IsEmpty()) {
        return;
    }

    HdxColorChannelTaskParams oldParams = 
        _delegate.GetParameter<HdxColorChannelTaskParams>(
            _colorChannelTaskId, HdTokens->params);

    if (params != oldParams) {
        _delegate.SetParameter(
            _colorChannelTaskId, HdTokens->params, params);

        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _colorChannelTaskId, HdChangeTracker::DirtyParams);
    }
}

void 
HdxTaskController::SetColorizeQuantizationEnabled(bool enabled)
{
    if (_colorizeTaskId.IsEmpty()) return;

    HdxColorizeTaskParams params = 
        _delegate.GetParameter<HdxColorizeTaskParams>(
            _colorizeTaskId, HdTokens->params);

    params.applyColorQuantization = enabled;

    _delegate.SetParameter(
        _colorizeTaskId, HdTokens->params, params);

    GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
        _colorizeTaskId, HdChangeTracker::DirtyParams);
}

void
HdxTaskController::_SetCameraParamForTasks(SdfPath const& id)
{
    if (_activeCameraId != id) {
        _activeCameraId = id;

        // Update tasks that take a camera task param.
        for (SdfPath const& renderTaskId : _renderTaskIds) {
            HdxRenderTaskParams params =
                _delegate.GetParameter<HdxRenderTaskParams>(
                    renderTaskId, HdTokens->params);
            params.camera = _activeCameraId;

            _delegate.SetParameter(renderTaskId, HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                renderTaskId, HdChangeTracker::DirtyParams);
        }
        
        if (!_simpleLightTaskId.IsEmpty()) {
            HdxSimpleLightTaskParams params =
                _delegate.GetParameter<HdxSimpleLightTaskParams>(
                    _simpleLightTaskId, HdTokens->params);
            params.cameraPath = _activeCameraId;
            _delegate.SetParameter(_simpleLightTaskId, HdTokens->params,
                                   params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _simpleLightTaskId, HdChangeTracker::DirtyParams);
        }

        if (!_shadowTaskId.IsEmpty()) {
            HdxShadowTaskParams params =
                _delegate.GetParameter<HdxShadowTaskParams>(
                    _shadowTaskId, HdTokens->params);
            params.camera = _activeCameraId;
            _delegate.SetParameter(_shadowTaskId, HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _shadowTaskId, HdChangeTracker::DirtyParams);
        }

        if (!_pickFromRenderBufferTaskId.IsEmpty()) {
            HdxPickFromRenderBufferTaskParams params =
                _delegate.GetParameter<HdxPickFromRenderBufferTaskParams>(
                    _pickFromRenderBufferTaskId, HdTokens->params);
            params.cameraId = _activeCameraId;
            _delegate.SetParameter(
                _pickFromRenderBufferTaskId, HdTokens->params, params);
            GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                _pickFromRenderBufferTaskId, HdChangeTracker::DirtyParams);
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
