//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdsi/sceneGlobalsSceneIndex.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include <cmath>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

// Checks whether t0 and t1 are equal when interpreted as time codes (similar
// to UsdTimeCode), that is the default time is encoded as NaN.
static
bool _IsEqualTimeCode(const double t0, const double t1)
{
    // a == NaN is always false. So catch the case where both
    // are NaN first.
    if (std::isnan(t0) && std::isnan(t1)) {
        return true;
    }

    // Normal comparison. Note that if only one is NaN, this still
    // returns false.
    return t0 == t1;
}

// -----------------------------------------------------------------------------
// HdsiSceneGlobalsSceneIndex::_SceneGlobalsSchemaDataSource
// -----------------------------------------------------------------------------
class HdsiSceneGlobalsSceneIndex::_SceneGlobalsSchemaDataSource
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SceneGlobalsSchemaDataSource);

    TfTokenVector
    GetNames() override;

    HdDataSourceBaseHandle
    Get(const TfToken &name) override;

private:
    _SceneGlobalsSchemaDataSource() = default;

    // The scene index owns this data source and mutates its state directly.
    friend class HdsiSceneGlobalsSceneIndex;

    std::optional<SdfPath> _activeRenderPassPrim;
    std::optional<SdfPath> _activeRenderSettingsPrim;
    std::optional<SdfPath> _primaryCameraPrim;
    std::optional<double> _currentFrame;
    std::optional<double> _timeCodesPerSecond;
    std::optional<int> _sceneStateId;
};

TfTokenVector
HdsiSceneGlobalsSceneIndex::_SceneGlobalsSchemaDataSource::GetNames()
{
    static const TfTokenVector names = {
        HdSceneGlobalsSchemaTokens->activeRenderPassPrim,
        HdSceneGlobalsSchemaTokens->activeRenderSettingsPrim,
        HdSceneGlobalsSchemaTokens->primaryCameraPrim,
        HdSceneGlobalsSchemaTokens->currentFrame,
        HdSceneGlobalsSchemaTokens->timeCodesPerSecond,
        HdSceneGlobalsSchemaTokens->sceneStateId
    };

    return names;
}

template<typename T>
static
HdDataSourceBaseHandle
_OptionalToRetainedDataSource(const std::optional<T> &value)
{
    if (value) {
        return HdRetainedTypedSampledDataSource<T>::New(*value);
    } else {
        return nullptr;
    }
}

HdDataSourceBaseHandle
HdsiSceneGlobalsSceneIndex::_SceneGlobalsSchemaDataSource::Get(const TfToken &name)
{
    if (name == HdSceneGlobalsSchemaTokens->activeRenderPassPrim) {
        return _OptionalToRetainedDataSource(_activeRenderPassPrim);
    }
    if (name == HdSceneGlobalsSchemaTokens->activeRenderSettingsPrim) {
        return _OptionalToRetainedDataSource(_activeRenderSettingsPrim);
    }
    if (name == HdSceneGlobalsSchemaTokens->primaryCameraPrim) {
        return _OptionalToRetainedDataSource(_primaryCameraPrim);
    }
    if (name == HdSceneGlobalsSchemaTokens->currentFrame) {
        return _OptionalToRetainedDataSource(_currentFrame);
    }
    if (name == HdSceneGlobalsSchemaTokens->timeCodesPerSecond) {
        return _OptionalToRetainedDataSource(_timeCodesPerSecond);
    }
    if (name == HdSceneGlobalsSchemaTokens->sceneStateId) {
        return _OptionalToRetainedDataSource(_sceneStateId);
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
// HdsiSceneGlobalsSceneIndex
// -----------------------------------------------------------------------------

/* static */
HdsiSceneGlobalsSceneIndexRefPtr
HdsiSceneGlobalsSceneIndex::New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    HdsiSceneGlobalsSceneIndexRefPtr const result =
        TfCreateRefPtr(new HdsiSceneGlobalsSceneIndex(inputSceneIndex));
    result->SetDisplayName("Scene Globals Scene Index");
    return result;
}

void
HdsiSceneGlobalsSceneIndex::SetActiveRenderPassPrimPath(
    const SdfPath &path)
{
    if (_sceneGlobalsSchemaDataSource->_activeRenderPassPrim == path) {
        return;
    }

    // A scene index downstream will invalidate and update the
    // sceneGlobals.activeRenderSettingsPrim locator (if the render pass points
    // to a valid render settings prim).
    // We keep things simple in this scene index.
    _sceneGlobalsSchemaDataSource->_activeRenderPassPrim = path;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetActiveRenderPassPrimLocator()}});
    }
}

