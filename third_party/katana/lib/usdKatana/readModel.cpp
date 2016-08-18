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
#include "usdKatana/attrMap.h"
#include "usdKatana/cache.h"
#include "usdKatana/readModel.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdRi/statements.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <FnLogging/FnLogging.h>

FnLogSetup("PxrUsdKatanaReadModel");

/*
 * Create the 'proxies' group attribute. The proxy hierarchy
 * is created using a StaticSceneCreate op.
 */
static FnKat::GroupAttribute
_GetViewerProxyAttr(const PxrUsdKatanaUsdInPrivateData& data)
{
    FnKat::GroupBuilder proxiesBuilder;

    proxiesBuilder.set("viewer.load.opType",
        FnKat::StringAttribute("StaticSceneCreate"));

    proxiesBuilder.set("viewer.load.opArgs.a.type",
        FnKat::StringAttribute("usd"));

    proxiesBuilder.set("viewer.load.opArgs.a.currentTime", 
        FnKat::DoubleAttribute(data.GetUsdInArgs()->GetCurrentTimeD()));

    proxiesBuilder.set("viewer.load.opArgs.a.fileName", 
        FnKat::StringAttribute(data.GetUsdInArgs()->GetFileName()));

    proxiesBuilder.set("viewer.load.opArgs.a.forcePopulateUsdStage", 
        FnKat::FloatAttribute(1));

    // XXX: Once everyone has switched to the op, change referencePath
    // to isolatePath here and in the USD VMP (2/25/2016).
    proxiesBuilder.set("viewer.load.opArgs.a.referencePath", 
        FnKat::StringAttribute(data.GetUsdPrim().GetPath().GetString()));

    proxiesBuilder.set("viewer.load.opArgs.a.rootLocation", 
        FnKat::StringAttribute(data.GetUsdInArgs()->GetRootLocationPath()));

    proxiesBuilder.set("viewer.load.opArgs.a.variants", 
        FnKat::StringAttribute(
            UsdKatanaCache::GetVariantSelectionString(
                data.GetUsdInArgs()->GetVariantSelections())));

    proxiesBuilder.set("viewer.load.opArgs.a.ignoreLayerRegex",
       FnKat::StringAttribute(data.GetUsdInArgs()->GetIgnoreLayerRegex()));

    return proxiesBuilder.build();
}

/*
 * Traverse the model hierarchy to build up a list of all named
 * coordinate systems and their scenegraph locations.
 *
 * XXX:
 * We plan to work with KatanaProcedural development in order to emit these
 * at the model root level.
 */
static bool
_BuildGlobalCoordinateSystems(
    const UsdPrim& prim, 
    const std::string& rootLocation,
    FnKat::GroupBuilder *coordSysBuilder)
{
    bool result = false;

    if (prim.GetPath() != SdfPath::AbsoluteRootPath())
    {
        UsdRiStatements riStatements(prim);
        SdfPathVector coordSysPaths;
        if (riStatements.GetModelCoordinateSystems(&coordSysPaths)
                and not coordSysPaths.empty())
        {
            TF_FOR_ALL(itr, coordSysPaths)
            {
                if (UsdRiStatements coordSysStmt =
                        UsdRiStatements(prim.GetStage()->GetPrimAtPath(*itr)))
                {
                    coordSysBuilder->set(
                        coordSysStmt.GetCoordinateSystem(),
                        FnKat::StringAttribute(rootLocation + itr->GetString()));

                    result = true;
                }
            }
        }
    }

    TF_FOR_ALL(itr, prim.GetFilteredChildren(UsdPrimIsModel))
    {
        result = result or _BuildGlobalCoordinateSystems(
            *itr, rootLocation, coordSysBuilder);
    }

    return result;
}

void
PxrUsdKatanaReadModel(
        const UsdPrim& prim,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    attrs.set("modelName", FnKat::StringAttribute(
        PxrUsdKatanaUtils::GetAssetName(prim)));

    //
    // Set the 'globals.coordinateSystems' attribute.
    //

    FnKat::GroupBuilder coordSysBuilder;
    if (_BuildGlobalCoordinateSystems(
            prim, data.GetUsdInArgs()->GetRootLocationPath(), &coordSysBuilder))
    {
        FnKat::GroupBuilder globalsBuilder;
        globalsBuilder.set("coordinateSystems", coordSysBuilder.build());
        attrs.set("globals", globalsBuilder.build());
    }

    bool isGroup = prim.IsGroup();

    //
    // Set the 'proxies' attribute for models that are not 
    // groups or kinds that need a proxy.
    //

    if (not isGroup or PxrUsdKatanaUtils::ModelGroupNeedsProxy(prim))
    {
        attrs.set("proxies", _GetViewerProxyAttr(data));
    }

    // Everything beyond this point does not apply to groups, so 
    // early exit if this model is a group.
    //
    if (isGroup)
    {
        return;
    }

    attrs.set("modelInstanceName", FnKat::StringAttribute(
        PxrUsdKatanaUtils::GetModelInstanceName(prim)));

    //
    // Set attributes for variant sets that apply (e.g. modelingVariant, 
    // lodVariant, shadingVariant).
    //
    
    for (const auto& regVarSet: UsdUtilsGetRegisteredVariantSets()) {

        // only handle the "always" persistent variant sets.
        switch (regVarSet.selectionExportPolicy) {
            case UsdUtilsRegisteredVariantSet::SelectionExportPolicy::Never:
            case UsdUtilsRegisteredVariantSet::SelectionExportPolicy::IfAuthored:
                continue;
            case UsdUtilsRegisteredVariantSet::SelectionExportPolicy::Always:
                break;
        }

        const std::string& varSetName = regVarSet.name;

        std::string variantSel;
        if (UsdVariantSet variant = prim.GetVariantSet(varSetName)) {
            variantSel = variant.GetVariantSelection();
        }
        if (not variantSel.empty()) {
            attrs.set(varSetName, FnKat::StringAttribute(variantSel));
        }
    }
}
