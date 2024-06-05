//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_POINTS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_POINTS_H

#include "pxr/pxr.h"
#include "hdPrman/gprim.h"
#include "pxr/imaging/hd/points.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_Points final : public HdPrman_Gprim<HdPoints>
{
public:
    using BASE = HdPrman_Gprim<HdPoints>;

    HF_MALLOC_TAG_NEW("new HdPrman_Points");

    HdPrman_Points(SdfPath const& id);

    HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    RtPrimVarList
    _ConvertGeometry(HdPrman_RenderParam *renderParam,
                     HdSceneDelegate *sceneDelegate,
                     const SdfPath &id,
                     RtUString *primType,
                     std::vector<HdGeomSubset> *geomSubsets) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_POINTS_H
