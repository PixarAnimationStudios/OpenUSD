//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_PI_PROTOTYPE_PROPAGATING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_PI_PROTOTYPE_PROPAGATING_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingPiPrototypePropagatingSceneIndex);

namespace UsdImagingPiPrototypePropagatingSceneIndex_Impl
{
using _ContextSharedPtr = std::shared_ptr<struct _Context>;
using _InstancerObserverUniquePtr = std::unique_ptr<class _InstancerObserver>;
}

/// \class UsdImagingPiPrototypePropagatingSceneIndex
///
/// A scene index translating USD point instancers into Hydra instancers.
///
/// It applies various USD semantics and populates the "instancedBy" schema,
/// including the prototypeRoot data source which is needed by the USD native
/// instancing scene index.
///
/// To achieve various USD behaviors, it has a (recursive) instancer observer
/// that inserts copies of prototypes processed through the prototype scene
/// index into appropriate places in namespace.
///
/// ---------------------------------------------------------------------------
///
/// Example 1:
///
/// USD:
///
/// def PointInstancer "MyInstancer"
/// {
///    rel prototypes = [
///        </MyInstancer/MyPrototypes/MyPrototype> ]
///    def Scope "MyPrototypes"
///    {
///        def Xform "MyPrototype"
///        {
///            def Sphere "MySphere"
///            {
///            }
///        }
///    }
/// }
///
/// Note that USD says that no geometry under a PointInstancer is drawn unless
/// it is targeted by a point instancer's prototype relationship.
///
/// Inputs of the PointPropagatingSceneIndex:
///
///     * _Context::mergingSceneIndex
///       (HdMergingSceneIndex)
///         * _Context::instancerSceneIndex
///           (HdRetainedSceneIndex, will rewrite prototypes of /MyInstancer to
///                                  [ /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55 ] )
///         * UsdImaging_PiPrototypeSceneIndex
///           (inserted by PointPropagatingSceneIndex::_instancerOberver
///                 which was constructed with
///                             instancer = ""
///                             prototypeRoot = /
///                             rerootedPrototypeRoot = /)
///           instancer = ""
///           prototypeRoot = /
///               * _Context::inputSceneIndex
///                 (argument to PointPropagatingSceneIndex,
///                  typically UsdImagingStageSceneIndex, maybe followed by
///                  other filtering scene indices)
///         * UsdImagingRerootingSceneIndex
///           (inserted recursively by PointPropagatingSceneIndex::_instancerObserver::_subinstancerObservers
///                 which was constructed with
///                             instancer = /MyInstancer
///                             prototypeRoot = /MyInstancer/MyPrototypes/MyPrototype
///                             rerootedPrototypeRoot = /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55)
///           srcPrefix = /MyInstancer/MyPrototypes/MyPrototype
///           dstPrefix = /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55
///               * UsdImaging_PiPrototypeSceneIndex
///                 instancer = /MyInstancer
///                 prototypeRoot = /MyInstancer/MyPrototypes/MyPrototype
///                     * UsdImagingRerootingSCeneIndex
///                       srcPrefix = dstPrefix = /MyInstancer/MyPrototypes/MyPrototype
///                           * _Context::inputSceneIndex
///
/// PointPropagatingSceneIndex:
///
/// /MyInstancer
///     primType: instancer
///     dataSource:
///         setting # [1]
///             prototypes = [/MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55]
///
/// /MyInstancer/MyPrototypes
/// /MyInstancer/MyPrototypes/MyPrototype
/// /MyInstancer/MyPrototypes/MyPrototype/MySphere
///     primType: "" # [2]
///     dataSource: unchanged
///
/// /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55 # [3]
///     primType: unchanged (from /MyInstancer/MyPrototypes/MyPrototype)
///     dataSource: (from /MyInstancer/MyPrototypes/MyPrototype)
///         setting # [4]
///             xform:resetXformStack = true
///             instancedBy:PrototypeRoot = /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55
///             instancedBy:paths = /MyInstancer
///
/// /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55/MySphere
///     dataSource: (from /MyInstancer/MyPrototypes/MyPrototype/MySphere)
///         setting # [5]
///             instancedBy:PrototypeRoot = /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55
///             instancedBy:paths = /MyInstancer
///
/// [1] Set through the retained scene index _Context::instancerSceneIndex to
/// point to the re-rooted copy of the prototype.
/// The prim entry was inserted by the (recursive) instancer observer
/// (instantiated with instancer = empty,
/// prototypeRoot = rerootedPrototypeRoot = /).
///
/// [2] Forced by the prototype scene index (instantiated with
/// instancer = empty and prototypeRoot = /).
/// In general, the prototype scene index forces the prim
/// type to empty for all descendants of instancers within the prototypeRoot.
///
/// [3] The re-rooted copy of /MyInstancer/MyPrototypes/MyPrototype inserted
/// by the instancer observer (instantiated with instancer = /MyInstancer,
/// prototypeRoot = /MyInstancer/MyPrototypes/MyPrototypes and
/// rerootedPrototypeRoot = /MyInstancer/MyPrototypes/MyPrototype/ForInstancer84e...f55
///
/// [4] Set by the prototype scene index (instantiated with
/// instancer = /MyInstancer,
/// prototypeRoot = /MyInstancer/MyPrototypes/MyPrototypes).
///
/// For the prototype root itself, it resets the xform stack so that the xform
/// of geometry within a prototype is relative to the root of the prototype.
/// Also see [5].
///
/// [5] Set by the same prototype scene index.
///
/// Sets instancedBy:prototypeRoot and instancedBy:paths on all prims that are
/// not descendants of an instancer within the prototype root.
///
/// ---------------------------------------------------------------------------
///
/// Example 2:
///
/// USD:
///
/// def PointInstancer "MyInstancer"
/// {
///     rel prototypes = [
///         </MyPrototypes/MyPrototype> ]
/// }
/// over "MyPrototypes"
/// {
///     def Xform "MyPrototype"
///     {
///         def Sphere "MySphere"
///         {
///         }
///     }
/// }
///
/// Note that the USD specification says that even though
/// /MyPrototype/MyPrototype is under an "over", it will be drawn (through
/// an instancer) since it is targeted by a PointInstancers' prototypes
/// relationship. Furthermore, if "MyPrototypes" is changed from an "over" to
/// a "def", MySphere would be drawn twice: once in its own right and once
/// being instanced by /MyInstancers.
///
/// PointPropagatingSceneIndex::
///
/// /MyInstancer
///     primType: instancer
///     dataSource:
///         setting # [1]
///             prototypes = [/MyPrototypes/MyPrototype/ForInstancer4e6...f36]
///
/// /MyPrototypes
/// /MyPrototypes/MyPrototype
/// /MyPrototypes/MyPrototype/MySphere
///     primType: "" # [2]
///     dataSource: unchanged
///
/// /MyPrototypes/MyPrototype/ForInstancer4e6...f36 # [3]
///     primType: unchanged (from /MyPrototypes/MyPrototype)
///     dataSource: (from /MyPrototypes/MyPrototype)
///         settings # [4]
///             xform:resetXformStack = true
///             instancedBy:PrototypeRoot = /MyPrototypes/MyPrototype/ForInstancer4e6...f36
///             instancedBy:paths = /MyInstancer
///
/// /MyPrototypes/MyPrototype/ForInstancer4e6...f36/MySphere
///     dataSource: (from /MyInstancer/MyPrototypes/MyPrototype/MySphere)
///         setting # [5]
///             instancedBy:PrototypeRoot = /MyPrototypes/MyPrototype/ForInstancer4e6...f36
///             instancedBy:paths = /MyInstancer
///
/// [1] As [1] in Example 1.
///
/// [2] Forced by the prototype scene index.
/// In general, the prototype scene index forces the prim type to
/// empty for all descendants of an over.
/// Note that we changed MyPrototypes from an "over" to a "def", there will be
/// prim's of type sphere in the scene index corresponding to the one USD prim:
/// one instanced through /MyInstancer and one not instanced.
///
/// [3] The re-rooted copy of /MyPrototypes/MyPrototype inserted
/// by the instancer observer (instantiated with instancer = /MyInstancer,
/// prototypeRoot = /MyPrototypes/MyPrototypes and
/// rerootedPrototypeRoot = /MyPrototypes/MyPrototype/ForInstancer4e6...f36
///
/// [4] Similar to [4] in Example 1.
///
/// [5] Similar to [5] in Example 1.
///
/// ---------------------------------------------------------------------------
///
/// Example 3:
///
/// USD:
///
/// def PointInstancer "MyInstancer"
/// {
///     rel prototypes = [
///         </MyInstancer/MyPrototype> ]
///     def Xform "MyPrototype"
///     {
///         def PointInstancer "MyNestedInstancer"
///         {
///             rel prototypes = [
///                 </MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype> ]
///             def Xform "MyNestedPrototype"
///             {
///                 def Sphere "MySphere"
///                 {
///                 }
///             }
///         }
///     }
/// }
///
/// Note that "MySphere" is instanced by two nested point instancers.
/// This will be realized by the PointPropagatingSceneIndex as follows:
/// PointPropagatingSceneIndex as follows:
///
/// /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f/MySphere
/// is instanced by
/// /MyInstancer/MyPrototype/ForInstancer6a3...234/MyNestedInstancer
/// is instanced by
/// /MyInstancer
///
/// PointPropagatingSceneIndex:
///
/// /MyInstancer
///     primType: instancer
///     dataSource:
///         setting # [1]
///             prototypes = [/MyInstancer/MyPrototype/ForInstancer6a3...234]
///
/// /MyInstancer/MyPrototype
/// /MyInstancer/MyPrototype/MyNestedInstancer
/// /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype
/// /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/MySphere
///     primType: "" # [2]
///     dataSource: unchanged
///
/// /MyInstancer/MyPrototype/ForInstancer6a3...234 # [3]
///     primType: unchanged (from /MyInstancer/MyPrototype)
///     dataSource: (from /MyInstancer/MyPrototype)
///         settings # [4]
///             xform:resetXformStack = true
///             instancedBy:PrototypeRoot = /MyInstancer/MyPrototype/ForInstancer6a3...234
///             instancedBy:paths = /MyInstancer
///
/// /MyInstancer/MyPrototype/ForInstancer6a3...234/MyNestedInstancer
///     primType: instancer
///     dataSource:
///         setting # [5]
///             prototypes = [/MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f]
///             instancedBy:PrototypeRoot = /MyInstancer/MyPrototype/ForInstancer6a3...234
///             instancedBy:paths = /MyInstancer
///
/// /MyInstancer/MyPrototype/ForInstancer6a3...234/MyNestedInstancer/MyNestedPrototype
/// /MyInstancer/MyPrototype/ForInstancer6a3...234/MyNestedInstancer/MyNestedPrototype/MySphere
///     primType: "" # [6]
///     dataSource: unchanged
///
/// /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f #[7]
///     primType: unchanged (from /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype)
///     dataSource: (from /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype)
///         settings # [8]
///             xform:resetXformStack = true
///             instancedBy:PrototypeRoot = /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f
///             instancedBy:paths = /MyInstancer/MyPrototype/ForInstancer6a3...234/MyNestedInstancer
///
/// /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f/MySphere
///     primType: sphere
///     dataSource:
///         settings # [9]
///             instancedBy:PrototypeRoot = /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f
///             instancedBy:paths = /MyInstancer/MyPrototype/ForInstancer6a3...234/MyNestedInstancer
///
/// [1] As [1] in Example 1.
///
/// [2] As [2] in Example 1.
///
/// [3] The re-rooted copy of /MyInstancer/MyPrototype inserted
/// by the instancer observer (instantiated with instancer = /MyInstancer,
/// prototypeRoot = /MyInstancer/MyPrototypes and
/// rerootedPrototypeRoot = /MyInstancer/MyPrototype/ForInstancer6a3...234).
///
/// [4] Similar to [4] in Example 1.
///
/// [5] Similar to [5] in Example 1.
///
/// [6] The prototype scene index forced the empty prim types on
/// all descendants of an instancer within the prototype root.
///
/// [7] The re-rooted copy of
/// /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype inserted
/// by the instancer observer (instantiated with
/// instancer = /MyInstancer/MyPrototype/MyNestedInstancer,
/// prototypeRoot = /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype,
/// rerootedPrototypeRoot = /MyInstancer/MyPrototype/MyNestedInstancer/MyNestedPrototype/ForInstancer8a2...51f)
///
/// Note that this copy is inserted by the instancer observer for
/// /MyInstancer/MyPrototype.
///
/// The instancer path is the path in the USD scene and will be changed by
/// a latter re-rooting scene index in the instancer observer.
///
/// Note the hash at the end of the rerootedPrototypeRoot was computed from the
/// calling the instancer observer by combining its rerootedPrototypeRoot with
/// the path of the instancer within its prototypeRoot.
///
/// [8] Similar to [4].
///
/// [9] Similar to [5].
///
class UsdImagingPiPrototypePropagatingSceneIndex final
                                : public HdFilteringSceneIndexBase
                                , public HdEncapsulatingSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImagingPiPrototypePropagatingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    USDIMAGING_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override;

    USDIMAGING_API
    std::vector<HdSceneIndexBaseRefPtr> GetEncapsulatedScenes() const override;

private:
    UsdImagingPiPrototypePropagatingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    friend class _MergingSceneIndexObserver;
    class _MergingSceneIndexObserver : public HdSceneIndexObserver
    {
    public:
        _MergingSceneIndexObserver(
            UsdImagingPiPrototypePropagatingSceneIndex * owner);

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
        UsdImagingPiPrototypePropagatingSceneIndex * const _owner;
    };

    UsdImagingPiPrototypePropagatingSceneIndex_Impl::
    _ContextSharedPtr const _context;

    _MergingSceneIndexObserver _mergingSceneIndexObserver;

    UsdImagingPiPrototypePropagatingSceneIndex_Impl::
    _InstancerObserverUniquePtr const _instancerObserver;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

