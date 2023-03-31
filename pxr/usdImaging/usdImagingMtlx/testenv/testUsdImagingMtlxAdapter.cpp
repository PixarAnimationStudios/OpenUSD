//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImagingMtlx/adapter.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
UsdImagingMtlxAdapterBasicTest()
{
    const std::string mtlxPath = "test.mtlx";
    const TfTokenVector shaderSourceTypes = {TfToken("mtlx")};
    const TfTokenVector renderContexts = {TfToken("mtlx")};
    HdMaterialNetworkMap materialNetworkMap;
    UsdImagingMtlxConvertMtlxToHdMaterialNetworkMap(
        mtlxPath,
        shaderSourceTypes,
        renderContexts,
        &materialNetworkMap);

    if (!TF_VERIFY(materialNetworkMap.map.size() > 0) ||
        !TF_VERIFY(materialNetworkMap.terminals.size() == 1)) {
        return false;
    }

    bool isVolume = false;
    HdMaterialNetwork2 network = HdConvertToHdMaterialNetwork2(materialNetworkMap, &isVolume);

    if (!TF_VERIFY(!isVolume)) {
        return false;
    }

    const std::vector<std::pair<SdfPath, std::string>> expectedNodes = {
        {SdfPath("/MaterialX/Materials/surfacematerial_4/ND_standard_surface_surfaceshader"), "ND_standard_surface_surfaceshader"},
        {SdfPath("/MaterialX/Materials/surfacematerial_4/NG/image_2"), "ND_image_color3"},
        {SdfPath("/MaterialX/Materials/surfacematerial_4/NG/texcoord_1"), "ND_texcoord_vector2"},
    };

    if (!TF_VERIFY(network.nodes.size() == expectedNodes.size())) {
        return false;
    }

    for (auto& entry : expectedNodes) {
        auto it = network.nodes.find(entry.first);
        if (!TF_VERIFY(it != network.nodes.end()) ||
            !TF_VERIFY(it->second.nodeTypeId == entry.second)) {
            return false;
        }
    }

    return true;
}

int 
main(int argc, char **argv)
{
    TfErrorMark mark;

    bool success = UsdImagingMtlxAdapterBasicTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
