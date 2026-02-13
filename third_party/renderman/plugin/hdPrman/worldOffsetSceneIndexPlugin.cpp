//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license
//
#include "hdPrman/worldOffsetSceneIndexPlugin.h"

#include "hdPrman/tokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_WorldOffsetSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Place towards the end of the chain.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 999;
    for (const auto& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName, HdPrmanPluginTokens->worldOffset, nullptr,
            insertionPhase, HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
    }
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (camera)
    ((riTraceWorldOffset, "ri:trace:worldoffset"))
    ((riTraceWorldOrigin, "ri:trace:worldorigin"))
    ((worldOffset, "worldoffset"))
);


static bool
_IsInstancePrototype(const HdContainerDataSourceHandle& primDs)
{
    HdInstancedBySchema instancedBy =
        HdInstancedBySchema::GetFromParent(primDs);
    if (!instancedBy) 
        return false;
    HdPathArrayDataSourceHandle pathsDs = instancedBy.GetPaths();
    if (!pathsDs) 
        return false;
    return !pathsDs->GetTypedValue(0.0f).empty();
}

static bool
_HasXform(const HdContainerDataSourceHandle& primDs)
{
    const TfTokenVector names = primDs->GetNames();
    return std::find(names.begin(), names.end(), HdXformSchemaTokens->xform)
        != names.end();
}

// Compute the worldoffset value for the input scene.
static GfVec3d
_ComputeWorldOffset(
    HdSceneIndexBaseRefPtr const& inputScene,
    SdfPath *renderSettingsPathUsed,
    SdfPath *cameraPathUsed)
{
    TRACE_FUNCTION();

    // RenderMan default for worldoffset.
    GfVec3d worldOffset(0.0);

    // Presume no settings or camera has been involved until we find otherwise.
    *renderSettingsPathUsed = SdfPath();
    *cameraPathUsed = SdfPath();

    // Retrieve the namespaced render settings from scene globals.
    const auto sceneGlobals =
        HdSceneGlobalsSchema::GetFromSceneIndex(inputScene);
    if (!sceneGlobals.IsDefined()) {
        return worldOffset;
    }
    const auto renderSettingsPrimPathDS =
        sceneGlobals.GetActiveRenderSettingsPrim();
    if (!renderSettingsPrimPathDS) {
        return worldOffset;
    }
    const SdfPath renderSettingsPrimPath =
        renderSettingsPrimPathDS->GetTypedValue(0.0f);
    const HdSceneIndexPrim renderSettingsPrim =
        inputScene->GetPrim(renderSettingsPrimPath);
    *renderSettingsPathUsed = renderSettingsPrimPath;
    HdRenderSettingsSchema rsSchema =
        HdRenderSettingsSchema::GetFromParent(renderSettingsPrim.dataSource);
    if (!rsSchema.IsDefined()) {
        return worldOffset;
    }
    HdContainerDataSourceHandle namespacedSettingsDS =
#if HD_API_VERSION >= 89
        rsSchema.GetNamespacedSettings().GetContainer();
#else
        rsSchema.GetNamespacedSettings();
#endif
    if (!namespacedSettingsDS) {
        return worldOffset;
    }

    // Check settings for trace:worldoffset.
    const auto worldOffsetDS =
        HdVec3dDataSource::Cast(
            namespacedSettingsDS->Get(_tokens->riTraceWorldOffset));
    if (worldOffsetDS) {
        worldOffset = worldOffsetDS->GetTypedValue(0.0f);
    }

    // Check settings for trace:worldorigin.
    const auto worldOriginDS =
        HdTokenDataSource::Cast(
            namespacedSettingsDS->Get(_tokens->riTraceWorldOrigin));
    if (!worldOriginDS) {
        // RenderMan default worldorigin is "world",
        // so no further action is needed.
        return worldOffset;
    }
    const TfToken worldOrigin = worldOriginDS->GetTypedValue(0.0f);
    if (worldOrigin != _tokens->camera) {
        // No need to override worldOrigin.
        return worldOffset;
    }

    // At this point we know we want to use the camera position as worldoffset.
    // Next step is to find the primary camera xform.
    auto cameraPathDS = sceneGlobals.GetPrimaryCameraPrim();
    if (!cameraPathDS) {
        TF_WARN("HdPrman: trace:worldorigin is set to 'camera' but no camera was provided "
                "in scene globals.");
        return worldOffset;
    }

    // Use the camera's local origin as the world offset.
    const SdfPath cameraPath = cameraPathDS->GetTypedValue(0.0f);
    const HdSceneIndexPrim cameraPrim = inputScene->GetPrim(cameraPath);
    const auto cameraXformDs = HdXformSchema::GetFromParent(cameraPrim.dataSource);
    if (!cameraXformDs) {
        return worldOffset;
    }
    const HdMatrixDataSourceHandle matrixDS = cameraXformDs.GetMatrix();
    if (!matrixDS) {
        return worldOffset;
    }
    const GfMatrix4d cameraXform = matrixDS->GetTypedValue(0.0f);
    *cameraPathUsed = cameraPath;
    // Add the camera position to any existing world offset.
    worldOffset += cameraXform.ExtractTranslation();
    return worldOffset;
}

