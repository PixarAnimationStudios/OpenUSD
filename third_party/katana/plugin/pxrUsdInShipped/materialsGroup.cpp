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
#include "usdKatana/blindDataObject.h"
#include "usdKatana/readBlindData.h"
#include "usdKatana/readMaterial.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdShade/material.h"

PXR_NAMESPACE_USING_DIRECTIVE

PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_LooksGroupOp, privateData, interface)
{
    UsdStageRefPtr stage = privateData.GetUsdInArgs()->GetStage();

    const std::string& rootLocation = interface.getRootLocationPath();

    //
    // Construct the group attribute argument for the StaticSceneCreate
    // op which will construct the materials scenegraph branch.
    //

    FnKat::GroupBuilder gb;

    TF_FOR_ALL(childIter, privateData.GetUsdPrim().GetChildren()) {
        const UsdPrim& child = *childIter;
        UsdShadeMaterial materialSchema(child);
        if (!materialSchema) {
            continue;
        }

        // do not flatten child material (specialize arcs) 
        // if we have any.
        bool flatten = !materialSchema.HasBaseMaterial();

        std::string location = 
            PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
                child.GetPath(), privateData);

        PxrUsdKatanaAttrMap attrs;
        PxrUsdKatanaReadMaterial(materialSchema, flatten, 
            privateData, attrs, rootLocation);

        // Read blind data.
        PxrUsdKatanaReadBlindData(
            UsdKatanaBlindDataObject(materialSchema), attrs);

        // location is "/root/world/geo/Model/Wood/Walnut/Aged"
        // where rootLocation is "/root/world/geo/Model/"
        // want to get, "c.Wood.c.Walnut.c.Aged"

        std::string cPath = "c." + TfStringReplace(
                location.substr(rootLocation.size()+1), "/", ".c.");

        gb.set(cPath + ".a", attrs.build());
    }

    interface.execOp("StaticSceneCreate", gb.build());

    interface.setAttr("type", FnKat::StringAttribute("materialgroup"));

    // This is an optimization to reduce the RIB size. Since material
    // assignments will resolve into actual material attributes at the
    // geometry locations, there is no need for the Looks scope to be emitted.
    //
    interface.setAttr("pruneRenderTraversal", FnKat::IntAttribute(1));
}
