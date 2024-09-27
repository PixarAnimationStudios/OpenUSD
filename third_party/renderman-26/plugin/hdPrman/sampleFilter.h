//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_SAMPLE_FILTER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_SAMPLE_FILTER_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/material.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;

class HdPrman_SampleFilter : public HdSprim
{
public:
    HdPrman_SampleFilter(SdfPath const& id);

    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;

    void Finalize(HdRenderParam *renderParam) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    void _CreateRmanSampleFilter(HdSceneDelegate *sceneDelegate,
                                 HdPrman_RenderParam *renderParam,
                                 SdfPath const& filterPrimPath,
                                 HdMaterialNode2 const& sampleFilterNode);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_SAMPLE_FILTER_H
