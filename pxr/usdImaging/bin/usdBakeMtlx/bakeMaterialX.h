//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_BIN_USD_BAKE_MTLX_BAKE_MATERIALX_H
#define PXR_USD_IMAGING_BIN_USD_BAKE_MTLX_BAKE_MATERIALX_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/bin/usdBakeMtlx/api.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/usd/usdShade/material.h"

#include <MaterialXCore/Library.h>

#include "pxr/base/tf/declarePtrs.h"
#include <string>

MATERIALX_NAMESPACE_BEGIN
    using FilePathVec = std::vector<class FilePath>;
    class FileSearchPath;
MATERIALX_NAMESPACE_END

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(UsdStage);

/// Read the MaterialX XML file at \p pathname convert and add to
/// the given USD \p stage.
USDBAKEMTLX_API
UsdStageRefPtr UsdBakeMtlxReadDocToStage(std::string const &pathname,
                                         UsdStageRefPtr stage);

/// Convert the given MaterialX Material from a UsdShadeaMaterial into a 
/// MaterialX Document and Bake it using MaterialX::TextureBaker, storing
/// the resulting mtlx Document at \p bakedMtlxFilename. Any resulting 
/// textures from the baking process will live in the same directory. 
USDBAKEMTLX_API
std::string UsdBakeMtlxBakeMaterial(
    UsdShadeMaterial const& mtlxMaterial,
    std::string const& bakedMtlxDir,
    int textureWidth,
    int textureHeight,
    bool bakeHdr,
    bool bakeAverage);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_IMAGING_BIN_USD_BAKE_MTLX_BAKE_MATERIALX_H
