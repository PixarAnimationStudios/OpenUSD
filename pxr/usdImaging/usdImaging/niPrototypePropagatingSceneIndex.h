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
///           forPrototype = false
///              * UsdImaging_NiPrototypePruningSceneIndex
///                forPrototype = false
///                  * inputSceneIndex (typically a UsdImagingPiPrototypePropagatingSceneIndex)
///         * UsdImaging_NiInstanceAggregationSceneIndex 
///           forPrototype = false
///              * UsdImaging_NiPrototypePruningSceneIndex
///                [... as above]
///         * UsdImagingRerootingSceneIndex
///           (inserted by _InstanceAggregationSceneIndexObserver::PrimsAdded
///            through _MergingSceneIndexEntry)
///           srcPrefix = /UsdNiInstancer
///           dstPrefix = /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///             * UsdImagingNiPrototypePropagatingSceneIndex
///               prototypeName = __Prototype_1
///                 * HdMergingSceneIndex
///                     * UsdImaging_NiPrototypeSceneIndex
///                       forPrototype = true
///                         * UsdImagingRerootingSceneIndex
///                           srcPrefix = /__PrototypeRoot1
///                           dstPrefix = /UsdNiInstancer/UsdNiPrototype
///                             * inputSceneIndex
///                     * UsdImaging_NiInstanceAggregationSceneIndex
///                       forPrototype = true
///                         * UsdImagingRerootingSceneIndex
///                           [... as just above]
///
/// UsdImagingNiPrototypePropagatingSceneIndex
///
/// /Cube_1
///     primType: ""
///     dataSource:
///         instance: # Useful for translating Usd proxy paths for selection.
///                   # See corresponding example in niInstanceAggregationIndex
///                   # for more details.
///             instancer: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///             prototypeId: 0
///             instanceId: 0
/// /MyPrototype # Not referenced from a different file, so appears here
///              # as non-prototype as well
///     primType: ""
/// /MyPrototype/MyCube
///     primType: cube
/// /UsdNiPropagatedPrototypes
/// /UsdNiPropagatedPrototypes/NoBindings
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1
///     primType: ""
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///     primType: instancer
///     dataSource:
///         instancerTopology:
///             instanceIndices:
///                 i0: [ 0 ]
///             prototypes: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype
///             instanceLocations: [ /Cube_1 ] # for picking
///         primvars:
///             instanceTransform:
///                 primvarValue: [ identity matrix ]
///                 interpolation: instance
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype
///     primType: ""
///     dataSource:
///         instancedBy:
///             paths: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer ]
///             prototypeRoot: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1 /UsdNiInstancer/UsdNiPrototype
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype/MyCube
///     primType: cube
///     dataSource:
///         instancedBy:
///             paths: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer ]
///             prototypeRoot: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1 /UsdNiInstancer/UsdNiPrototype
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
///            instancer: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer
///            prototypeId: 0
///            instanceId: 0
/// /UsdNiPropagatedPrototypes
/// /UsdNiPropagatedPrototypes/NoBindings
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2
///    primType: ""
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer
///    primType: instancer
///    dataSource:
///        instanerTopology:
///            prototypes: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype ]
///        ...
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/_Prototype_2
///    primType: ""
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/_Prototype_2/MyNestedInstance
///    primType: ""
///    dataSource:
///             instancer: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///             prototypeId: 0
///             instanceId: 0
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1
///    primType: ""
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///    primType: instancer
///    dataSource:
///        instanerTopology:
///            prototypes: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype ]
///        ...
///        instancedBy:
///            paths: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer ]
///            prototypeRoot: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype
///    primType: ""
///    dataSource:
///        instancedBy:
///            paths: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer ]
///            prototypeRoot: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype/MyCube
///    primType: "cube"
///    dataSource:
///        instancedBy:
///            paths: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/UsdPiPrototype/UsdNiInstancer ]
///            prototypeRoot: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype
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
        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override;

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
        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override;
        
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
