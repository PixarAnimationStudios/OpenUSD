//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
