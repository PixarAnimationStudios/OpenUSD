//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DISPLAY_FILTER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DISPLAY_FILTER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/material.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;

class HdPrman_DisplayFilter : public HdSprim
{
public:
    HdPrman_DisplayFilter(SdfPath const& id);

    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;

    void Finalize(HdRenderParam *renderParam) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    void _CreateRmanDisplayFilter(HdSceneDelegate *sceneDelegate,
                                  HdPrman_RenderParam *renderParam,
                                  SdfPath const& filterPrimPath,
                                  HdMaterialNode2 const& displayFilterNode);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DISPLAY_FILTER_H
