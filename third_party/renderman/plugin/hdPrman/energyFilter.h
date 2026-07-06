//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ENERGY_FILTER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ENERGY_FILTER_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/material.h"

#include "Riley.h"

#if _PRMANAPI_VERSION_MAJOR_ >= 27

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;

class HdPrman_EnergyFilter : public HdSprim
{
public:
    HdPrman_EnergyFilter(SdfPath const& id);

    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;

    void Finalize(HdRenderParam *renderParam) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    void _CreateRmanEnergyFilter(HdSceneDelegate *sceneDelegate,
                                 HdPrman_RenderParam *renderParam,
                                 SdfPath const& filterPrimPath,
                                 HdMaterialNode2 const& energyFilterNode);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // _PRMANAPI_VERSION_MAJOR_ >= 27
#endif // PXR_VERSION >= 2308

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ENERGY_FILTER_H
