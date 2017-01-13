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
#include "usdKatana/readMaterial.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/utils.h"
#include "usdKatana/baseMaterialHelpers.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/usdGeom/scope.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/utils.h"

#include "pxr/usd/usdRi/lookAPI.h"
#include "pxr/usd/usdRi/risObject.h"
#include "pxr/usd/usdRi/risOslPattern.h"
#include "pxr/usd/usdUI/nodeGraphNodeAPI.h"

#include <FnGeolibServices/FnAttributeFunctionUtil.h>
#include <FnLogging/FnLogging.h>

#include "pxr/usd/usdHydra/tokens.h"

#include <stack>

FnLogSetup("PxrUsdKatanaReadMaterial");

using std::string;
using std::vector;
using FnKat::GroupBuilder;

static std::string 
_CreateShadingNode(
        UsdPrim shadingNode,
        double currentTime,
        GroupBuilder& nodesBuilder,
        GroupBuilder& interfaceBuilder,
        const std::string & targetName);

FnKat::Attribute 
_GetMaterialAttr(
        const UsdShadeMaterial& materialSchema,
        double currentTime,
        bool flatten);

void
_UnrollInterfaceFromPrim(
        const UsdPrim& prim, 
        const std::string& paramPrefix,
        GroupBuilder& materialBuilder,
        GroupBuilder& interfaceBuilder);

void
PxrUsdKatanaReadMaterial(
        const UsdShadeMaterial& material,
        bool flatten,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs,
        const std::string& looksGroupLocation)
{
    UsdStageRefPtr stage = material.GetPrim().GetStage();
    SdfPath primPath = material.GetPrim().GetPath();

    // we do this before ReadPrim because ReadPrim calls ReadBlindData which we
    // don't want to stomp here.
    attrs.set("material", _GetMaterialAttr(
        material, data.GetUsdInArgs()->GetCurrentTime(), flatten));

    PxrUsdKatanaReadPrim(material.GetPrim(), data, attrs);

    const std::string& parentPrefix = (looksGroupLocation.empty()) ?
        data.GetUsdInArgs()->GetRootLocationPath() : looksGroupLocation;

    attrs.set("katanaLookPath", FnKat::StringAttribute(
        PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
            primPath, data).substr(parentPrefix.size()+1)));
    
    attrs.set("type", FnKat::StringAttribute("material"));

    // clears out prmanStatements.
    attrs.set("prmanStatements", FnKat::Attribute());
}


////////////////////////////////////////////////////////////////////////
// Protected methods

void 
_GatherShadingParameters(
    const UsdShadeShader &shaderSchema, 
    const string &handle,
    double currentTime,

    GroupBuilder& nodesBuilder,
    GroupBuilder& paramsBuilder,
    GroupBuilder& interfaceBuilder,
    GroupBuilder& connectionsBuilder,
    const std::string & targetName)
{
    UsdPrim prim = shaderSchema.GetPrim();
    string primName = prim.GetName();

    std::vector<UsdShadeParameter> shaderParams = shaderSchema.GetParameters();
    TF_FOR_ALL(shadeParamIter, shaderParams) {
        UsdShadeParameter shaderParam = *shadeParamIter;
        std::string inputParamId = shaderParam.GetAttr().GetName();

        // We do not try to extract presentation metadata from parameters -
        // only material interface attributes should bother recording such.
        
        if (shaderParam.IsConnected()) {
            UsdShadeConnectableAPI source;
            TfToken outputName;

            if (shaderParam.GetConnectedSource(&source, &outputName)) {
                std::string targetHandle = _CreateShadingNode(
                        source.GetPrim(), 
                        currentTime,
                        nodesBuilder, 
                        interfaceBuilder,
                        targetName);

                // Check the relationship representing this connection
                // to see if the targets come from a base material.
                // Ignore them if so.
                const TfToken relName = shaderParam.GetConnectionRelName();
                const UsdRelationship rel = prim.GetRelationship(relName);
                if (!PxrUsdKatana_AreRelTargetsFromBaseMaterial(rel)) {
                    // These targets are local, so include them.
                    connectionsBuilder.set(
                        inputParamId, 
                        FnKat::StringAttribute(
                            outputName.GetString() + "@" + targetHandle));
                }
            }
        }

        // produce the value here and let katana handle the connection part
        // correctly..
        UsdAttribute attr = shaderParam.GetAttr();
        VtValue vtValue;
        if (!attr.Get(&vtValue, currentTime)) {
            continue;
        }

        // If the attribute value comes from a base material, leave it
        // empty -- we will inherit it from the parent katana material.
        if (!PxrUsdKatana_IsAttrValFromBaseMaterial(attr)) {
            paramsBuilder.set(inputParamId,
                    PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, true));
        }
    }
    
    // XXX check for info attrs as they're not strictly parameters but
    //     necessary for hydra shading (currently)
    if (targetName == "display")
    {
        std::vector<UsdProperty> props = 
                prim.GetPropertiesInNamespace("info");
        
        for (std::vector<UsdProperty>::const_iterator I =
                props.begin(), E = props.end(); I != E; ++I)
        {
            const UsdProperty & prop = (*I);
            
            if (UsdAttribute attr = prop.As<UsdAttribute>())
            {
                VtValue vtValue;
                if (!attr.Get(&vtValue, currentTime))
                {
                    continue;
                }
                
                paramsBuilder.set(attr.GetName().GetString(),
                        PxrUsdKatanaUtils::ConvertVtValueToKatAttr(
                                vtValue, true));
            }
        }
    }
}


