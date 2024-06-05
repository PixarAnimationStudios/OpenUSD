//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_COORD_SYS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_COORD_SYS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/version.h"
#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdPrman_RenderParam;

/// \class HdPrmanCoordSys
///
/// A representation for coordinate systems.
///
class HdPrmanCoordSys final : public HdCoordSys 
{
public:
    HdPrmanCoordSys(SdfPath const& id);
    ~HdPrmanCoordSys() override;

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    
#if HD_API_VERSION < 53
    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HdDirtyBits GetInitialDirtyBitsMask() const override;
#endif

    riley::CoordinateSystemId GetCoordSysId() const { return _coordSysId; }

    /// Return true if this material is valid.
    bool IsValid() const;

    void Finalize(HdRenderParam *renderParam) override;

private:
    void _ResetCoordSys(HdPrman_RenderParam *renderParam);

    riley::CoordinateSystemId _coordSysId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_COORD_SYS_H
