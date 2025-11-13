//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDER_INDEX_ADAPTER_SCENE_INDEX_H
#define PXR_IMAGING_HD_RENDER_INDEX_ADAPTER_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderDelegate;
class HdRenderIndex;
struct HdRenderDelegateInfo;
TF_DECLARE_REF_PTRS(HdRenderIndexAdapterSceneIndex);

/// \class HdRenderIndexAdapterSceneIndex
///
/// A scene index for "front-end" emulation. That is, the scene index
/// is populated from an HdSceneDelegate through the render index owned
/// by the scene index.
///
class HdRenderIndexAdapterSceneIndex : public HdSceneIndexBase
{
public:
    static HdRenderIndexAdapterSceneIndexRefPtr New(
        const HdRenderDelegateInfo &info)
    {
        return TfCreateRefPtr(new HdRenderIndexAdapterSceneIndex(info));
    }

    HdRenderIndex * GetRenderIndex() const { return _renderIndex.get(); }
    
    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    HD_API
    ~HdRenderIndexAdapterSceneIndex() override;

private:
    HD_API
    HdRenderIndexAdapterSceneIndex(const HdRenderDelegateInfo &info);

    std::unique_ptr<HdRenderDelegate> const _renderDelegate;
    std::unique_ptr<HdRenderIndex> const _renderIndex;

    friend class _Observer;

    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(HdRenderIndexAdapterSceneIndex * const owner)
         : _owner(owner) {}

        HD_API
        void PrimsAdded(
                const HdSceneIndexBase &sender,
                const AddedPrimEntries &entries) override;

        HD_API
        void PrimsRemoved(
                const HdSceneIndexBase &sender,
                const RemovedPrimEntries &entries) override;

        HD_API
        void PrimsDirtied(
                const HdSceneIndexBase &sender,
                const DirtiedPrimEntries &entries) override;

        HD_API
        void PrimsRenamed(
                const HdSceneIndexBase &sender,
                const RenamedPrimEntries &entries) override;
    private:
        HdRenderIndexAdapterSceneIndex *_owner;
    };

    _Observer _observer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
