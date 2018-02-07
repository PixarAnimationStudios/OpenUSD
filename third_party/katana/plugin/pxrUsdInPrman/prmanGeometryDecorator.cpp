//
// Copyright 2017 Pixar
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

#include "pxrUsdInPrman/declarePackageOps.h"

#include "pxr/usd/usdGeom/mesh.h"

PXR_NAMESPACE_USING_DIRECTIVE



PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInPrman_LocationDecorator,
        privateData, opArgs, interface)
{
    std::string locationType =
            FnKat::StringAttribute(interface.getOutputAttr("type")
                    ).getValue("", false);
    
    if (locationType == "subdmesh")
    {
        UsdGeomMesh mesh(privateData.GetUsdPrim());
        if (mesh)
        {
            TfToken scheme;
            if (mesh.GetSubdivisionSchemeAttr().Get(&scheme) && 
                    scheme != UsdGeomTokens->none)
            {
                // USD deviates from Katana only in the 'catmullClark' token.
                static char const *catclark("catmull-clark");
                char const *katScheme = 
                    (scheme == UsdGeomTokens->catmullClark
                            ? catclark : scheme.GetText());
                
                interface.setAttr("prmanStatements.subdivisionMesh.scheme",
                         FnKat::StringAttribute(katScheme));
            }
        }
    }
}

