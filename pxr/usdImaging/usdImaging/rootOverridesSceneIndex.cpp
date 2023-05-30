//
// Copyright 2023 Pixar
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
