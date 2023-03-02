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
#ifndef PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_PROPAGATING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_PROPAGATING_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdMergingSceneIndex);

TF_DECLARE_REF_PTRS(UsdImagingNiPrototypePropagatingSceneIndex);

/// \class UsdImagingNiPrototypePropagatingSceneIndex
///
/// A scene index implementing USD native instancing. If combined
/// with the UsdImagingPiPrototypePropagatingSceneIndex, the native
/// instancing scene index has to be run after the point instancing
/// scene index.
///
/// This scene index uses the UsdImgingNiInstanceAggregationSceneIndex
/// to find all instances, aggregate them and insert instancers for
/// each set of aggregated instances. This scene index then inserts
/// copies of the corresponding USD prototype underneath each of these
/// instancers. Each of these copies is actually a
/// UsdImagingNiPrototypePropagatingSceneIndex itself. This way, we can
/// handle nested native instancing. In other words, we can call the
/// UsdImagingNiPrototypePropagatingSceneIndex for a USD prototype and
/// it will find the instances within that prototype.
///
/// The instancing scene index uses the instancedBy:prototypeRoot
/// of the input scene index during aggregation.
/// Typically, the input scene index will be a
/// UsdImagingPiPrototypePropagatingSceneIndex which populates
/// instancedBy:prototypeRoot based on which point instancer is
/// instancing a prim.
///
/// This scene index is implemented by a merging scene index.
/// One input to the merging scene index is the
/// UsdImaging_NiPrototypeSceneIndex which prepares the prototype
/// for which this scene index was created.
/// Another input to the merging scene index is the
/// UsdImaging_NiInstanceAggregationSceneIndex that will insert the
/// instancers for the instances within this prototype.
/// The _InstanceAggregationSceneIndexObserver will observe the
/// latter scene index to add
/// respective UsdImagingNiPrototypePropagatingSceneIndex's under
/// each instancer.
///
/// Example 1 (also see Example 1 in niInstanceAggregationSceneIndex.h)
///
/// USD:
///
/// def Xform "MyPrototype"
/// {
///     def Cube "MyCube"
///     {
///     }
/// }
///
/// def "Cube_1" (
///     instanceable = true
///     references = </MyPrototype>
/// {
/// }
///
/// Inputs of the UsdImagingNiPrototypePropagatingSceneIndex(inputSceneIndex):
///
///     * HdMergingSceneIndex
///         * UsdImaging_NiPrototypeSceneIndex
///           prototypeRoot = ""
///             * HdFlatteningSceneIndex
///                 * UsdImaging_NiPrototypePruningSceneIndex
///                   prototypeRoot = ""
///                     * inputSceneIndex (typically a UsdImagingPiPrototypePropagatingSceneIndex)
///         * UsdImaging_NiInstanceAggregationSceneIndex 
///             * HdFlatteningSceneIndex
///                 [... as above]
///         * UsdImagingRerootingSceneIndex
///           (inserted by _InstanceAggregationSceneIndexObserver::PrimsAdded
///            through _MergingSceneIndexEntry)
///           srcPrefix = /
///           dstPrefix = /__Usd_Prototypes/NoBindings/__Prototype_1
///             * UsdImagingNiPrototypePropagatingSceneIndex
///               prototypeName = __Prototype_1
///                 * HdMergingSceneIndex
///                     * UsdImaging_NiPrototypeSceneIndex
///                       prototypeRoot = /__PrototypeRoot1
///                         * HdFlatteningSceneIndex
///                             * UsdImagingRerootingSceneIndex
///                               srcPrefix = /__PrototypeRoot1
///                               dstPrefix = /__PrototypeRoot1
///                                 * inputSceneIndex
///                     * UsdImaging_NiInstanceAggregationSceneIndex 
///                         * HdFlatteningSceneIndex
///                             [... as just above]
///
/// UsdImagingNiPrototypePropagatingSceneIndex
///
/// /Cube_1
///     primType: ""
///     dataSource:
///         instance: # Useful for translating Usd proxy paths for selection.
///                   # See corresponding example in niInstanceAggregationIndex
///                   # for more details.
///             instancer: /__Usd_Prototypes/NoBindings/__Prototype_1
///             prototypeId: 0
///             instanceId: 0
/// /MyPrototype # Not referenced from a different file, so appears here
///              # as non-prototype as well
///     primType: ""
/// /MyPrototype/MyCube
///     primType: cube
/// /__Usd_Prototypes
///     primType: ""
/// /__Usd_Prototypes/NoBindings
///     primType: ""
/// /__Usd_Prototypes/NoBindings/__Prototype_1
///     primType: instancer
///     dataSource:
///         instancerTopology:
///             instanceIndices:
///                 i0: [ 0 ]
///             prototypes: [ /__Usd_Prototypes/NoBindings/__Prototype_1/__Protoype_1 ]
///             instanceLocations: [ /Cube_1 ] # for picking
///         primvars:
///             instanceTransform:
///                 primvarValue: [ identity matrix ]
///                 interpolation: instance
/// /__Usd_Prototypes/NoBindings/__Prototype_1/__Prototype_1
///     primType: ""
///         instancedBy:
///             paths: [ /__Usd_Prototypes/NoBindings/__Prototype_1 ]
///         prototypeRoot: /__Usd_Prototypes/NoBindings/__Prototype_1 /__Prototype_1
/// /__Usd_Prototypes/NoBindings/__Prototype_1/__Prototype_1/MyCube
///     primType: cube
///         instancedBy:
///             paths: [ /__Usd_Prototypes/NoBindings/__Prototype_1 ]
///         prototypeRoot: /__Usd_Prototypes/NoBindings/__Prototype_1 /__Prototype_1
///
/// Example 2:
///
/// def Xform "MyNestedPrototype" # Will become USD prototype /__Prototype_1
/// {
///    def Cube "MyCube"
///    {
///    }
/// }
///
/// def Xform "MyPrototype" # Will become USD prototype /__Prototype_2
/// {
///    def "MyNestedInstance" (
///         instanceable = true
///         references = </MyNestedPrototype> )
///     {
///     }
/// }
/// 
/// def Xform "MyInstance"  (
///     instanceable = true
///     references = </MyPrototype>)
/// {
/// 
/// }
/// 
/// UsdImagingNiPrototypePropagatingSceneIndex
///
/// ...
/// /MyInstance
///    primType: ""
///    dataSource:
///        instance:
///            instancer: /__Usd_Prototypes/NoBindings/__Prototype_2
///            prototypeId: 0
///            instanceId: 0
/// /__UsdPrototypes
/// /__UsdPrototypes/NoBindings
///    primType: ""
/// /__UsdPrototypes/NoBindings/__Prototype_2
///    primType: instancer
///    dataSource:
///        instanerTopology:
///            prototypes: [ /__UsdPrototypes/NoBindings/__Prototype_2 ]
///        ...
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2
///    primType: ""
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/MyNestedInstance
///    primType: ""
///    dataSource:
///             instancer: /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1
///             prototypeId: 0
///             instanceId: 0
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings
///    primType: ""
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1
///    primType: instancer
///    dataSource:
///        instanerTopology:
///            prototypes: [ /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1/__Prototype_1 ]
///        ...
///        instancedBy:
///            paths: [ /__UsdPrototypes/NoBindings/__Prototype_2 ]
///            prototypeRoot: /__UsdPrototypes/NoBindings/__Prototype_2/__Prototype_2
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1/__Prototype_1
///    primType: ""
///    dataSource:
///        instancedBy:
///            paths: [ /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1 ]
///            prototypeRoot: /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1/__Prototype_1
/// /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1/__Prototype_1/MyCube
///    primType: "cube"
///    dataSource:
///        instancedBy:
///            paths: [ /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1 ]
///            prototypeRoot: /__UsdPrototypes/NoBindings/__Prototype_2/_Prototype_2/__UsdPrototypes/NoBindings/__Prototype_1/__Prototype_1
/// ...
///
///
class UsdImagingNiPrototypePropagatingSceneIndex final
                                : public HdFilteringSceneIndexBase
{
public:

    USDIMAGING_API
    static UsdImagingNiPrototypePropagatingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    USDIMAGING_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override;

private:
    class _SceneIndexCache;
    using _SceneIndexCacheSharedPtr =
        std::shared_ptr<_SceneIndexCache>;

    class _MergingSceneIndexEntry;
    using _MergingSceneIndexEntryUniquePtr =
        std::unique_ptr<_MergingSceneIndexEntry>;

    friend class _InstanceAggregationSceneIndexObserver;
    class _InstanceAggregationSceneIndexObserver : public HdSceneIndexObserver
    {
    public:
        _InstanceAggregationSceneIndexObserver(
            UsdImagingNiPrototypePropagatingSceneIndex * owner);

        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override;
        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override;
        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override;

    private:
        UsdImagingNiPrototypePropagatingSceneIndex * const _owner;
    };

    friend class _MergingSceneIndexObserver;
    class _MergingSceneIndexObserver : public HdSceneIndexObserver
    {
    public:
        _MergingSceneIndexObserver(
            UsdImagingNiPrototypePropagatingSceneIndex * owner);

        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override;
        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override;
        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override;
    private:
        UsdImagingNiPrototypePropagatingSceneIndex * const _owner;
    };

    // Use prototypeName to instantiate for "scene root".
    static UsdImagingNiPrototypePropagatingSceneIndexRefPtr _New(
        const TfToken &prototypeName,
        _SceneIndexCacheSharedPtr const &cache);

    UsdImagingNiPrototypePropagatingSceneIndex(
        const TfToken &prototypeName,
        _SceneIndexCacheSharedPtr const &cache);

    void _Populate(HdSceneIndexBaseRefPtr const &instanceAggregationSceneIndex);
    void _AddPrim(const SdfPath &primPath);
    void _RemovePrim(const SdfPath &primPath);

    const TfToken _prototypeName;
    _SceneIndexCacheSharedPtr const _cache;

    HdMergingSceneIndexRefPtr const _mergingSceneIndex;

    std::map<SdfPath, _MergingSceneIndexEntryUniquePtr>
        _instancersToMergingSceneIndexEntry;

    _InstanceAggregationSceneIndexObserver
        _instanceAggregationSceneIndexObserver;
    _MergingSceneIndexObserver _mergingSceneIndexObserver;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
