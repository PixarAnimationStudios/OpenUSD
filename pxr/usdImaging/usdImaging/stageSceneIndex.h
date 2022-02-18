//
// Copyright 2022 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/usd/usd/stage.h"

#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingPrimAdapter;
using UsdImagingPrimAdapterSharedPtr = std::shared_ptr<UsdImagingPrimAdapter>;

class UsdImagingStageSceneIndex;
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);

class UsdImagingStageSceneIndex : public HdSceneIndexBase
{
public:

    static UsdImagingStageSceneIndexRefPtr New() {
        return TfCreateRefPtr(new UsdImagingStageSceneIndex());
    }

    // ------------------------------------------------------------------------
    // Scene index API

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath & primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath & primPath) const override;

    // ------------------------------------------------------------------------
    // App-facing API

    // Set the USD stage to pull data from. Note that this will delete all
    // scene index prims and reset stage global data.
    USDIMAGING_API
    void SetStage(UsdStageRefPtr stage);

    // Traverse the scene collecting imaging prims, and then call PrimsAdded.
    USDIMAGING_API
    void Populate();

    // Set the time, and call PrimsDirtied for any time-varying attributes.
    USDIMAGING_API
    void SetTime(UsdTimeCode time);

    // Return the current time.
    USDIMAGING_API
    UsdTimeCode GetTime() const;

private:
    USDIMAGING_API
    UsdImagingStageSceneIndex();

    Usd_PrimFlagsConjunction _GetTraversalPredicate() const;

    void _PopulateAdapterMap();

    // Adapter delegation.
    UsdImagingPrimAdapterSharedPtr _AdapterLookup(UsdPrim prim) const;

    TfTokenVector _GetImagingSubprims(
            UsdPrim prim) const;
    TfToken _GetImagingSubprimType(
            UsdPrim prim, TfToken const& subprim) const;
    HdContainerDataSourceHandle _GetImagingSubprimData(
            UsdPrim prim, TfToken const& subprim) const;

    class _StageGlobals : public UsdImagingDataSourceStageGlobals
    {
    public:
        // Datasource-facing API
        void FlagAsTimeVarying(
            const SdfPath & primPath,
            const HdDataSourceLocator & locator) const override;

        UsdTimeCode GetTime() const override;

        // Scene index-facing API
        void SetTime(UsdTimeCode time,
                HdSceneIndexObserver::DirtiedPrimEntries *dirtied);
        void Clear();

    private:
        struct _PathHashCompare {
            static bool equal(const SdfPath &a, const SdfPath &b) {
                return a == b;
            }
            static size_t hash(const SdfPath &p) {
                return hash_value(p);
            }
        };
        using _VariabilityMap = tbb::concurrent_hash_map<SdfPath,
                HdDataSourceLocatorSet, _PathHashCompare>;
        mutable _VariabilityMap _timeVaryingLocators;

        UsdTimeCode _time;
    };

    UsdStageRefPtr _stage;
    _StageGlobals _stageGlobals;

    // Usd Prim Type to Adapter lookup table.
    using _AdapterMap = TfHashMap<TfToken, UsdImagingPrimAdapterSharedPtr, 
                TfToken::HashFunctor>;
    _AdapterMap _adapterMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
