//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"

#include "pxr/base/trace/trace.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/systemSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class _RerootingSceneIndexPathDataSource : public HdPathDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RerootingSceneIndexPathDataSource)

    _RerootingSceneIndexPathDataSource(
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix,
        HdPathDataSourceHandle const &inputDataSource)
      : _srcPrefix(srcPrefix)
      , _dstPrefix(dstPrefix)
      , _inputDataSource(inputDataSource)
    {
    }

    VtValue GetValue(const Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const Time startTime,
        const Time endTime,
        std::vector<Time> * const outSampleTimes) override
    {
        if (!_inputDataSource) {
            return false;
        }

        return _inputDataSource->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    SdfPath GetTypedValue(const Time shutterOffset) override
    {
        if (!_inputDataSource) {
            return SdfPath();
        }

        const SdfPath srcPath = _inputDataSource->GetTypedValue(shutterOffset);
        return srcPath.ReplacePrefix(_srcPrefix, _dstPrefix);
    }

private:
    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
    HdPathDataSourceHandle const _inputDataSource;
};

// ----------------------------------------------------------------------------

class _RerootingSceneIndexPathArrayDataSource : public HdPathArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RerootingSceneIndexPathArrayDataSource)

    _RerootingSceneIndexPathArrayDataSource(
        const SdfPath& srcPrefix,
        const SdfPath& dstPrefix,
        HdPathArrayDataSourceHandle const & inputDataSource)
      : _srcPrefix(srcPrefix)
      , _dstPrefix(dstPrefix)
      , _inputDataSource(inputDataSource)
    {
    }

    VtValue GetValue(const Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const Time startTime,
        const Time endTime,
        std::vector<Time>*  const outSampleTimes) override
    {
        if (!_inputDataSource) {
            return false;
        }

        return _inputDataSource->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

    VtArray<SdfPath> GetTypedValue(const Time shutterOffset) override
    {
        if (!_inputDataSource) {
            return {};
        }

        VtArray<SdfPath> result
            = _inputDataSource->GetTypedValue(shutterOffset);

        const size_t n = result.size();

        if (n == 0) {
            return result;
        }

        size_t i = 0;

        // If _srcPrefix is absolute root path, we know that we
        // need to translate every path.
        if (!_srcPrefix.IsAbsoluteRootPath()) {
            // Find the first element where we need to change the path.
            //
            // Use const & so that paths[i] does not trigger VtArray
            // to make a copy.
            const VtArray<SdfPath> &paths = result.AsConst();
            while (!paths[i].HasPrefix(_srcPrefix)) {
                ++i;
                if (i == n) {
                    // No need to modify result if no path needed
                    // to be changed.
                    return result;
                }
            }
        }

        // Starting with the first element where the path matched the
        // prefix, process it and all following elements.
        for (; i < n; i++) {
            SdfPath &path = result[i];
            path = path.ReplacePrefix(_srcPrefix, _dstPrefix);
        }

        return result;
    }

private:
    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
    HdPathArrayDataSourceHandle const _inputDataSource;
};

// ----------------------------------------------------------------------------

class _RerootingSceneIndexContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RerootingSceneIndexContainerDataSource)

    _RerootingSceneIndexContainerDataSource(
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix,
        HdContainerDataSourceHandle const &inputDataSource)
      : _srcPrefix(srcPrefix)
      , _dstPrefix(dstPrefix)
      , _inputDataSource(inputDataSource)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_inputDataSource) {
            return {};
        }

        return _inputDataSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_inputDataSource) {
            return nullptr;
        }

        // wrap child containers so that we can wrap their children
        HdDataSourceBaseHandle const childSource = _inputDataSource->Get(name);
        if (!childSource) {
            return nullptr;
        }

        if (auto childContainer =
                HdContainerDataSource::Cast(childSource)) {
            return New(_srcPrefix, _dstPrefix, std::move(childContainer));
        }

        if (auto childPathDataSource =
                HdTypedSampledDataSource<SdfPath>::Cast(childSource)) {
            return _RerootingSceneIndexPathDataSource::New(
                _srcPrefix, _dstPrefix, childPathDataSource);
        }

        if (auto childPathArrayDataSource =
                HdTypedSampledDataSource<VtArray<SdfPath>>::Cast(
                    childSource)) {
            return _RerootingSceneIndexPathArrayDataSource::New(
                _srcPrefix, _dstPrefix, childPathArrayDataSource);
        }

        return childSource;
    }

private:
    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
    HdContainerDataSourceHandle const _inputDataSource;
};

} // anonymous namespace

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
            prim.dataSource = _RerootingSceneIndexContainerDataSource::New(
                _srcPrefix, _dstPrefix, prim.dataSource);
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
