//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_LEGACY_PRIM_SCENE_INDEX_H
#define PXR_IMAGING_HD_LEGACY_PRIM_SCENE_INDEX_H

#include "pxr/imaging/hd/retainedSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

TF_DECLARE_REF_PTRS(HdLegacyPrimSceneIndex);

/// \class HdLegacyPrimSceneIndex
///
/// Extends HdRetainedSceneIndex to instantiate and dirty HdDataSourceLegacyPrim
/// data sources.
/// 
/// During emulation of legacy HdSceneDelegates, HdRenderIndex forwards
/// prim insertion calls here to produce a comparable HdSceneIndex
/// representation 
class HdLegacyPrimSceneIndex : public HdRetainedSceneIndex
{
public:

    static HdLegacyPrimSceneIndexRefPtr New() {
        return TfCreateRefPtr(new HdLegacyPrimSceneIndex);
    }

    /// custom insertion wrapper called by HdRenderIndex during population
    /// of legacy HdSceneDelegates
    void AddLegacyPrim(SdfPath const &id, TfToken const &type,
        HdSceneDelegate *sceneDelegate);

    /// Remove only the prim at \p id without affecting children.
    ///
    /// If \p id has children, it is replaced by an entry with no type
    /// and no data source.  If \p id does not have children, it is
    /// removed from the retained scene index.
    ///
    /// This is called by HdRenderIndex on behalf of legacy
    /// HdSceneDelegates to emulate the original behavior of
    /// Remove{B,R,S}Prim, which did not remove children.
    ///
    void RemovePrim(SdfPath const &id);

    /// extends to also call DirtyPrim on HdDataSourceLegacyPrim
    void DirtyPrims(
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
