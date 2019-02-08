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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readMaterial.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/utils.h"
#include "usdKatana/baseMaterialHelpers.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/usd/usdGeom/scope.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/utils.h"

#include "pxr/usd/usdRi/materialAPI.h"
#include "pxr/usd/usdRi/risObject.h"
#include "pxr/usd/usdRi/risOslPattern.h"
#include "pxr/usd/usdRi/rslShader.h"
#include "pxr/usd/usdUI/nodeGraphNodeAPI.h"

#include <FnGeolibServices/FnAttributeFunctionUtil.h>
#include <FnLogging/FnLogging.h>
#include <pystring/pystring.h>

#include "pxr/usd/usdHydra/tokens.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE


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
        const std::string & targetName,
        bool flatten);

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
        const std::string& looksGroupLocation,
        const std::string& materialDestinationLocation)
{
    UsdPrim prim = material.GetPrim();
    UsdStageRefPtr stage = prim.GetStage();
    SdfPath primPath = prim.GetPath();
    std::string katanaPath = prim.GetName();

    // we do this before ReadPrim because ReadPrim calls ReadBlindData 
    // (primvars only) which we don't want to stomp here.
    attrs.set("material", _GetMaterialAttr(
        material, data.GetCurrentTime(), flatten));

    const std::string& parentPrefix = (looksGroupLocation.empty()) ?
        data.GetUsdInArgs()->GetRootLocationPath() : looksGroupLocation;
    
    std::string fullKatanaPath = !materialDestinationLocation.empty()
            ? materialDestinationLocation
            : PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
                    primPath, data);

    if (!fullKatanaPath.empty() &&
            pystring::startswith(fullKatanaPath, parentPrefix)) {
        katanaPath = fullKatanaPath.substr(parentPrefix.size()+1);

        // these paths are relative in katana
        if (!katanaPath.empty() && katanaPath[0] == '/') {
            katanaPath = katanaPath.substr(1);
        }
    }


    attrs.set("material.katanaPath", FnKat::StringAttribute(katanaPath));
    attrs.set("material.usdPrimName", FnKat::StringAttribute(prim.GetName()));

    PxrUsdKatanaReadPrim(material.GetPrim(), data, attrs);

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
    const std::string & targetName,
    bool flatten)
{
    UsdPrim prim = shaderSchema.GetPrim();
    string primName = prim.GetName();

    std::vector<UsdShadeInput> shaderInputs = shaderSchema.GetInputs();
    TF_FOR_ALL(shaderInputIter, shaderInputs) {
        UsdShadeInput shaderInput = *shaderInputIter;
        std::string inputId = shaderInput.GetBaseName();

        // We do not try to extract presentation metadata from parameters -
        // only material interface attributes should bother recording such.

        // We can have multiple incoming connection, we get a whole set of paths
        SdfPathVector sourcePaths;
        if (UsdShadeConnectableAPI::GetRawConnectedSourcePaths(
                shaderInput, &sourcePaths)) {

            bool multipleConnections = sourcePaths.size() > 1;

            // Check the relationship(s) representing this connection to see if
            // the targets come from a base material. If so, ignore them.
            bool createConnections =
                    flatten ||
                    !UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial(
                            shaderInput);

            int connectionIdx = 0;
            for (const SdfPath& sourcePath : sourcePaths) {

                // We only care about connections to output properties
                if (not sourcePath.IsPropertyPath())
                    continue;

                UsdShadeConnectableAPI source =
                        UsdShadeConnectableAPI::Get(prim.GetStage(),
                                                    sourcePath.GetPrimPath());
                if (not static_cast<bool>(source))
                    continue;

                TfToken sourceName;
                UsdShadeAttributeType sourceType;
                std::tie(sourceName, sourceType) =
                        UsdShadeUtils::GetBaseNameAndType(
                                sourcePath.GetNameToken());

                if (sourceType != UsdShadeAttributeType::Output)
                    continue;

                std::string targetHandle = _CreateShadingNode(
                        source.GetPrim(),
                        currentTime,
                        nodesBuilder,
                        interfaceBuilder,
                        targetName,
                        flatten);

                if (createConnections) {
                    // These targets are local, so include them.
                    string connAttrName = inputId;

                    // In the case of multiple input connections for array
                    // types, we append a ":idx" to the name
                    if (multipleConnections) {
                        connAttrName += ":" + std::to_string(connectionIdx);
                        connectionIdx++;
                    }

                    connectionsBuilder.set(
                        connAttrName,
                        FnKat::StringAttribute(
                            sourceName.GetString() + "@" + targetHandle));
                }
            }

        } else {
            // This input may author an opinion which blocks connections (eg, a
            // connection from a base material). A blocked connection manifests
            // as an authored connection, but no connections can be determined.
            UsdAttribute inputAttr = shaderInput.GetAttr();
            bool hasAuthoredConnections = inputAttr.HasAuthoredConnections();
            SdfPathVector conns;
            inputAttr.GetConnections(&conns);

            // Use a NullAttribute to capture the block
            if (hasAuthoredConnections and conns.empty()) {
                connectionsBuilder.set(inputId, FnKat::NullAttribute());
            }
        }

        // produce the value here and let katana handle the connection part
        // correctly..
        UsdAttribute attr = shaderInput.GetAttr();
        VtValue vtValue;
        if (!attr.Get(&vtValue, currentTime)) {
            continue;
        }

        // If the attribute value comes from a base material, leave it
        // empty -- we will inherit it from the parent katana material.
        if (flatten || 
            !PxrUsdKatana_IsAttrValFromBaseMaterial(attr)) {
            paramsBuilder.set(inputId,
                    PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue));
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
                        PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue));
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
        const std::string & targetName,
        bool flatten)
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

    if (UsdShadeShader shaderSchema = UsdShadeShader(shadingNode)) {
        validData = true;
        SdfAssetPath fileAssetPath;

        // only use the fallback OSL test if the targetName is "prman" as
        // it will issue benign but confusing errors to the shell for
        // display shaders
        if (targetName == "prman")
        {
            TfToken id;
            shaderSchema.GetIdAttr().Get(&id, currentTime);
            std::string oslIdString = id.GetString();
            
            if (!pystring::endswith(oslIdString, ".oso"))
            {
                oslIdString = "osl:" + oslIdString;
            }
            
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

        GroupBuilder paramsBuilder;
        GroupBuilder connectionsBuilder;

        _GatherShadingParameters(shaderSchema, handle, currentTime,
            nodesBuilder, paramsBuilder, 
            interfaceBuilder, connectionsBuilder, targetName, flatten);


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

    if (validData) {
        if (flatten || 
            !PxrUsdKatana_IsPrimDefFromBaseMaterial(shadingNode)) {
            shdNodeAttr.set("name", FnKat::StringAttribute(handle));
            shdNodeAttr.set("srcName", FnKat::StringAttribute(handle));
            shdNodeAttr.set("target", FnKat::StringAttribute(targetName));
        }
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
    UsdRiMaterialAPI riMaterialAPI(materialPrim);
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
    UsdShadeShader surfaceShader = riMaterialAPI.GetSurface(
            /*ignoreBaseMaterial*/ not flatten);
    if (surfaceShader.GetPrim()) {
        std::string handle = _CreateShadingNode(
            surfaceShader.GetPrim(), currentTime,
            nodesBuilder, interfaceBuilder, "prman", flatten);
    
        // If the source shader type is an RslShader, then publish it 
        // as a prmanSurface terminal. If not, fallback to the 
        // prmanBxdf terminal.
        UsdRiRslShader rslShader(surfaceShader.GetPrim());
        if (rslShader) {
            terminalsBuilder.set("prmanSurface",
                                 FnKat::StringAttribute(handle));
        } else {
            terminalsBuilder.set("prmanBxdf",
                                 FnKat::StringAttribute(handle));
        }
    }

    // look for displacement
    UsdShadeShader displacementShader = riMaterialAPI.GetDisplacement(
            /*ignoreBaseMaterial*/ not flatten);
    if (displacementShader.GetPrim()) {
        string handle = _CreateShadingNode(
            displacementShader.GetPrim(), currentTime,
            nodesBuilder, interfaceBuilder, "prman", flatten);
        terminalsBuilder.set("prmanDisplacement",
                             FnKat::StringAttribute(handle));
    }

    // look for coshaders
    // XXX: Can we simply delete this section?
    // coshaders should not be used anywhere.
    if (UsdRelationship coshadersRel = 
            materialPrim.GetRelationship(TfToken("riLook:coshaders"))) {
        if (flatten ||
                !PxrUsdKatana_AreRelTargetsFromBaseMaterial(coshadersRel)) {
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
                            nodesBuilder, interfaceBuilder, "prman", flatten);

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

    // XXX BEGIN This code is in support of Subgraph workflows
    //           and is currently necessary to match equivalent SGG behavior
     
    // Look for labeled patterns - TODO: replace with UsdShade::ShadingSubgraph
    vector<UsdProperty> properties = 
        materialPrim.GetPropertiesInNamespace("patternTerminal");
    if (properties.size()) {
        TF_FOR_ALL(propIter, properties) {
            // if (propIter->Is<UsdRelationship>()) {
            UsdRelationship rel = propIter->As<UsdRelationship>();
            if (not rel) {
                continue;
            }

            SdfPathVector targetPaths;
            rel.GetForwardedTargets(&targetPaths);
            if (targetPaths.size() == 0) {
                continue;
            }
            if (targetPaths.size() > 1) {
                FnLogWarn(
                    "Multiple targets for one output port detected on look:" << 
                    materialPrim.GetPath());
            }

            const SdfPath targetPath = targetPaths[0];
            if (not targetPath.IsPropertyPath()) {
                FnLogWarn("Pattern wants a usd property path, not a prim: "
                    << targetPath.GetString());
                continue;
            }
            
            SdfPath nodePath = targetPath.GetPrimPath();
    
            if (UsdPrim patternPrim =
                    stage->GetPrimAtPath(nodePath)) {

                string propertyName = targetPath.GetName();
                string patternPort = propertyName.substr(
                    propertyName.find(':')+1);

                string terminalName = rel.GetName();
                terminalName = terminalName.substr(terminalName.find(':')+1);
    
                string handle = _CreateShadingNode(
                    patternPrim, currentTime, nodesBuilder,
                            interfaceBuilder, "prman", flatten);
                terminalsBuilder.set("prmanCustom_"+terminalName,
                    FnKat::StringAttribute(handle));
                terminalsBuilder.set("prmanCustom_"+terminalName+"Port",
                    FnKat::StringAttribute(patternPort));
            } 
            else {
                FnLogWarn("Pattern does not exist at "
                            << targetPath.GetString());
            }
        }
    }
    // XXX END
    
    bool foundGlslfxTerminal = false;
    if (UsdShadeOutput glslfxOut = materialSchema.GetSurfaceOutput(
                HioGlslfxTokens->glslfx)) {
        if (flatten || 
            !glslfxOut.IsSourceConnectionFromBaseMaterial()) 
        {
            UsdShadeConnectableAPI source;
            TfToken sourceName;
            UsdShadeAttributeType sourceType;
            if (glslfxOut.GetConnectedSource(&source, &sourceName, 
                                                &sourceType)) {
                foundGlslfxTerminal = true;
                string handle = _CreateShadingNode(
                    source.GetPrim(), currentTime,
                    nodesBuilder, interfaceBuilder, "display", flatten);

                terminalsBuilder.set("displayBxdf",
                                        FnKat::StringAttribute(handle));
            }                                    
        }
    }

    // XXX: This code is deprecated and should be removed soon, along with all 
    // other uses of the deprecated usdHydra API.
    // 
    // XXX, Because of relationship forwarding, there are possible name
    //      clashes with the standard prman shading.
    if (!foundGlslfxTerminal) {
        if (UsdRelationship bxdfRel = materialPrim.GetRelationship(
                    UsdHydraTokens->displayLookBxdf)) {
            if (flatten ||
                    !PxrUsdKatana_AreRelTargetsFromBaseMaterial(bxdfRel)) {
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
                            nodesBuilder, interfaceBuilder, "display", flatten);

                        terminalsBuilder.set("displayBxdf",
                                                FnKat::StringAttribute(handle));
                    } else {
                        FnLogWarn("Bxdf does not exist at "
                                    << targetPath.GetString());
                    }
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
                        nodesBuilder, interfaceBuilder, "prman", flatten);
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
    // Which makes it so we can't use the materialSchema.GetInterfaceInputs()
    // (because /PaintedMetal_Material.Paint_Base_Color doesn't have the
    // corresponding "ri" interfaceInput connection).
    //
    // that should really be on
    // /PaintedMetal_Material/Paint_.Base_Color  which does have that
    // connection.
    // 
    UsdShadeMaterial materialSchema(prim);
    std::vector<UsdShadeInput> interfaceInputs = 
        materialSchema.GetInterfaceInputs();
    UsdShadeNodeGraph::InterfaceInputConsumersMap interfaceInputConsumers =
        materialSchema.ComputeInterfaceInputConsumersMap(
            /*computeTransitiveMapping*/ true);

    TF_FOR_ALL(interfaceInputIter, interfaceInputs) {
        UsdShadeInput interfaceInput = *interfaceInputIter;

        // Skip invalid interface inputs.
        if (!interfaceInput.GetAttr()) { 
            continue;
        }

        const TfToken& paramName = interfaceInput.GetBaseName();
        const std::string renamedParam = paramPrefix + paramName.GetString();

        // handle parameters with values 
        VtValue attrVal;
        if (interfaceInput.GetAttr().Get(&attrVal) && !attrVal.IsEmpty()) {
            materialBuilder.set(
                    TfStringPrintf("parameters.%s", renamedParam.c_str()),
                    PxrUsdKatanaUtils::ConvertVtValueToKatAttr(attrVal));
        }

        if (interfaceInputConsumers.count(interfaceInput) == 0) {
            continue;
        }
            
        const std::vector<UsdShadeInput> &consumers = 
            interfaceInputConsumers.at(interfaceInput);

        for (const UsdShadeInput &consumer : consumers) {
            UsdPrim consumerPrim = consumer.GetPrim();
            
            TfToken inputName = consumer.GetBaseName();

            std::string handle = PxrUsdKatanaUtils::GenerateShadingNodeHandle(
                consumerPrim);

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

        // USD's group delimiter is :, whereas Katana's is .
        std::string page = TfStringReplace(
                interfaceInput.GetDisplayGroup(), ":", ".");
        if (!page.empty()) {
            std::string pageKey = renamedParam + ".hints.page";
            interfaceBuilder.set(pageKey, FnKat::StringAttribute(page), true);
        }

        std::string doc = interfaceInput.GetDocumentation();
        if (!doc.empty()) {
            std::string docKey = renamedParam + ".hints.help";

            doc = TfStringReplace(doc, "'", "\"");
            doc = TfStringReplace(doc, "\n", "\\n");

            interfaceBuilder.set(docKey, FnKat::StringAttribute(doc), true);
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

