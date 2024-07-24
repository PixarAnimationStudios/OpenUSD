//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiMaterialPrimvarTransferSceneIndex);

/// Transfers primvars present on the locally bound material. Any matching
/// primvar already present will have a stronger opinion.
/// 
/// As it's expected that primvars inherited by the destination location
/// should have a stronger opinion than those transfered here, inherited
/// primvars must be flattened in advance of this scene index.
/// 
/// This is in support of shading workflows.
///
/// This also declares dependencies to ensure invalidation if a
/// HdDependencyForwardingSceneIndex is present downstream. (Because those
/// dependencies are computed on demand, no meaningful additional work is done
/// otherwise.)
/// 
class HdsiMaterialPrimvarTransferSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiMaterialPrimvarTransferSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

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

private:
    HdsiMaterialPrimvarTransferSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~HdsiMaterialPrimvarTransferSceneIndex() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
