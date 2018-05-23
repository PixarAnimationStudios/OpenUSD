//
// Copyright 2018 Pixar
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
#include "shadingModeRegistry.h"
#include "shaderWrapper.h"

#include <OP/OP_Node.h>
#include <UT/UT_String.h>
#include <VOP/VOP_Node.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION_WITH_TAG(GusdShadingModeRegistry, rib) {
    GusdShadingModeRegistry::getInstance().registerExporter(
        "rib", "RIB", [](OP_Node* opNode,
                         const UsdStagePtr& stage,
                         const SdfPath& looksPath,
                         const GusdShadingModeRegistry::HouMaterialMap& houMaterialMap,
                         const std::string& shaderOutDir) {
            for (const auto& assignment: houMaterialMap) {
                VOP_Node* materialVop = opNode->findVOPNode(assignment.first.c_str());
                if (materialVop == nullptr ||
                    strcmp(materialVop->getRenderMask(), "RIB") != 0) {
                    continue;
                }

                UT_String vopPath(materialVop->getFullPath());
                vopPath.forceAlphaNumeric();
                SdfPath path = looksPath.AppendPath(SdfPath(vopPath.toStdString()));

                GusdShaderWrapper shader(materialVop, stage, path.GetString(), shaderOutDir);
                for (const auto& primPath: assignment.second) {
                    UsdPrim prim = stage->GetPrimAtPath(primPath);
                    shader.bind(prim);
                }
            }
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
