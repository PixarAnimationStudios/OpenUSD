//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_NI_INSTANCE_AGGREGATION_SCENE_INDEX_H
#define PXR_USD_IMAGING_NI_INSTANCE_AGGREGATION_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImaging_NiInstanceAggregationSceneIndex_Impl
{
TF_DECLARE_WEAK_PTRS(_InstanceObserver);
}

TF_DECLARE_REF_PTRS(UsdImaging_NiInstanceAggregationSceneIndex);

/// \class UsdImaging_NiInstanceAggregationSceneIndex
///
/// Looks for instances (prims with usdPrototypePath) in the input scene
/// index. Computes which instances can be aggregated together.
/// It returns an instancer for each set of aggregated instances. That instancer
/// instances the corresponding (native) prototype. It also returns prims
/// (of empty type) that group the instancers and that provide bindings
/// (such as material bindings).
///
/// Instances can be aggregated together if they have the same:
/// 1. "enclosing prototype root", i.e., the path from the data source
/// at instancedBy:prototypeRoot. It is populated by the point instancing
/// scene delegates. In other words, we can only aggregate instances
/// that are instanced by the same point instancer. If not instanced by
/// a point instancer, the enclosing prototype root is simply /.
/// 2. Same bindings. That is, data source on native instances identified
/// by instanceDataSurceNames.
/// 3. The same prototype (path from the data source at usdPrototypePath).
///
/// The corresponding instancer will be inserted under the enclosing
/// prototype root with the relative path indicating what the bindings and
/// the prototype are. The instancer's primvars:instanceTransforms will be
/// populated from the instances' xform:matrix values. The instancer's
/// instancedBy data source is taking from the "enclosing prototype root"
/// (for compatibility with point instancing) and falls back to value
/// determined by the given prototype root.
///
/// E.g., when there are no bindings and the prototype is
/// __Prototype_1, the instancer path will be
/// /UsdNiPropagatesPrototyped/NoBindings/__Prototype_1/UsdNiInstancer.
/// If there are bindings, a hash will be computed, e.g.,
/// /UsdNiPropagatedPrototypes/Bindings32f...723/__Prototype_1/UsdNiInstancer.
/// In that case, /UsdNiPropagatedPrototypes/Bindings32f...723 will be a prim with a
/// copy of the bindings from one of the instances with that binding hash.
///
/// For nested instancing, the UsdImaging_NiInstanceAggregationSceneIndex can
/// be called with the path of a native USD prototype. It will then aggregate
/// native instances within that USD prototype.
///
/// This scene index is implemented by a retained scene index. The
/// (non-recursive) _InstanceObserver observers the input scene index to
/// add, modify or remove binding scopes and instances.
///
/// Example 1:
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
/// UsdImagingStageSceneIndex:
///
/// /MyPrototype # Not referenced from a different file, so appears here
///              # as non-prototype as well
///     primType: ""
/// /MyPrototype/MyCube
///     primType: cube
/// /__Prototype_1
///     dataSource:
///         isUsdPrototype: true
/// /__Prototype_1/MyCube
///     primType: cube
/// /Cube_1
///     usdPrototypePath: /__Prototype_1
///
/// UsdImaging_NiInstanceAggregationSceneIndex (with empty prototype root)
///
/// /Cube_1
///     primType: ""
///     dataSource:
///         instance: # Not relevant for rendering,
///                   # but useful to translate Usd proxy path for, e.g.,
///                   # selection
///             instancer: /_UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///             prototypeId: 0 # Index into instancer's instanceIndices vector
///                            # data source, always 0 since instancer never
///                            # has more than one prototype.
///             instanceId: 0 # Index into VtIntArray at
///                           # instancer's instanceIndices i0.
///                           # The indexed element in VtIntArray was added by
///                           # the instance aggregation because of this
///                           # instance.
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
///             prototypes: [ /UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype ]
///             instanceLocations: [ /Cube_1 ] # for picking
///         primvars:
///             hydra:instanceTransforms:
///                 primvarValue: [ identity matrix ]
///                 interpolation: instance
///
/// Example 2:
///
/// USD:
///
/// def Xform "MyPrototype"
/// {
///     def Cube "MyCube"
///     {
///          rel:material:binding = </MyMaterial>
///     }
/// }
///
/// def "Cube_1" (
///     instanceable = true
///     references = </MyPrototype>
/// {
/// }
///
/// UsdImaging_NiInstanceAggregationSceneIndex (with empty prototype root)
///
/// /Cube_1
///     ... # Similar to above
/// /UsdNiPropagatedPrototypes
///     primType: ""
/// /UsdNiPropagatedPrototypes/Binding312...436
///     primType: ""
///     dataSource:
///         materialBinding:
///             "": /MyMaterial
/// /UsdNiPropagatedPrototypes/Binding312...436/__Prototype_1
///     primType: ""
/// /UsdNiPropagatedPrototypes/Binding312...436/__Prototype_1/UsdNiInstancer
///     primType: instancer
///     dataSource:
///         instancerTopology:
///             instanceIndices:
///                 i0: 0
///             prototypes: [ /UsdNiPropagatedPrototypes/Binding312...436/__Prototype_1/UsdNiInstancer/UsdNiPrototype ]
///             instanceLocations: [ /Cube_1 ] # for picking
///         primvars:
///             hydra:instanceTransforms:
///                 primvarValue: [ identity matrix ]
///                 interpolation: instance
///
/// Example 3:
///
/// USD:
///
/// def Xform "MyNativePrototype"
/// {
///     def Cube "MyCube"
///     {
///     }
/// }
///
/// def PointInstancer "MyPointInstancer"
/// {
///     rel prototypes = [
///         </MyPointInstancer/MyPointPrototype> ]
///     def "MyPointPrototype" (
///         instanceable = true
///         references = </MyNativePrototype>
///     {
///     }
/// }
///
/// UsdImaging_NiInstanceAggregationSceneIndex (with empty prototype root
/// after point instancing scene index):
///
/// /MyPointInstancer
/// /MyPointInstancer/MyPointPrototype
/// /MyPointInstancer/MyPointPrototype/ForInstancer434...256 # Where point instancer inserted copy of /MyPointPrototype
///                                                          # It will be the enclosing prototype root for the instance.
///     primType: ""
///     dataSource:
///         instance:
///             instancer: /MyPointInstancer/MyPointPrototype/ForInstancer434...256/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///             prototypeId: 0
///             instanceId: 0
/// /MyPointInstancer/MyPointPrototype/ForInstancer434...256/UsdNiPropagatedPrototypes
/// /MyPointInstancer/MyPointPrototype/ForInstancer434...256/UsdNiPropagatedPrototypes/NoBindings
/// /MyPointInstancer/MyPointPrototype/ForInstancer434...256/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1
///     primType: ""
/// /MyPointInstancer/MyPointPrototype/ForInstancer434...256/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer
///     primType: instancer
///         instancerTopology:
///             instanceIndices:
///                 i0: [ 0 ]
///             prototypes: [ /MyPointInstancer/MyPointPrototype/ForInstancer434...256/UsdNiPropagatedPrototypes/NoBindings/__Prototype_1/UsdNiInstancer/UsdNiPrototype ]
///             instanceLocations: [ /Cube_1 ] # for picking
///         primvars:
///             hydra:instanceTransforms:
///                 primvarValue: [ identity matrix ]
///                 interpolation: instance
///
class UsdImaging_NiInstanceAggregationSceneIndex final
                : public HdFilteringSceneIndexBase
{
public:
    // forNativePrototype = false indicates that this scene index is
    // instantiated for the USD stage with all USD prototypes filtered out.
    // forNativePrototype = true indicates that it is instantiated for a USD
    // prototype and the instancers it adds for the instancers within this
    // prototype need to have the instancedBy data source populated in turn.
    //
    // instanceDataSourceNames are the names of the data sources of a native
    // instance prim that need to have the same values for the instances to
    // be aggregated. A copy of these data sources is bundled into the
    // prim data source for the binding scope.
    // 
    static UsdImaging_NiInstanceAggregationSceneIndexRefPtr New(
            HdSceneIndexBaseRefPtr const &inputScene,
            const bool forNativePrototype,
            const TfTokenVector &instanceDataSourceNames)
    {
        return TfCreateRefPtr(
            new UsdImaging_NiInstanceAggregationSceneIndex(
                inputScene, forNativePrototype, instanceDataSourceNames));
    }

    ~UsdImaging_NiInstanceAggregationSceneIndex() override;

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override;

    // If the given path is for an instancer in this scene index, returns the
    // name of the prototype that this instancer is meant to instance.
    // Otherwise, return empty token.
    static
    TfToken GetPrototypeNameFromInstancerPath(const SdfPath &primPath);

    // If the given path is for an instancer in this scene index, returns
    // the path of the scope containing the bindings that need to be
    // applied to the prototype. Otherwise, return empty token.
    //
    // That is the data source of that scope needs to be used when resolving
    // the opinions on the prototype that this instancer is meant to instance.
    static
    SdfPath GetBindingScopeFromInstancerPath(const SdfPath &primPath);

private:
    friend class _RetainedSceneIndexObserver;
    class _RetainedSceneIndexObserver : public HdSceneIndexObserver
    {
    public:
        _RetainedSceneIndexObserver(
            UsdImaging_NiInstanceAggregationSceneIndex * owner);

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
        UsdImaging_NiInstanceAggregationSceneIndex * const _owner;
    };

    UsdImaging_NiInstanceAggregationSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        bool forNativePrototype,
        const TfTokenVector &instanceDataSourceNames);

    const TfTokenVector _instanceDataSourceNames;

    std::unique_ptr<
        UsdImaging_NiInstanceAggregationSceneIndex_Impl::
        _InstanceObserver> const _instanceObserver;
    _RetainedSceneIndexObserver _retainedSceneIndexObserver;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_INSTANCE_AGGREGATION_SCENE_INDEX_H
