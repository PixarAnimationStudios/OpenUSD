//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/taskControllerSceneIndex.h"

#include "pxr/imaging/hdx/aovInputTask.h"
#include "pxr/imaging/hdx/boundingBoxTask.h"
#include "pxr/imaging/hdx/colorizeSelectionTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/freeCameraPrimDataSource.h"
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
#include "pxr/imaging/hdx/skydomeTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/visualizeAovTask.h"

#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/legacyTaskSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/renderBufferSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _rendererPluginNameTokens,

    ((storm, "HdStormRendererPlugin"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _primNameTokens,

    (camera)
    (AOVs)
    (lights)

    (colorizeSelectionTask)
    (pickTask)
    (renderTask)
    (aovInputTask)
    (simpleLightTask)
    (shadowTask)
    (skydomeTask)
    (oitResolveTask)
    (presentTask)
    (visualizeAovTask)
    (colorCorrectionTask)
    (selectionTask)
    (pickFromRenderBufferTask)
    (boundingBoxTask)

);

TF_DEFINE_PRIVATE_TOKENS(
    _materialTokens,

    (lightShader)

    (PxrDistantLight) 
    (PxrDomeLight)
);

TF_DEFINE_ENV_SETTING(HDX_MSAA_SAMPLE_COUNT, 4,
                      "MSAA sample count. Set to 1 to disable MSAA.");

// Distant Light values
static const float DISTANT_LIGHT_ANGLE = 0.53;
static const float DISTANT_LIGHT_INTENSITY = 15000.0;

namespace {

template<typename T>
typename HdTypedSampledDataSource<T>::Handle
_ToRetainedDataSource(const T &value)
{
    return HdRetainedTypedSampledDataSource<T>::New(value);
}

SdfPath
_CameraPath(const SdfPath &prefix)
{
    return prefix.AppendChild(_primNameTokens->camera);
}

template<typename Task>
const TfToken &
_TaskPrimName();

// Path for tasks determined by task type.
// Used by all tasks except for the Storm HdxRenderTask's since there are
// several tasks of the same type HdxRenderTask in its task graph.
template<typename Task>
SdfPath
_TaskPrimPath(const SdfPath &prefix)
{
    return prefix.AppendChild(_TaskPrimName<Task>());
}

// Scope for all render buffers (for AOVs) - so that we can easily find them.
SdfPath
_AovScopePath(const SdfPath &prefix)
{
    return prefix.AppendChild(_primNameTokens->AOVs);
}

TfToken
_SafeAovPrimName(const TfToken &name)
{
    if (SdfPath::IsValidIdentifier(name)) {
        return name;
    } else {
        return TfToken(
            TfStringPrintf(
                "%s_Hash%zu",
                TfMakeValidIdentifier(name.GetString()).c_str(),
                name.Hash()));
    }
}

SdfPath
_AovPath(const SdfPath &prefix, const TfToken &name)
{
    return _AovScopePath(prefix).AppendChild(_SafeAovPrimName(name));
}

// Scope for all lights managed by this scene index - so that we can easily
// find them.
SdfPath
_LightScopePath(const SdfPath &prefix)
{
    return prefix.AppendChild(_primNameTokens->lights);
}

SdfPath
_LightPath(const SdfPath &prefix, const size_t i)
{
    return _LightScopePath(prefix).AppendChild(
        TfToken(TfStringPrintf("light_%zu", i)));
}

///////////////////////////////////////////////////////////////////////////////
// Simple light task for Storm
template<>
const TfToken &
_TaskPrimName<HdxSimpleLightTask>()
{
    return _primNameTokens->simpleLightTask;
}

HdxSimpleLightTaskParams
_SimpleLightTaskParams(const SdfPath &prefix)
{
    HdxSimpleLightTaskParams params;
    params.cameraPath = _CameraPath(prefix);
    return params;
}

///////////////////////////////////////////////////////////////////////////////
// Shadow task for Storm (creates shadow maps)
template<>
const TfToken &
_TaskPrimName<HdxShadowTask>()
{
    return _primNameTokens->shadowTask;
}

///////////////////////////////////////////////////////////////////////////////
// Skydome task for Storm
template<>
const TfToken &
_TaskPrimName<HdxSkydomeTask>()
{
    return _primNameTokens->skydomeTask;
}

HdRprimCollection
_SkydomeTaskCollection()
{
    return {
        HdTokens->geometry,
        HdReprSelector(HdReprTokens->smoothHull),
        /* forcedRepr = */ false,
        HdStMaterialTagTokens->defaultMaterialTag };
}

///////////////////////////////////////////////////////////////////////////////
// Render task for generic renderer
template<>
const TfToken &
_TaskPrimName<HdxRenderTask>()
{
    return _primNameTokens->renderTask;
}

HdRprimCollection
_RenderTaskCollection(const TfToken &materialTag = {})
{
    HdRprimCollection collection(
        HdTokens->geometry,
        HdReprSelector(HdReprTokens->smoothHull),
        /* forcedRepr = */ false,
        materialTag);
    collection.SetRootPath(SdfPath::AbsoluteRootPath());
    return collection;
}

///////////////////////////////////////////////////////////////////////////////
// Render tasks for Storm

SdfPath
_StormRenderTaskPath(const SdfPath &prefix, const TfToken &materialTag)
{
    return prefix.AppendChild(
        TfToken("renderTask_" + materialTag.GetString()));
}

// The default and masked material tags share the same blend state, but
// we classify them as separate because in the general case, masked
// materials use fragment shader discards while the defaultMaterialTag
// should not.
HdxRenderTaskParams
_StormRenderTaskParamsDefaultMaterialTagAndMasked()
{
    HdxRenderTaskParams params;
    params.blendEnable = false;
    params.depthMaskEnable = true;
    params.enableAlphaToCoverage = true;

    return params;
}

HdxRenderTaskParams
_StormRenderTaskParamsAdditive()
{
    HdxRenderTaskParams params;
    // Additive blend -- so no sorting of drawItems is needed
    params.blendEnable = true;
    // For color, we are setting all factors to ONE.
    //
    // This means we are expecting pre-multiplied alpha coming out
    // of the shader: vec4(rgb*a, a).  Setting ColorSrc to
    // HdBlendFactorSourceAlpha would give less control on the
    // shader side, since it means we would force a pre-multiplied
    // alpha step on the color coming out of the shader.
    //
    params.blendColorOp = HdBlendOpAdd;
    params.blendColorSrcFactor = HdBlendFactorOne;
    params.blendColorDstFactor = HdBlendFactorOne;

    // For alpha, we set the factors so that the alpha in the
    // framebuffer won't change.  Recall that the geometry in the
    // additive render pass is supposed to be emitting light but
    // be fully transparent, that is alpha = 0, so that the order
    // in which it is drawn doesn't matter.
    params.blendAlphaOp = HdBlendOpAdd;
    params.blendAlphaSrcFactor = HdBlendFactorZero;
    params.blendAlphaDstFactor = HdBlendFactorOne;

    // Translucent objects should not block each other in depth buffer
    params.depthMaskEnable = false;

    // Since we are using alpha blending, we disable screen door
    // transparency for this renderpass.
    params.enableAlphaToCoverage = false;

    return params;
}

HdxRenderTaskParams
_StormRenderTaskParamsTranslucent()
{
    HdxRenderTaskParams params;

    // OIT is using its own buffers which are only per pixel and not per
    // sample. Thus, we resolve the AOVs before starting to render any
    // OIT geometry and only use the resolved AOVs from then on.
    params.useAovMultiSample = false;

    return params;
}

HdxRenderTaskParams
_StormRenderTaskParamsVolume()
{
    HdxRenderTaskParams params;

    // See above comment about OIT.
    params.useAovMultiSample = false;

    // Disable alpha-to-coverage for the volume render task, as nothing
    // (including alpha) gets written to fragments during this task.
    params.enableAlphaToCoverage = false;

    return params;
}

///////////////////////////////////////////////////////////////////////////////
// AOV input task (maps render buffer's to textures in task context)
template<>
const TfToken &
_TaskPrimName<HdxAovInputTask>()
{
    return _primNameTokens->aovInputTask;
}

///////////////////////////////////////////////////////////////////////////////
// OIT resolve task for Storm (reads back from depth AOV)
template<>
const TfToken &
_TaskPrimName<HdxOitResolveTask>()
{
    return _primNameTokens->oitResolveTask;
}

HdxOitResolveTaskParams
_OitResolveTaskParams()
{
    HdxOitResolveTaskParams params;
    // OIT is using its own buffers which are only per pixel and not per
    // sample. Thus, we resolve the AOVs before starting to render any
    // OIT geometry and only use the resolved AOVs from then on.
    params.useAovMultiSample = false;
    return params;
}

///////////////////////////////////////////////////////////////////////////////
// Selection task for Storm
template<>
const TfToken &
_TaskPrimName<HdxSelectionTask>()
{
    return _primNameTokens->selectionTask;
}

HdxSelectionTaskParams
_SelectionTaskParams()
{
    HdxSelectionTaskParams params;
    params.enableSelectionHighlight = true;
    params.enableLocateHighlight = true;
    params.selectionColor = GfVec4f(1,1,0,1);
    params.locateColor = GfVec4f(0,0,1,1);
    return params;
}

///////////////////////////////////////////////////////////////////////////////
// Selection task for generic renderer (uses id buffer to color)
template<>
const TfToken &
_TaskPrimName<HdxColorizeSelectionTask>()
{
    return _primNameTokens->colorizeSelectionTask;
}

HdxColorizeSelectionTaskParams
_ColorizeSelectionTaskParams()
{
    HdxColorizeSelectionTaskParams params;
    params.enableSelectionHighlight = true;
    params.enableLocateHighlight    = true;
    params.selectionColor  = GfVec4f(0.2f, 1.0f, 0.4f, 1.0f);
    params.locateColor     = GfVec4f(0.0f, 0.0f, 1.0f, 1.0f);
    return params;
}

///////////////////////////////////////////////////////////////////////////////
// Color correction task
template<>
const TfToken &
_TaskPrimName<HdxColorCorrectionTask>()
{
    return _primNameTokens->colorCorrectionTask;
}

///////////////////////////////////////////////////////////////////////////////
// Visualize AOV task (for non-color AOVs)
template<>
const TfToken &
_TaskPrimName<HdxVisualizeAovTask>()
{
    return _primNameTokens->visualizeAovTask;
}

///////////////////////////////////////////////////////////////////////////////
// Present task (blits texture from AOV to current framebuffer)
template<>
const TfToken &
_TaskPrimName<HdxPresentTask>()
{
    return _primNameTokens->presentTask;
}

///////////////////////////////////////////////////////////////////////////////
// Pick task for Storm
template<>
const TfToken &
_TaskPrimName<HdxPickTask>()
{
    return _primNameTokens->pickTask;
}

///////////////////////////////////////////////////////////////////////////////
// Pick task for generic renderer
template<>
const TfToken &
_TaskPrimName<HdxPickFromRenderBufferTask>()
{
    return _primNameTokens->pickFromRenderBufferTask;
}

///////////////////////////////////////////////////////////////////////////////
// Bounding box task
template<>
const TfToken &
_TaskPrimName<HdxBoundingBoxTask>()
{
    return _primNameTokens->boundingBoxTask;
}

///////////////////////////////////////////////////////////////////////////////
// Data source for locator "task" conforming to HdLegacyTaskSchema

template<typename TaskParams>
class _LegacyTaskSchemaDataSource : public HdContainerDataSource
{
public:
    using This = _LegacyTaskSchemaDataSource<TaskParams>;

    HD_DECLARE_DATASOURCE(This);

    HdLegacyTaskFactorySharedPtr factory;
    TaskParams params;
    HdRprimCollection collection;
    TfTokenVector renderTags;

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdLegacyTaskSchemaTokens->factory) {
            return _ToRetainedDataSource(factory);
        }
        if (name == HdLegacyTaskSchemaTokens->parameters) {
            return _ToRetainedDataSource(params);
        }
        if (name == HdLegacyTaskSchemaTokens->collection) {
            return _ToRetainedDataSource(collection);
        }
        if (name == HdLegacyTaskSchemaTokens->renderTags) {
            return _ToRetainedDataSource(renderTags);
        }
        return nullptr;
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdLegacyTaskSchemaTokens->factory,
            HdLegacyTaskSchemaTokens->parameters,
            HdLegacyTaskSchemaTokens->collection,
            HdLegacyTaskSchemaTokens->renderTags
        };
        return result;
    }

