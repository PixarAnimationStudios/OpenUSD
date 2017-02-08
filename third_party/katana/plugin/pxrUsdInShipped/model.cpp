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
#include "usdKatana/readModel.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"
#include "usdKatana/tokens.h"

#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"

PXR_NAMESPACE_USING_DIRECTIVE


PXRUSDKATANA_USDIN_PLUGIN_DEFINE(PxrUsdInCore_ModelOp, privateData, interface)
{
    PxrUsdKatanaAttrMap attrs;

    const UsdPrim& prim = privateData.GetUsdPrim();
    
    if (prim.HasAssetInfo()) {
        PxrUsdKatanaReadModel(prim, privateData, attrs);
    } 

    // If 'type' has been set to something other than 'group' by
    // a different PxrUsdIn plugin, leave it alone. It means a 
    // more specific USD type applied. Otherwise, set 'type' here
    // based on the model kind.
    //
    if (FnKat::StringAttribute(interface.getOutputAttr("type")
            ).getValue("", false) == "group")
    {
        if (prim.IsGroup())
        {
            if (PxrUsdKatanaUtils::ModelGroupIsAssembly(prim))
            {
                interface.setAttr("type",
                    FnKat::StringAttribute("assembly"));
            }
        }
        else
        {
            TfToken kind;
            UsdModelAPI(prim).GetKind(&kind);
            if (kind == KindTokens->subcomponent)
            {
                interface.setAttr("type",
                    FnKat::StringAttribute("subcomponent"));
            }
            else
            {
                interface.setAttr("type",
                    FnKat::StringAttribute("component"));
            }
        }
    }
    else
    {
        // XXX Katana 2.1v2: If 'type' has already been set to 
        // something other than 'group', strip off the proxies
        // attribute because Katana crashes if it has both a
        // proxies attribute and a vmp for the type.
        attrs.del("proxies");
    }

    attrs.toInterface(interface);

    // when checking for a looks group, swap in the master if the prim is an instance
    UsdPrim lookPrim = (prim.IsInstance() && !privateData.GetMasterPath().IsEmpty()) ?
        prim.GetMaster().GetChild(TfToken(UsdKatanaTokens->katanaLooksScopeName))
            : prim.GetChild(TfToken(UsdKatanaTokens->katanaLooksScopeName));

        
        
    if (lookPrim)
    {
        interface.setAttr(UsdKatanaTokens->katanaLooksChildNameExclusionAttrName,
                FnKat::IntAttribute(1));
        interface.createChild(TfToken(UsdKatanaTokens->katanaLooksScopeName),
                "UsdInCore_LooksGroupOp",
                FnKat::GroupAttribute(),
                FnKat::GeolibCookInterface::ResetRootTrue,
                new PxrUsdKatanaUsdInPrivateData(
                    lookPrim,
                    privateData.GetUsdInArgs(),
                    &privateData),
                PxrUsdKatanaUsdInPrivateData::Delete);
    }

    // early exit for models that are groups
    if (prim.IsGroup())
    {
        return;
    }

    interface.createChild("ConstraintTargets", 
            "UsdInCore_ConstraintsOp", 
            FnKat::GroupAttribute(), 
            FnKat::GeolibCookInterface::ResetRootTrue,
            new PxrUsdKatanaUsdInPrivateData(
                prim,
                privateData.GetUsdInArgs(),
                &privateData),
            PxrUsdKatanaUsdInPrivateData::Delete);
}
