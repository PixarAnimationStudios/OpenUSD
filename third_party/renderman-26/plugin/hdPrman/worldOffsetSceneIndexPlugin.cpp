//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license
//
#include "hdPrman/worldOffsetSceneIndexPlugin.h"

#include "hdPrman/tokens.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"

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
    HdMatrixDataSourceHandle _input;
    GfVec3d _worldOffset;
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
    HdContainerDataSourceHandle _input;
    GfVec3d _worldOffset;
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

        if (name == HdXformSchemaTokens->xform)
            if (HdXformSchema xformSchema = HdXformSchema(HdContainerDataSource::Cast(dataSource)))
                if (HdBoolDataSourceHandle resetXformStack = xformSchema.GetResetXformStack())
                    if (resetXformStack->GetTypedValue(0.0f))
                        return _WorldOffsetXformDataSource::New(xformSchema.GetContainer(), _worldOffset);

        return dataSource;
    }

#if PXR_VERSION < 2302
    bool Has(const TfToken& name) override
    {
        return _input->Has(name);
    }
#endif

private:
    HdContainerDataSourceHandle _input;
    GfVec3d _worldOffset;
};

HD_DECLARE_DATASOURCE_HANDLES(_WorldOffsetPrimDataSource);

//////////////////////////////
// World Offset Scene Index //
//////////////////////////////

TF_DECLARE_REF_PTRS(_WorldOffsetSceneIndex);

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
        if (!prim.dataSource || _IsInstance(prim.dataSource))
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

    bool _IsInstance(const HdContainerDataSourceHandle& primDs) const
    {
        HdInstancedBySchema instancedBy = HdInstancedBySchema::GetFromParent(primDs);
        if (!instancedBy) 
            return false;
        HdPathArrayDataSourceHandle pathsDs = instancedBy.GetPaths();
        if (!pathsDs) 
            return false;
        return !pathsDs->GetTypedValue(0.0f).empty();
    }

    bool _HasXform(const HdContainerDataSourceHandle& primDs) const
    {
        const TfTokenVector names = primDs->GetNames();
        return std::find(names.begin(), names.end(), HdXformSchemaTokens->xform) != names.end();
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
        if (prim.dataSource && !_IsInstance(prim.dataSource) && _HasXform(prim.dataSource))
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

/////////////////////////////////////
// World Offset Scene Index Plugin //
/////////////////////////////////////

SdfPath HdPrman_WorldOffsetSceneIndexPlugin::_renderCamera = SdfPath::EmptyPath();
GfVec3d HdPrman_WorldOffsetSceneIndexPlugin::_worldOffset = GfVec3d(0.0, 0.0, 0.0);
GfVec3d HdPrman_WorldOffsetSceneIndexPlugin::_cameraOffset = GfVec3d(0.0, 0.0, 0.0);

HdPrman_WorldOffsetSceneIndexPlugin::HdPrman_WorldOffsetSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_WorldOffsetSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    return _WorldOffsetSceneIndex::New(inputScene);
}

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

PXR_NAMESPACE_CLOSE_SCOPE