private:
    _LegacyTaskSchemaDataSource(
        HdLegacyTaskFactorySharedPtr const &factory,
        const TaskParams &params,
        const HdRprimCollection &collection,
        const TfTokenVector &renderTags)
     : factory(factory)
     , params(params)
     , collection(collection)
     , renderTags(renderTags)
    {
    }
};

// Data for a task prim.
template<typename Task>
HdContainerDataSourceHandle
_TaskPrimDataSource(
    const typename Task::TaskParams &params,
    const HdRprimCollection &collection,
    const TfTokenVector &renderTags)
{
    static HdLegacyTaskFactorySharedPtr const factory =
        HdMakeLegacyTaskFactory<Task>();

    return HdRetainedContainerDataSource::New(
        HdLegacyTaskSchema::GetSchemaToken(),
        _LegacyTaskSchemaDataSource<typename Task::TaskParams>::New(
            factory, params, collection, renderTags));
}

// Entry to add task prim to a retained scene index.
// Prim path is determined from prefix and task type.
template<typename Task>
HdRetainedSceneIndex::AddedPrimEntry
_TaskAddEntry(
    const SdfPath &prefix,
    const typename Task::TaskParams &params = {},
    const HdRprimCollection &collection = {},
    const TfTokenVector &renderTags = { })
{
    return {
        _TaskPrimPath<Task>(prefix),
        HdPrimTypeTokens->task,
        _TaskPrimDataSource<Task>(params, collection, renderTags)
    };
}

template<typename TaskParams>
typename _LegacyTaskSchemaDataSource<TaskParams>::Handle
_GetTaskSchemaDataSource(HdContainerDataSourceHandle const &primSource)
{
    using DS = _LegacyTaskSchemaDataSource<TaskParams>;
    return
        DS::Cast(HdLegacyTaskSchema::GetFromParent(primSource).GetContainer());
}

// Get pointer to task params from a prim's data source.
template<typename TaskParams>
TaskParams *
_GetTaskParams(HdContainerDataSourceHandle const &primSource)
{
    auto const ds =
        _GetTaskSchemaDataSource<TaskParams>(primSource);
    if (!ds) {
        return nullptr;
    }
    return &ds->params;
}

template<typename TaskParams>
TaskParams *
_GetTaskParamsAtPath(HdSceneIndexBaseRefPtr const &sceneIndex,
                     const SdfPath &path)
{
    HdSceneIndexPrim const prim = sceneIndex->GetPrim(path);
    return _GetTaskParams<TaskParams>(prim.dataSource);
}

// Get pointer to task params from the retained scene index.
// Prim path is determined from prefix and task type.
template<typename Task>
typename Task::TaskParams *
_GetTaskParamsForTask(HdSceneIndexBaseRefPtr const &sceneIndex,
                      const SdfPath &prefix)
{
    using TaskParams = typename Task::TaskParams;

    return _GetTaskParamsAtPath<TaskParams>(
        sceneIndex, _TaskPrimPath<Task>(prefix));
}

// Get collection from task data source
HdRprimCollection *
_GetCollectionAtPath(HdSceneIndexBaseRefPtr const &sceneIndex,
                     const SdfPath &path)
{
    HdSceneIndexPrim const prim = sceneIndex->GetPrim(path);
    auto const ds =
        _GetTaskSchemaDataSource<HdxRenderTaskParams>(
            prim.dataSource);
    if (!ds) {
        return nullptr;
    }
    return &ds->collection;
}

// Get render tags from task data source
template<typename TaskParams>
TfTokenVector *
_GetRenderTagsAtPath(HdSceneIndexBaseRefPtr const &sceneIndex,
                     const SdfPath &path)
{
    HdSceneIndexPrim const prim = sceneIndex->GetPrim(path);
    auto const ds =
        _GetTaskSchemaDataSource<TaskParams>(
            prim.dataSource);
    if (!ds) {
        return nullptr;
    }
    return &ds->renderTags;
}

// Get pointer to render tags from the retained scene index.
// Prim path is determined from prefix and task type.
template<typename Task>
TfTokenVector *
_GetRenderTagsForTask(HdSceneIndexBaseRefPtr const &sceneIndex,
                      const SdfPath &prefix)
{
    using TaskParams = typename Task::TaskParams;

    return _GetRenderTagsAtPath<TaskParams>(
        sceneIndex, _TaskPrimPath<Task>(prefix));
}

// Entry to dirty task params in a retained scene index.
// Prim Path is determined from prefix and task type.
template<typename Task>
void
_AddDirtyParamsEntry(
    const SdfPath &prefix,
    HdSceneIndexObserver::DirtiedPrimEntries * const entries)
{
    static const HdDataSourceLocatorSet locators{
        HdLegacyTaskSchema::GetParametersLocator() };
    entries->push_back({ _TaskPrimPath<Task>(prefix), locators});
}

// Dirty task params in retained scene index.
// Prim path is determined from prefix and task type.
template<typename Task>
void
_SendDirtyParamsEntry(
    HdRetainedSceneIndexRefPtr const &retainedSceneIndex,
    const SdfPath &prefix)
{
    HdSceneIndexObserver::DirtiedPrimEntries entries;
    _AddDirtyParamsEntry<Task>(prefix, &entries);
    retainedSceneIndex->DirtyPrims(entries);
}

template<typename Task>
HdRetainedSceneIndex::AddedPrimEntry
_StormRenderTaskAddEntry(
    const SdfPath &prefix,
    const TfToken &materialTag,
    const HdxRenderTaskParams &params)
{
    return {
        _StormRenderTaskPath(prefix, materialTag),
        HdPrimTypeTokens->task,
        _TaskPrimDataSource<Task>(
            params,
            _RenderTaskCollection(materialTag),
            { HdRenderTagTokens->geometry })
    };
}