static HdSceneIndexPrim
_ApplyWorldOffsetToRenderSettings(
    HdSceneIndexPrim const& prim,
    GfVec3d worldOffset)
{
    static const auto worldOffsetDs =
        HdCreateTypedRetainedDataSource(VtValue(_tokens->worldOffset));
    return {
        prim.primType,
        HdContainerDataSourceEditor(prim.dataSource)
            // ri:trace:worldorigin -> "worldoffset"
            .Set(
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riTraceWorldOrigin),
                worldOffsetDs)
            // ri:trace:worldoffset -> worldOffset
            .Set(
                HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                    .Append(_tokens->riTraceWorldOffset),
                HdCreateTypedRetainedDataSource(VtValue(worldOffset)))
            .Finish()
    };
}

/////////////////////////////////////
// World Offset Matrix Data Source //
/////////////////////////////////////

class _WorldOffsetMatrixDataSource final : public HdMatrixDataSource
{
public:
    using Time = HdSampledDataSource::Time;

    HD_DECLARE_DATASOURCE(_WorldOffsetMatrixDataSource);

    _WorldOffsetMatrixDataSource(
        const HdMatrixDataSourceHandle& input, const GfVec3d& worldOffset)
        : _input(input)
        , _worldOffset(worldOffset)
    {
    }

    GfMatrix4d GetTypedValue(Time shutterOffset) override
    {
        if (!_input)
            return GfMatrix4d();

        GfMatrix4d matrix = _input->GetTypedValue(shutterOffset);
        matrix[3][0] -= _worldOffset[0];
        matrix[3][1] -= _worldOffset[1];
        matrix[3][2] -= _worldOffset[2];
        return matrix;
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        if (!_input)
            return false;

        return _input->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    const HdMatrixDataSourceHandle _input;
    const GfVec3d _worldOffset;
};

HD_DECLARE_DATASOURCE_HANDLES(_WorldOffsetMatrixDataSource);

////////////////////////////////////
// World Offset Xform Data Source //
////////////////////////////////////

class _WorldOffsetXformDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_WorldOffsetXformDataSource);

    _WorldOffsetXformDataSource(
        const HdContainerDataSourceHandle& input, const GfVec3d& worldOffset)
        : _input(input)
        , _worldOffset(worldOffset)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_input)
            return {};

        return _input->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_input)
            return nullptr;

        const HdDataSourceBaseHandle dataSource = _input->Get(name);

        if (name == HdXformSchemaTokens->matrix) {
            if (HdMatrixDataSourceHandle matrixSource = HdMatrixDataSource::Cast(dataSource)) {
                return _WorldOffsetMatrixDataSource::New(matrixSource, _worldOffset);
            }
        }

        return dataSource;
    }

