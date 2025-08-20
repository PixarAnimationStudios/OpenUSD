//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/instanceProxyPathTranslationSceneIndex.h"
#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/stackTrace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace { // anonymous implementation namespace

// Helper for recursive path translation in data sources.
static HdDataSourceBaseHandle
_TranslateDataSource(
    HdDataSourceBaseHandle const& ds,
    UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr const& si);

// XXX The data source implementation here resembles that in
// HdPrefixingSceneIndex; perhaps we can find a way to generalize it.

// Data source that recursively wraps data sources to apply path translation,
// given a HdVectorDataSource.
class _VectorDs : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VectorDs);

    size_t GetNumElements() override {
        return _underlyingDs->GetNumElements();
    }
    HdDataSourceBaseHandle GetElement(size_t i) override {
        return _TranslateDataSource(_underlyingDs->GetElement(i), _sceneIndex);
    }

private:
    _VectorDs(
        UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr const&
            sceneIndex,
        HdVectorDataSourceHandle const& underlyingDs)
    : _sceneIndex(sceneIndex)
    , _underlyingDs(underlyingDs)
    {
    }

    const UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr _sceneIndex;
    const HdVectorDataSourceHandle _underlyingDs;
};

// Data source that recursively wraps data sources to apply path translation,
// given a HdContainerDataSource.
class _ContainerDs : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ContainerDs);

    TfTokenVector GetNames() override {
        return _underlyingDs->GetNames();
    }
    HdDataSourceBaseHandle Get(TfToken const& name) override {
        return _TranslateDataSource(_underlyingDs->Get(name), _sceneIndex);
    }

protected:
    _ContainerDs(
        UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr const&
            sceneIndex,
        HdContainerDataSourceHandle const& underlyingDs)
    : _sceneIndex(sceneIndex)
    , _underlyingDs(underlyingDs)
    {
    }

    const UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr _sceneIndex;
    const HdContainerDataSourceHandle _underlyingDs;
};

// Data source that recursively wraps data sources to apply path translation,
// given a prim-level conatiner data source.
class _PrimDs : public _ContainerDs
{
public:
    HD_DECLARE_DATASOURCE(_PrimDs);

    HdDataSourceBaseHandle Get(TfToken const& name) override {
        return _sceneIndex->ShouldTranslatePathsForDataSourceName(name)
            ? _ContainerDs::Get(name)
            : _underlyingDs->Get(name);
    }

private:
    _PrimDs(
        UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr const&
            sceneIndex,
        HdContainerDataSourceHandle const& underlyingDs)
    : _ContainerDs(sceneIndex, underlyingDs)
    {
    }
};

// Apply instance path-translation and recursive wrapping to the data source.
static HdDataSourceBaseHandle
_TranslateDataSource(
    HdDataSourceBaseHandle const& ds,
    UsdImaging_InstanceProxyPathTranslationSceneIndexConstPtr const& si)
{
    // Translate SdfPath-valued data sources.
    if (auto pathDs = HdPathDataSource::Cast(ds)) {
        SdfPath path = pathDs->GetTypedValue(0.0); 
        return HdRetainedTypedSampledDataSource<SdfPath>
            ::New(si->TranslatePath(path));
    }

    // Translate VtArray<SdfPath>-valued data sources.
    if (auto pathArrayDs = HdPathArrayDataSource::Cast(ds)) {
        VtArray<SdfPath> pathArray = pathArrayDs->GetTypedValue(0.0);
        for (SdfPath& path: pathArray) {
            path = si->TranslatePath(path);
        }
        return HdRetainedTypedSampledDataSource<
            VtArray<SdfPath>>::New(pathArray);
    }

    // Recursively wrap container data sources.
    if (auto containerDs = HdContainerDataSource::Cast(ds)) {
        return _ContainerDs::New(si, containerDs);
    }

    // Recursively wrap vector data sources.
    if (auto vectorDs = HdVectorDataSource::Cast(ds)) {
        return _VectorDs::New(si, vectorDs);
    }

    return ds;
}

} // end anonymous implementation namespace


UsdImaging_InstanceProxyPathTranslationSceneIndexRefPtr
UsdImaging_InstanceProxyPathTranslationSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    TfTokenVector const& proxyPathTranslationDataSourceNames)
{
    return TfCreateRefPtr(
        new UsdImaging_InstanceProxyPathTranslationSceneIndex(inputSceneIndex,
            proxyPathTranslationDataSourceNames));
}

UsdImaging_InstanceProxyPathTranslationSceneIndex::
UsdImaging_InstanceProxyPathTranslationSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    TfTokenVector const& proxyPathTranslationDataSourceNames)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _proxyPathDataSourceNames(proxyPathTranslationDataSourceNames)
{
}

HdSceneIndexPrim
UsdImaging_InstanceProxyPathTranslationSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource = _PrimDs::New(TfCreateWeakPtr(this), prim.dataSource);
    }
    return prim;
}

SdfPathVector
UsdImaging_InstanceProxyPathTranslationSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

bool
UsdImaging_InstanceProxyPathTranslationSceneIndex::
    ShouldTranslatePathsForDataSourceName(TfToken const& name) const
{
    return std::find(_proxyPathDataSourceNames.begin(),
                     _proxyPathDataSourceNames.end(),
                     name) != _proxyPathDataSourceNames.end();
}

