//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_INSTANCE_PROXY_PATH_TRANSLATION_SCENE_INDEX_H
#define PXR_USD_IMAGING_INSTANCE_PROXY_PATH_TRANSLATION_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdMergingSceneIndex);
TF_DECLARE_WEAK_AND_REF_PTRS(UsdImaging_InstanceProxyPathTranslationSceneIndex);

/// \class UsdImaging_InstanceProxyPathTranslationSceneIndex
///
/// A scene index that translates SdfPath-valued data sources pointing
/// under instances to point to the corresponding prototype paths.
///
/// This scene index tracks instancer prims, maintaing a mapping from
/// instancerTopology.instanceLocations to the matching prototype paths.
/// The mapping is applied as a prefix substitution to SdfPath
/// values returned by upstream data sources.
///
class UsdImaging_InstanceProxyPathTranslationSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImaging_InstanceProxyPathTranslationSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        TfTokenVector const& proxyPathDataSourceNames);

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // Returns true if path translation should be applied to paths under
    // the given named data source.
    bool ShouldTranslatePathsForDataSourceName(TfToken const& name) const;

    // Translates the given path from instance to instancer prototype.
    // If the given path is not at or under an instance, returns it unchagned.
    SdfPath TranslatePath(SdfPath const& path) const;

private:
    UsdImaging_InstanceProxyPathTranslationSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        TfTokenVector const& proxyPathDataSourceNames);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    void _UpdateInstancerLocations(const SdfPath &instancerPath);

private: // data
    //Data source names under which to apply proxy path translation.
    const TfTokenVector _proxyPathDataSourceNames;
    // Unordered set of paths.
    using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;
    // Map of instancer paths to instance paths.
    std::unordered_map<SdfPath, _PathSet, TfHash> _instancerToInstancesMap;
    // Map of instance path to prototype path.
    std::unordered_map<SdfPath, SdfPath, TfHash> _instanceToPrototypePathMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