// NOTE: the Ris codepath doesn't use the interfaceBuilder
std::string 
_CreateShadingNode(
        UsdPrim shadingNode,
        double currentTime,
        GroupBuilder& nodesBuilder,
        GroupBuilder& interfaceBuilder,
        const std::string & targetName)
{
    std::string handle = PxrUsdKatanaUtils::GenerateShadingNodeHandle(shadingNode);
    if (handle.empty()) {
        return "";
    }

    // Check if we know about this node already
    FnKat::GroupAttribute curNodes = nodesBuilder.build(
            nodesBuilder.BuildAndRetain);
    if (curNodes.getChildByName(handle).isValid()) {
        // If so, just return and don't create anything
        return handle;
    }

    // Create an empty group at the handle to prevent infinite recursion
    nodesBuilder.set(handle, GroupBuilder().build());

    GroupBuilder shdNodeAttr;
    bool validData = false;

#if defined(USDRI_LOOK_API_VERSION)
    if (UsdRiRslShader rslShaderObjectSchema = UsdRiRslShader(shadingNode))
#else
    if (UsdRiRslShaderObject rslShaderObjectSchema = UsdRiRslShaderObject(shadingNode)) 
#endif
    {
        validData = true;
        SdfAssetPath sloAssetPath;
        rslShaderObjectSchema.GetSloPathAttr().Get(
            &sloAssetPath, currentTime);
        
        // add to shader list
        shdNodeAttr.set("type", FnKat::StringAttribute(
            sloAssetPath.GetAssetPath()));

        GroupBuilder paramsBuilder;
        GroupBuilder connectionsBuilder;

        _GatherShadingParameters(rslShaderObjectSchema, handle, currentTime,
            nodesBuilder, paramsBuilder, 
            interfaceBuilder, connectionsBuilder, targetName);

        // if no content, don't bother creating the group.
        FnKat::GroupAttribute paramsAttr = paramsBuilder.build();
        if (paramsAttr.getNumberOfChildren() > 0) {
            shdNodeAttr.set("parameters", paramsAttr);
        }
        FnKat::GroupAttribute connectionsAttr = connectionsBuilder.build();
        if (connectionsAttr.getNumberOfChildren() > 0) {
            shdNodeAttr.set("connections", connectionsAttr);
        }
    }
    else if (UsdShadeShader shaderSchema = UsdShadeShader(shadingNode)) {
        validData = true;
        SdfAssetPath fileAssetPath;

        UsdRiRisOslPattern oslSchema(shadingNode);
        UsdRiRisObject risObjectSchema(shadingNode);
        if (oslSchema) {
            // Katana handles osl shaders a bit better than simply
            // using PxrOsl pattern. Convert to katana osl node.
            oslSchema.GetOslPathAttr().Get(
                &fileAssetPath, currentTime);
            if (!PxrUsdKatana_IsAttrValFromBaseMaterial(
                oslSchema.GetOslPathAttr())) {
                shdNodeAttr.set("type", FnKat::StringAttribute(
                            "osl:" + fileAssetPath.GetAssetPath()));
            }
        }
        else if (risObjectSchema){
            risObjectSchema.GetFilePathAttr().Get(
                &fileAssetPath, currentTime);
            if (!PxrUsdKatana_IsAttrValFromBaseMaterial(
                risObjectSchema.GetFilePathAttr())) {
                shdNodeAttr.set("type", FnKat::StringAttribute(
                    fileAssetPath.GetAssetPath()));
            }
        } 
        else {
            
            // only use the fallback OSL test if the targetName is "prman" as
            // it will issue benign but confusing errors to the shell for
            // display shaders
            if (targetName == "prman")
            {
                TfToken id;
                shaderSchema.GetIdAttr().Get(&id, currentTime);
                std::string oslIdString = id.GetString();
                oslIdString = "osl:" + oslIdString;
                FnKat::StringAttribute oslIdAttr = FnKat::StringAttribute(oslIdString);
                FnAttribute::GroupAttribute shaderInfoAttr = 
                         FnGeolibServices::FnAttributeFunctionUtil::run(
                                 "PRManGetShaderParameterInfo", oslIdAttr);
                if (shaderInfoAttr.isValid()) 
                    shdNodeAttr.set("type", oslIdAttr);
                else 
                    shdNodeAttr.set(
                        "type", FnKat::StringAttribute(id.GetString()));
            }
            else
            {
                TfToken id;
                shaderSchema.GetIdAttr().Get(&id, currentTime);
                shdNodeAttr.set(
                        "type", FnKat::StringAttribute(id.GetString()));
            }
        }

        GroupBuilder paramsBuilder;
        GroupBuilder connectionsBuilder;

        _GatherShadingParameters(risObjectSchema, handle, currentTime,
            nodesBuilder, paramsBuilder, 
            interfaceBuilder, connectionsBuilder, targetName);


        FnKat::GroupAttribute paramsAttr = paramsBuilder.build();
        if (paramsAttr.getNumberOfChildren() > 0) {
            shdNodeAttr.set("parameters", paramsAttr);
        }
        FnKat::GroupAttribute connectionsAttr = connectionsBuilder.build();
        if (connectionsAttr.getNumberOfChildren() > 0) {
            shdNodeAttr.set("connections", connectionsAttr);
        }

        // read position
        UsdUINodeGraphNodeAPI nodeApi(shadingNode);
        UsdAttribute posAttr = nodeApi.GetPosAttr();
        if (posAttr) {
            GfVec2f pos;
            if (posAttr.Get(&pos)) {
                float posArray[2] = {pos[0], pos[1]};
                shdNodeAttr.set(
                    "hints.pos", FnKat::FloatAttribute(posArray, 2, 2));
            }
        }
        // read displayColor
        UsdAttribute displayColorAttr = nodeApi.GetDisplayColorAttr();
        if (displayColorAttr) {
            GfVec3f displayColor;
            if (displayColorAttr.Get(&displayColor)) {
                float displayColorArray[3] = {
                    displayColor[0], displayColor[1], displayColor[2]};
                shdNodeAttr.set(
                    "hints.displayColor", 
                    FnKat::FloatAttribute(displayColorArray, 3, 3));
            }
        }
    }

    if (validData && !PxrUsdKatana_IsPrimDefFromBaseMaterial(shadingNode)) {
        shdNodeAttr.set("name", FnKat::StringAttribute(handle));
        shdNodeAttr.set("srcName", FnKat::StringAttribute(handle));
        shdNodeAttr.set("target", FnKat::StringAttribute(targetName));
    }

    nodesBuilder.set(handle, shdNodeAttr.build());
    return handle;
}


