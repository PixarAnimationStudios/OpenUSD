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
#include "usdKatana/readConstraintTarget.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usdGeom/constraintTarget.h"

PXR_NAMESPACE_USING_DIRECTIVE

PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_ConstraintsOp, privateData, opArgs, interface)
{
    //
    // Construct the group attribute argument for the StaticSceneCreate
    // op which will construct the constraint targets scenegraph branch.
    //

    FnKat::GroupBuilder gb;

    gb.set("scenegraph.stopExpand.a.tabs", FnKat::IntAttribute(1));

    // TODO: SHould this use the ModelAPI?
    std::vector<UsdProperty> constraintTargets =
        privateData.GetUsdPrim().GetPropertiesInNamespace("constraintTargets");

    for (auto constraintTargetProperty : constraintTargets)
    {
        UsdAttribute constraintTargetAttr = constraintTargetProperty.As<UsdAttribute>();
        UsdResolveInfo constraintTargetResolveInfo = constraintTargetAttr.GetResolveInfo();
        if (!constraintTargetResolveInfo.ValueIsBlocked()){
            UsdGeomConstraintTarget constraintTarget(constraintTargetAttr);

            std::string constraintName = constraintTarget.GetIdentifier().GetString();

            if (constraintName.empty())
            {
                // If no identifier was authored, use the full namespace path to
                // construct the constraint attribute name to avoid name collisions.
                std::vector<std::string> nameElements = constraintTargetAttr.SplitName();
                constraintName = TfStringJoin(nameElements.begin()+1, 
                                        nameElements.end(), "_");
            }

            PxrUsdKatanaAttrMap attrs;
            PxrUsdKatanaReadConstraintTarget(
                constraintTarget, privateData, attrs);

            gb.set("c." + constraintName + ".a", attrs.build());
        }
    }

    interface.execOp("StaticSceneCreate", gb.build());

    interface.setAttr("type", FnKat::StringAttribute("constraintgroup"));
}
