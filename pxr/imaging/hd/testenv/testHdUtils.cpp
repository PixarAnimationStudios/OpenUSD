//
// Copyright 2024 Pixar
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
    materialNetwork.nodes.reserve(3);

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
