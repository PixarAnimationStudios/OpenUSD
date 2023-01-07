//
// Copyright 2021 Pixar
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
            const SdfPath &prefix,
            HdPathDataSourceHandle inputDataSource)
        : _prefix(prefix)
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

        if (result.IsAbsolutePath()) {
            return result.ReplacePrefix(SdfPath::AbsoluteRootPath(), _prefix);
        }

        return result;
    }

private:

    const SdfPath _prefix;
    const HdPathDataSourceHandle _inputDataSource;
};

// ----------------------------------------------------------------------------

class Hd_PrefixingSceneIndexPathArrayDataSource
    : public HdTypedSampledDataSource<VtArray<SdfPath>>
{
public:
    HD_DECLARE_DATASOURCE(Hd_PrefixingSceneIndexPathArrayDataSource)

    Hd_PrefixingSceneIndexPathArrayDataSource(
            const SdfPath &prefix,
            HdPathArrayDataSourceHandle inputDataSource)
        : _prefix(prefix)
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
            if (path.IsAbsolutePath()) {
                path = path.ReplacePrefix(SdfPath::AbsoluteRootPath(), _prefix);
            }
        }

        return result;
    }

private:

    const SdfPath _prefix;
    const HdPathArrayDataSourceHandle _inputDataSource;
};

// ----------------------------------------------------------------------------

class Hd_PrefixingSceneIndexContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_PrefixingSceneIndexContainerDataSource)

    Hd_PrefixingSceneIndexContainerDataSource(
            const SdfPath &prefix,
            HdContainerDataSourceHandle inputDataSource)
        : _prefix(prefix)
        , _inputDataSource(inputDataSource)
    {
    }

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
        if (HdDataSourceBaseHandle childSource =
                _inputDataSource->Get(name)) {

            if (auto childContainer =
                    HdContainerDataSource::Cast(childSource)) {
                return New(_prefix, std::move(childContainer));
            }

            if (auto childPathDataSource =
                    HdTypedSampledDataSource<SdfPath>::Cast(childSource)) {

                return Hd_PrefixingSceneIndexPathDataSource::New(
                        _prefix, childPathDataSource);
            }

            if (auto childPathArrayDataSource =
                    HdTypedSampledDataSource<VtArray<SdfPath>>::Cast(
                        childSource)) {
                return Hd_PrefixingSceneIndexPathArrayDataSource::New(
                        _prefix, childPathArrayDataSource);
            }

            return childSource;
        }

        return nullptr;
    }

private:
    const SdfPath _prefix;
    const HdContainerDataSourceHandle _inputDataSource;
};

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
        const SdfPath& prefix, HdContainerDataSourceHandle inputDataSource)
        : Parent(prefix, inputDataSource)
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
        return {TfToken(), nullptr};
    }

    const SdfPath inputScenePath = _RemovePathPrefix(primPath);
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
                    New(_prefix, prim.dataSource);
        }
        else {
            // Create a container data source to handle prefixing SdfPath values
            prim.dataSource = Hd_PrefixingSceneIndexContainerDataSource::New(
                _prefix, prim.dataSource);

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
            _RemovePathPrefix(primPath));

        for (SdfPath &path : result) {
            path = _prefix.AppendPath(
                path.MakeRelativePath(SdfPath::AbsoluteRootPath()));
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
        prefixedEntries.emplace_back(
            _AddPathPrefix(entry.primPath), entry.primType);
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
        prefixedEntries.push_back(_AddPathPrefix(entry.primPath));
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
                _AddPathPrefix(entry.primPath), entry.dirtyLocators);
    }

    _SendPrimsDirtied(prefixedEntries);
}

inline SdfPath 
HdPrefixingSceneIndex::_AddPathPrefix(const SdfPath &primPath) const 
{
    return primPath.ReplacePrefix(SdfPath::AbsoluteRootPath(), _prefix);
}

inline SdfPath 
HdPrefixingSceneIndex::_RemovePathPrefix(const SdfPath &primPath) const 
{
    return primPath.ReplacePrefix(_prefix, SdfPath::AbsoluteRootPath());
}

PXR_NAMESPACE_CLOSE_SCOPE