///////////////////////////////////////////////////////////////////////////////
// Data source conforming to HdRenderBufferSchema.
//
class _RenderBufferSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderBufferSchemaDataSource);

    GfVec3i dimensions;
    HdFormat format;
    bool multiSampled;
    uint32_t msaaSampleCount;

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdRenderBufferSchemaTokens->dimensions) {
            return _ToRetainedDataSource(dimensions);
        }
        if (name == HdRenderBufferSchemaTokens->format) {
            return _ToRetainedDataSource(format);
        }
        if (name == HdRenderBufferSchemaTokens->multiSampled) {
            return _ToRetainedDataSource(multiSampled);
        }
        if (name == HdStRenderBufferTokens->stormMsaaSampleCount) {
            return _ToRetainedDataSource(msaaSampleCount);
        }
        return nullptr;
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdRenderBufferSchemaTokens->dimensions,
            HdRenderBufferSchemaTokens->format,
            HdRenderBufferSchemaTokens->multiSampled,
            HdStRenderBufferTokens->stormMsaaSampleCount
        };
        return result;
    }

private:
    _RenderBufferSchemaDataSource(
        const GfVec3i &dimensions,
        const HdFormat format,
        const bool multiSampled,
        const uint32_t msaaSampleCount)
     : dimensions(dimensions)
     , format(format)
     , multiSampled(multiSampled)
     , msaaSampleCount(msaaSampleCount)
    {
    }
};

HD_DECLARE_DATASOURCE_HANDLES(_RenderBufferSchemaDataSource);

// Data source for a render buffer prim.
HdContainerDataSourceHandle
_RenderBufferPrimDataSource(
    const GfVec3i &dimensions,
    const HdFormat format,
    const bool multiSampled,
    const uint32_t msaaSampleCount)
{
    return HdRetainedContainerDataSource::New(
        HdRenderBufferSchema::GetSchemaToken(),
        _RenderBufferSchemaDataSource::New(
            dimensions, format, multiSampled, msaaSampleCount));
}

///////////////////////////////////////////////////////////////////////////////
// Lights

SdfAssetPath
_DomeLightTexture(const GlfSimpleLight &light)
{
    const SdfAssetPath &assetPath = light.GetDomeLightTextureFile();
    if (assetPath == SdfAssetPath()) {
        static const SdfAssetPath defaultAssetPath(
            HdxPackageDefaultDomeLightTexture(),
            HdxPackageDefaultDomeLightTexture());
        return defaultAssetPath;
    }
    return assetPath;
}

///////////////////////////////////////////////////////////////////////////////
// Data source for locator "light" conforming to HdLightSchema
//
class _LightSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightSchemaDataSource);

    std::shared_ptr<GlfSimpleLight> const light;

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdLightTokens->intensity) {
            if (!_isForStorm && !light->IsDomeLight()) {
                // Initialize distant light specific parameters
                static HdDataSourceBaseHandle const ds =
                    _ToRetainedDataSource(DISTANT_LIGHT_INTENSITY);
                return ds;
            } else {
                static HdDataSourceBaseHandle const ds =
                    _ToRetainedDataSource(1.0f);
                return ds;
            }
        }
        if (name == HdLightTokens->exposure) {
            static HdDataSourceBaseHandle const ds =
                _ToRetainedDataSource(0.0f);
            return ds;
        }
        if (name == HdLightTokens->normalize) {
            return _ToRetainedDataSource(false);
        }
        if (name == HdLightTokens->color) {
            static HdDataSourceBaseHandle const ds =
                _ToRetainedDataSource(GfVec3f(1,1,1));
            return ds;
        }
        if (name == HdLightTokens->angle) {
            if (!_isForStorm && !light->IsDomeLight()) {
                // Initialize distant light specific parameters
                static HdDataSourceBaseHandle const ds =
                    _ToRetainedDataSource(DISTANT_LIGHT_ANGLE);
                return ds;
            } else {
                return nullptr;
            }
        }
        if (name == HdLightTokens->shadowEnable) {
            if (light->IsDomeLight()) {
                return _ToRetainedDataSource(true);
            }
            if (!_isForStorm) {
                // Initialize distant light specific parameters
                return _ToRetainedDataSource(false);
            }
            return nullptr;
        }
        if (name == HdLightTokens->params) {
            return _ToRetainedDataSource(*light);
        }
        if (name == HdLightTokens->textureFile) {
            if (light->IsDomeLight()) {
                return _ToRetainedDataSource(_DomeLightTexture(*light));
            }
            return nullptr;
        }

        return nullptr;
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdLightTokens->intensity,
            HdLightTokens->exposure,
            HdLightTokens->normalize,
            HdLightTokens->color,
            HdLightTokens->shadowEnable,
            HdLightTokens->params,
            HdLightTokens->textureFile
        };
        return result;
    }

private:
    _LightSchemaDataSource(
        std::shared_ptr<GlfSimpleLight> light,
        const bool isForStorm)
     : light(std::move(light))
     , _isForStorm(isForStorm)
    {
    }

    const bool _isForStorm;
};

HdTokenDataSourceHandle
_MaterialNodeIdentifier(const bool isDomeLight)
{
    // XXX Using these Pxr**Light tokens works for now since HdPrman is
    // currently the only renderer that supports material networks for lights.
    if (isDomeLight) {
        static HdTokenDataSourceHandle const ds =
            _ToRetainedDataSource(
                _materialTokens->PxrDomeLight);
        return ds;
    } else {
        static HdTokenDataSourceHandle const ds =
            _ToRetainedDataSource(
                _materialTokens->PxrDistantLight);
        return ds;
    }
}

template<typename T>
HdContainerDataSourceHandle
_ToMaterialNodeParameter(const T &value)
{
    return
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ToRetainedDataSource(value))
            .Build();
}

GfMatrix4d
_TransformFromPosition(const GfVec4d &position)
{
    return
        GfMatrix4d(1.0)
            .SetTranslateOnly(GfVec3d(position[0], position[1], position[2]));
}

HdContainerDataSourceHandle
_MaterialNodeParameters(const GlfSimpleLight &light)
{
    TfTokenVector names;
    std::vector<HdDataSourceBaseHandle> values;

    names.push_back(HdLightTokens->exposure);
    values.push_back(_ToMaterialNodeParameter(0.0f));

    names.push_back(HdLightTokens->normalize);
    values.push_back(_ToMaterialNodeParameter(false));

    names.push_back(HdLightTokens->color);
    values.push_back(_ToMaterialNodeParameter(GfVec3f(1, 1, 1)));

    if (light.IsDomeLight()) {
        names.push_back(HdTokens->transform);
        values.push_back(_ToMaterialNodeParameter(light.GetTransform()));

        names.push_back(HdLightTokens->intensity);
        values.push_back(_ToMaterialNodeParameter(1.0f));

        names.push_back(HdLightTokens->shadowEnable);
        values.push_back(_ToMaterialNodeParameter(true));

        names.push_back(HdLightTokens->textureFile);
        values.push_back(_ToMaterialNodeParameter(_DomeLightTexture(light)));
    } else {
        // For the camera light, initialize the transform based on the
        // SimpleLight position
        names.push_back(HdTokens->transform);
        values.push_back(
            _ToMaterialNodeParameter(
                _TransformFromPosition(light.GetPosition())));

        names.push_back(HdLightTokens->intensity);
        values.push_back(_ToMaterialNodeParameter(DISTANT_LIGHT_INTENSITY));

        names.push_back(HdLightTokens->angle);
        values.push_back(_ToMaterialNodeParameter(DISTANT_LIGHT_ANGLE));

        names.push_back(HdLightTokens->shadowEnable);
        values.push_back(_ToMaterialNodeParameter(false));

        constexpr float pi(M_PI);

        // We assume that the color specified for these "simple" lights means
        // that it is the expected color a white Lambertian surface would have
        // if one of these colored "simple" lights was pointed directly at it.
        // To achieve this, the light color needs to be scaled appropriately.
        names.push_back(HdLightTokens->diffuse);
        values.push_back(_ToMaterialNodeParameter(pi));

        names.push_back(HdLightTokens->specular);
        values.push_back(_ToMaterialNodeParameter(pi));
    }

    return HdMaterialNodeParameterContainerSchema::BuildRetained(
        names.size(), names.data(), values.data());
}

HdContainerDataSourceHandle
_MaterialNode(const GlfSimpleLight &light)
{
    return
        HdMaterialNodeSchema::Builder()
            .SetNodeIdentifier(_MaterialNodeIdentifier(light.IsDomeLight()))
            .SetParameters(_MaterialNodeParameters(light))
            .Build();
}

HdContainerDataSourceHandle
_MaterialNodes(const GlfSimpleLight &light)
{
    static const TfToken names[] = {
        _materialTokens->lightShader
    };
    HdDataSourceBaseHandle const values[] = {
        _MaterialNode(light)
    };

    static_assert(std::size(names) == std::size(values));

    return
        HdMaterialNodeContainerSchema::BuildRetained(
            std::size(names), names, values);
}

HdContainerDataSourceHandle
_MaterialTerminals()
{
    const TfToken names[] = {
        HdMaterialTerminalTokens->light
    };
    HdDataSourceBaseHandle const values[] = {
        HdMaterialConnectionSchema::Builder()
            .SetUpstreamNodePath(
                _ToRetainedDataSource(
                    _materialTokens->lightShader))
            .SetUpstreamNodeOutputName(
                _ToRetainedDataSource(
                    HdMaterialTerminalTokens->light))
        .Build()
    };

    static_assert(std::size(names) == std::size(values));

    return
        HdMaterialConnectionContainerSchema::BuildRetained(
            std::size(names), names, values);
}

