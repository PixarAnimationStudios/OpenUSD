//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/rerootingContainerDataSource.h"

#include "pxr/base/trace/trace.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/systemSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingRerootingSceneIndex::UsdImagingRerootingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputScene,
    const SdfPath &srcPrefix,
    const SdfPath &dstPrefix)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
  , _srcPrefix(srcPrefix)
  , _dstPrefix(dstPrefix)
  , _dstPrefixes(dstPrefix.GetPrefixes())
  , _srcEqualsDst(srcPrefix == dstPrefix)
  , _srcPrefixIsRoot(srcPrefix.IsAbsoluteRootPath())
{
    // The paths are usually too long and don't display well at all.
    if (false) {
        SetDisplayName(
            TfStringPrintf(
                "Rerooting %s to %s", srcPrefix.GetText(), dstPrefix.GetText()));
    }
}

UsdImagingRerootingSceneIndex::~UsdImagingRerootingSceneIndex() = default;

HdSceneIndexPrim
UsdImagingRerootingSceneIndex::GetPrim(const SdfPath& primPath) const
{
    if (!primPath.HasPrefix(_dstPrefix)) {
        return { TfToken(), nullptr };
    }

    const SdfPath inputScenePath
        = _srcEqualsDst ? primPath : _DstPathToSrcPath(primPath);

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(inputScenePath);

    if (prim.dataSource) {
        // Wrap the container data source so that paths are properly re-mapped.
        // When src == dst, we can short-circuit this.
        if (!_srcEqualsDst) {
            prim.dataSource = UsdImagingRerootingContainerDataSource::New(
                prim.dataSource, _srcPrefix, _dstPrefix);
        }

        // If we are at the dst root, we'll compose the system data source.
        if (primPath == _dstPrefix) {
            prim.dataSource = HdOverlayContainerDataSource::New(
                HdSystemSchema::ComposeAsPrimDataSource(
                    _GetInputSceneIndex(), inputScenePath, nullptr),
                prim.dataSource);
        }
    }

    return prim;
}

SdfPathVector
UsdImagingRerootingSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    // For paths that are below our "dstPrefix", we will remap them to our input
    // scene.
    if (primPath.HasPrefix(_dstPrefix)) {
        if (_srcEqualsDst) {
            return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        }

        SdfPathVector result
            = _GetInputSceneIndex()->GetChildPrimPaths(
                _DstPathToSrcPath(primPath));
        for (SdfPath& path : result) {
            path = _SrcPathToDstPath(path);
        }
        return result;
    }

    // For paths that prefix our "dstPrefix", we need to make sure we return the
    // the corresponding child so that we can get to our "dstPrefix".  For example,
    // if we've rerooted to "/A/B/C/D" and primPath is "/A/B", we want to return
    // "/A/B/C".
    if (_dstPrefix.HasPrefix(primPath)) {
        return { _dstPrefixes[primPath.GetPathElementCount()] };
    }

    return {};
}

void
UsdImagingRerootingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries prefixedEntries;
    prefixedEntries.reserve(entries.size());

    if (_srcEqualsDst) {
        for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
            if (entry.primPath.HasPrefix(_srcPrefix)) {
                prefixedEntries.push_back(entry);
            }
        }
    } else if (_srcPrefixIsRoot) {
        for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
            prefixedEntries.emplace_back(
                _SrcPathToDstPath(entry.primPath), entry.primType);
        }
    } else {
        for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
            if (entry.primPath.HasPrefix(_srcPrefix)) {
                prefixedEntries.emplace_back(
                    _SrcPathToDstPath(entry.primPath), entry.primType);
            }
        }
    }

    _SendPrimsAdded(prefixedEntries);
}

void
UsdImagingRerootingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::RemovedPrimEntries prefixedEntries;
    prefixedEntries.reserve(entries.size());

    if (_srcEqualsDst) {
        for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
            if (entry.primPath.HasPrefix(_srcPrefix)) {
                prefixedEntries.push_back(entry);
            }
            if (_srcPrefix.HasPrefix(entry.primPath)) {
                _SendPrimsRemoved({{_dstPrefix}});
                return;
            }
        }
    } else if (_srcPrefixIsRoot) {
        for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
            prefixedEntries.emplace_back(_SrcPathToDstPath(entry.primPath));
        }
    } else {            
        for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
            if (entry.primPath.HasPrefix(_srcPrefix)) {
                prefixedEntries.emplace_back(_SrcPathToDstPath(entry.primPath));
            }
            if (_srcPrefix.HasPrefix(entry.primPath)) {
                _SendPrimsRemoved({{_dstPrefix}});
                return;
            }
        }
    }

    _SendPrimsRemoved(prefixedEntries);
}

void
UsdImagingRerootingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::DirtiedPrimEntries prefixedEntries;
    prefixedEntries.reserve(entries.size());

    if (_srcEqualsDst) {
        for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
            if (entry.primPath.HasPrefix(_srcPrefix)) {
                prefixedEntries.push_back(entry);
            }
        }
    } else if (_srcPrefixIsRoot) {
        for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
            prefixedEntries.emplace_back(
                _SrcPathToDstPath(entry.primPath), entry.dirtyLocators);
        }
    } else {
        for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
            if (entry.primPath.HasPrefix(_srcPrefix)) {
                prefixedEntries.emplace_back(
                    _SrcPathToDstPath(entry.primPath), entry.dirtyLocators);
            }
        }
    }

    _SendPrimsDirtied(prefixedEntries);
}

inline SdfPath
UsdImagingRerootingSceneIndex::_SrcPathToDstPath(
    const SdfPath& primPath) const
{
    return primPath.ReplacePrefix(_srcPrefix, _dstPrefix);
}

inline SdfPath
UsdImagingRerootingSceneIndex::_DstPathToSrcPath(
    const SdfPath& primPath) const
{
    return primPath.ReplacePrefix(_dstPrefix, _srcPrefix);
}

PXR_NAMESPACE_CLOSE_SCOPE
