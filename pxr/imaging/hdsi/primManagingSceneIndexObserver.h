//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_PRIM_MANAGING_SCENE_INDEX_OBSERVER_H
#define PXR_IMAGING_HDSI_PRIM_MANAGING_SCENE_INDEX_OBSERVER_H

/// \file hdsi/primManagingSceneIndexObserver.h

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PRIM_MANAGING_SCENE_INDEX_OBSERVER_TOKENS \
    (primFactory)

TF_DECLARE_PUBLIC_TOKENS(HdsiPrimManagingSceneIndexObserverTokens, HDSI_API,
                         HDSI_PRIM_MANAGING_SCENE_INDEX_OBSERVER_TOKENS);

TF_DECLARE_REF_PTRS(HdsiPrimManagingSceneIndexObserver);

/// \class HdsiPrimManagingSceneIndexObserver
///
/// A scene index observer that turns prims in the observed scene index
/// into instances (of RAII subclasses) of PrimBase using the given prim
/// factory.
///
/// This observer is an analogue to the HdPrimTypeIndex in the old hydra
/// API (though we do not have separate observers for b/s/r-prims and
/// instead rely on the observed filtering scene index (e.g.,
/// the HdsiPrimTypeNoticeBatchingSceneIndex) to batch notices
/// in a way respecting dependencies).
///
/// More precisely, a AddedPrimEntry results in a call to the prim factory
/// (this also applies to prims that exist in the observed scene index at the
/// time the observer was instantiated).
///
/// The observer manages a map from paths to PrimBase handles so that
/// subsequent a DirtiedPrimEntry or RemovedPrimEntry results in a call to
/// PrimBase::Dirty or releases the handles to the PrimBase's at paths
/// prefixed by the RemovedPrimEntry's path.
///
class HdsiPrimManagingSceneIndexObserver
    : public HdSceneIndexObserver, public TfRefBase
{
public:
    /// \class HdsiPrimManagingSceneIndexObserver::PrimBase
    ///
    /// Base class for prims managed by the observer.
    ///
    class PrimBase
    {
    public:
        HDSI_API
        virtual ~PrimBase();
        
        void Dirty(
            const DirtiedPrimEntry &entry,
            const HdsiPrimManagingSceneIndexObserver * observer)
        {
            // NVI so that we can later implement things like a
            // mutex guarding against Dirty from several threads
            // on the same prim.
            _Dirty(entry, observer);
        }

    private:
        virtual void _Dirty(
            const DirtiedPrimEntry &entry,
            const HdsiPrimManagingSceneIndexObserver * observer) = 0;
    };
    using PrimBaseHandle = std::shared_ptr<PrimBase>;

    /// \class HdsiPrimManagingSceneIndexObserver::PrimFactoryBase
    ///
    /// Base class for a prim factory given to the observer.
    ///
    class PrimFactoryBase 
    {
    public:
        HDSI_API
        virtual ~PrimFactoryBase();
        virtual PrimBaseHandle CreatePrim(
            const AddedPrimEntry &entry,
            const HdsiPrimManagingSceneIndexObserver * observer) = 0;
    };
    using PrimFactoryBaseHandle = std::shared_ptr<PrimFactoryBase>;

    /// C'tor. Prim factory can be given through inputArgs as
    /// PrimFactoryBaseHandle typed data source under
    /// HdsiPrimManagingSceneIndexObserverTokens->primFactory key.
    ///
    static HdsiPrimManagingSceneIndexObserverRefPtr New(
            HdSceneIndexBaseRefPtr const &sceneIndex,
            HdContainerDataSourceHandle const &inputArgs) {
        return TfCreateRefPtr(
            new HdsiPrimManagingSceneIndexObserver(
                sceneIndex, inputArgs));
    }

    HDSI_API
    ~HdsiPrimManagingSceneIndexObserver() override;

    /// Get observed scene index.
    ///
    const HdSceneIndexBaseRefPtr &GetSceneIndex() const {
        return _sceneIndex;
    }

    /// Get managed prim at path.
    ///
    /// Clients can prolong life-time of prim by holding on to the
    /// resulting handle.
    ///
    HDSI_API
    const PrimBaseHandle &GetPrim(const SdfPath &primPath) const;

    /// Get managed prim cast to a particular type.
    template<typename PrimType>
    std::shared_ptr<PrimType> GetTypedPrim(const SdfPath &primPath) const
    {
        return std::dynamic_pointer_cast<PrimType>(GetPrim(primPath));
    }

protected:
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
    HDSI_API
    HdsiPrimManagingSceneIndexObserver(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

    HdSceneIndexBaseRefPtr const _sceneIndex;
    PrimFactoryBaseHandle const _primFactory;

    // _prims defined after _primFactory so that all prims
    // are destroyed before the handle to _primFactory is
    // released.
    std::map<SdfPath, PrimBaseHandle> _prims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