#if PXR_VERSION < 2302
    bool Has(const TfToken& name) override
    {
        return _input->Has(name);
    }
#endif

private:
    const HdContainerDataSourceHandle _input;
    const GfVec3d _worldOffset;
};

HD_DECLARE_DATASOURCE_HANDLES(_WorldOffsetXformDataSource);

///////////////////////////////////
// World Offset Prim Data Source //
///////////////////////////////////

class _WorldOffsetPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_WorldOffsetPrimDataSource);

    _WorldOffsetPrimDataSource(
        const HdContainerDataSourceHandle& input, const GfVec3d& worldOffset)
        : _input(input)
        , _worldOffset(worldOffset)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_input)
            return {};

        return _input->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_input)
            return nullptr;

        const HdDataSourceBaseHandle dataSource = _input->Get(name);

        if (name == HdXformSchemaTokens->xform) {
            if (HdXformSchema xformSchema = HdXformSchema(HdContainerDataSource::Cast(dataSource))) {
                if (HdBoolDataSourceHandle resetXformStack = xformSchema.GetResetXformStack()) {
                    // Apply _worldOffset anywhere we see a transform from world space.
                    if (resetXformStack->GetTypedValue(0.0f)) {
                        return _WorldOffsetXformDataSource::New(xformSchema.GetContainer(), _worldOffset);
                    }
                }
            }
        }

        return dataSource;
    }

#if PXR_VERSION < 2302
    bool Has(const TfToken& name) override
    {
        return _input->Has(name);
    }
#endif

private:
    const HdContainerDataSourceHandle _input;
    const GfVec3d _worldOffset;
};

HD_DECLARE_DATASOURCE_HANDLES(_WorldOffsetPrimDataSource);

//////////////////////////////
// World Offset Scene Index //
//////////////////////////////

TF_DECLARE_REF_PTRS(_WorldOffsetSceneIndex);

#if PXR_VERSION >= 2505