FnKat::Attribute 
_GetMaterialAttr(
        const UsdShadeMaterial& materialSchema,
        double currentTime,
        bool flatten)
{
    UsdPrim materialPrim = materialSchema.GetPrim();
    
    // TODO: we need a hasA schema
    UsdRiLookAPI riLookAPI(materialPrim);
    UsdStageWeakPtr stage = materialPrim.GetStage();

    GroupBuilder materialBuilder;
    materialBuilder.set("style", FnKat::StringAttribute("network"));
    GroupBuilder nodesBuilder;
    GroupBuilder interfaceBuilder;
    GroupBuilder terminalsBuilder;

    /////////////////
    // RSL SECTION
    /////////////////

    // look for surface
    if (UsdRelationship surfaceRel = riLookAPI.GetSurfaceRel()) {
        if (!PxrUsdKatana_AreRelTargetsFromBaseMaterial(surfaceRel)) {
            SdfPathVector targetPaths;
            surfaceRel.GetForwardedTargets(&targetPaths);
            if (targetPaths.size() > 1) {
                FnLogWarn("Multiple surfaces detected on look:" << 
                    materialPrim.GetPath());
            }
            if (targetPaths.size() > 0) {
                const SdfPath targetPath = targetPaths[0];
                if (UsdPrim surfacePrim =
                    stage->GetPrimAtPath(targetPath)) {

                    std::string handle = _CreateShadingNode(
                        surfacePrim, currentTime,
                        nodesBuilder, interfaceBuilder, "prman");
                    terminalsBuilder.set("prmanSurface",
                                         FnKat::StringAttribute(handle));
                } else {
                    FnLogWarn("Surface shader does not exist at:" << 
                              targetPath.GetString());
                }
            }
        }
    }

    // look for displacement
    if (UsdRelationship displacementRel = riLookAPI.GetDisplacementRel()) {
        if (!PxrUsdKatana_AreRelTargetsFromBaseMaterial(displacementRel)) {
            SdfPathVector targetPaths;
            displacementRel.GetForwardedTargets(&targetPaths);
        
            if (targetPaths.size() > 1) {
                FnLogWarn("Multiple displacement detected on look:" << 
                    materialPrim.GetPath());
            }
            if (targetPaths.size() > 0) {
                const SdfPath targetPath = targetPaths[0];
                if (UsdPrim displacementPrim =
                    stage->GetPrimAtPath(targetPath)) {

                    string handle = _CreateShadingNode(
                        displacementPrim, currentTime,
                        nodesBuilder, interfaceBuilder, "prman");
                    terminalsBuilder.set("prmanDisplacement",
                                         FnKat::StringAttribute(handle));
                } else {
                    FnLogWarn("Displacement shader does not exist at:" << 
                              targetPath.GetString());
                }
            }
        }
    }

    // look for coshaders
    if (UsdRelationship coshadersRel = riLookAPI.GetCoshadersRel()) {
        if (!PxrUsdKatana_AreRelTargetsFromBaseMaterial(coshadersRel)) {
            SdfPathVector targetPaths;
            coshadersRel.GetForwardedTargets(&targetPaths);
            if (targetPaths.size() > 0) {
                SdfPath targetPath;
                for (size_t i = 0; i<targetPaths.size(); ++i){
                    targetPath = targetPaths[i];

                    if (UsdPrim shadingNodePrim =
                        stage->GetPrimAtPath(targetPath)) {

                        string shortHandle = shadingNodePrim.GetName();
                        
                        std::string handle = _CreateShadingNode(
                            shadingNodePrim, currentTime,
                            nodesBuilder, interfaceBuilder, "prman");

                        terminalsBuilder.set("prmanCoshaders."+shortHandle,
                                             FnKat::StringAttribute(handle));
                    } else {
                        FnLogWarn("Coshader does not exist at:" << 
                                  targetPath.GetString());
                    }
                }
            }
        }
    }

    /////////////////
    // RIS SECTION
    /////////////////
    // this does not exclude the rsl part

    // look for bxdf's
    if (UsdRelationship bxdfRel = riLookAPI.GetBxdfRel()) {
        if (!PxrUsdKatana_AreRelTargetsFromBaseMaterial(bxdfRel)) {
            SdfPathVector targetPaths;
            bxdfRel.GetForwardedTargets(&targetPaths);
        
            if (targetPaths.size() > 1) {
                FnLogWarn("Multiple bxdf detected on look:" << 
                    materialPrim.GetPath());
            }
            if (targetPaths.size() > 0) {
                const SdfPath targetPath = targetPaths[0];
                if (UsdPrim bxdfPrim =
                    stage->GetPrimAtPath(targetPath)) {

                    string handle = _CreateShadingNode(
                        bxdfPrim, currentTime,
                        nodesBuilder, interfaceBuilder, "prman");

                    terminalsBuilder.set("prmanBxdf",
                                            FnKat::StringAttribute(handle));
                } else {
                    FnLogWarn("Bxdf does not exist at "
                                << targetPath.GetString());
                }
            }
            else {
                FnLogWarn("No bxdf detected on look:" 
                        << materialPrim.GetPath());
            }
        }
    }


    // XXX, Because of relationship forwarding, there are possible name
    //      clashes with the standard prman shading.
    if (UsdRelationship bxdfRel =
        materialPrim.GetRelationship(UsdHydraTokens->displayLookBxdf))
    {
        if (!PxrUsdKatana_AreRelTargetsFromBaseMaterial(bxdfRel)) {
            SdfPathVector targetPaths;
            bxdfRel.GetForwardedTargets(&targetPaths);
            
            if (targetPaths.size() > 1) {
                FnLogWarn("Multiple displayLook bxdf detected on look:" << 
                    materialPrim.GetPath());
            }
            if (targetPaths.size() > 0) {
                const SdfPath targetPath = targetPaths[0];
                if (UsdPrim bxdfPrim =
                    stage->GetPrimAtPath(targetPath)) {
                    
                    string handle = _CreateShadingNode(
                        bxdfPrim, currentTime,
                        nodesBuilder, interfaceBuilder, "display");

                    terminalsBuilder.set("displayBxdf",
                                            FnKat::StringAttribute(handle));
                } else {
                    FnLogWarn("Bxdf does not exist at "
                                << targetPath.GetString());
                }
            }
        }
    }
    


    // with the current implementation of ris, there are
    // no patterns that are unbound or not connected directly
    // to bxdf's.

    // generate interface for materialPrim and also any "contiguous" scopes
    // that are we encounter.
    //
    // XXX: is this behavior unique to katana or do we stick this
    // into the schema?
    std::stack<UsdPrim> dfs;
    dfs.push(materialPrim);
    while (!dfs.empty()) {
        UsdPrim curr = dfs.top();
        dfs.pop();

        std::string paramPrefix;
        if (curr != materialPrim) {
            if (curr.IsA<UsdShadeShader>()) {
                // XXX: Because we're using a lookDerivesFrom
                // relationship instead of a USD composition construct,
                // we'll need to create every shading node instead of
                // relying on traversing the bxdf.
                // We can remove this once the "derives" usd composition
                // works, along with partial composition
                _CreateShadingNode(curr, currentTime,
                        nodesBuilder, interfaceBuilder, "prman");
            }

            if (!curr.IsA<UsdGeomScope>()) {
                continue;
            }

            paramPrefix = PxrUsdKatanaUtils::GenerateShadingNodeHandle(curr);
        }

        _UnrollInterfaceFromPrim(curr, 
                paramPrefix,
                materialBuilder,
                interfaceBuilder);

        TF_FOR_ALL(childIter, curr.GetChildren()) {
            dfs.push(*childIter);
        }
    }
    
    // Gather prman statements
    FnKat::GroupBuilder statementsBuilder;
    PxrUsdKatanaReadPrimPrmanStatements(materialPrim, currentTime, statementsBuilder);

    materialBuilder.set("nodes", nodesBuilder.build());
    materialBuilder.set("terminals", terminalsBuilder.build());
    materialBuilder.set("interface", interfaceBuilder.build());
    FnKat::GroupAttribute statements = statementsBuilder.build();
    if (statements.getNumberOfChildren()) {
        materialBuilder.set("underlayAttrs.prmanStatements", statements);
    }

    FnAttribute::GroupAttribute localMaterialAttr = materialBuilder.build();

    if (flatten) {
        // check for parent, and compose with it
        // XXX:
        // Eventually, this "derivesFrom" relationship will be
        // a "derives" composition in usd, in which case we'll have to
        // rewrite this to use partial usd composition
        //
        // Note that there are additional workarounds in using the
        // "derivesFrom"/BaseMaterial relationship in the non-op SGG that
        // would need to be replicated here if the USD Material AttributeFn
        // were to use the PxrUsdIn op instead, particularly with respect to
        // the tree structure that the non-op the SGG creates
        // See _ConvertUsdMAterialPathToKatLocation in
        // katanapkg/plugin/sgg/usd/utils.cpp
        
        if (materialSchema.HasBaseMaterial()) {
            SdfPath baseMaterialPath = materialSchema.GetBaseMaterialPath();
            if (UsdShadeMaterial baseMaterial = UsdShadeMaterial::Get(stage, baseMaterialPath)) {
                // Make a fake context to grab parent data, and recurse on that
                FnKat::GroupAttribute parentMaterial = _GetMaterialAttr(baseMaterial, currentTime, true);
                FnAttribute::GroupBuilder flatMaterialBuilder;
                flatMaterialBuilder.update(parentMaterial);
                flatMaterialBuilder.deepUpdate(localMaterialAttr);
                return flatMaterialBuilder.build();
            }
            else {
                FnLogError(TfStringPrintf("ERROR: Expected UsdShadeMaterial at %s\n",
                        baseMaterialPath.GetText()).c_str());
            }
        }
    }
    return localMaterialAttr;
}

