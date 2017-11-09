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
#include "usdKatana/readLight.h"
#include "usdKatana/usdInPluginRegistry.h"
#include "usdKatana/utils.h"
#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usdRi/pxrAovLight.h"
#include <FnGeolibServices/FnBuiltInOpArgsUtil.h>

PXR_NAMESPACE_USING_DIRECTIVE

PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_LightOp, privateData, opArgs, interface)
{
    PxrUsdKatanaUsdInArgsRefPtr usdInArgs = privateData.GetUsdInArgs();
    PxrUsdKatanaAttrMap attrs;
    
    UsdLuxLight light(privateData.GetUsdPrim());

    PxrUsdKatanaReadLight(
        light,
        privateData,
        attrs);

    attrs.toInterface(interface);


    // Tell UsdIn to skip all children; we'll create them ourselves.
    interface.setAttr("__UsdIn.skipAllChildren", FnKat::IntAttribute(1));

    // Light filters.
    SdfPathVector filterPaths;
    light.GetFiltersRel().GetForwardedTargets(&filterPaths);
    if (!filterPaths.empty()) {
        // XXX For now the importAsReferences codepath is disabled.
        // To support light filter references we need to specify
        // info.gaffer.packageClass (and possibly more), otherwise
        // the gaffer infrastructure will mark these references
        // as orphaned.
        bool importAsReferences = false;
        if (importAsReferences) {
            // Create "light filter reference" child locations
            // TODO: We also need to handle the case of regular
            // light filters sitting as children below this light.
            FnGeolibServices::StaticSceneCreateOpArgsBuilder sscb(false);
            for (const SdfPath &filterPath: filterPaths) {
                const std::string ref_location = filterPath.GetName();
                const std::string filter_location =
                    PxrUsdKatanaUtils::
                    ConvertUsdPathToKatLocation(filterPath, usdInArgs);
                sscb.createEmptyLocation(ref_location,
                    "light filter reference");
                sscb.setAttrAtLocation(ref_location,
                    "info.gaffer.referencePath",
                    FnAttribute::StringAttribute(filter_location));
            }
            interface.execOp("StaticSceneCreate", sscb.build());
        } else {
            // Expand light filters directly beneath this light.
            for (const SdfPath &filterPath: filterPaths) {
                if (UsdPrim filterPrim =
                    usdInArgs->GetStage()->GetPrimAtPath(filterPath)) {

                    interface.createChild(
                        filterPath.GetName(),
                        // Use the top-level PxrUsdIn op to get proper
                        // op dispatch, including site-specific plugins.
                        // (We can't use empty string to re-run this
                        // same op because we are already in the
                        // light-specific op, and we need to run a
                        // light-filter op instead.)
                        "PxrUsdIn",
                        opArgs,
                        FnKat::GeolibCookInterface::ResetRootFalse,
                        new PxrUsdKatanaUsdInPrivateData(
                            filterPrim, usdInArgs, &privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
                }
            }
        }
    }
}

namespace {

static
void
lightListFnc(PxrUsdKatanaUtilsLightListAccess& lightList)
{
    UsdPrim prim = lightList.GetPrim();
    if (prim.IsA<UsdLuxLight>()) {
        UsdLuxLight light(prim);
        lightList.Set("path", lightList.GetLocation());
        bool enabled = lightList.SetLinks(light.GetLightLinkingAPI(), "light");
        lightList.Set("enable", enabled);
        lightList.SetLinks(light.GetShadowLinkingAPI(), "shadow");
    }
    if (prim.IsA<UsdRiPxrAovLight>()) {
        lightList.Set("hasAOV", true);
    }
}

}

void
registerPxrUsdInShippedLightLightListFnc()
{
    PxrUsdKatanaUsdInPluginRegistry::RegisterLightListFnc(lightListFnc);
}
