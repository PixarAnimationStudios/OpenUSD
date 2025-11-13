//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/prefixingSceneIndex.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/systemSchema.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE


namespace
{

class Hd_PrefixingSceneIndexPathDataSource
    : public HdTypedSampledDataSource<SdfPath>
{
public:
    HD_DECLARE_DATASOURCE(Hd_PrefixingSceneIndexPathDataSource)

    Hd_PrefixingSceneIndexPathDataSource(
            const HdPrefixingSceneIndex &si,
            HdPathDataSourceHandle inputDataSource)
        : _si(si)
        , _inputDataSource(inputDataSource)
    {
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        if (_inputDataSource) {
            return _inputDataSource->GetContributingSampleTimesForInterval(
                    startTime, endTime, outSampleTimes);
        }

        return false;
    }

    SdfPath GetTypedValue(Time shutterOffset) override
    {
        if (!_inputDataSource) {
            return SdfPath();
        }

        SdfPath result = _inputDataSource->GetTypedValue(shutterOffset);

        return _si.AddPrefix(result);
    }

private:

    const HdPrefixingSceneIndex &_si;
    const HdPathDataSourceHandle _inputDataSource;
};

// ----------------------------------------------------------------------------

class Hd_PrefixingSceneIndexPathArrayDataSource
    : public HdTypedSampledDataSource<VtArray<SdfPath>>
{
public:
    HD_DECLARE_DATASOURCE(Hd_PrefixingSceneIndexPathArrayDataSource)

    Hd_PrefixingSceneIndexPathArrayDataSource(
            const HdPrefixingSceneIndex &si,
            HdPathArrayDataSourceHandle inputDataSource)
        : _si(si)
        , _inputDataSource(inputDataSource)
    {
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        if (_inputDataSource) {
            return _inputDataSource->GetContributingSampleTimesForInterval(
                    startTime, endTime, outSampleTimes);
        }

        return false;
    }

    VtArray<SdfPath> GetTypedValue(Time shutterOffset) override
    {
        if (!_inputDataSource) {
            return VtArray<SdfPath>();
        }

        VtArray<SdfPath> result =
            _inputDataSource->GetTypedValue(shutterOffset);

        // cases in which this will not require altering the result are less
        // common so we acknowledge that this will trigger copy-on-write.
        for (SdfPath &path : result) {
            path = _si.AddPrefix(path);
        }

        return result;
    }

private:

    const HdPrefixingSceneIndex &_si;
    const HdPathArrayDataSourceHandle _inputDataSource;
};

// ----------------------------------------------------------------------------

HdDataSourceBaseHandle
Hd_PrefixingSceneIndexCreateDataSource(
    const HdPrefixingSceneIndex &si,
    HdDataSourceBaseHandle const &inputDataSource);

class Hd_PrefixingSceneIndexVectorDataSource : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_PrefixingSceneIndexVectorDataSource)

    size_t GetNumElements() {
        return _inputDataSource->GetNumElements();
    }

    HdDataSourceBaseHandle GetElement(const size_t element) {
        return Hd_PrefixingSceneIndexCreateDataSource(
            _si,
            _inputDataSource->GetElement(element));
    }

private:
    Hd_PrefixingSceneIndexVectorDataSource(
            const HdPrefixingSceneIndex &si,
            HdVectorDataSourceHandle inputDataSource)
        : _si(si)
        , _inputDataSource(std::move(inputDataSource))
    {
    }

    const HdPrefixingSceneIndex &_si;
    const HdVectorDataSourceHandle _inputDataSource;
};

class Hd_PrefixingSceneIndexContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_PrefixingSceneIndexContainerDataSource)

    TfTokenVector GetNames() override
    {
        if (_inputDataSource) {
            return _inputDataSource->GetNames();
        }
        return {};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (!_inputDataSource) {
            return nullptr;
        }

        // wrap child containers so that we can wrap their children
        return Hd_PrefixingSceneIndexCreateDataSource(
            _si,
            _inputDataSource->Get(name));
    }