/* static */ 
void
_UnrollInterfaceFromPrim(const UsdPrim& prim, 
        const std::string& paramPrefix,
        GroupBuilder& materialBuilder,
        GroupBuilder& interfaceBuilder)
{
    UsdStageRefPtr stage = prim.GetStage();


    // TODO: Right now, the exporter doesn't always move thing into
    // the right spot.  for example, we have "Paint_Base_Color" on
    // /PaintedMetal_Material.Paint_Base_Color
    // Which makes it so we can't use the riLookAPI.GetInterfaceAttributes
    // (because /PaintedMetal_Material.Paint_Base_Color doesn't have the
    // corresponding "ri" interface connection).
    //
    // that should really be on
    // /PaintedMetal_Material/Paint_.Base_Color  which does have that
    // connection.
    // 
    UsdShadeMaterial materialSchema(prim);
    std::vector<UsdShadeInterfaceAttribute> interfaceAttrs = materialSchema.GetInterfaceAttributes(TfToken());
    TF_FOR_ALL(interfaceAttrIter, interfaceAttrs) {
        UsdShadeInterfaceAttribute interfaceAttribute = *interfaceAttrIter;

        const TfToken& paramName = interfaceAttribute.GetName();
        const std::string renamedParam = paramPrefix + paramName.GetString();

        // handle parameters with values 
        VtValue attrVal;
        if (interfaceAttribute.Get(&attrVal) && !attrVal.IsEmpty()) {
            materialBuilder.set(
                    TfStringPrintf("parameters.%s", renamedParam.c_str()),
                    PxrUsdKatanaUtils::ConvertVtValueToKatAttr(attrVal, true));
        }

    }

    UsdRiLookAPI riLookAPI(prim);
    std::vector<UsdShadeInterfaceAttribute> riInterfaceAttrs = riLookAPI.GetInterfaceAttributes();
    TF_FOR_ALL(interfaceAttrIter, riInterfaceAttrs) {
        UsdShadeInterfaceAttribute interfaceAttribute = *interfaceAttrIter;

        const TfToken& paramName = interfaceAttribute.GetName();
        const std::string renamedParam = paramPrefix + paramName.GetString();

        // handle parameter connections
        std::vector<UsdShadeParameter> recipients = riLookAPI.GetInterfaceRecipientParameters(
                interfaceAttribute);
        TF_FOR_ALL(recipientIter, recipients) {
            UsdShadeParameter recipient = *recipientIter;

            UsdPrim recipientPrim = recipient.GetAttr().GetPrim();
            TfToken inputName = recipient.GetAttr().GetName();

            std::string handle = PxrUsdKatanaUtils::GenerateShadingNodeHandle(recipientPrim);

            std::string srcKey = renamedParam + ".src";

            std::string srcVal = TfStringPrintf(
                    "%s.%s",
                    handle.c_str(),
                    inputName.GetText());

            interfaceBuilder.set(
                    srcKey.c_str(),
                    FnKat::StringAttribute(srcVal),
                    true);
        }

        // USD's group delimeter is :, whereas Katana's is .
        std::string page = TfStringReplace(
                interfaceAttribute.GetDisplayGroup(), ":", ".");
        if (!page.empty()) {
            std::string pageKey = renamedParam + ".hints.page";
            interfaceBuilder.set(pageKey, FnKat::StringAttribute(page), true);
        }

        std::string doc = interfaceAttribute.GetDocumentation();
        if (!doc.empty()) {
            std::string docKey = renamedParam + ".hints.help";

            doc = TfStringReplace(doc, "'", "\"");
            doc = TfStringReplace(doc, "\n", "\\n");

            interfaceBuilder.set(docKey, FnKat::StringAttribute(doc), true);
        }
    }
}

