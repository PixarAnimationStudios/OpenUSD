//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
