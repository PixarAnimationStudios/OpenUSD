//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingRootOverridesSceneIndex_Impl
{

struct _RootOverlayInfo
{
    GfMatrix4d transform = GfMatrix4d(1.0);
    bool visibility = true;
};

// Data source for locator xform/matrix
class _MatrixSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MatrixSource);

    VtValue GetValue(const Time t) {
        return VtValue(GetTypedValue(t));
    }

    GfMatrix4d GetTypedValue(const Time t) {
        return _info->transform;
    }

    bool GetContributingSampleTimesForInterval(Time, Time, std::vector<Time>*)
    {
        return false;
    }

private:
    _MatrixSource(_RootOverlayInfoSharedPtr const &info)
     : _info(info)
    {
    }

    _RootOverlayInfoSharedPtr const _info;
};

// Data source for locator visibility/visibility
class _VisibilitySource : public HdBoolDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VisibilitySource);

    VtValue GetValue(const Time t) {
        return VtValue(GetTypedValue(t));
    }

    bool GetTypedValue(const Time t) {
        return _info->visibility;
    }

    bool GetContributingSampleTimesForInterval(Time, Time, std::vector<Time>*)
    {
        return false;
    }

private:
    _VisibilitySource(_RootOverlayInfoSharedPtr const &info)
     : _info(info)
    {
    }

    _RootOverlayInfoSharedPtr const _info;
};

}

using namespace UsdImagingRootOverridesSceneIndex_Impl;

UsdImagingRootOverridesSceneIndexRefPtr
UsdImagingRootOverridesSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdImagingRootOverridesSceneIndex(
            inputSceneIndex));
}

UsdImagingRootOverridesSceneIndex::
UsdImagingRootOverridesSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _rootOverlayInfo(std::make_shared<_RootOverlayInfo>())
  , _rootOverlayDs(
      HdRetainedContainerDataSource::New(
          HdXformSchema::GetSchemaToken(),
          HdRetainedContainerDataSource::New(
              HdXformSchemaTokens->matrix,
              _MatrixSource::New(_rootOverlayInfo)),
          HdVisibilitySchema::GetSchemaToken(),
          HdRetainedContainerDataSource::New(
              HdVisibilitySchemaTokens->visibility,
              _VisibilitySource::New(_rootOverlayInfo))))
{
}

void
UsdImagingRootOverridesSceneIndex::SetRootTransform(
    const GfMatrix4d &transform)
{
    if (_rootOverlayInfo->transform == transform) {
        return;
    }

    _rootOverlayInfo->transform = transform;

    static const HdSceneIndexObserver::DirtiedPrimEntries entries{
        { SdfPath::AbsoluteRootPath(),
          HdDataSourceLocatorSet{
                HdXformSchema::GetDefaultLocator()
                    .Append(HdXformSchemaTokens->matrix)}}};

    _SendPrimsDirtied(entries);
}

const GfMatrix4d&
UsdImagingRootOverridesSceneIndex::GetRootTransform() const
{
    return _rootOverlayInfo->transform;
}

void
UsdImagingRootOverridesSceneIndex::SetRootVisibility(
    const bool visibility)
{
    if (_rootOverlayInfo->visibility == visibility) {
        return;
    }

    _rootOverlayInfo->visibility = visibility;

    static const HdSceneIndexObserver::DirtiedPrimEntries entries{
        { SdfPath::AbsoluteRootPath(),
          HdDataSourceLocatorSet{
                HdVisibilitySchema::GetDefaultLocator()
                    .Append(HdVisibilitySchemaTokens->visibility)}}};

    _SendPrimsDirtied(entries);
}

const bool
UsdImagingRootOverridesSceneIndex::GetRootVisibility() const
{
    return _rootOverlayInfo->visibility;
}

HdSceneIndexPrim
UsdImagingRootOverridesSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (primPath == SdfPath::AbsoluteRootPath()) {
        prim.dataSource = HdOverlayContainerDataSource::New(
            _rootOverlayDs,
            prim.dataSource);
    }

    return prim;
}

SdfPathVector
UsdImagingRootOverridesSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingRootOverridesSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImagingRootOverridesSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

void
UsdImagingRootOverridesSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