protected:
    Hd_PrefixingSceneIndexContainerDataSource(
            const HdPrefixingSceneIndex &si,
            HdContainerDataSourceHandle inputDataSource)
        : _si(si)
        , _inputDataSource(std::move(inputDataSource))
    {
    }

private:
    const HdPrefixingSceneIndex &_si;
    const HdContainerDataSourceHandle _inputDataSource;
};

HdDataSourceBaseHandle
Hd_PrefixingSceneIndexCreateDataSource(
    const HdPrefixingSceneIndex &si,
    HdDataSourceBaseHandle const &inputDataSource)
{
    if (!inputDataSource) {
        return nullptr;
    }

    if (auto containerDs = HdContainerDataSource::Cast(inputDataSource)) {
        return Hd_PrefixingSceneIndexContainerDataSource::New(
            si, std::move(containerDs));
    }

    if (auto vectorDs = HdVectorDataSource::Cast(inputDataSource)) {
        return Hd_PrefixingSceneIndexVectorDataSource::New(
            si, std::move(vectorDs));
    }

    if (auto pathDataSource =
            HdTypedSampledDataSource<SdfPath>::Cast(inputDataSource)) {
        return Hd_PrefixingSceneIndexPathDataSource::New(
            si, pathDataSource);
    }

    if (auto pathArrayDataSource =
            HdTypedSampledDataSource<VtArray<SdfPath>>::Cast(inputDataSource)) {
        return Hd_PrefixingSceneIndexPathArrayDataSource::New(
            si, pathArrayDataSource);
    }

    return inputDataSource;
}

// This class is a data source for the inputScene's absolute root prim's data
// source.  It erases the "system" container, since that will instead be
// underlayed on the inputScene's root prims.
class Hd_PrefixingSceneIndexAbsoluteRootPrimContainerDataSource final
    : public Hd_PrefixingSceneIndexContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(
        Hd_PrefixingSceneIndexAbsoluteRootPrimContainerDataSource)

    using Parent = Hd_PrefixingSceneIndexContainerDataSource;

    Hd_PrefixingSceneIndexAbsoluteRootPrimContainerDataSource(
        const HdPrefixingSceneIndex &si,
        HdContainerDataSourceHandle inputDataSource)
        : Parent(si, inputDataSource)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector names = Parent::GetNames();
        names.erase(
            std::remove(
                names.begin(), names.end(), HdSystemSchemaTokens->system),
            names.end());
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (name == HdSystemSchemaTokens->system) {
            return nullptr;
        }
        return Parent::Get(name);
    }
};

} // anonymous namespace

HdPrefixingSceneIndex::HdPrefixingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene, const SdfPath &prefix)
    : HdSingleInputFilteringSceneIndexBase(inputScene)
    , _prefix(prefix)
{
}

HdSceneIndexPrim
HdPrefixingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    if (!primPath.HasPrefix(_prefix)) {
        if (_prefix.HasPrefix(primPath)) {
            static HdContainerDataSourceHandle const empty =
                HdRetainedContainerDataSource::New();
            return {TfToken(), empty};
        } else {
            return {TfToken(), nullptr};
        }
    }

    const SdfPath inputScenePath = RemovePrefix(primPath);
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(inputScenePath);

    // We'll need to take care of the HdSystemSchema.
    //
    // Suppose our input scene index looks like:
    // / 
    //   ChildA
    //   ChildB
    //
    // Where the absolute root (/) has the "system" container data.  Suppose
    // we're prefixing with /X, meaning the resulting sceneIndex will look like:
    // /
    //   X
    //     ChildA 
    //     ChildB
    //
    // We handle these cases:
    // 1.  We need to make sure /X does *not* have the system container.  If it
    //     did, then /X/other would errantly get the
    //     system data applied to it.
    // 2.  /X/ChildA and /X/ChildB need to get the system container.
    if (prim.dataSource) {
        const bool isInputSceneAbsoluteRoot = inputScenePath.IsAbsoluteRootPath();
        if (isInputSceneAbsoluteRoot) {
            // This takes care of the HdSystemSchema case 1.
            prim.dataSource
                = Hd_PrefixingSceneIndexAbsoluteRootPrimContainerDataSource::
                    New(*this, prim.dataSource);
        }
        else {
            // Create a container data source to handle prefixing SdfPath values
            prim.dataSource = Hd_PrefixingSceneIndexContainerDataSource::New(
                *this, prim.dataSource);

            const bool isRootPrimPath = inputScenePath.IsRootPrimPath();
            if (isRootPrimPath) {
                // This takes care of the HdSystemSchema case 2.
                prim.dataSource = HdOverlayContainerDataSource::New(
                    HdSystemSchema::ComposeAsPrimDataSource(
                        _GetInputSceneIndex(), inputScenePath, nullptr),
                    prim.dataSource);
            }
        }
    }

    return prim;
}

