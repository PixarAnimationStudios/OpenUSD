//
// Copyright 2017 Pixar
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
#include <iostream>

#include <FnAttribute/FnGroupBuilder.h>

#include "pxr/pxr.h"
#ifdef _WIN32
#   include <FnPlatform/Windows.h>
#endif
#include "pxr/usd/usdShade/material.h"

#include "usdKatana/attrMap.h"
#include "usdKatana/cache.h"
#include "usdKatana/blindDataObject.h"
#include "usdKatana/readBlindData.h"
#include "usdKatana/readMaterial.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxrUsdInShipped/attrfnc_materialReference.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Create the material attribute for referenced USD materials
MaterialReferenceAttrFncCache::IMPLPtr 
MaterialReferenceAttrFncCache::createValue(
        const FnAttribute::Attribute & attr)
{
    FnAttribute::GroupAttribute args = attr;
    if (!args.isValid()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }
    FnAttribute::StringAttribute assetAttr = args.getChildByName("asset");
    if (!assetAttr.isValid()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }
    std::string asset = assetAttr.getValue();
    if (asset.empty()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }

    FnAttribute::StringAttribute materialPathAttr = args.getChildByName(
        "materialPath");
    if (!materialPathAttr.isValid()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }
    std::string materialPath = materialPathAttr.getValue();
    if (materialPath.empty()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }
    if (materialPath[0] != '/') {
        materialPath = "/" + materialPath;
    }

    FnAttribute::StringAttribute looksGroupLocationAttr = 
        args.getChildByName("looksGroupLocation");
    std::string looksGroupLocation;
    if (looksGroupLocationAttr.isValid()) {
        looksGroupLocation = looksGroupLocationAttr.getValue(
            "", false);
        if (!looksGroupLocation.empty()) {
            materialPath = looksGroupLocation + materialPath;
        }
    }

    int flatten = FnKat::IntAttribute(
        args.getChildByName("flatten")).getValue(0, false);

    FnAttribute::GroupAttribute sessionAttr;
    std::string sessionLocation = "";
    std::string ignoreLayerRegex = "";
    UsdStageRefPtr stage = UsdKatanaCache::GetInstance().GetStage(
        asset, 
        sessionAttr, sessionLocation,
        ignoreLayerRegex, 
        true /* forcePopulate */);

    if (!stage) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }

    UsdPrim prim = stage->GetPrimAtPath(SdfPath(materialPath));
    if (!prim) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }

    ArgsBuilder ab;
    ab.stage = stage;
    PxrUsdKatanaUsdInArgsRefPtr usdInArgs = ab.build();
    PxrUsdKatanaUsdInPrivateData data(prim, usdInArgs);

    UsdShadeMaterial materialSchema(prim);
    PxrUsdKatanaAttrMap attrs;
    PxrUsdKatanaReadMaterial(
        materialSchema,
        bool(flatten),
        data,
        attrs,
        looksGroupLocation);

    // include all the blind data
    UsdKatanaBlindDataObject kbd(prim);
    PxrUsdKatanaReadBlindData(
        kbd,
        attrs);

    FnAttribute::GroupAttribute allMaterialAttributes = attrs.build();
    return IMPLPtr(new FnAttribute::GroupAttribute(
        allMaterialAttributes.getChildByName("material")));
}



///////////////////////////////////////////////////////////////////////


// Query the cache
MaterialReferenceAttrFncCache g_MaterialReferenceAttrFncCache;
FnAttribute::Attribute MaterialReferenceAttrFnc::run(
    FnAttribute::Attribute args)
{
    return *g_MaterialReferenceAttrFncCache.getValue(args);
}

// Flush the cache
void MaterialReferenceAttrFnc::flush()
{
    g_MaterialReferenceAttrFncCache.clear();
}



// Create the material attribute for referenced USD materials
LibraryMaterialNamesAttrFncCache::IMPLPtr 
LibraryMaterialNamesAttrFncCache::createValue(
        const FnAttribute::Attribute & attr)
{
    FnAttribute::GroupAttribute args = attr;
    if (!args.isValid()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }
    FnAttribute::StringAttribute assetAttr = args.getChildByName("asset");
    if (!assetAttr.isValid()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }

    std::string asset = assetAttr.getValue();
    if (asset.empty()) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }

    FnAttribute::GroupAttribute sessionAttr;
    std::string sessionLocation = "";
    std::string ignoreLayerRegex = "";
    UsdStageRefPtr stage = UsdKatanaCache::GetInstance().GetStage(
        asset, 
        sessionAttr, sessionLocation,
        ignoreLayerRegex, 
        true /* forcePopulate */);

    if (!stage) {
        return IMPLPtr(new FnAttribute::GroupAttribute());
    }

    // Find all materials on this shader library
    // first: get all looks at the root
    std::vector<std::string> materialNames;

    UsdPrim root = stage->GetPseudoRoot();
    auto children = root.GetChildren();
    for (auto childIt = children.begin(); 
            childIt != children.end(); 
            ++childIt) {

        UsdShadeMaterial materialSchema(*childIt);
        if (!materialSchema) {
            continue;
        }

        // this is a pixar specific policy
        // omit invalid materials?
        // if either name or base material starts with "_"
        if (TfStringStartsWith(childIt->GetName().GetString(), "_")) {
            continue;
        }
        // base material - implemented via derivesFrom relationships
        std::string baseMaterialPath = 
            materialSchema.GetBaseMaterialPath().GetString();
        if (TfStringStartsWith(baseMaterialPath, "/_")) {
            continue;
        }
        // specialized
        std::string specializedPath;
        SdfPathListOp pathsListOp;
        childIt->GetMetadata(SdfFieldKeys->Specializes, &pathsListOp);
        if (pathsListOp.HasKeys()) {
            std::vector<SdfPath> itemVector;
            if (pathsListOp.IsExplicit())
                itemVector = pathsListOp.GetExplicitItems();
            else
                itemVector = pathsListOp.GetAddedItems();
            if (itemVector.size()) {
                specializedPath = itemVector[0].GetString();
                if (TfStringStartsWith(specializedPath, "/_")) {
                    continue;
                }
            }
        }

        materialNames.push_back(childIt->GetName());

    }

    FnAttribute::GroupBuilder gb;
    gb.set("materialNames", FnAttribute::StringAttribute(materialNames));
    if (stage->GetPrimAtPath(SdfPath("/LooksDerivedStructure"))) {
        // older looks derived structure, for derivesFrom-based 
        // shader libraries
        gb.set("root", FnAttribute::StringAttribute("/LooksDerivedStructure"));
    }
    else {
        // newer, for specialize arcs
        gb.set("root", FnAttribute::StringAttribute(""));
    }

    return IMPLPtr(new FnAttribute::GroupAttribute(gb.build()));
}



///////////////////////////////////////////////////////////////////////


// Query the cache
LibraryMaterialNamesAttrFncCache g_LibraryMaterialNamesAttrFncCache;
FnAttribute::Attribute LibraryMaterialNamesAttrFnc::run(
    FnAttribute::Attribute args)
{
    return *g_LibraryMaterialNamesAttrFncCache.getValue(args);
}

// Flush the cache
void LibraryMaterialNamesAttrFnc::flush()
{
    g_LibraryMaterialNamesAttrFncCache.clear();
}