HdContainerDataSourceHandle
_MaterialNetwork(const GlfSimpleLight &light)
{
    static HdContainerDataSourceHandle const terminals = _MaterialTerminals();
    return
        HdMaterialNetworkSchema::Builder()
        .SetNodes(_MaterialNodes(light))
        .SetTerminals(terminals)
        .Build();
}

HdContainerDataSourceHandle
_Material(const GlfSimpleLight &light)
{
    static const TfToken names[] = {
        HdMaterialSchemaTokens->universalRenderContext
    };
    HdDataSourceBaseHandle const values[] = {
        _MaterialNetwork(light)
    };

    static_assert(std::size(names) == std::size(values));

    return HdMaterialSchema::BuildRetained(std::size(names), names, values);
}

// Data source for light prim
//
class _LightPrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightPrimDataSource);

    std::shared_ptr<GlfSimpleLight> const light;

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdLightSchema::GetSchemaToken()) {
            return _LightSchemaDataSource::New(
                light, _isForStorm);
        }
        if (name == HdMaterialSchema::GetSchemaToken()) {
            return _Material(*light);
        }
        if (name == HdXformSchema::GetSchemaToken()) {
            return HdXformSchema::Builder()
                .SetMatrix(
                    _ToRetainedDataSource(light->GetTransform()))
                .Build();
        }
        return nullptr;
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdLightSchema::GetSchemaToken(),
            HdMaterialSchema::GetSchemaToken(),
            HdXformSchema::GetSchemaToken()
        };
        return result;
    }

private:
    _LightPrimDataSource(
        const GlfSimpleLight &light,
        const bool isForStorm)
     : light(std::make_shared<GlfSimpleLight>(light))
     , _isForStorm(isForStorm)
    {
    }

    const bool _isForStorm;
};

HD_DECLARE_DATASOURCE_HANDLES(_LightPrimDataSource);

}

// ---------------------------------------------------------------------------
// Task controller implementation.

HdxTaskControllerSceneIndexRefPtr
HdxTaskControllerSceneIndex::New(
    const SdfPath &prefix,
    const TfToken &rendererPluginName,
    const AovDescriptorCallback &aovDescriptorCallback,
    const bool gpuEnabled)
{
    return TfCreateRefPtr(
        new HdxTaskControllerSceneIndex(
            prefix, rendererPluginName, aovDescriptorCallback, gpuEnabled));
}


HdxTaskControllerSceneIndex::HdxTaskControllerSceneIndex(
    const SdfPath &prefix,
    const TfToken &rendererPluginName,
    const AovDescriptorCallback &aovDescriptorCallback,
    const bool gpuEnabled)
 : _prefix(prefix)
 , _isForStorm(rendererPluginName == _rendererPluginNameTokens->storm)
 , _aovDescriptorCallback(aovDescriptorCallback)
 , _runGpuAovTasks(gpuEnabled || _isForStorm)
 , _retainedSceneIndex(HdRetainedSceneIndex::New())
 , _renderBufferSize(0, 0)
 , _viewport(0, 0, 1, 1)
 , _observer(this)
{
    _retainedSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));

    if (_IsForStorm()) {
        if (!gpuEnabled) {
            TF_WARN("Trying to use Storm while disabling the GPU.");
        }

        _CreateStormTasks();

        // XXX AOVs are OFF by default for Storm TaskController because hybrid
        // rendering in Presto spawns an UsdImagingGLEngine, which creates a
        // task controlller. But the Hydrid rendering setups are not yet AOV
        // ready since it breaks main cam zoom operations expressed via viewport
        // manipulation.
        // App (UsdView) for now calls engine->SetRendererAov(color) to enable.
        // SetRenderOutputs({HdAovTokens->color});
    } else {
        _CreateGenericTasks();

        // Initialize the AOV system to render color. Note:
        // SetRenderOutputs special-cases color to include support for
        // depth-compositing and selection highlighting/picking.
        SetRenderOutputs({HdAovTokens->color});
    }

    _retainedSceneIndex->AddPrims(
        {{ _CameraPath(_prefix),
           HdPrimTypeTokens->camera,
           HdxFreeCameraPrimDataSource::New() }});
}

bool
HdxTaskControllerSceneIndex::_IsForStorm() const
{
    return _isForStorm;
}

HdxTaskControllerSceneIndex::~HdxTaskControllerSceneIndex() = default;

HdSceneIndexPrim
HdxTaskControllerSceneIndex::GetPrim(const SdfPath &primPath) const
{
    return _retainedSceneIndex->GetPrim(primPath);
}

SdfPathVector
HdxTaskControllerSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _retainedSceneIndex->GetChildPrimPaths(primPath);
}

void
HdxTaskControllerSceneIndex::_CreateStormTasks()
{
    _retainedSceneIndex->AddPrims({
        _TaskAddEntry<HdxSimpleLightTask>(
            _prefix,
            _SimpleLightTaskParams(_prefix)),
        _TaskAddEntry<HdxShadowTask>(
            _prefix,
            HdxShadowTaskParams(),
            HdRprimCollection(),
            { HdRenderTagTokens->geometry })
    });

    // All tasks using HdxRenderTaskParams.
    const HdRetainedSceneIndex::AddedPrimEntries renderTaskAddEntries{
        _TaskAddEntry<HdxSkydomeTask>(
            _prefix,
            HdxRenderTaskParams(),
            _SkydomeTaskCollection()),
        _StormRenderTaskAddEntry<HdxRenderTask>(
            _prefix,
            HdStMaterialTagTokens->defaultMaterialTag,
            _StormRenderTaskParamsDefaultMaterialTagAndMasked()),
        _StormRenderTaskAddEntry<HdxRenderTask>(
            _prefix,
            HdStMaterialTagTokens->masked,
            _StormRenderTaskParamsDefaultMaterialTagAndMasked()),
        _StormRenderTaskAddEntry<HdxRenderTask>(
            _prefix,
            HdStMaterialTagTokens->additive,
            _StormRenderTaskParamsAdditive()),
        _StormRenderTaskAddEntry<HdxOitRenderTask>(
            _prefix,
            HdStMaterialTagTokens->translucent,
            _StormRenderTaskParamsTranslucent()),
        _StormRenderTaskAddEntry<HdxOitVolumeRenderTask>(
            _prefix,
            HdStMaterialTagTokens->volume,
            _StormRenderTaskParamsVolume()),
    };

    for (const HdRetainedSceneIndex::AddedPrimEntry &entry
             : renderTaskAddEntries) {
        _renderTaskPaths.push_back(entry.primPath);
    }

    _retainedSceneIndex->AddPrims(renderTaskAddEntries);

    _retainedSceneIndex->AddPrims({
        _TaskAddEntry<HdxAovInputTask>(
            _prefix),
        _TaskAddEntry<HdxOitResolveTask>(
            _prefix,
            _OitResolveTaskParams()),
        _TaskAddEntry<HdxSelectionTask>(
            _prefix,
            _SelectionTaskParams()),
        _TaskAddEntry<HdxColorCorrectionTask>(
            _prefix),
        _TaskAddEntry<HdxVisualizeAovTask>(
            _prefix),
        _TaskAddEntry<HdxPresentTask>(
            _prefix),
        _TaskAddEntry<HdxPickTask>(
            _prefix),
        _TaskAddEntry<HdxBoundingBoxTask>(
            _prefix)
    });
}

void
HdxTaskControllerSceneIndex::_CreateGenericTasks()
{
    // All tasks using HdxRenderTaskParams.
    const HdRetainedSceneIndex::AddedPrimEntries renderTaskAddEntries{
        _TaskAddEntry<HdxRenderTask>(
            _prefix,
            HdxRenderTaskParams(),
            _RenderTaskCollection(),
            { HdRenderTagTokens->geometry })};

    for (const HdRetainedSceneIndex::AddedPrimEntry &entry
             : renderTaskAddEntries) {
        _renderTaskPaths.push_back(entry.primPath);
    }

    _retainedSceneIndex->AddPrims(renderTaskAddEntries);

    if (_runGpuAovTasks) {
        _retainedSceneIndex->AddPrims({
            _TaskAddEntry<HdxAovInputTask>(
                _prefix),
            _TaskAddEntry<HdxColorizeSelectionTask>(
                _prefix,
                _ColorizeSelectionTaskParams()),
            _TaskAddEntry<HdxColorCorrectionTask>(
                _prefix),
            _TaskAddEntry<HdxVisualizeAovTask>(
                _prefix),
            _TaskAddEntry<HdxPresentTask>(
                _prefix),
            _TaskAddEntry<HdxPickFromRenderBufferTask>(
                _prefix),
            _TaskAddEntry<HdxBoundingBoxTask>(
                _prefix)
        });
    }
}

static
bool
_StormShadowsEnabled(HdSceneIndexBaseRefPtr const sceneIndex,
                     const SdfPath &prefix)
{
    using Task = HdxSimpleLightTask;

    const HdxSimpleLightTaskParams * const params =
        _GetTaskParamsForTask<Task>(sceneIndex, prefix);
    if (!params) {
        return false;
    }
    return params->enableShadows;
}