void
HdsiSceneGlobalsSceneIndex::SetActiveRenderSettingsPrimPath(
    const SdfPath &path)
{
    if (_sceneGlobalsSchemaDataSource->_activeRenderSettingsPrim == path) {
        return;
    }

    _sceneGlobalsSchemaDataSource->_activeRenderSettingsPrim = path;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetActiveRenderSettingsPrimLocator()}});
    }
}

void
HdsiSceneGlobalsSceneIndex::SetPrimaryCameraPrimPath(
    const SdfPath &path)
{
    if (_sceneGlobalsSchemaDataSource->_primaryCameraPrim == path) {
        return;
    }

    _sceneGlobalsSchemaDataSource->_primaryCameraPrim = path;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetPrimaryCameraPrimLocator()}});
    }
}

void
HdsiSceneGlobalsSceneIndex::SetCurrentFrame(double currentFrame)
{
    // XXX We might need to add a flag to force dirtying of the Frame locator 
    // even if the time has not changed 
    if (_sceneGlobalsSchemaDataSource->_currentFrame &&
        _IsEqualTimeCode(*_sceneGlobalsSchemaDataSource->_currentFrame, currentFrame)) {
        return;
    }

    _sceneGlobalsSchemaDataSource->_currentFrame = currentFrame;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetCurrentFrameLocator()}});
    }
}

void
HdsiSceneGlobalsSceneIndex::SetTimeCodesPerSecond(double timeCodesPerSecond)
{
    if (_sceneGlobalsSchemaDataSource->_timeCodesPerSecond == timeCodesPerSecond) {
        return;
    }

    _sceneGlobalsSchemaDataSource->_timeCodesPerSecond = timeCodesPerSecond;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetTimeCodesPerSecondLocator()}});
    }
}

void
HdsiSceneGlobalsSceneIndex::SetSceneStateId(int id)
{
    if (_sceneGlobalsSchemaDataSource->_sceneStateId == id) {
        return;
    }

    _sceneGlobalsSchemaDataSource->_sceneStateId = id;

    if (_IsObserved()) {
        _SendPrimsDirtied({{
            HdSceneGlobalsSchema::GetDefaultPrimPath(),
            HdSceneGlobalsSchema::GetSceneStateIdLocator()}});
    }
}

HdSceneIndexPrim
HdsiSceneGlobalsSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Overlay a data source at the scene globals locator for the default prim.
    if (primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
        prim.dataSource =
            HdCreateOverlayContainerDataSource(
                HdRetainedContainerDataSource::New(
                    HdSceneGlobalsSchemaTokens->sceneGlobals,
                    _sceneGlobalsSchemaDataSource),
                prim.dataSource);
    }

    return prim;
}

SdfPathVector
HdsiSceneGlobalsSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

HdsiSceneGlobalsSceneIndex::HdsiSceneGlobalsSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _sceneGlobalsSchemaDataSource(_SceneGlobalsSchemaDataSource::New())
{}

void
HdsiSceneGlobalsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiSceneGlobalsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    // XXX Since this is now a filtering scene index, handle removals of
    //     the active render settings prim.
    _SendPrimsRemoved(entries);
}

void
HdsiSceneGlobalsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

// XXX Handle renames by sending a dirty notice that the active render settings
//     prim has changed.

PXR_NAMESPACE_CLOSE_SCOPE