SdfPathVector
HdPrefixingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // In the case that primPath has our prefix, we just strip out that
    // prefix and let the input scene index handle it.
    if (primPath.HasPrefix(_prefix)) {
        SdfPathVector result = _GetInputSceneIndex()->GetChildPrimPaths(
            RemovePrefix(primPath));

        for (SdfPath &path : result) {
            path = AddPrefix(path);
        }

        return result;
    }

    // Okay now since primPath does not share our prefix, then we check to 
    // see if primPath is contained within _prefix so that we return the next
    // element that matches. For example if our prefix is "/A/B/C/D" and 
    // primPath is "/A/B", we'd like to return "C".
    if (_prefix.HasPrefix(primPath)) {
        return {_prefix.GetPrefixes()[primPath.GetPathElementCount()]};
    }

    return {};
}

void
HdPrefixingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries prefixedEntries;
    prefixedEntries.reserve(entries.size());

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        // We take PrimsAdded as an opportunity to populate our
        // ReplacePrefix cache.
        const SdfPath& path = entry.primPath;
        const SdfPath& prefixed = path.ReplacePrefix(
            SdfPath::AbsoluteRootPath(), _prefix, /* fixTargetPaths = */false);
        _addPrefixMap[path] = prefixed;
        _removePrefixMap[prefixed] = path;

        prefixedEntries.emplace_back(prefixed, entry.primType);
    }

    _SendPrimsAdded(prefixedEntries);
}

void
HdPrefixingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::RemovedPrimEntries prefixedEntries;
    prefixedEntries.reserve(entries.size());

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        prefixedEntries.push_back(AddPrefix(entry.primPath));
    }

    _SendPrimsRemoved(prefixedEntries);
}

void
HdPrefixingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::DirtiedPrimEntries prefixedEntries;
    prefixedEntries.reserve(entries.size());

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        prefixedEntries.emplace_back(
                AddPrefix(entry.primPath), entry.dirtyLocators);
    }

    _SendPrimsDirtied(prefixedEntries);
}

inline SdfPath 
HdPrefixingSceneIndex::AddPrefix(const SdfPath &primPath) const 
{
    // Hopefully we hit the cached version of this!
    auto it = _addPrefixMap.find(primPath);
    if (it != _addPrefixMap.end()) {
        return it->second;
    }

    // If not, for some reason, just do the prefix replacement.
    return primPath.ReplacePrefix(
        SdfPath::AbsoluteRootPath(), _prefix, /* fixTargetPaths = */false);
}

inline SdfPath 
HdPrefixingSceneIndex::RemovePrefix(const SdfPath &primPath) const 
{
    // See above.
    auto it = _removePrefixMap.find(primPath);
    if (it != _removePrefixMap.end()) {
        return it->second;
    }

    return primPath.ReplacePrefix(
        _prefix, SdfPath::AbsoluteRootPath(), /* fixTargetPaths = */false);
}

PXR_NAMESPACE_CLOSE_SCOPE