// This version uses scene globals to specify worldorigin and worldoffset.
class _WorldOffsetSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _WorldOffsetSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(new _WorldOffsetSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (_worldOffset == GfVec3d(0.0)) {
            // No _worldOffset to apply.
            return prim;
        }

        if (!prim.dataSource || _IsInstancePrototype(prim.dataSource)) {
            // Do not apply _worldOffset to instance prototypes.
            // Instead, _worldOffset will be applied to the instances.
            return prim;
        }

        if (prim.primType == HdPrimTypeTokens->renderSettings) {
            if (primPath == _renderSettingsPath) {
                // Apply _worldOffset to the render settings.
                return _ApplyWorldOffsetToRenderSettings(prim, _worldOffset);
            }
            return prim;
        }

        if (primPath == SdfPath::AbsoluteRootPath() ||
            (prim.primType == HdPrimTypeTokens->instancer &&
             !_HasXform(prim.dataSource)))
        {
            // Apply _worldOffset to the absolute root and to
            // instancers that do not already provide an xform.
            // Note that we have handled instance prototypes above,
            // so this will not apply to nested instancers, only
            // top-level instancers.
            const static HdDataSourceBaseHandle xformDataSource =
                HdXformSchema::BuildRetained(
                    HdRetainedTypedSampledDataSource<GfMatrix4d>::
                        New(GfMatrix4d().SetIdentity()),
                    HdRetainedTypedSampledDataSource<bool>::New(true)
                );
            const static HdContainerDataSourceHandle xformContainerDataSource =
                HdRetainedContainerDataSource::New(1,
                    &HdXformSchemaTokens->xform, &xformDataSource);
            HdContainerDataSourceHandle handles[2] = {
                xformContainerDataSource, prim.dataSource };
            return {
                prim.primType,
                _WorldOffsetPrimDataSource::New(
                    HdOverlayContainerDataSource::New(2, handles),
                    _worldOffset
                )
            };
        }

        // Apply _worldOffset to all other prims.
        return {
            prim.primType,
            _WorldOffsetPrimDataSource::New(prim.dataSource, _worldOffset)
        };
    }

    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _WorldOffsetSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex)
        : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
        , _worldOffset(0.0)
    {
        // Compute initial _worldOffset state from input scene.
        _UpdateWorldOffset();
    }

    // Recompute the value of _worldOffset and return true if it has changed.
    bool _UpdateWorldOffset()
    {
        HdSceneIndexBaseRefPtr const& inputScene = _GetInputSceneIndex();

        const GfVec3d newWorldOffset = _ComputeWorldOffset(
            inputScene, &_renderSettingsPath, &_cameraPath);

        if (newWorldOffset == _worldOffset) {
            // No invalidation needed to the scene.
            return false;
    }

        _worldOffset = newWorldOffset;
        return true;
    }

    // Traverse the input scene and dirty all transforms.
    void _ComputeDirtiedPrims(
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries) const
    {
        HdSceneIndexBaseRefPtr const& inputScene = _GetInputSceneIndex();

        for (const SdfPath &path: HdSceneIndexPrimView(inputScene)) {
            const HdSceneIndexPrim prim = inputScene->GetPrim(path);
            if (prim.dataSource && !_IsInstancePrototype(prim.dataSource)) {
                if (_HasXform(prim.dataSource)) {
                    // This index overrides xform schemas.
                    dirtiedEntries->push_back({ 
                        path, 
                        HdXformSchema::GetDefaultLocator() 
                    });
                }
                if (path == _renderSettingsPath) {
                    // This index overrides ri:trace:worldorigin and
                    // ri:trace:worldoffset.
                    dirtiedEntries->push_back({ 
                        path, 
                        HdRenderSettingsSchema::GetNamespacedSettingsLocator() 
                    });
                }
            }
        }
    }

    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override
    {
        // If we added a render settings prim, camera, or scene globals,
        // check for world offset updates.
        bool needToUpdateWorldOffset = false;
        for (const auto &entry: entries) {
            if (entry.primType == HdPrimTypeTokens->renderSettings ||
                entry.primType == HdPrimTypeTokens->camera ||
                entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
                needToUpdateWorldOffset = true;
                break;
            }
        }
        HdSceneIndexObserver::DirtiedPrimEntries extraDirtiedEntries;
        if (needToUpdateWorldOffset) {
            const bool offsetDidChange = _UpdateWorldOffset();
            // Skip computing dirtied entries on first-pass population.
            if (_hasPopulated && offsetDidChange) {
                _ComputeDirtiedPrims(&extraDirtiedEntries);
            }
        }

        if (!_hasPopulated) {
            _hasPopulated = true;
    }

        _SendPrimsAdded(entries);
        _SendPrimsDirtied(extraDirtiedEntries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override
    {
        // If we dirtied the scene globals, render settings, or camera used,
        // check to update the world offset.
        bool needToUpdateWorldOffset = false;
        for (const auto &entry: entries) {
            if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath() ||
                (!_cameraPath.IsEmpty() && entry.primPath == _cameraPath) ||
                (!_renderSettingsPath.IsEmpty()
                 && entry.primPath == _renderSettingsPath)) {
                needToUpdateWorldOffset = true;
                break;
            }
        }
        HdSceneIndexObserver::DirtiedPrimEntries extraDirtiedEntries;
        if (needToUpdateWorldOffset) {
            const bool offsetDidChange = _UpdateWorldOffset();
            if (offsetDidChange) {
                _ComputeDirtiedPrims(&extraDirtiedEntries);
        }
    }

        _SendPrimsRemoved(entries);
        _SendPrimsDirtied(extraDirtiedEntries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override
    {
        // If we dirtied the scene globals, render settings, or camera used,
        // check to update the world offset.
        bool needToUpdateWorldOffset = false;
        for (const auto &entry: entries) {
            if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath() ||
                (!_cameraPath.IsEmpty() && entry.primPath == _cameraPath) ||
                (!_renderSettingsPath.IsEmpty()
                 && entry.primPath == _renderSettingsPath)) {
                needToUpdateWorldOffset = true;
                break;
            }
        }
        HdSceneIndexObserver::DirtiedPrimEntries extraDirtiedEntries;
        if (needToUpdateWorldOffset) {
            const bool offsetDidChange = _UpdateWorldOffset();
            if (offsetDidChange) {
                _ComputeDirtiedPrims(&extraDirtiedEntries);
            }
    }

        _SendPrimsDirtied(entries);
        _SendPrimsDirtied(extraDirtiedEntries);
    }

private:
    // The value in _worldOffset defines the new desired origin for the "offset"
    // world space.  All scene xforms are updated with an additional translation
    // from _worldOffset back to the origin.  That is, this value is subtracted
    // from the "translation" part of the object-to-world matrix.
    GfVec3d _worldOffset;
    // The path of the active render settings prim used to populate
    // _worldOffset. Empty if no render settings were used.
    SdfPath _renderSettingsPath;
    // The path of the camera used to populate _worldOffset.
    // Empty if no camera was used.
    SdfPath _cameraPath;

    bool _hasPopulated = false;
};

#else

// This prior version used global variables and public setter methods for
// the offsets; the newer version below uses scene globals.  There are
// enough differences between them that we just define them separately.
class _WorldOffsetSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _WorldOffsetSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(new _WorldOffsetSceneIndex(inputSceneIndex));
    }

    virtual HdSceneIndexPrim GetPrim(const SdfPath& primPath) const
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (!prim.dataSource || _IsInstancePrototype(prim.dataSource))
            return prim;

        // Add the transform to the root. Underlay an identity xform data
        // source incase the original root doesn't have one.
        if (primPath == SdfPath::AbsoluteRootPath()) {
            const static HdDataSourceBaseHandle xformDataSource = HdXformSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(GfMatrix4d().SetIdentity()),
                HdRetainedTypedSampledDataSource<bool>::New(true)
            );
            const static HdContainerDataSourceHandle xformContainerDataSource = HdRetainedContainerDataSource::New(
                1, &HdXformSchemaTokens->xform, &xformDataSource
            );
            HdContainerDataSourceHandle handles[2] = { xformContainerDataSource, prim.dataSource };
            return {
                prim.primType,
                _WorldOffsetPrimDataSource::New(
                    HdOverlayContainerDataSource::New(2, handles),
                    _worldOffset + _cameraOffset
                )
            };
        }

        // Add the transform to all other prims.
        return {
            prim.primType,
            _WorldOffsetPrimDataSource::New(prim.dataSource, _worldOffset + _cameraOffset)
        };
    }

    virtual SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _WorldOffsetSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex)
        : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
        , _worldOffset(0.0, 0.0, 0.0)
        , _renderCamera(HdPrman_WorldOffsetSceneIndexPlugin::GetRenderCamera())
    {
        _SetCameraOffset();
    }

    virtual void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries)
    {
        _SendPrimsAdded(entries);
    }

    virtual void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries)
    {
        _SendPrimsRemoved(entries);
    }

    virtual void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries)
    {
        // Previous Total Offset
        const GfVec3d prevOffset = _worldOffset + _cameraOffset;

        // Set Camera Offset
        bool dirtyCamera = _renderCamera
            != HdPrman_WorldOffsetSceneIndexPlugin::GetRenderCamera();
        for (auto entry = entries.begin(); !dirtyCamera && entry != entries.end(); entry++)
            dirtyCamera = dirtyCamera || (entry->primPath == _renderCamera);
        if (dirtyCamera) {
            _renderCamera = HdPrman_WorldOffsetSceneIndexPlugin::GetRenderCamera();
            _SetCameraOffset();
        }

        // Set World Offset
        if (_worldOffset != HdPrman_WorldOffsetSceneIndexPlugin::GetWorldOffset())
            _worldOffset = HdPrman_WorldOffsetSceneIndexPlugin::GetWorldOffset();

        // If Offset Changed Then Dirty Prims With Transforms
        if (prevOffset != (_worldOffset + _cameraOffset)) {
            HdSceneIndexObserver::DirtiedPrimEntries dirtyEntries{{
                SdfPath::AbsoluteRootPath(), HdXformSchema::GetDefaultLocator()
            }};
            _GetPrimsDirtied(dirtyEntries);
            _SendPrimsDirtied(dirtyEntries);
        }

        _SendPrimsDirtied(entries);
    }

    void _GetPrimsDirtied(
        HdSceneIndexObserver::DirtiedPrimEntries& dirtyEntries,
        const SdfPath& path = SdfPath::AbsoluteRootPath())
    {
        // Check Matrix Source Exists
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
        if (prim.dataSource && !_IsInstancePrototype(prim.dataSource) && _HasXform(prim.dataSource))
            dirtyEntries.push_back({ 
                path, 
                HdXformSchema::GetDefaultLocator() 
            });

        // Check Child Prims
        for (const SdfPath& child : _GetInputSceneIndex()->GetChildPrimPaths(path)) {
            _GetPrimsDirtied(dirtyEntries, child);
        }
    }

    void _SetCameraOffset()
    {
        // Get Camera Translation
        _cameraOffset = GfVec3d(0.0, 0.0, 0.0);
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(_renderCamera);
        if (HdXformSchema xformSchema = HdXformSchema::GetFromParent(prim.dataSource))
            if (HdMatrixDataSourceHandle matrixSource = xformSchema.GetMatrix())
                _cameraOffset = matrixSource->GetTypedValue(0.f).ExtractTranslation();
        HdPrman_WorldOffsetSceneIndexPlugin::SetCameraOffset(_cameraOffset);
    }

    GfVec3d _worldOffset;
    GfVec3d _cameraOffset;
    SdfPath _renderCamera;
};

