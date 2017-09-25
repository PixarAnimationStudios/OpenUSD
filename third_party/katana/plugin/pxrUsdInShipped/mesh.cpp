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
#include "pxrUsdInShipped/declareCoreOps.h"

#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readMesh.h"
#include "usdKatana/utils.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdGeom/faceSetAPI.h"
#include "pxr/usd/usdGeom/mesh.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(USD_KATANA_IMPORT_FACESET_API, true, 
                      "Whether face-sets encoded using the deprecated "
                      "UsdGeomFaceSetAPI schema must be imported by PxrUsdIn.")

static void 
_CreateFaceSetsFromFaceSetAPI(
        const UsdPrim& prim,
        const PxrUsdKatanaUsdInPrivateData &data,
        FnKat::GeolibCookInterface& interface);

PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_MeshOp, privateData, interface)
{
    PxrUsdKatanaAttrMap attrs;

    const UsdPrim& prim = privateData.GetUsdPrim();

    PxrUsdKatanaReadMesh(
        UsdGeomMesh(prim), privateData, attrs);

    attrs.toInterface(interface);

    if (TfGetEnvSetting(USD_KATANA_IMPORT_FACESET_API) and 
        UsdShadeMaterial::HasMaterialFaceSet(prim)) 
    {
        _CreateFaceSetsFromFaceSetAPI(prim, privateData, interface);
    }
}

// For now, this is only used by the mesh op.  If this logic needs to be
// accessed elsewhere, it should move down into usdKatana.
static void 
_CreateFaceSetsFromFaceSetAPI(
        const UsdPrim& prim,
        const PxrUsdKatanaUsdInPrivateData &data,
        FnKat::GeolibCookInterface& interface)
{
    UsdGeomFaceSetAPI faceSet = UsdShadeMaterial::GetMaterialFaceSet(prim);
    bool isPartition = faceSet.GetIsPartition();;
    if (!isPartition) {
        TF_WARN("Found face set on prim <%s> that is not a partition.", 
                prim.GetPath().GetText());
        // continue here?
    }

    const double currentTime = data.GetCurrentTime();

    VtIntArray faceCounts, faceIndices;
    faceSet.GetFaceCounts(&faceCounts, currentTime);
    faceSet.GetFaceIndices(&faceIndices, currentTime);

    SdfPathVector bindingTargets;
    faceSet.GetBindingTargets(&bindingTargets);

    size_t faceSetIdxStart = 0;
    for(size_t faceSetIdx = 0; faceSetIdx < faceCounts.size(); ++faceSetIdx) {
        size_t faceCount = faceCounts[faceSetIdx];

        FnKat::GroupBuilder faceSetAttrs;

        faceSetAttrs.set("type", FnKat::StringAttribute("faceset"));
        faceSetAttrs.set("materialAssign", FnKat::StringAttribute(
            PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
                bindingTargets[faceSetIdx], data)));

        FnKat::IntBuilder facesBuilder;
        {
            std::vector<int> faceIndicesVec(faceCount);
            for (size_t faceIndicesIdx = 0; faceIndicesIdx < faceCount; ++faceIndicesIdx)
            {
                faceIndicesVec[faceIndicesIdx] = 
                    faceIndices[faceSetIdxStart + faceIndicesIdx];
            }
            faceSetIdxStart += faceCount;
            facesBuilder.set(faceIndicesVec);
        }
        faceSetAttrs.set("geometry.faces", facesBuilder.build());

        std::string faceSetName = TfStringPrintf("faceset_%zu", faceSetIdx);

        FnKat::GroupBuilder staticSceneCreateAttrs;
        staticSceneCreateAttrs.set("a", faceSetAttrs.build());
        interface.createChild(
            faceSetName,
            "StaticSceneCreate",
            staticSceneCreateAttrs.build());
    }
}