SdfPathVector
HdxTaskControllerSceneIndex::_GetRenderingTaskPathsForStorm() const
{
    SdfPathVector paths;

    paths.push_back(_TaskPrimPath<HdxSimpleLightTask>(_prefix));

    if (_StormShadowsEnabled(_retainedSceneIndex, _prefix)) {
        // Only enable the shadow task (which renders shadow maps) if shadows
        // are enabled.
        paths.push_back(_TaskPrimPath<HdxShadowTask>(_prefix));
    }

    paths.push_back(_TaskPrimPath<HdxSkydomeTask>(_prefix));
    paths.push_back(
        _StormRenderTaskPath(
            _prefix, HdStMaterialTagTokens->defaultMaterialTag));
    paths.push_back(
        _StormRenderTaskPath(
            _prefix, HdStMaterialTagTokens->masked));
    paths.push_back(
        _StormRenderTaskPath(
            _prefix, HdStMaterialTagTokens->additive));
    paths.push_back(
        _StormRenderTaskPath(
            _prefix, HdStMaterialTagTokens->translucent));
    // Take the aov results from the render tasks, resolve the multisample
    // images and put the results into gpu textures onto shared context.
    paths.push_back(_TaskPrimPath<HdxAovInputTask>(_prefix));
    paths.push_back(_TaskPrimPath<HdxBoundingBoxTask>(_prefix));

    // The volume render pass needs to read the (resolved) depth AOV (with
    // the opaque geometry) and thus runs after the HdxAovInputTask.
    paths.push_back(
        _StormRenderTaskPath(
            _prefix, HdStMaterialTagTokens->volume));
    // Resolve OIT data from translucent and volume and merge into color
    // target.
    paths.push_back(_TaskPrimPath<HdxOitResolveTask>(_prefix));
    paths.push_back(_TaskPrimPath<HdxSelectionTask>(_prefix));

    return paths;
}

SdfPathVector
HdxTaskControllerSceneIndex::_GetRenderingTaskPathsForGenericRenderer() const
{
    SdfPathVector paths;

    paths.push_back(_TaskPrimPath<HdxRenderTask>(_prefix));

    if (!_runGpuAovTasks) {
        return paths;
    }

    paths.push_back(_TaskPrimPath<HdxAovInputTask>(_prefix));
    paths.push_back(_TaskPrimPath<HdxBoundingBoxTask>(_prefix));

    if (_viewportAov == HdAovTokens->color) {
        // Only non-color AOVs need special colorization for viz.
        paths.push_back(_TaskPrimPath<HdxColorizeSelectionTask>(_prefix));
    }

    return paths;
}

bool
_ColorCorrectionEnabled(HdSceneIndexBaseRefPtr const &sceneIndex,
                        const SdfPath &prefix)
{
    using Task = HdxColorCorrectionTask;

    const HdxColorCorrectionTaskParams * const params =
        _GetTaskParamsForTask<Task>(sceneIndex, prefix);
    if (!params) {
        return false;
    }
    const TfToken mode = params->colorCorrectionMode;
    if (mode.IsEmpty()) {
        return false;
    }
    return mode != HdxColorCorrectionTokens->disabled;
}

SdfPathVector
HdxTaskControllerSceneIndex::GetRenderingTaskPaths() const
{
    SdfPathVector paths;
    if (_IsForStorm()) {
        paths = _GetRenderingTaskPathsForStorm();
    } else {
        paths = _GetRenderingTaskPathsForGenericRenderer();
    }

    if (!_runGpuAovTasks) {
        return paths;
    }

    if (_ColorCorrectionEnabled(_retainedSceneIndex, _prefix)) {
        // Apply color correction / grading (convert to display colors)
        paths.push_back(_TaskPrimPath<HdxColorCorrectionTask>(_prefix));
    }

    // Only non-color AOVs need special colorization for viz.
    if (_viewportAov != HdAovTokens->color) {
        paths.push_back(_TaskPrimPath<HdxVisualizeAovTask>(_prefix));
    }

    // Render pixels to screen
    paths.push_back(_TaskPrimPath<HdxPresentTask>(_prefix));

    return paths;
}

SdfPathVector
HdxTaskControllerSceneIndex::GetPickingTaskPaths() const
{
    if (_IsForStorm()) {
        return { _TaskPrimPath<HdxPickTask>(_prefix) };
    } else {
        return { _TaskPrimPath<HdxPickFromRenderBufferTask>(_prefix) };
    }
}

SdfPath
HdxTaskControllerSceneIndex::GetRenderBufferPath(const TfToken &aovName) const
{
    return _AovPath(_prefix, aovName);
}

// When we're asked to render "color", we treat that as final color,
// complete with depth-compositing and selection, so we in-line add
// some extra buffers if they weren't already requested.
static
TfTokenVector
_ResolvedRenderOutputs(const TfTokenVector &aovNames,
                       const bool isForStorm)
{
    bool hasColor = false;
    bool hasDepth = false;
    bool hasPrimId = false;
    bool hasElementId = false;
    bool hasInstanceId = false;

    for (const TfToken &renderOutput : aovNames) {
        if (renderOutput == HdAovTokens->color) {
            hasColor = true;
        }
        if (renderOutput == HdAovTokens->depth) {
            hasDepth = true;
        }
        if (renderOutput == HdAovTokens->primId) {
            hasPrimId = true;
        }
        if (renderOutput == HdAovTokens->elementId) {
            hasElementId = true;
        }
        if (renderOutput == HdAovTokens->instanceId) {
            hasInstanceId = true;
        }
    }

    TfTokenVector result = aovNames;

    if (isForStorm) {
        if (!hasDepth) {
            result.push_back(HdAovTokens->depth);
        }
    } else {
        // For a backend like PrMan/Embree we fill not just the color buffer,
        // but also buffers that are used during picking.
        if (hasColor) {
            if (!hasDepth) {
                result.push_back(HdAovTokens->depth);
            }
            if (!hasPrimId) {
                result.push_back(HdAovTokens->primId);
            }
            if (!hasElementId) {
                result.push_back(HdAovTokens->elementId);
            }
            if (!hasInstanceId) {
                result.push_back(HdAovTokens->instanceId);
            }
        }
    }

    return result;
}

void
HdxTaskControllerSceneIndex::SetRenderOutputs(
    const TfTokenVector &aovNames)
{
    if (_aovNames == aovNames) {
        return;
    }
    _aovNames = _aovNames;

    _SetRenderOutputs(_ResolvedRenderOutputs(aovNames, _IsForStorm()));

    // For AOV visualization, if only one output was specified, send it
    // to the viewer; otherwise, disable colorization.
    if (aovNames.size() == 1) {
        SetViewportRenderOutput(aovNames[0]);
    } else {
        SetViewportRenderOutput(TfToken());
    }

    // XXX: The viewport data plumbed to tasks unfortunately depends on whether
    // aovs are being used.
    _SetCameraFramingForTasks();
}

static GfVec2i
_ViewportToRenderBufferSize(const GfVec4d& viewport)
{
    // Ignore the viewport offset and use its size as the aov size.
    // XXX: This is fragile and doesn't handle viewport tricks,
    // such as camera zoom. In the future, we expect to improve the
    // API to better communicate AOV sizing, fill region and camera
    // zoom.
    return GfVec2i(viewport[2], viewport[3]);
}

static GfVec3i
_ToVec3i(const GfVec2i &v)
{
    return { v[0], v[1], 1 };
}

GfVec3i
HdxTaskControllerSceneIndex::_RenderBufferDimensions() const
{
    return _ToVec3i(
        _renderBufferSize != GfVec2i(0)
            ? _renderBufferSize
            : _ViewportToRenderBufferSize(_viewport));
}