#endif

/////////////////////////////////////
// World Offset Scene Index Plugin //
/////////////////////////////////////

HdPrman_WorldOffsetSceneIndexPlugin::HdPrman_WorldOffsetSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_WorldOffsetSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    return _WorldOffsetSceneIndex::New(inputScene);
}

#if PXR_VERSION < 2505
SdfPath HdPrman_WorldOffsetSceneIndexPlugin::_renderCamera = SdfPath::EmptyPath();
GfVec3d HdPrman_WorldOffsetSceneIndexPlugin::_worldOffset = GfVec3d(0.0, 0.0, 0.0);
GfVec3d HdPrman_WorldOffsetSceneIndexPlugin::_cameraOffset = GfVec3d(0.0, 0.0, 0.0);

void HdPrman_WorldOffsetSceneIndexPlugin::SetWorldOffset(const GfVec3d& worldOffset)
{
    _worldOffset = worldOffset;
}

const GfVec3d& HdPrman_WorldOffsetSceneIndexPlugin::GetWorldOffset()
{
    return _worldOffset;
}

void HdPrman_WorldOffsetSceneIndexPlugin::SetCameraOffset(const GfVec3d& cameraOffset)
{
    _cameraOffset = cameraOffset;
}

const GfVec3d& HdPrman_WorldOffsetSceneIndexPlugin::GetCameraOffset()
{
    return _cameraOffset;
}

void HdPrman_WorldOffsetSceneIndexPlugin::SetRenderCamera(const SdfPath& renderCamera)
{
    _renderCamera = renderCamera;
}

const SdfPath& HdPrman_WorldOffsetSceneIndexPlugin::GetRenderCamera()
{
    return _renderCamera;
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
