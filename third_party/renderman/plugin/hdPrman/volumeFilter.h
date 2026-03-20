//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_FILTER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_FILTER_H

#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"

#include <Riley.h>
#include <RileyIds.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdPrman_RenderParam;

/// \class HdPrman_VolumeFilter
///
/// A representation for volume filters.
///
class HdPrman_VolumeFilter final : public HdSprim 
{
public:
    HdPrman_VolumeFilter(SdfPath const& id);
    ~HdPrman_VolumeFilter() override;

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    /// Make sure this object has been updated in Riley.
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

#endif  // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_FILTER_H
