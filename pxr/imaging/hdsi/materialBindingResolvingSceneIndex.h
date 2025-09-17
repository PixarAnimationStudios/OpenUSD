//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_MATERIAL_BINDING_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_MATERIAL_BINDING_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiMaterialBindingResolvingSceneIndex);

/// Scene Index that resolves materialBindings that have multiple purposes into
/// a single purpose.  The first binding encountered in \p purposePriorityOrder
/// will be provided as \p dstPurpose.
class HdsiMaterialBindingResolvingSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiMaterialBindingResolvingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const TfTokenVector& purposePriorityOrder,
        const TfToken& dstPurpose);

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected: // HdSingleInputFilteringSceneIndexBase overrides
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

protected:
    HdsiMaterialBindingResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const TfTokenVector& purposePriorityOrder,
        const TfToken& dstPurpose);
    ~HdsiMaterialBindingResolvingSceneIndex() override;

private:
    TfTokenVector _purposePriorityOrder;
    TfToken _dstPurpose;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
