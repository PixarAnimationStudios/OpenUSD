//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_PRIM_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_PRIM_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyPrimBase.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanRileyGlobalsSchema;
using HdPrman_RileyGlobalsPrimHandle =
    std::shared_ptr<class HdPrman_RileyGlobalsPrim>;

/// \class HdPrman_RileyGlobalsPrim
///
/// Set riley global parameters such as options using the
/// HdPrmanRileyGlobalsSchema.
///
class HdPrman_RileyGlobalsPrim : public HdPrman_RileyPrimBase
{
public:
    HdPrman_RileyGlobalsPrim(
        HdContainerDataSourceHandle const &primSource,
        const HdsiPrimManagingSceneIndexObserver *observer,
        HdPrman_RenderParam * renderParam);

    ~HdPrman_RileyGlobalsPrim() override;

protected:
    void _Dirty(
        const HdSceneIndexObserver::DirtiedPrimEntry &entry,
        const HdsiPrimManagingSceneIndexObserver * observer) override;

private:
    void _SetOptions(HdPrmanRileyGlobalsSchema);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif
