//
// Copyright 2020 Pixar
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

#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_USING_DIRECTIVE

void TestHasConnectableAPI()
{
    // schema type examples which should have connectableAPI behavior
    static const TfType& usdShadeShaderType = TfType::Find<UsdShadeShader>();
    static const TfType& usdShadeMaterialType = TfType::Find<UsdShadeMaterial>();
    static const TfType& usdShadeNodeGraphType = TfType::Find<UsdShadeNodeGraph>();
    TF_AXIOM(UsdShadeConnectableAPI::HasConnectableAPI<UsdShadeShader>());
    TF_AXIOM(UsdShadeConnectableAPI::HasConnectableAPI<UsdShadeMaterial>());
    TF_AXIOM(UsdShadeConnectableAPI::HasConnectableAPI<UsdShadeNodeGraph>());
    TF_AXIOM(UsdShadeConnectableAPI::HasConnectableAPI(usdShadeShaderType));
    TF_AXIOM(UsdShadeConnectableAPI::HasConnectableAPI(usdShadeMaterialType));
    TF_AXIOM(UsdShadeConnectableAPI::HasConnectableAPI(usdShadeNodeGraphType));

    // schema type examples which should NOT have a connectableAPI behavior
    static const TfType& usdGeomSphereType = TfType::Find<UsdGeomSphere>();
    TF_AXIOM(!UsdShadeConnectableAPI::HasConnectableAPI<UsdGeomSphere>());
    TF_AXIOM(!UsdShadeConnectableAPI::HasConnectableAPI(usdGeomSphereType));
}

int main(int argc, char** argv)
{
    TestHasConnectableAPI();
    printf("Passed!\n");
    return EXIT_SUCCESS;
}