void
HdxTaskControllerSceneIndex::_SetRenderOutputs(
    const TfTokenVector &aovNames)
{
    _retainedSceneIndex->RemovePrims({_AovScopePath(_prefix)});

    const GfVec3i dimensions = _RenderBufferDimensions();

    const uint32_t msaaSampleCount =
        std::clamp(TfGetEnvSetting(HDX_MSAA_SAMPLE_COUNT), 1, 16);

    HdRetainedSceneIndex::AddedPrimEntries addedPrimEntries;
    HdRenderPassAovBindingVector aovBindings;
    int depthAovBindingIndex = -1;

    for (const TfToken &aovName : aovNames) {
        if (!_aovDescriptorCallback) {
            TF_CODING_ERROR(
                "No aovDescriptorCallback given to "
                "HdxTaskControllerSceneIndex.");
            break;
        }

        // Use callback to get default AOV descriptors from render delegate.
        const HdAovDescriptor desc = _aovDescriptorCallback(aovName);
        if (desc.format == HdFormatInvalid) {
            // The backend doesn't support this AOV, so skip it.
            continue;
        }

        const SdfPath aovPath = _AovPath(_prefix, aovName);

        addedPrimEntries.push_back(
            { aovPath,
              HdPrimTypeTokens->renderBuffer,
              _RenderBufferPrimDataSource(
                  dimensions,
                  desc.format,
                  desc.multiSampled && msaaSampleCount > 1,
                  msaaSampleCount) });

        if (aovName == HdAovTokens->depth) {
            depthAovBindingIndex = aovBindings.size();
        }

        HdRenderPassAovBinding aovBinding;
        aovBinding.aovName = aovName;
        aovBinding.clearValue = desc.clearValue;
        aovBinding.renderBufferId = aovPath;
        aovBinding.aovSettings = desc.aovSettings;

        aovBindings.push_back(std::move(aovBinding));
    }

    _retainedSceneIndex->AddPrims(addedPrimEntries);

    const SdfPath volumeId =
        _StormRenderTaskPath(_prefix, HdStMaterialTagTokens->volume);

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    // Set AOV bindings on render tasks
    for (SdfPath const& taskPath : _renderTaskPaths) {
        HdxRenderTaskParams * const params =
            _GetTaskParamsAtPath<HdxRenderTaskParams>(
                _retainedSceneIndex, taskPath);

        if (!params) {
            continue;
        }

        params->aovBindings = aovBindings;
        if (taskPath == volumeId) {
            // The Storm Volume tasks reads the depth AOV.
            if (depthAovBindingIndex >= 0) {
                params->aovInputBindings = { aovBindings[depthAovBindingIndex] };
            }
        }

        static const HdDataSourceLocatorSet locators{
            HdLegacyTaskSchema::GetParametersLocator() };
        dirtiedPrimEntries.push_back({taskPath, locators});

        // Only the first render task clears the AOVs - so erase the clearValue.
        for (HdRenderPassAovBinding &aovBinding : aovBindings) {
            aovBinding.clearValue = VtValue();
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::SetViewportRenderOutput(const TfToken &aovName)
{
    if (_viewportAov == aovName) {
        return;
    }
    _viewportAov = aovName;

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    {
        using Task = HdxAovInputTask;

        if (HdxAovInputTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            params->aovBufferPath = SdfPath::EmptyPath();
            params->depthBufferPath = SdfPath::EmptyPath();
            if (!aovName.IsEmpty()) {
                params->aovBufferPath = _AovPath(_prefix, aovName);
            }
            if (aovName == HdAovTokens->color) {
                params->depthBufferPath = _AovPath(_prefix, HdAovTokens->depth);
            }

            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    {
        using Task = HdxColorizeSelectionTask;

        if (HdxColorizeSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (aovName == HdAovTokens->color) {
                // If we're rendering color, make sure the colorize selection
                // task has the proper id buffers...
                params->primIdBufferPath =
                    _AovPath(_prefix, HdAovTokens->primId);
                params->instanceIdBufferPath =
                    _AovPath(_prefix, HdAovTokens->instanceId);
                params->elementIdBufferPath =
                    _AovPath(_prefix, HdAovTokens->elementId);
            } else {
                params->primIdBufferPath = SdfPath::EmptyPath();
                params->instanceIdBufferPath = SdfPath::EmptyPath();
                params->elementIdBufferPath = SdfPath::EmptyPath();
            }

            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    {
        using Task = HdxPickFromRenderBufferTask;

        if (HdxPickFromRenderBufferTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (aovName == HdAovTokens->color) {
                // If we're rendering color, make sure the pick task has the
                // proper id & depth buffers...
                params->primIdBufferPath =
                    _AovPath(_prefix, HdAovTokens->primId);
                params->instanceIdBufferPath =
                    _AovPath(_prefix, HdAovTokens->instanceId);
                params->elementIdBufferPath =
                    _AovPath(_prefix, HdAovTokens->elementId);
                params->depthBufferPath =
                    _AovPath(_prefix, HdAovTokens->depth);
            } else {
                params->primIdBufferPath = SdfPath::EmptyPath();
                params->instanceIdBufferPath = SdfPath::EmptyPath();
                params->elementIdBufferPath = SdfPath::EmptyPath();
                params->depthBufferPath = SdfPath::EmptyPath();
            }

            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    {
        using Task = HdxColorCorrectionTask;

        if (HdxColorCorrectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            params->aovName = aovName;

            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    {
        using Task = HdxVisualizeAovTask;

        if (HdxVisualizeAovTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            params->aovName = aovName;

            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    {
        using Task = HdxBoundingBoxTask;

        if (HdxBoundingBoxTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            params->aovName = aovName;

            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::SetRenderOutputSettings(
    const TfToken &aovName,
    const HdAovDescriptor &desc)
{
    // HdAovDescriptor contains data for both the renderbuffer descriptor,
    // and the renderpass aov binding. Update them both.

    // Render buffer descriptor

    const SdfPath renderBufferPath = _AovPath(_prefix, aovName);

    HdSceneIndexPrim const prim =
        _retainedSceneIndex->GetPrim(renderBufferPath);
    _RenderBufferSchemaDataSourceHandle const ds =
        _RenderBufferSchemaDataSource::Cast(
            HdRenderBufferSchema::GetFromParent(prim.dataSource)
                .GetContainer());
    // Check if we're setting a value for a nonexistent AOV.
    if (!ds) {
        TF_WARN("Render output %s doesn't exist", aovName.GetText());
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    {
        HdDataSourceLocatorSet dirtyLocators;

        if (ds->format != desc.format) {
            ds->format = desc.format;
            dirtyLocators.insert(HdRenderBufferSchema::GetFormatLocator());
        }
        if (ds->multiSampled != desc.multiSampled) {
            ds->multiSampled = desc.multiSampled;
            dirtyLocators.insert(HdRenderBufferSchema::GetMultiSampledLocator());
        }

        if (!dirtyLocators.IsEmpty()) {
            dirtiedPrimEntries.push_back({renderBufferPath, dirtyLocators});
        }
    }

    // Render pass AOV bindings

    VtValue clearValue = desc.clearValue;

    for (const SdfPath &taskPath : _renderTaskPaths) {

        HdxRenderTaskParams * const params =
            _GetTaskParamsAtPath<HdxRenderTaskParams>(
                _retainedSceneIndex, taskPath);

        if (!params) {
            continue;
        }

        for (HdRenderPassAovBinding &aovBinding : params->aovBindings) {
            if (aovBinding.renderBufferId != renderBufferPath) {
                continue;
            }
            bool changed = false;
            if (aovBinding.clearValue != clearValue) {
                aovBinding.clearValue = clearValue;
                changed = true;
            }
            if (aovBinding.aovSettings != desc.aovSettings) {
                aovBinding.aovSettings = desc.aovSettings;
                changed = true;
            }
            if (changed) {
                static const HdDataSourceLocatorSet locators{
                    HdLegacyTaskSchema::GetParametersLocator() };
                dirtiedPrimEntries.push_back({taskPath, locators});
            }
            break;
        }

        // Only the first RenderTask should clear the AOV
        clearValue = VtValue();
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

HdAovDescriptor
HdxTaskControllerSceneIndex::GetRenderOutputSettings(const TfToken &aovName) const
{
    const SdfPath renderBufferPath = _AovPath(_prefix, aovName);

    HdSceneIndexPrim const prim =
        _retainedSceneIndex->GetPrim(renderBufferPath);
    _RenderBufferSchemaDataSourceHandle const ds =
        _RenderBufferSchemaDataSource::Cast(
            HdRenderBufferSchema::GetFromParent(prim.dataSource)
                .GetContainer());
    if (!ds) {
        // Check if we're getting a value for a nonexistent AOV.
        return {};
    }

    HdAovDescriptor desc;
    desc.format = ds->format;
    desc.multiSampled = ds->multiSampled;

    if (_renderTaskPaths.empty()) {
        return desc;
    }

    const SdfPath& taskPath = _renderTaskPaths.front();

    HdxRenderTaskParams * const params =
        _GetTaskParamsAtPath<HdxRenderTaskParams>(
            _retainedSceneIndex, taskPath);

    if (!params) {
        return desc;
    }

    for (const HdRenderPassAovBinding &aovBinding : params->aovBindings) {
        if (aovBinding.renderBufferId != renderBufferPath) {
            continue;
        }
        desc.clearValue = aovBinding.clearValue;
        desc.aovSettings = aovBinding.aovSettings;
        break;
    }

    return desc;
}


void
HdxTaskControllerSceneIndex::SetCollection(const HdRprimCollection &collection)
{
    // XXX For now we assume the application calling to set a new
    //     collection does not know or setup the material tags and does not
    //     split up the collection according to material tags.
    //     In order to ignore materialTags when comparing collections we need
    //     to copy the old tag into the new collection. Since the provided
    //     collection is const, we need to make a not-ideal copy.
    HdRprimCollection newCollection = collection;

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    for (const SdfPath &taskPath : _renderTaskPaths) {
        HdRprimCollection * const taskCollection =
            _GetCollectionAtPath(_retainedSceneIndex, taskPath);
        if (!taskCollection) {
            continue;
        }

        newCollection.SetMaterialTag(taskCollection->GetMaterialTag());

        if (*taskCollection == newCollection) {
            continue;
        }

        *taskCollection = newCollection;

        static const HdDataSourceLocatorSet locators{
            HdLegacyTaskSchema::GetCollectionLocator() };
        dirtiedPrimEntries.push_back({ taskPath, locators });
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::SetRenderParams(const HdxRenderTaskParams &params)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    for (const SdfPath &taskPath : _renderTaskPaths) {
        HdxRenderTaskParams * const taskParams =
            _GetTaskParamsAtPath<HdxRenderTaskParams>(
                _retainedSceneIndex, taskPath);
        if (!taskParams) {
            TF_CODING_ERROR(
                "Expected task params for task %s",
                taskPath.GetText());
            continue;
        }

        // We explicitly ignore input camera, viewport, aovBindings, and
        // aov multisample settings because these are internally managed.
        HdxRenderTaskParams newParams = params;
        newParams.camera = taskParams->camera;
        newParams.viewport = taskParams->viewport;
        newParams.framing = taskParams->framing;
        newParams.overrideWindowPolicy = taskParams->overrideWindowPolicy;
        newParams.aovBindings = taskParams->aovBindings;
        newParams.aovInputBindings = taskParams->aovInputBindings;
        newParams.useAovMultiSample = taskParams->useAovMultiSample;
        newParams.resolveAovMultiSample = taskParams->resolveAovMultiSample;

        // We also explicitly manage blend params, set earlier based on the
        // material tag.
        newParams.blendEnable = taskParams->blendEnable;
        newParams.depthMaskEnable = taskParams->depthMaskEnable;
        newParams.enableAlphaToCoverage = taskParams->enableAlphaToCoverage;
        newParams.blendColorOp = taskParams->blendColorOp;
        newParams.blendColorSrcFactor = taskParams->blendColorSrcFactor;
        newParams.blendColorDstFactor = taskParams->blendColorDstFactor;
        newParams.blendAlphaOp = taskParams->blendAlphaOp;
        newParams.blendAlphaSrcFactor = taskParams->blendAlphaSrcFactor;
        newParams.blendAlphaDstFactor = taskParams->blendAlphaDstFactor;
        newParams.depthMaskEnable = taskParams->depthMaskEnable;

        if (*taskParams == newParams) {
            continue;
        }

        *taskParams = newParams;

        static const HdDataSourceLocatorSet locators{
            HdLegacyTaskSchema::GetParametersLocator() };
        dirtiedPrimEntries.push_back({ taskPath, locators });
    }

    // Update shadow task in case materials have been enabled/disabled
    {
        using Task = HdxShadowTask;
        if (HdxShadowTaskParams * const taskParams =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (taskParams->enableSceneMaterials !=
                                params.enableSceneMaterials) {
                taskParams->enableSceneMaterials =
                                params.enableSceneMaterials;
                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    // Update pick task
    {
        using Task = HdxPickTask;
        if (HdxPickTaskParams * const taskParams =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (taskParams->cullStyle != params.cullStyle ||
                taskParams->enableSceneMaterials !=
                                params.enableSceneMaterials) {
                taskParams->cullStyle = params.cullStyle;
                taskParams->enableSceneMaterials =
                                params.enableSceneMaterials;
                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}


void
HdxTaskControllerSceneIndex::SetRenderTags(const TfTokenVector &renderTags)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    for (const SdfPath &taskPath : _renderTaskPaths) {
        TfTokenVector * const taskRenderTags =
            _GetRenderTagsAtPath<HdxRenderTaskParams>(
                _retainedSceneIndex, taskPath);
        if (!taskRenderTags) {
            continue;
        }
        if (*taskRenderTags == renderTags) {
            continue;
        }
        *taskRenderTags = renderTags;

        static const HdDataSourceLocatorSet locators{
            HdLegacyTaskSchema::GetRenderTagsLocator() };
        dirtiedPrimEntries.push_back({ taskPath, locators });
    }

    {
        using Task = HdxPickTask;

        if (TfTokenVector * const taskRenderTags =
                _GetRenderTagsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (*taskRenderTags != renderTags) {
                *taskRenderTags = renderTags;

                const SdfPath taskPath = _TaskPrimPath<Task>(_prefix);
                static const HdDataSourceLocatorSet locators{
                    HdLegacyTaskSchema::GetRenderTagsLocator() };
                dirtiedPrimEntries.push_back({ taskPath, locators });
            }
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}


void
HdxTaskControllerSceneIndex::SetShadowParams(const HdxShadowTaskParams &params)
{
    using Task = HdxShadowTask;

    HdxShadowTaskParams * const taskParams =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!taskParams) {
        return;
    }

    HdxShadowTaskParams newParams = params;
    newParams.enableSceneMaterials = taskParams->enableSceneMaterials;

    if (*taskParams == newParams) {
        return;
    }
    *taskParams = newParams;

    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetEnableShadows(const bool enable)
{
    using Task = HdxSimpleLightTask;

    HdxSimpleLightTaskParams * const params =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!params) {
        return;
    }
    if (params->enableShadows == enable) {
        return;
    }
    params->enableShadows = enable;
    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetEnableSelection(const bool enable)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    {
        using Task = HdxSelectionTask;

        if (HdxSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->enableSelectionHighlight != enable ||
                params->enableLocateHighlight != enable) {
                params->enableSelectionHighlight = enable;
                params->enableLocateHighlight = enable;

                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    {
        using Task = HdxColorizeSelectionTask;

        if (HdxColorizeSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->enableSelectionHighlight != enable ||
                params->enableLocateHighlight != enable) {
                params->enableSelectionHighlight = enable;
                params->enableLocateHighlight = enable;

                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::SetSelectionColor(const GfVec4f &color)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    {
        using Task = HdxSelectionTask;

        if (HdxSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->selectionColor != color) {
                params->selectionColor = color;
                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    {
        using Task = HdxColorizeSelectionTask;

        if (HdxColorizeSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->selectionColor != color) {
                params->selectionColor = color;
                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::SetSelectionLocateColor(const GfVec4f &color)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    {
        using Task = HdxSelectionTask;

        if (HdxSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->locateColor != color) {
                params->locateColor = color;
                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    {
        using Task = HdxColorizeSelectionTask;

        if (HdxColorizeSelectionTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->locateColor != color) {
                params->locateColor = color;
                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::SetSelectionEnableOutline(const bool enableOutline)
{
    using Task = HdxColorizeSelectionTask;

    HdxColorizeSelectionTaskParams * const params =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!params) {
        return;
    }

    if (params->enableOutline == enableOutline) {
        return;
    }
    params->enableOutline = enableOutline;
    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetSelectionOutlineRadius(
    const unsigned int outlineRadius)
{
    using Task = HdxColorizeSelectionTask;

    HdxColorizeSelectionTaskParams * const params =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!params) {
        return;
    }

    if (params->outlineRadius == outlineRadius) {
        return;
    }
    params->outlineRadius = outlineRadius;
    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

static
const TfToken &
_GetPrimType(const GlfSimpleLight &light, const bool isForStorm)
{
    if (light.IsDomeLight()) {
        return HdPrimTypeTokens->domeLight;
    }
    if (isForStorm) {
        return HdPrimTypeTokens->simpleLight;
    } else {
        return HdPrimTypeTokens->distantLight;
    }
}

void
HdxTaskControllerSceneIndex::_SetLights(const GlfSimpleLightVector &lights)
{
    // HdxTaskController inserts a set of light prims to represent the lights
    // passed in through the simple lighting context (lights vector). These are 
    // managed by the task controller scene index, and not by scene description;
    // they represent the application state.
    
    size_t i = 0;

    HdRetainedSceneIndex::AddedPrimEntries addedPrimEntries;
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;
    HdSceneIndexObserver::RemovedPrimEntries removedPrimEntries;

    for (; i < lights.size(); ++i) {
        const GlfSimpleLight &light = lights[i];
        const SdfPath primPath = _LightPath(_prefix, i);
        const TfToken &primType = _GetPrimType(light, _isForStorm);

        const HdSceneIndexPrim prim = _retainedSceneIndex->GetPrim(primPath);
        if (prim.primType == primType) {
            _LightPrimDataSourceHandle const ds =
                _LightPrimDataSource::Cast(prim.dataSource);
            if (!TF_VERIFY(ds)) {
                continue;
            }
            if (*ds->light == light) {
                continue;
            }
            *ds->light = light;

            static const HdDataSourceLocatorSet dirtyLocators{
                HdLightSchema::GetDefaultLocator(),
                HdMaterialSchema::GetDefaultLocator(),
                HdXformSchema::GetDefaultLocator()};
            dirtiedPrimEntries.push_back({ primPath, dirtyLocators });
        } else {
            addedPrimEntries.push_back(
                { primPath,
                  primType,
                  _LightPrimDataSource::New(light, _IsForStorm()) });
        }
    }

    while (true) {
        const SdfPath primPath = _LightPath(_prefix, i);
        const HdSceneIndexPrim prim = _retainedSceneIndex->GetPrim(primPath);
        if (!prim.dataSource) {
            break;
        }
        removedPrimEntries.push_back({primPath});
        ++i;
    }

    if (!addedPrimEntries.empty()) {
        _retainedSceneIndex->AddPrims(addedPrimEntries);
    }
    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
    if (!removedPrimEntries.empty()) {
        _retainedSceneIndex->RemovePrims(removedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::_SetSimpleLightTaskParams(
    GlfSimpleLightingContextPtr const &src)
{
    // If simpleLightTask exists, process the lighting context's material
    // parameters as well. These are passed in through the simple light task's
    // "params" field, so we need to update that field if the material
    // parameters changed.
    //
    // It's unfortunate that the lighting context is split this way.

    using Task = HdxSimpleLightTask;

    HdxSimpleLightTaskParams * const params =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!params) {
        return;
    }

    if (params->sceneAmbient == src->GetSceneAmbient() &&
        params->material == src->GetMaterial()) {
        return;
    }

    params->sceneAmbient = src->GetSceneAmbient();
    params->material = src->GetMaterial();

    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    if (!src) {
        TF_CODING_ERROR("Null lighting context");
        return;
    }

    // Process the Built-in lights
    _SetLights(src->GetLights());

    _SetSimpleLightTaskParams(src);
}

void
HdxTaskControllerSceneIndex::SetRenderViewport(GfVec4d const& viewport)
{
    if (_viewport == viewport) {
        return;
    }
    _viewport = viewport;

    // Update the params for tasks that consume viewport info.
    _SetCameraFramingForTasks();

    // Update all of the render buffer sizes as well.
    _SetRenderBufferSize();
}

void
HdxTaskControllerSceneIndex::SetRenderBufferSize(const GfVec2i &size)
{
    if (_renderBufferSize == size) {
        return;
    }

    _renderBufferSize = size;

    _SetRenderBufferSize();
}

void
HdxTaskControllerSceneIndex::SetFraming(const CameraUtilFraming &framing)
{
    _framing = framing;
    _SetCameraFramingForTasks();
}

void
HdxTaskControllerSceneIndex::SetOverrideWindowPolicy(
    const std::optional<CameraUtilConformWindowPolicy> &policy)
{
    _overrideWindowPolicy = policy;
    _SetCameraFramingForTasks();
}

void
HdxTaskControllerSceneIndex::SetFreeCameraMatrices(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix)
{
    const SdfPath primPath = _CameraPath(_prefix);

    HdSceneIndexPrim const prim = _retainedSceneIndex->GetPrim(primPath);
    auto const ds = HdxFreeCameraPrimDataSource::Cast(prim.dataSource);
    if (!ds) {
        TF_CODING_ERROR("No camera at %s in retained scene index.",
                        primPath.GetText());
        return;
    }

    HdDataSourceLocatorSet locators;
    ds->SetViewAndProjectionMatrix(viewMatrix, projectionMatrix, &locators);

    if (locators.IsEmpty()) {
        return;
    }

    _retainedSceneIndex->DirtyPrims(
        { { primPath, std::move(locators) } });

    SetCameraPath(primPath);
}

void
HdxTaskControllerSceneIndex::
SetFreeCameraClipPlanes(const std::vector<GfVec4d> &clippingPlanes)
{
    const SdfPath primPath = _CameraPath(_prefix);

    HdSceneIndexPrim const prim = _retainedSceneIndex->GetPrim(primPath);
    auto const ds = HdxFreeCameraPrimDataSource::Cast(prim.dataSource);
    if (!ds) {
        TF_CODING_ERROR("No camera at %s in retained scene index.",
                        primPath.GetText());
        return;
    }

    HdDataSourceLocatorSet locators;
    ds->SetClippingPlanes({clippingPlanes.begin(), clippingPlanes.end()});

    if (locators.IsEmpty()) {
        return;
    }

    _retainedSceneIndex->DirtyPrims(
        { { primPath, std::move(locators) } });
}

void
HdxTaskControllerSceneIndex::SetColorCorrectionParams(
    const HdxColorCorrectionTaskParams &params)
{
    using Task = HdxColorCorrectionTask;

    HdxColorCorrectionTaskParams * const taskParams =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!taskParams) {
        return;
    }

    HdxColorCorrectionTaskParams newParams = params;
    newParams.aovName = taskParams->aovName;
    if (*taskParams == newParams) {
        return;
    }
    *taskParams = newParams;

    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetBBoxParams(
    const HdxBoundingBoxTaskParams &params)
{
    using Task = HdxBoundingBoxTask;

    HdxBoundingBoxTaskParams * const taskParams =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!taskParams) {
        return;
    }

    // We only take the params that will be coming from outside this
    // HdxTaskControllerSceneIndex instance.
    HdxBoundingBoxTaskParams newParams = *taskParams;
    newParams.bboxes = params.bboxes;
    newParams.color = params.color;
    newParams.dashSize = params.dashSize;

    if (*taskParams == newParams) {
        return;
    }
    *taskParams = newParams;

    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetEnablePresentation(const bool enabled)
{
    using Task = HdxPresentTask;

    HdxPresentTaskParams * const params =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!params) {
        return;
    }
    if (params->enabled == enabled) {
        return;
    }
    params->enabled = enabled;

    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetPresentationOutput(
    const TfToken &api, const VtValue &framebuffer)
{
    using Task = HdxPresentTask;

    HdxPresentTaskParams * const params =
        _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix);
    if (!params) {
        return;
    }
    if (params->dstApi == api && params->dstFramebuffer == framebuffer) {
        return;
    }
    params->dstApi = api;
    params->dstFramebuffer = framebuffer;

    _SendDirtyParamsEntry<Task>(_retainedSceneIndex, _prefix);
}

void
HdxTaskControllerSceneIndex::SetCameraPath(const SdfPath &id)
{
    if (_activeCameraId == id) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    _activeCameraId = id;

    // Update tasks that take a camera task param.
    for (const SdfPath &taskPath : _renderTaskPaths) {
        HdxRenderTaskParams * const params =
            _GetTaskParamsAtPath<HdxRenderTaskParams>(
                _retainedSceneIndex, taskPath);
        if (!params) {
            continue;
        }
        params->camera = _activeCameraId;

        static const HdDataSourceLocatorSet locators{
            HdLegacyTaskSchema::GetParametersLocator() };
        dirtiedPrimEntries.push_back({taskPath, locators});
    }

    {
        using Task = HdxSimpleLightTask;

        if (HdxSimpleLightTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            params->cameraPath = _activeCameraId;
            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    {
        using Task = HdxPickFromRenderBufferTask;

        if (HdxPickFromRenderBufferTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            params->cameraId = _activeCameraId;
            _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

static
bool
_UsingAovs(HdSceneIndexBaseRefPtr const sceneIndex,
           const SdfPath &prefix)
{
    const SdfPath path = _AovScopePath(prefix);
    return !sceneIndex->GetChildPrimPaths(path).empty();
}

static
GfVec4i
_ToVec4i(const GfVec4d &v)
{
    return GfVec4i(int(v[0]), int(v[1]), int(v[2]), int(v[3]));
}

void
HdxTaskControllerSceneIndex::_SetCameraFramingForTasks()
{
    // When aovs are in use, the expectation is that each aov is resized to
    // the non-masked region and we render only the necessary pixels.
    // The composition step (i.e., the present task) uses the viewport
    // offset to update the unmasked region of the bound framebuffer.
    const GfVec4d adjustedViewport =
        _UsingAovs(_retainedSceneIndex, _prefix)
            ? GfVec4d(0, 0, _viewport[2], _viewport[3])
            : _viewport;

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    for (const SdfPath &taskPath : _renderTaskPaths) {
        HdxRenderTaskParams * const params =
            _GetTaskParamsAtPath<HdxRenderTaskParams>(
                _retainedSceneIndex, taskPath);
        if (!params) {
            continue;
        }

        if (params->viewport == adjustedViewport &&
            params->framing == _framing &&
            params->overrideWindowPolicy == _overrideWindowPolicy) {
            continue;
        }

        params->viewport = adjustedViewport;
        params->framing = _framing;
        params->overrideWindowPolicy = _overrideWindowPolicy;

        static const HdDataSourceLocatorSet locators{
            HdLegacyTaskSchema::GetParametersLocator() };
        dirtiedPrimEntries.push_back({taskPath, locators});
    }

    {
        using Task = HdxPickFromRenderBufferTask;

        if (HdxPickFromRenderBufferTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            if (params->viewport != adjustedViewport ||
                params->framing != _framing ||
                params->overrideWindowPolicy != _overrideWindowPolicy) {

                params->framing = _framing;
                params->overrideWindowPolicy = _overrideWindowPolicy;
                params->viewport = adjustedViewport;

                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    {
        using Task = HdxPresentTask;

        if (HdxPresentTaskParams * const params =
                _GetTaskParamsForTask<Task>(_retainedSceneIndex, _prefix)) {
            // The composition step uses the viewport passed in by the
            // application, which may have a non-zero offset for things like
            // camera masking.
            const GfVec4i dstRegion =
                _framing.IsValid()
                    ? GfVec4i(0, 0, _renderBufferSize[0], _renderBufferSize[1])
                    : _ToVec4i(_viewport);

            if (params->dstRegion != dstRegion) {
                params->dstRegion = dstRegion;

                _AddDirtyParamsEntry<Task>(_prefix, &dirtiedPrimEntries);
            }
        }
    }

    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::_SetRenderBufferSize()
{
    const GfVec3i dimensions = _RenderBufferDimensions();

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrimEntries;

    const SdfPath path = _AovScopePath(_prefix);
    for (const SdfPath &renderBufferPath :
             _retainedSceneIndex->GetChildPrimPaths(path)) {
        HdSceneIndexPrim const prim =
            _retainedSceneIndex->GetPrim(renderBufferPath);
        _RenderBufferSchemaDataSourceHandle const ds =
            _RenderBufferSchemaDataSource::Cast(
                HdRenderBufferSchema::GetFromParent(prim.dataSource)
                    .GetContainer());
        if (!ds) {
            continue;
        }

        if (ds->dimensions == dimensions) {
            continue;
        }
        ds->dimensions = dimensions;

        static const HdDataSourceLocatorSet locators{
            HdRenderBufferSchema::GetDimensionsLocator()};
        dirtiedPrimEntries.push_back({renderBufferPath, locators});
    }
    if (!dirtiedPrimEntries.empty()) {
        _retainedSceneIndex->DirtyPrims(dirtiedPrimEntries);
    }
}

void
HdxTaskControllerSceneIndex::_Observer::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    _owner->_SendPrimsAdded(entries);
}

void
HdxTaskControllerSceneIndex::_Observer::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    _owner->_SendPrimsRemoved(entries);
}

void
HdxTaskControllerSceneIndex::_Observer::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    _owner->_SendPrimsDirtied(entries);
}

void
HdxTaskControllerSceneIndex::_Observer::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    _owner->_SendPrimsRenamed(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
