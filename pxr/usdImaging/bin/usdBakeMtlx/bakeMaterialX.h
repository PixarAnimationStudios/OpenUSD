//
// Copyright 2022 Pixar
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

#ifndef PXR_USD_IMAGING_BIN_USD_BAKE_MTLX_BAKE_MATERIALX_H
#define PXR_USD_IMAGING_BIN_USD_BAKE_MTLX_BAKE_MATERIALX_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/bin/usdBakeMtlx/api.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/usdShade/material.h"

#include "pxr/base/tf/declarePtrs.h"
#include <string>

namespace MaterialX {
    using FilePathVec = std::vector<class FilePath>;
    class FileSearchPath;
}

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