SdfPath
UsdImaging_InstanceProxyPathTranslationSceneIndex::TranslatePath(
    SdfPath const& path) const
{
    if (_instanceToPrototypePathMap.empty()) {
        // There are no instance->prototype translations to apply.
        return path;
    }

    // Look for the closest enclosing instance path.
    for (SdfPath p = path; p.IsPrimPath(); p = p.GetParentPath()) {
        auto it = _instanceToPrototypePathMap.find(p);
        if (it != _instanceToPrototypePathMap.end()) {
            // Only translate strictly-descendant paths.  Do not
            // translate paths that directly point at an instance.
            //
            // TODO: Need to add support for nested instancing here.
            //
            if (path != it->first) {
                return path.ReplacePrefix(it->first, it->second);
            } else {
                return path;
            }
        }
    }

    // No enclosing instance was found, so return path as-is.
    return path;
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    for (const auto& entry: entries) {
        if (entry.primType == HdPrimTypeTokens->instancer) {
            _UpdateInstancerLocations(entry.primPath);
        }
    }
    _SendPrimsAdded(entries);
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    for (const auto& entry: entries) {
        if (_instancerToInstancesMap.find(entry.primPath) !=
            _instancerToInstancesMap.end()) {
            _UpdateInstancerLocations(entry.primPath);
        }
    }
    _SendPrimsRemoved(entries);
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    for (const auto& entry: entries) {
        // If instancerTopology was dirtied, update instance map.
        if (entry.dirtyLocators.Intersects(
                HdInstancerTopologySchema::GetDefaultLocator()) &&
            (_instancerToInstancesMap.find(entry.primPath) !=
             _instancerToInstancesMap.end())) {
            _UpdateInstancerLocations(entry.primPath);
        }
    }
    _SendPrimsDirtied(entries);
}

void
UsdImaging_InstanceProxyPathTranslationSceneIndex::_UpdateInstancerLocations(
    const SdfPath &instancerPath)
{
    TRACE_FUNCTION();

    const HdSceneIndexPrim instancerPrim =
        _GetInputSceneIndex()->GetPrim(instancerPath);

    // Check if the prim no longer exists, or is no longer an instancer.
    if (!instancerPrim.dataSource
        || instancerPrim.primType != HdPrimTypeTokens->instancer)  {
        // Remove all associated instance->prototype mappings.
        auto instancerIt = _instancerToInstancesMap.find(instancerPath);
        if (TF_VERIFY(instancerIt != _instancerToInstancesMap.end())) {
            for (SdfPath const& instancePath: instancerIt->second) {
                // Remove this instance->prototype mapping.
                _instanceToPrototypePathMap.erase(instancePath);
            }
            // Remove the instancer entry.
            _instancerToInstancesMap.erase(instancerIt);
        }
        return;
    }

    const HdInstancerTopologySchema topologySchema =
        HdInstancerTopologySchema::GetFromParent(
            instancerPrim.dataSource);
    const HdPathArrayDataSourceHandle prototypesDs =
        topologySchema.GetPrototypes();
    if (!prototypesDs) {
        return;
    }
    const HdPathArrayDataSourceHandle instanceLocationsDs =
        topologySchema.GetInstanceLocations();
    if (!instanceLocationsDs) {
        return;
    }
    const HdIntArrayVectorSchema instanceIndicesSchema =
        topologySchema.GetInstanceIndices();
    const VtArray<SdfPath> prototypePaths =
        prototypesDs->GetTypedValue(0.0);
    const VtArray<SdfPath> instanceLocations =
        instanceLocationsDs->GetTypedValue(0.0);

    // Prototype path and instance index arrays must correspond.
    if (prototypePaths.size() != instanceIndicesSchema.GetNumElements()) {
        // An upstream scene index produced invalid instancer topology.
        TF_CODING_ERROR(
            "Prototype path count and instance indices count do not match "
            "(%zu vs %zu) for instancer <%s>; cannot translate proxy paths.",
            prototypePaths.size(), instanceIndicesSchema.GetNumElements(),
            instancerPath.GetText());
        return;
    }

    // We will update the set of instance paths for this instancer.
    _PathSet &instancePathSet = _instancerToInstancesMap[instancerPath];

    // Clear out any prior entries; we'll rebuild them below.
    {
        for (SdfPath const& instancePath: instancePathSet) {
            _instanceToPrototypePathMap.erase(instancePath);
        }
        instancePathSet.clear();
    }

    // For each prototype:
    for (size_t prototypeIndex=0, n=prototypePaths.size();
        prototypeIndex < n; ++prototypeIndex) {
        const SdfPath& prototypePath = prototypePaths[prototypeIndex];
        // Get the instance indices using the prototype.
        VtArray<int> instanceIndices;
        if (HdIntArrayDataSourceHandle indicesDs =
            instanceIndicesSchema.GetElement(prototypeIndex)) {
            instanceIndices = indicesDs->GetTypedValue(0);
        }
        // Map each instance location to its prototype's path.
        for (int i: instanceIndices) {
            if (i < 0 || i >= (int) instanceLocations.size()) {
                TF_CODING_ERROR(
                    "Invalid instance index %i for instancer <%s>",
                    i, instancerPath.GetText());
                continue;
            }
            const SdfPath& instancePath = instanceLocations[i];
            instancePathSet.insert(instancePath);
            // We need to check for and disregard cases where
            // the instance is an ancestor of its prototype.
            // See testUsdImagingGLPointInstancer/pi_ni_material2.usda
            // for an example where this occurs.
            if (!instancePath.HasPrefix(prototypePath) &&
                !prototypePath.HasPrefix(instancePath)) {
                _instanceToPrototypePathMap[instancePath] = prototypePath;
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
