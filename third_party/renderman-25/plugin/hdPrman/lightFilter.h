//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LIGHT_FILTER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LIGHT_FILTER_H

#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

#include <Riley.h>
#include <RileyIds.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

// For now, the procs in this file are boiler plate for when hdPrman needs to
// have light filters become prime citizens.  This will probably happen when
// its time to implement shared light filters.  For now, light filters are
// handled inside the lights in light.cpp.
//
// Also, for now base the HdPrmanLightFilter class on HdSprim as there
// currently is no HdLightFilter class.

class HdSceneDelegate;
class HdPrman_RenderParam;

/// \class HdPrmanLightFilter
///
/// A representation for light filters.
///
class HdPrmanLightFilter final : public HdSprim 
{
public:
    HdPrmanLightFilter(SdfPath const& id, TfToken const& lightFilterType);
    ~HdPrmanLightFilter() override;

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Make sure this material has been updated in Riley.
    void SyncToRiley(
        HdSceneDelegate *sceneDelegate,
        HdPrman_RenderParam *param,
        riley::Riley *riley);

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Finalize(HdRenderParam *renderParam) override;

    riley::CoordinateSystemId GetCoordSysId();

private:
    void _SyncToRileyWithLock(
        HdSceneDelegate *sceneDelegate,
        HdPrman_RenderParam *param,
        riley::Riley *riley);

    riley::CoordinateSystemId _coordSysId;
    mutable std::mutex _syncToRileyMutex;
    bool _rileyIsInSync;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LIGHT_FILTER_H
