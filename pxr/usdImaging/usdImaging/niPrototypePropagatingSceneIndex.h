//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_PROPAGATING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_PROPAGATING_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSourceHash.h"
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
/// This scene index uses the UsdImagingNiInstanceAggregationSceneIndex
/// to find all instances, aggregate them and insert instancers for
/// each set of aggregated instances. This scene index then inserts
/// flattened and possibly further transformed (e.g. applying draw mode)
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
/// This scene index is implemented by a merging scene index with the
/// following inputs:
/// - a scene index ultimately tracing back to UsdImaging_NiPrototypeSceneIndex
///   which prepares the prototype for which this scene index was created.
///   The scene indices applied after UsdImaging_NiPrototypeSceneIndex
///   include a flattening scene index as well as scene indices
///   that can be specified through a callback by a user (typically,
///   the draw mode scene index).
/// - the UsdImaging_NiInstanceAggregationSceneIndex instantiated from the
///   above scene index. The instance aggregation scene index will insert the
///   instancers for the instances within this prototype.
/// - More UsdImagingNiPrototypePropagatingSceneIndex's:
///   The _InstanceAggregationSceneIndexObserver will observe the
///   latter scene index to add
///   respective UsdImagingNiPrototypePropagatingSceneIndex's under
///   each instancer.
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
///         * UsdImagingDrawModeSceneIndex (through SceneIndexAppendCallback)
///              * HdFlatteningSceneIndex 
///                inputArgs = UsdImagingFlattenedDataSourceProviders()
///                [So model:drawMode is also flattened]
///                  * UsdImaging_NiPrototypeSceneIndex
///                      forPrototype = false
///                      prototypeRootOverlayDs = null
///                      * UsdImaging_NiPrototypePruningSceneIndex
///                        forPrototype = false
///                          * inputSceneIndex (typically a UsdImagingPiPrototypePropagatingSceneIndex)
///         * UsdImaging_NiInstanceAggregationSceneIndex 
///           forPrototype = false
///           instanceDataSourceNames = ['materialBindings', 'purpose', 'model']
///              * UsdImagingDrawModeSceneIndex
///                [... as above]
///         * UsdImagingRerootingSceneIndex
///           (inserted by _InstanceAggregationSceneIndexObserver::PrimsAdded
///            through _MergingSceneIndexEntry)
///           srcPrefix = /UsdNiInstancer
///           dstPrefix = /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///             * UsdImagingNiPrototypePropagatingSceneIndex
///               prototypeName = __Prototype_1
///                 * HdMergingSceneIndex
///                     * UsdImagingDrawModeSceneIndex (through SceneIndexAppendCallback)
///                         * HdFlatteningSceneIndex 
///                           inputArgs = UsdImagingFlattenedDataSourceProviders()
///                           [So model:drawMode is also flattened]
///                             * UsdImaging_NiPrototypeSceneIndex
///                               forPrototype = true
///                                 * UsdImagingRerootingSceneIndex
///                                   srcPrefix = /__PrototypeRoot1
///                                   dstPrefix = /UsdNiInstancer/UsdNiPrototype
///                                     * inputSceneIndex
///                     * UsdImaging_NiInstanceAggregationSceneIndex
///                       forPrototype = true
///                         * UsdImagingDrawModeSceneIndex
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
///             instancer: /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer
///             prototypeId: 0
///             instanceId: 0
///         purpose: # From flattening scene index
///             purpose: geometry 
///         xform: # From flattening scene index
///             matrix: [ identity matrix]
///         primOrigin:
///             scenePath: HdPrimOriginSchema::OriginPath(/Cube_1)
///         ...
/// /MyPrototype # Not referenced from a different file, so appears here
///              # as non-prototype as well
///     primType: ""
/// /MyPrototype/MyCube
///     primType: cube
/// /UsdNiPropagatedPrototypes
///     primType: ""
/// /UsdNiPropagatedPrototypes/Bindings_423...234
///     primType: ""
///     dataSource:
///         purpose: # Added by instance aggregation scene index, copied from /Cube_1
///             purpose: geometry
///         # No xform, visibility (never copied by instance aggregation, written to
///                                 instancer instead)
/// /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1
///     primType: ""
/// /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer
///     primType: instancer
///     dataSource:
///         instancerTopology:
///             instanceIndices:
///                 i0: [ 0 ]
///             prototypes: [ /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer/UsdNiPrototype
///             instanceLocations: [ /Cube_1 ] # for picking
///         primvars:
///             hydra:instanceTransforms:
///                 primvarValue: [ identity matrix ]
///                 interpolation: instance
/// /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer/UsdNiPrototype
///     primType: ""
///     dataSource:
///         instancedBy:
///             paths: [ /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer ]
///             prototypeRoot: /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1 /UsdNiInstancer/UsdNiPrototype
///         purpose: # Added by prototype scene index, copied from /UsdNiPropagatedPrototypes/Bindings_423...234
///                  # Flattened scene index did not touch it.
///             purpose: geometry
///         xform: # From flattening scene index
///             matrix: [ identity matrix ]
///             resetXformStack: true
///         primOrigin:
///             scenePath: HdPrimOriginSchema::OriginPath(.)
/// /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer/UsdNiPrototype/MyCube
///     primType: cube
///     dataSource:
///         instancedBy:
///             paths: [ /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1/UsdNiInstancer ]
///             prototypeRoot: /UsdNiPropagatedPrototypes/Bindings_423...234/__Prototype_1 /UsdNiInstancer/UsdNiPrototype
///         purpose: # From flattening scene index
///             purpose: geometry
///         xform: # From flattening scene index
///             matrix: [ identity matrix ]
///             resetXformStack: true
///         primOrigin:
///             scenePath: HdPrimOriginSchema::OriginPath(MyCube)
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
///        ...
/// /UsdNiPropagatedPrototypes
///    primType: ""
/// /UsdNiPropagatedPrototypes/NoBindings
///    primType: ""
///    dataSource:
///        ...
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
///        ...
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings
///    primType: ""
///    dataSource:
///        ...
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
///        ...
/// /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype/MyCube
///    primType: "cube"
///    dataSource:
///        instancedBy:
///            paths: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/UsdPiPrototype/UsdNiInstancer ]
///            prototypeRoot: /UsdNiPropagatedPrototypes/NoBindings/__Prototype_2/UsdNiInstancer/UsdPiPrototype/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdPiPrototype
///        ...
///
///
class UsdImagingNiPrototypePropagatingSceneIndex final
                                : public HdFilteringSceneIndexBase
                                , public HdEncapsulatingSceneIndexBase
{
public:
    using SceneIndexAppendCallback =
        std::function<
            HdSceneIndexBaseRefPtr(const HdSceneIndexBaseRefPtr &inputScene)>;

    // instanceDataSourceNames are the names of the data sources of a native
    // instance prim that need to have the same values for the instances to
    // be aggregated. A copy of these data sources is bundled into the
    // prim data source for the binding scope.
    //
    // When propagating a prototype by inserting the scene index isolating
    // that prototype into the merging scene index implementing this scene
    // index, we also call sceneIndexAppendCallback.
    //
    // The use case is for the UsdImagingDrawModeSceneIndex.
    //
    USDIMAGING_API
    static UsdImagingNiPrototypePropagatingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const TfTokenVector &instanceDataSourceNames,
        const SceneIndexAppendCallback &sceneIndexAppendCallback);

    USDIMAGING_API
    ~UsdImagingNiPrototypePropagatingSceneIndex() override;

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    USDIMAGING_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override;

    USDIMAGING_API
    std::vector<HdSceneIndexBaseRefPtr> GetEncapsulatedScenes() const override;

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
        HdContainerDataSourceHandle const &prototypeRootOverlayDs,
        _SceneIndexCacheSharedPtr const &cache);

    UsdImagingNiPrototypePropagatingSceneIndex(
        const TfToken &prototypeName,
        HdContainerDataSourceHandle const &prototypeRootOverlayDs,
        _SceneIndexCacheSharedPtr const &cache);

    void _Populate(HdSceneIndexBaseRefPtr const &instanceAggregationSceneIndex);
    void _AddPrim(const SdfPath &primPath);
    void _RemovePrim(const SdfPath &primPath);

    const TfToken _prototypeName;
    const HdDataSourceHashType _prototypeRootOverlayDsHash;
    _SceneIndexCacheSharedPtr const _cache;

    HdMergingSceneIndexRefPtr _mergingSceneIndex;

    std::map<SdfPath, _MergingSceneIndexEntryUniquePtr>
        _instancersToMergingSceneIndexEntry;

    HdSceneIndexBaseRefPtr _instanceAggregationSceneIndex;

    _InstanceAggregationSceneIndexObserver
        _instanceAggregationSceneIndexObserver;
    _MergingSceneIndexObserver _mergingSceneIndexObserver;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
