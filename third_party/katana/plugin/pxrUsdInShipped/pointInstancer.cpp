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
#include "usdKatana/readPointInstancer.h"

#include "pxr/usd/usdGeom/pointInstancer.h"

PXR_NAMESPACE_USING_DIRECTIVE

PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_PointInstancerOp, privateData, interface)
{
    // Attr maps that are modified.
    //
    PxrUsdKatanaAttrMap outputAttrMap;
    PxrUsdKatanaAttrMap sourcesAttrMap;
    PxrUsdKatanaAttrMap instancesAttrMap;

    // Attr maps that are parsed.
    //
    PxrUsdKatanaAttrMap inputAttrMap;

    // Populate input attrs.
    //
    inputAttrMap.set("outputLocationPath",
            FnKat::StringAttribute(interface.getOutputLocationPath()));

    PxrUsdKatanaReadPointInstancer(
            UsdGeomPointInstancer(privateData.GetUsdPrim()),
            privateData, outputAttrMap, sourcesAttrMap, instancesAttrMap,
            inputAttrMap);

    // Send output attrs directly to the interface.
    //
    outputAttrMap.toInterface(interface);

    // Early exit if any errors were encountered.
    //
    if (FnKat::StringAttribute(interface.getOutputAttr("type")
            ).getValue("", false) == "error")
    {
        return;
    }

    // Build out the attr maps that were modified.
    //
    FnKat::GroupAttribute sourcesSSCAttrs = sourcesAttrMap.build();
    FnKat::GroupAttribute instancesSSCAttrs = instancesAttrMap.build();
    if (not sourcesSSCAttrs.isValid() or not instancesSSCAttrs.isValid())
    {
        return;
    }

    // Tell UsdIn to skip all children; we'll create them ourselves.
    //
    interface.setAttr("__UsdIn.skipAllChildren", FnKat::IntAttribute(1));

    // Create sources child using BuildIntermediate.
    //
    PxrUsdKatanaUsdInArgsRefPtr usdInArgs = privateData.GetUsdInArgs();
    FnKat::GroupAttribute childAttrs = sourcesSSCAttrs.getChildByName("c");
    for (int64_t i = 0; i < childAttrs.getNumberOfChildren(); ++i)
    {
        interface.createChild(
            childAttrs.getChildName(i),
            "PxrUsdIn.BuildIntermediate",
            FnKat::GroupBuilder()
                .update(interface.getOpArg())
                .set("staticScene", childAttrs.getChildByIndex(i))
                .build(),
            FnKat::GeolibCookInterface::ResetRootFalse,
            new PxrUsdKatanaUsdInPrivateData(
                    usdInArgs->GetRootPrim(), usdInArgs, &privateData),
            PxrUsdKatanaUsdInPrivateData::Delete);
    }

    // Create 'instances' child using StaticSceneCreate.
    //
    interface.execOp("StaticSceneCreate", instancesSSCAttrs);
}
