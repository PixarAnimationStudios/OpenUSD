//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readGeomSubset.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdShade/material.h"

#include <FnLogging/FnLogging.h>

PXR_NAMESPACE_OPEN_SCOPE

FnLogSetup("PxrUsdKatanaReadGeomSubset");

void
PxrUsdKatanaReadGeomSubset(
        const UsdGeomSubset& geomSubset,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    PxrUsdKatanaReadPrim(geomSubset.GetPrim(), data, attrs);

    // We only import facesets.
    TfToken elementType;
    if (geomSubset.GetElementTypeAttr().Get(&elementType) && 
        elementType != UsdGeomTokens->face) {
        return;
    }
    
    attrs.set("type", FnKat::StringAttribute("faceset"));

    TfToken familyName;
    geomSubset.GetFamilyNameAttr().Get(&familyName);
    attrs.set("info.usd.GeomSubset.familyName", 
              FnKat::StringAttribute(familyName.GetString()));

    FnKat::IntBuilder facesBuilder;
    VtIntArray indices;
    geomSubset.GetIndicesAttr().Get(&indices, data.GetCurrentTime());
    std::vector<int> indicesVec;
    // Copy values to a vector, which IntBuilder expects.
    indicesVec.assign(indices.begin(), indices.end());
    facesBuilder.set(indicesVec);
    attrs.set("geometry.faces", facesBuilder.build());
}

PXR_NAMESPACE_CLOSE_SCOPE

