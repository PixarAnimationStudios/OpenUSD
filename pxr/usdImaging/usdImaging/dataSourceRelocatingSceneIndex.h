//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATASOURCE_RELOCATING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_DATASOURCE_RELOCATING_SCENE_INDEX_H

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImaging_DataSourceRelocatingSceneIndex);

class UsdImaging_DataSourceRelocatingSceneIndex final: 
    public HdSingleInputFilteringSceneIndexBase
{
public:
    static UsdImaging_DataSourceRelocatingSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdDataSourceLocator& srcLocator, 
        const HdDataSourceLocator& dstLocator,
        const bool forNativeInstance) {
        return TfCreateRefPtr(
            new UsdImaging_DataSourceRelocatingSceneIndex(
                inputSceneIndex, srcLocator, dstLocator, forNativeInstance));
    }

protected:
    UsdImaging_DataSourceRelocatingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdDataSourceLocator& srcLocator, 
        const HdDataSourceLocator& dstLocator,
        const bool forNativeInstance): 
        HdSingleInputFilteringSceneIndexBase(inputSceneIndex),
        _srcLocator(srcLocator), _dstLocator(dstLocator),
        _dirtyDstLocators(
            HdContainerDataSourceEditor::ComputeDirtyLocators({_dstLocator})),
        _forNativeInstance(forNativeInstance) {}

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

    void _PrimsAdded(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override {
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    const HdDataSourceLocator _srcLocator;
    const HdDataSourceLocator _dstLocator;
    const HdDataSourceLocatorSet _dirtyDstLocators;
    const bool _forNativeInstance;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
