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

PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_PointInstancerOp, privateData, opArgs, interface)
{
    UsdGeomPointInstancer instancer =
        UsdGeomPointInstancer(privateData.GetUsdPrim());

    // Generate input attr map for consumption by the reader.
    //
    PxrUsdKatanaAttrMap inputAttrMap;

    // Get the instancer's Katana location.
    //
    inputAttrMap.set("outputLocationPath",
            FnKat::StringAttribute(interface.getOutputLocationPath()));

    // Pass along PxrUsdIn op args.
    //
    inputAttrMap.set("opArgs", opArgs);

    // Generate output attr maps.
    //
    // Instancer attr map: describes the instancer itself
    // Sources attr map: describes the instancer's "instance source" children.
    // Instances attr map: describes the instancer's "instance array" child.
    //
    PxrUsdKatanaAttrMap instancerAttrMap;
    PxrUsdKatanaAttrMap sourcesAttrMap;
    PxrUsdKatanaAttrMap instancesAttrMap;
    PxrUsdKatanaReadPointInstancer(
            instancer, privateData, instancerAttrMap, sourcesAttrMap,
            instancesAttrMap, inputAttrMap);

    // Send instancer attrs directly to the interface.
    //
    instancerAttrMap.toInterface(interface);

    // Tell UsdIn to skip all children; we'll create them ourselves below.
    //
    interface.setAttr("__UsdIn.skipAllChildren", FnKat::IntAttribute(1));

    // Early exit if any errors were encountered.
    //
    if (FnKat::StringAttribute(interface.getOutputAttr("errorMessage"))
            .isValid() ||
        FnKat::StringAttribute(interface.getOutputAttr("warningMessage"))
            .isValid()) {
        return;
    }

    // Build the other output attr maps.
    //
    FnKat::GroupAttribute sourcesSSCAttrs = sourcesAttrMap.build();
    FnKat::GroupAttribute instancesSSCAttrs = instancesAttrMap.build();
    if (!sourcesSSCAttrs.isValid() || !instancesSSCAttrs.isValid())
    {
        return;
    }

    // Create "instance source" children using BuildIntermediate.
    //
    PxrUsdKatanaUsdInArgsRefPtr usdInArgs = privateData.GetUsdInArgs();
    FnKat::GroupAttribute childAttrs = sourcesSSCAttrs.getChildByName("c");
    for (int64_t i = 0; i < childAttrs.getNumberOfChildren(); ++i)
    {
        interface.createChild(
            childAttrs.getChildName(i),
            "PxrUsdIn.BuildIntermediate",
            FnKat::GroupBuilder()
                .update(opArgs)
                .set("staticScene", childAttrs.getChildByIndex(i))
                .build(),
            FnKat::GeolibCookInterface::ResetRootFalse,
            new PxrUsdKatanaUsdInPrivateData(
                    usdInArgs->GetRootPrim(), usdInArgs, &privateData),
            PxrUsdKatanaUsdInPrivateData::Delete);
    }

    // Create "instance array" child using StaticSceneCreate.
    //
    interface.execOp("StaticSceneCreate", instancesSSCAttrs);
}
