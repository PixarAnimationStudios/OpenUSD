//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_FACTORY_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_FACTORY_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "pxr/imaging/hdsi/primManagingSceneIndexObserver.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;

using HdPrman_RileyPrimFactoryHandle =
    std::shared_ptr<class HdPrman_RileyPrimFactory>;

/// \class HdPrman_RileyPrimFactory
///
/// Implements HdsiPrimManagingSceneIndexObserver::PrimFactoryBase
/// to translate scene index prims into Riley::Create/Modify/DeleteFoo
/// calls.
///
class HdPrman_RileyPrimFactory
   : public HdsiPrimManagingSceneIndexObserver::PrimFactoryBase
{
public:
    /// HdPrman_RenderParam's needed to access Riley.
    HdPrman_RileyPrimFactory(
        HdPrman_RenderParam * renderParam);

    HdsiPrimManagingSceneIndexObserver::PrimBaseHandle
    CreatePrim(
        const HdSceneIndexObserver::AddedPrimEntry &entry,
        const HdsiPrimManagingSceneIndexObserver * observer) override;

    static const HdContainerDataSourceHandle &
    GetPrimTypeNoticeBatchingSceneIndexInputArgs();

private:
    HdPrman_RenderParam * const _renderParam;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif

