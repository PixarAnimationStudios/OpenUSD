
//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_EXT_COMPUTATION_DEPENDENCY_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_EXT_COMPUTATION_DEPENDENCY_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiExtComputationDependencySceneIndex);

/// \class HdsiExtComputationDependencySceneIndex
///
/// Adds dependencies to dependencies schema for ext computations.
///
/// More precisely, it adds a dependency of the value of an output of a
/// computation on any input value or the value of any output of another
/// computation serving as computation input.
/// E.g., it adds a dependency of the locator
/// extComputation/outputs/FOO/value on extComputation/inputValues
/// (on the same ext computation prim) or extComputation/outputs/BAR/value
/// (on a different ext computation prim).
///
/// For an ext computation primvar (on a non-ext computation prim), it
/// adds a dependency on the corresponding primvar value on the input of the
/// respective computation output.
/// E.g., it adds a dependency of the locator
/// primvars/FOO/primvarValue on extComputation/outputs/FOO/value (on the
/// ext computation prim identified by the path data source at
/// extComputationPrimvars/FOO/sourceComputation).
///
/// Also adds dependencies for these dependencies.
///
class HdsiExtComputationDependencySceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiExtComputationDependencySceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    HDSI_API
    HdsiExtComputationDependencySceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_H
