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
#ifndef PXR_IMAGING_HD_FLATTENING_SCENE_H
#define PXR_IMAGING_HD_FLATTENING_SCENE_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/usd/sdf/pathTable.h"
#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

using HdFlattenedDataSourceProviderSharedPtr =
    std::shared_ptr<class HdFlattenedDataSourceProvider>;
using HdFlattenedDataSourceProviderSharedPtrVector =
    std::vector<HdFlattenedDataSourceProviderSharedPtr>;

namespace HdFlatteningSceneIndex_Impl
{
constexpr uint32_t _smallVectorSize = 8;
using _DataSourceLocatorSetVector =
    TfSmallVector<HdDataSourceLocatorSet, _smallVectorSize>;
}

TF_DECLARE_REF_PTRS(HdFlatteningSceneIndex);

///
/// \class HdFlatteningSceneIndex
///
/// A scene index that observes an input scene index and produces a comparable
/// scene in which inherited state is represented at leaf prims.
///
/// This kind of representation is typically required for legacy-style 
/// render delegates that require all the information to be available at the
/// leaf prims.
///
class HdFlatteningSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// Creates a new flattening scene index.
    /// inputArgs maps names to HdFlattenedDataSourceProviderSharedPtr's.
    /// That provider flattens the data sources under the locator name
    /// in each prim source.
    ///
    static HdFlatteningSceneIndexRefPtr New(
                HdSceneIndexBaseRefPtr const &inputScene,
                HdContainerDataSourceHandle const &inputArgs) {
        return TfCreateRefPtr(
            new HdFlatteningSceneIndex(inputScene, inputArgs));
    }

    HD_API
    ~HdFlatteningSceneIndex() override;

    // satisfying HdSceneIndexBase
    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API 
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// Data sources under locator name in a prim source get flattened.
    const TfTokenVector &
    GetFlattenedDataSourceNames() const {
        return _dataSourceNames;
    }

    /// Providers in the same order as GetFlattenedDataSourceNames.
    const HdFlattenedDataSourceProviderSharedPtrVector &
    GetFlattenedDataSourceProviders() const {
        return _dataSourceProviders;
    }

protected:

    HD_API
    HdFlatteningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        HdContainerDataSourceHandle const &inputArgs);

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    using _DataSourceLocatorSetVector =
        HdFlatteningSceneIndex_Impl::_DataSourceLocatorSetVector;

    // Consolidate _recentPrims into _prims.
    void _ConsolidateRecentPrims();

    void _DirtyHierarchy(
        const SdfPath &primPath,
        const _DataSourceLocatorSetVector &relativeDirtyLocators,
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries);

    void _PrimDirtied(
        const HdSceneIndexObserver::DirtiedPrimEntry &entry,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries);

    // _dataSourceNames and _dataSourceProviders run in parallel
    // and indicate that a data source at locator name in a prim data
    // source gets flattened by provider.
    TfTokenVector _dataSourceNames;
    HdFlattenedDataSourceProviderSharedPtrVector _dataSourceProviders;

    // Stores all data source names - convenient to quickly send out
    // dirty messages for ancestors of resynced prims.
    HdDataSourceLocatorSet _dataSourceLocatorSet;
    // Stores universal set for each name in data source names - convenient
    // to quickly invalidate all relevant data sourced of ancestors of
    // resynced prim.
    _DataSourceLocatorSetVector _relativeDataSourceLocators;

    // members
    using _PrimTable = SdfPathTable<HdSceneIndexPrim>;
    _PrimTable _prims;

    struct _PathHashCompare {
        static bool equal(const SdfPath &a, const SdfPath &b) {
            return a == b;
        }
        static size_t hash(const SdfPath &path) {
            return hash_value(path);
        }
    };
    using _RecentPrimTable =
        tbb::concurrent_hash_map<SdfPath, HdSceneIndexPrim, _PathHashCompare>;
    mutable _RecentPrimTable _recentPrims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
