//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_BASE_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_BASE_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "pxr/imaging/hdsi/primManagingSceneIndexObserver.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;

/// \class HdPrman_RileyPrimBase
///
/// A base class for prims wrapping Riley objects. It provides access
/// to Riley so that subclasses can call Riley::Create/Modify/DeleteFoo.
///
class HdPrman_RileyPrimBase
   : public HdsiPrimManagingSceneIndexObserver::PrimBase
{
protected:
    // HdPrman_RenderParam needed to get Riley.
    HdPrman_RileyPrimBase(HdPrman_RenderParam * renderParam);

    // Does necessary things (such as stopping the render) so that
    // calls to, e.g., Riley::Create are safe.
    riley::Riley * _AcquireRiley();

    const GfVec2f &_GetShutterInterval();

private:
    HdPrman_RenderParam * const _renderParam;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif

