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

#include <FnGeolib/op/FnGeolibOp.h>

#include "pxr/pxr.h"
#include "usdKatana/usdInPluginRegistry.h"

#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usdGeom/basisCurves.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/nurbsPatch.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdShade/look.h"

#include "pxrUsdInShipped/attrfnc_materialReference.h"

PXR_NAMESPACE_USING_DIRECTIVE


void registerPxrUsdInShippedUiUtils();

DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_XformOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_ScopeOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_MeshOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_NurbsPatchOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_PointsOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_BasisCurvesOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_LookOp)

DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_ModelOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_CameraOp)

DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_ConstraintsOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInCore_LooksGroupOp)

DEFINE_ATTRIBUTEFUNCTION_PLUGIN(MaterialReferenceAttrFnc);
DEFINE_ATTRIBUTEFUNCTION_PLUGIN(LibraryMaterialNamesAttrFnc);

void registerPlugins()
{
    REGISTER_PLUGIN(PxrUsdInCore_XformOp, "PxrUsdInCore_XformOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_ScopeOp, "PxrUsdInCore_ScopeOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_MeshOp, "PxrUsdInCore_MeshOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_NurbsPatchOp, "PxrUsdInCore_NurbsPatchOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_PointsOp, "PxrUsdInCore_PointsOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_BasisCurvesOp, "PxrUsdInCore_BasisCurvesOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_LookOp, "PxrUsdInCore_LookOp", 0, 1);

    REGISTER_PLUGIN(PxrUsdInCore_ModelOp, "PxrUsdInCore_ModelOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_CameraOp, "PxrUsdInCore_CameraOp", 0, 1);

    REGISTER_PLUGIN(PxrUsdInCore_ConstraintsOp, "UsdInCore_ConstraintsOp", 0, 1);
    REGISTER_PLUGIN(PxrUsdInCore_LooksGroupOp, "UsdInCore_LooksGroupOp", 0, 1);

    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomXform>("PxrUsdInCore_XformOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomScope>("PxrUsdInCore_ScopeOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomMesh>("PxrUsdInCore_MeshOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomNurbsPatch>("PxrUsdInCore_NurbsPatchOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomPoints>("PxrUsdInCore_PointsOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomBasisCurves>("PxrUsdInCore_BasisCurvesOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdShadeLook>("PxrUsdInCore_LookOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterUsdType<UsdGeomCamera>("PxrUsdInCore_CameraOp");

    // register a default op to handle prims with unknown types
    PxrUsdKatanaUsdInPluginRegistry::RegisterUnknownUsdType("PxrUsdInCore_ScopeOp");

    PxrUsdKatanaUsdInPluginRegistry::RegisterKind(KindTokens->model, "PxrUsdInCore_ModelOp");
    PxrUsdKatanaUsdInPluginRegistry::RegisterKind(KindTokens->subcomponent, "PxrUsdInCore_ModelOp");
    
    registerPxrUsdInShippedUiUtils();

    REGISTER_PLUGIN(MaterialReferenceAttrFnc, "PxrUsdInMaterialReference", 0, 1);
    REGISTER_PLUGIN(LibraryMaterialNamesAttrFnc, "PxrUsdInLibraryMaterialNames", 0, 1);
}
