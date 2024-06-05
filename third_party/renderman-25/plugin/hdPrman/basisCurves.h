//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_BASIS_CURVES_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_BASIS_CURVES_H

#include "pxr/pxr.h"
#include "hdPrman/gprim.h"
#include "pxr/imaging/hd/basisCurves.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_BasisCurves final : public HdPrman_Gprim<HdBasisCurves>
{
public:
    using BASE = HdPrman_Gprim<HdBasisCurves>;

    HF_MALLOC_TAG_NEW("new HdPrman_BasisCurves");

    HdPrman_BasisCurves(SdfPath const& id);

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

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_BASIS_CURVES_H
