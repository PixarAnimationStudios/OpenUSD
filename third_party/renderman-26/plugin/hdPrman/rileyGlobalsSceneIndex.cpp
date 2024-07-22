// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/rileyGlobalsSceneIndex.h"

#include "hdPrman/rileyGlobalsSchema.h"
#include "hdPrman/rileyParamSchema.h"
#include "hdPrman/rileyParamListSchema.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

static
const SdfPath &
_GetGlobalsPrimPath()
{
    static const SdfPath path("/__rileyGlobals__");
    return path;
}

static
const TfToken &
_GetRileyFrameToken()
{
    static const TfToken token(RixStr.k_Ri_Frame.CStr());
    return token;
}

static
const HdContainerDataSourceHandle &
_GetDependencies()
{
    static TfToken names[]  = {
        TfToken("__frame")
    };
    static HdDataSourceBaseHandle values[] = {
        HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    HdSceneGlobalsSchema::GetDefaultPrimPath()))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdSceneGlobalsSchema::GetCurrentFrameLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdPrmanRileyGlobalsSchema::GetOptionsLocator()
                        .Append(HdPrmanRileyParamListSchemaTokens->params)
                        .Append(_GetRileyFrameToken())))
            .Build()
    };

    static const HdContainerDataSourceHandle ds =
        HdDependenciesSchema::BuildRetained(
            std::size(names),
            names,
            values);

    return ds;
}

/* static */
HdPrman_RileyGlobalsSceneIndexRefPtr
HdPrman_RileyGlobalsSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new HdPrman_RileyGlobalsSceneIndex(inputSceneIndex, inputArgs));
}

HdPrman_RileyGlobalsSceneIndex::HdPrman_RileyGlobalsSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdPrman_RileyGlobalsSceneIndex::~HdPrman_RileyGlobalsSceneIndex() = default;

HdContainerDataSourceHandle
HdPrman_RileyGlobalsSceneIndex::_GetRileyOptions() const
{
    HdSceneGlobalsSchema schema = HdSceneGlobalsSchema::GetFromParent(
        _GetInputSceneIndex()->GetPrim(
            HdSceneGlobalsSchema::GetDefaultPrimPath()).dataSource);

    if (HdDoubleDataSourceHandle const currentFrameDs =
            schema.GetCurrentFrame())
    {
        const double frame = currentFrameDs->GetTypedValue(0.0f);
        if (!std::isnan(frame)) {
            return HdRetainedContainerDataSource::New(
                _GetRileyFrameToken(),
                HdPrmanRileyParamSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<int>::New(
                            int(frame)))
                    .Build());
        }
    }

    return nullptr;
}

HdContainerDataSourceHandle
HdPrman_RileyGlobalsSceneIndex::_GetGlobalsPrimSource() const
{
    return
        HdRetainedContainerDataSource::New(
            HdPrmanRileyGlobalsSchema::GetSchemaToken(),
            HdPrmanRileyGlobalsSchema::Builder()
                .SetOptions(
                    HdPrmanRileyParamListSchema::Builder()
                        .SetParams(_GetRileyOptions())
                        .Build())
                .Build(),
            HdDependenciesSchema::GetSchemaToken(),
            _GetDependencies());
}

HdSceneIndexPrim
HdPrman_RileyGlobalsSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    if (primPath == _GetGlobalsPrimPath()) {
        return {
            HdPrmanRileyPrimTypeTokens->globals,
            _GetGlobalsPrimSource() };
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdPrman_RileyGlobalsSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    SdfPathVector result = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    if (primPath == SdfPath::AbsoluteRootPath()) {
        result.push_back(_GetGlobalsPrimPath());
    }

    return result;
}

void
HdPrman_RileyGlobalsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdPrman_RileyGlobalsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdPrman_RileyGlobalsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
