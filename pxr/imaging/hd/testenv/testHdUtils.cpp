//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/utils.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <fstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static
bool
BasicTest()
{
    // Create a representation of a material network
    HdMaterialNetwork materialNetwork;
    materialNetwork.nodes.reserve(4);

    const SdfPath materialPath("/Asset/Looks/Material");

    HdMaterialNode textureNode;
    textureNode.path = SdfPath("/Asset/Looks/Material/Texture");
    textureNode.identifier = TfToken("Texture_5");
    textureNode.parameters[TfToken("inputs:filename")] = \
        VtValue("studio/patterns/checkerboard/checkerboard.tex");
    materialNetwork.nodes.push_back(textureNode);

    HdMaterialNode materialLayerNode;
    materialLayerNode.path = SdfPath("/Asset/Looks/Material/MaterialLayer");
    materialLayerNode.identifier = TfToken("MaterialLayer_3");
    materialNetwork.nodes.push_back(materialLayerNode);

    HdMaterialNode standInNode;
    standInNode.path = SdfPath("/Asset/Looks/Material/StandIn");
    standInNode.identifier = TfToken("PbsNetworkMaterialStandIn_3");
    materialNetwork.nodes.push_back(standInNode);

    const TfToken primvarToken("Primvar_0");
    materialNetwork.primvars.push_back(primvarToken);

    HdMaterialNode primvarNode;
    primvarNode.path = SdfPath("/Asset/Looks/Material/Primvar_0Reader");
    primvarNode.identifier = TfToken("PrimvarReader_float3");
    primvarNode.parameters[TfToken("varname")] = primvarToken;
    primvarNode.parameters[TfToken("fallback")] = VtValue(GfVec3f(1.0f, 1.0f, 1.0f));
    materialNetwork.nodes.push_back(primvarNode);

    // Connect the nodes
    HdMaterialRelationship textureMaterialLayerRel;
    textureMaterialLayerRel.inputId = textureNode.path;
    textureMaterialLayerRel.inputName = TfToken("resultRGB");
    textureMaterialLayerRel.outputId = materialLayerNode.path;
    textureMaterialLayerRel.outputName = TfToken("albedo");
    materialNetwork.relationships.push_back(textureMaterialLayerRel);

    HdMaterialRelationship materialLayerStandInRel;
    materialLayerStandInRel.inputId = materialLayerNode.path;
    materialLayerStandInRel.inputName = TfToken("pbsMaterialOut");
    materialLayerStandInRel.outputId = standInNode.path;
    materialLayerStandInRel.outputName = TfToken("multiMaterialIn");
    materialNetwork.relationships.push_back(materialLayerStandInRel);

    pxr::HdMaterialRelationship primvarRel;
    primvarRel.inputId = primvarNode.path;
    primvarRel.inputName = TfToken("result");
    primvarRel.outputId = materialPath;
    primvarRel.outputName = primvarToken;
    materialNetwork.relationships.push_back(primvarRel);

    HdMaterialNetworkMap networkMap;
    networkMap.map[TfToken("surface")] = materialNetwork;

    HdContainerDataSourceHandle ds = 
        HdUtils::ConvertHdMaterialNetworkToHdMaterialSchema(networkMap);

    std::ofstream outdata("testHdUtils_material.txt", std::ios::out);
    HdDebugPrintDataSource(outdata, ds);
    outdata.close();

    return true;
}

int main()
{
    TfErrorMark mark;
    bool success = BasicTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
