//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/domeLightCameraVisibilitySceneIndex.h"

#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    HdsiDomeLightCameraVisibilitySceneIndexTokens,
    HDSI_DOME_LIGHT_CAMERA_VISIBILITY_SCENE_INDEX_TOKENS);

namespace HdsiDomeLightCameraVisibilitySceneIndex_Impl
{

class _CameraVisibilityDataSource : public HdTypedSampledDataSource<bool>
{
public:
    HD_DECLARE_DATASOURCE(_CameraVisibilityDataSource);

    bool cameraVisibility = true;

    VtValue GetValue(const Time shutterOffset) {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetTypedValue(const Time shutterOffset) {
        return cameraVisibility;
    }

    bool GetContributingSampleTimesForInterval(
        Time, Time, std::vector<Time> *) {
        return false;
    }

private:
    _CameraVisibilityDataSource() {}    
};

}

using namespace HdsiDomeLightCameraVisibilitySceneIndex_Impl;

/* static */
HdsiDomeLightCameraVisibilitySceneIndexRefPtr
HdsiDomeLightCameraVisibilitySceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    HdsiDomeLightCameraVisibilitySceneIndexRefPtr const result =
        TfCreateRefPtr(
            new HdsiDomeLightCameraVisibilitySceneIndex(
                inputSceneIndex));
    result->SetDisplayName("Dome Light Camera Visibility Scene Index");
    return result;
}


HdsiDomeLightCameraVisibilitySceneIndex::HdsiDomeLightCameraVisibilitySceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
 : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
 , _cameraVisibilityDataSource(_CameraVisibilityDataSource::New())
{
}

HdsiDomeLightCameraVisibilitySceneIndex::
~HdsiDomeLightCameraVisibilitySceneIndex() = default;

void
HdsiDomeLightCameraVisibilitySceneIndex::SetDomeLightCameraVisibility(
    const bool visibility)
{
    TRACE_FUNCTION();

    if (_cameraVisibilityDataSource->cameraVisibility == visibility) {
        return;
    }

    _cameraVisibilityDataSource->cameraVisibility = visibility;

    if (!_IsObserved()) {
        return;
    }

    if (_domeLightPaths.empty()) {
        return;
    }
    
    HdSceneIndexObserver::DirtiedPrimEntries entries;
    
    for (const SdfPath &primPath : _domeLightPaths) {
        static const HdDataSourceLocatorSet locators{
            HdLightSchema::GetDefaultLocator()
            .Append(
                HdsiDomeLightCameraVisibilitySceneIndexTokens
                    ->cameraVisibility)
        };
        entries.emplace_back(primPath, locators);
    }

    _SendPrimsDirtied(entries);
}

HdSceneIndexPrim
HdsiDomeLightCameraVisibilitySceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == HdPrimTypeTokens->domeLight) {
        prim.dataSource = HdOverlayContainerDataSource::New(
            HdRetainedContainerDataSource::New(
                HdLightSchema::GetSchemaToken(),
                HdRetainedContainerDataSource::New(
                    HdsiDomeLightCameraVisibilitySceneIndexTokens
                        ->cameraVisibility,
                    _cameraVisibilityDataSource)),
            prim.dataSource);
    }
    return prim;
}

SdfPathVector
HdsiDomeLightCameraVisibilitySceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiDomeLightCameraVisibilitySceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();
    
    {
        TRACE_SCOPE("Loop over prims added");
        
        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            if (entry.primType == HdPrimTypeTokens->domeLight) {
                _domeLightPaths.insert(entry.primPath);
            } else {
                _domeLightPaths.erase(entry.primPath);
            }
        }
    }
    
    _SendPrimsAdded(entries);
}

void
HdsiDomeLightCameraVisibilitySceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_domeLightPaths.empty()) {
        TRACE_SCOPE("Loop over prims removed");
        
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            auto it = _domeLightPaths.lower_bound(entry.primPath);
            while (it != _domeLightPaths.end() &&
                   it->HasPrefix(entry.primPath)) {
                it = _domeLightPaths.erase(it);
            }
        }
    }
    
    _SendPrimsRemoved(entries);
}

void
HdsiDomeLightCameraVisibilitySceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
