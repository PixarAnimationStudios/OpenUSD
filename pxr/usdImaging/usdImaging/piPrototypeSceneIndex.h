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
#ifndef PXR_USD_IMAGING_USD_IMAGING_PI_PROTOTYPE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_PI_PROTOTYPE_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImaging_PiPrototypeSceneIndex);

/// \class UsdImaging_PiPrototypeSceneIndex
///
/// A scene index that prepares all prims under a given prototype root
/// to be instanced by the given point instancer. It is supposed to be
/// preceeded by a UsdImagingRerootingSceneIndex and is used by the
/// prototype propagating scene index.
///
/// It forces an empty type on prims that are under an instancer within
/// the prototype (and this scene index could also do this for prim's
/// under a USD "over" within the prototype in the future).
///
/// It also adds an instanced by data source to all prims within the
/// prototype whose type has not been forced to empty. It also adds
/// xform:resetXformStack to the prototype root.
///
/// Examples:
///
/// Assume that UsdImaging_PiPrototypeSceneIndexRefPtr is called with
/// the following arguments:
///      inputSceneIndex = UsdImagingRerootingSceneIndex(
///              UsdImagingStageSceneIndex with the below USD stage
///              prototypeRoot, prototypeRoot)
///      instancer = "/MyInstancer",
///      prototypeRoot = "/Prototypes/Prototype"
///
/// on the following stage:
///
/// over "MyPrototypes"
/// {
///    def Xform "MyPrototype"
///    {
///        def Sphere "MySphere"
///        {
///        }
///        def PointInstancer "MyNestedInstancer"
///        {
///            rel prototypes = [
///                </MyPrototypes/MyPrototype/MyNestedInstancer/MyNestedPrototypes/MyNestedPrototype>,
///            ]
///            def Xform "MyNestedPrototypes"
///            {
///                def Xform "MyNestedPrototype"
///                {
///                }
///            }
///        }
///        over "MyOver"
///        {
///            def "MyOtherPrototype"
///            {
///            }
///        }
///    }
/// }
///
/// This scene index will change the prim types and data sources as follows:
///
/// /MyPrototypes:
///     primType: "" # by re-rooting scene index
///     dataSource: nullptr # by re-rooting scene index
/// /MyPrototypes/MyProtoype:
///     primType: unchanged
///     dataSource:
///         setting
///             xform:resetXformStack = true # make all xforms relative
///                                          # to prototype root
///             instancedBy:prototypeRoot = /MyPrototypes/MyPrototype
///             instancedBy:instancedBy = /MyInstancer
/// /MyPrototypes/MyPrototype/MySphere and
/// /MyPrototypes/MyPrototype/MyInstancer
///     primType: unchanged
///     dataSource:
///         setting
///             instancedBy:prototypeRoot = /MyPrototypes/MyPrototype
///             instancedBy:instancedBy = /MyInstancer
/// /MyPrototypes/MyPrototype/MyNestedPrototypes
///     primType: "" # Prims under a point instancer are not drawn unless
///                  # they are targeted by a point instancer's prototypes
///                  # relationship.
///     dataSource: unchanged # So that inherited values such as the
///                           # material binding are seen by a prims inserted
///                           # under this prim later. E.g. from a copy of
///                           # MyNestedPrototype inserted by the propagating
///                           # scene index for MyNestedInstancer.
/// /MyPrototypes/MyPrototype/MyNestedPrototypes/MyNestedPrototype
///     primType: "" # The prims in MyNestedPrototypes will be drawn by
///                  # inserting another copy.
///     dataSource: unchanged
/// /MyPrototypes/MyOver
///     primType: "" # Over prims are not drawn.
///     dataSource: unchanged
/// /MyPrototypes/MyOver/MyOtherPrototype
///     primType: "" # Descendants of over prims are not drawn unless
///                  # targeted by a point instancer's prototype relationship.
///                  # In that case, a copy of MyOtherPrototype would be
///                  # inserted by the propagating scene index.
///     dataSource: unchanged.
///
class UsdImaging_PiPrototypeSceneIndex final
                                : public HdSingleInputFilteringSceneIndexBase
{
public:
    static UsdImaging_PiPrototypeSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &instancer,
        const SdfPath &prototypeRoot);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    UsdImaging_PiPrototypeSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &instancer,
        const SdfPath &prototypeRoot);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _Populate();

    SdfPath _prototypeRoot;
    HdContainerDataSourceHandle _underlaySource;
    HdContainerDataSourceHandle _prototypeRootOverlaySource;

    // Instancers and overs within the prototype.
    // Note that this does not include instancers or overs nested
    // under an instancer or over.
    using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;
    _PathSet _instancersAndOvers;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
