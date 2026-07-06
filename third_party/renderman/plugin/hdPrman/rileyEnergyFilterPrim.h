//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_ENERGY_FILTER_PRIM_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_ENERGY_FILTER_PRIM_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyPrimBase.h"

#if _PRMANAPI_VERSION_MAJOR_ >= 27

PXR_NAMESPACE_OPEN_SCOPE

using HdPrman_RileyEnergyFilterPrimHandle =
    std::shared_ptr<class HdPrman_RileyEnergyFilterPrim>;

/// \class HdPrman_RileyEnergyFilterPrim
///
/// Wraps a riley EnergyFilter object, initializing or updating it
/// using the HdPrmanRileyEnergyFilterSchema.
///
/// Unlike sample/display filters, energy filters are standalone Riley
/// objects (not part of the render view).
///
class HdPrman_RileyEnergyFilterPrim : public HdPrman_RileyPrimBase
{
public:
    HdPrman_RileyEnergyFilterPrim(
        HdContainerDataSourceHandle const &primSource,
        const HdsiPrimManagingSceneIndexObserver *observer,
        HdPrman_RenderParam * renderParam);

    ~HdPrman_RileyEnergyFilterPrim() override;

    using RileyId = riley::EnergyFilterId;

    const RileyId &GetRileyId() const { return _rileyId; }

protected:
    void _Dirty(
        const HdSceneIndexObserver::DirtiedPrimEntry &entry,
        const HdsiPrimManagingSceneIndexObserver * observer) override;

private:
    RileyId _rileyId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // _PRMANAPI_VERSION_MAJOR_ >= 27
#endif // HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif
