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
#include <FnGeolibServices/FnBuiltInOpArgsUtil.h>

#include "usdKatana/blindDataObject.h"
#include "usdKatana/cache.h"
#include "usdKatana/locks.h"
#include "usdKatana/readBlindData.h"
#include "usdKatana/usdInPluginRegistry.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdShade/material.h"

#include <FnGeolib/op/FnGeolibOp.h>
#include <FnLogging/FnLogging.h>

#include "usdKatana/utils.h"

// just for info
#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/base/tf/pathUtils.h"

#include <pystring/pystring.h>
#include <stdio.h>

#include <sstream>

FnLogSetup("PxrUsdIn")

namespace FnKat = Foundry::Katana;

// convenience macro to report an error.
#define ERROR(...)\
    interface.setAttr("type", Foundry::Katana::StringAttribute("error"));\
    interface.setAttr("errorMesssage", Foundry::Katana::StringAttribute(TfStringPrintf(__VA_ARGS__)));

// Give these types some shorter names.
typedef PxrUsdKatanaUsdInPrivateData::MaterialHierarchy MaterialHierarchy;
typedef std::shared_ptr<const MaterialHierarchy> MaterialHierarchyPtr;

// Helper to scan the immediate children of the given parentPrim for usd
// materials, analyze their specializes arcs, and assemble the hierarchy.
//
// Note that this only scans a set of sibling prims -- we don't (say)
// go deeply scan the entire scene here.  Instead, we rely on katana
// traversal to discover and build up the material hierarchy as we go,
// using a copy-on-write approach to share work across ops evaluating
// different branches of the scene.
//
static void
_UpdateMaterialHierarchyForChildPrims(const UsdPrim &parentPrim,
                                      MaterialHierarchyPtr &materials)
{
    const UsdStageWeakPtr stage = parentPrim.GetStage();

    // Locally accumulate any new material hierarchy we discover.
    MaterialHierarchy local;

    // Scan immediate children prims for materials that directly
    // specialize other materials.
    for (const auto& childPrim : parentPrim.GetChildren()) {
        // If childPrim is not a material, skip it.
        if (!UsdShadeMaterial(childPrim)) {
            continue;
        }
        // Check if childPrim has an immediate specializes arc,
        // i.e. one right below the root of the prim composition.
        //
        // If there are specializes arcs deeper in the tree,
        // we assume it is a library material that has been
        // referenced in, and we do not expose the base material
        // separately.
        //
        const PcpPrimIndex &index = childPrim.GetPrimIndex();
        for(const PcpNodeRef &node: index.GetNodeRange()) {
            if (PcpIsSpecializesArc(node.GetArcType())
                && node.GetOriginNode() == index.GetRootNode()) {
                // Found a root specializes arc.
                const SdfPath derivedPath = childPrim.GetPath();
                const SdfPath basePath = node.GetPath();
                const UsdPrim basePrim = stage->GetPrimAtPath(basePath);
                // If the base is anything but a material, ignore it.
                if (!UsdShadeMaterial(basePrim)) {
                    continue;
                }
                // The childPrim material derives from the basePrim material.
                // Link them into the hierarchy.
                local.baseMaterialPath[derivedPath] = basePath;
                local.derivedMaterialPaths[basePath].push_back(derivedPath);
            }
        }
    }

    // If we discovered any new materials, merge them into the existing
    // hierarchy of materials.  To do this we fork a new clone of the
    // material hierarchy to pass down the katana op chain, which is
    // why we defer doing this until we are certain we have new materials.
    if (!local.baseMaterialPath.empty()) {
        for (const auto &pair: materials->baseMaterialPath) {
            local.baseMaterialPath.insert(pair);
        }
        for (const auto &pair: materials->derivedMaterialPaths) {
            // Maintain order: insert the existing derived materials
            // at front of the vector.
            std::vector<SdfPath> &vec = local.derivedMaterialPaths[pair.first];
            vec.insert(vec.begin(), pair.second.begin(), pair.second.end());
        }
        // Update pointer to own the newly modifed copy of the hierarchy.
        materials.reset(new MaterialHierarchy(local));
    }
}

// see overview.dox for more documentation.
class PxrUsdInOp : public FnKat::GeolibOp
{

public:

    static void setup(FnKat::GeolibSetupInterface &interface)
    {
        // Tell katana that it's safe to run this op in a runtime concurrently
        // with other runtimes.
        interface.setThreading(
                FnKat::GeolibSetupInterface::ThreadModeConcurrent);

        _hasSiteKinds = PxrUsdKatanaUsdInPluginRegistry::HasKindsForSite();
    }

    static void cook(FnKat::GeolibCookInterface &interface)
    {
        boost::shared_lock<boost::upgrade_mutex> 
            readerLock(UsdKatanaGetStageLock());


        PxrUsdKatanaUsdInPrivateData* privateData = 
            static_cast<PxrUsdKatanaUsdInPrivateData*>(interface.getPrivateData());

        FnKat::GroupAttribute opArgs = interface.getOpArg();
        
        // Get usdInArgs.
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs;
        if (privateData) {
            usdInArgs = privateData->GetUsdInArgs();
        } else {
            FnKat::GroupAttribute additionalOpArgs;
            usdInArgs = InitUsdInArgs(interface, additionalOpArgs);
            opArgs = FnKat::GroupBuilder()
                .update(opArgs)
                .deepUpdate(additionalOpArgs)
                .build();
        }
        // Validate usdInArgs.
        if (!usdInArgs) {
            ERROR("Could not initialize PxrUsdIn usdInArgs.");
            return;
        }
        
        if (!usdInArgs->GetErrorMessage().empty())
        {
            ERROR(usdInArgs->GetErrorMessage().c_str());
            return;
        }

        // Get usd prim.
        UsdPrim prim = interface.atRoot()
            ? usdInArgs->GetRootPrim()
            : privateData->GetUsdPrim();
        // Validate usd prim.
        if (!prim) {
            ERROR("No prim at %s",
                  interface.getRelativeOutputLocationPath().c_str());
            return;
        }

        // Update material hierarchy.
        MaterialHierarchyPtr materialHierarchy;
        if (privateData) {
            // Continue using the hierarchy provided by the parent op.
            materialHierarchy = privateData->GetMaterialHierarchy();
        }
        if (!materialHierarchy) {
            // Allocate an empty material hierarchy.
            materialHierarchy.reset(new MaterialHierarchy);
        }
        _UpdateMaterialHierarchyForChildPrims(prim, materialHierarchy);

        // A flag to force the use of default motion samples. This is only set
        // when at root and an isolate path is specified. In this case, we must
        // check whether the isolated path starts with any of the user-specified
        // paths that should use the default motion sample times.
        //
        bool useDefaultMotion = false;

        if (interface.atRoot()) {
            interface.stopChildTraversal();

            if (!usdInArgs->GetIsolatePath().empty())
            {
                useDefaultMotion = GetDefaultMotionAtRoot(usdInArgs);
            }

            // XXX This info currently gets used to determine whether
            // to correctively rotate cameras. The camera's zUp needs to be
            // recorded until we have no more USD z-Up assets and the katana
            // assets have no more prerotate camera nodes.
            interface.setAttr("info.usd.stageIsZup",
                    FnKat::IntAttribute(UsdUtilsGetCamerasAreZup(usdInArgs->GetStage())));

            // Construct the global camera list at the USD scene root.
            //
            FnKat::StringBuilder cameraListBuilder;

            SdfPathVector cameraPaths = PxrUsdKatanaUtils::FindCameraPaths(prim.GetStage());

            TF_FOR_ALL(cameraPathIt, cameraPaths)
            {
                const std::string path = (*cameraPathIt).GetString();

                // only add cameras to the camera list that are beneath the isolate prim path
                if (path.find(usdInArgs->GetIsolatePath()) != std::string::npos)
                {
                    cameraListBuilder.push_back(
                        TfNormPath(usdInArgs->GetRootLocationPath()+"/"+
                            path.substr(usdInArgs->GetIsolatePath().size())));
                }
            }

            FnKat::StringAttribute cameraListAttr = cameraListBuilder.build();
            if (cameraListAttr.getNumberOfValues() > 0)
            {
                interface.setAttr("cameraList", cameraListAttr);
            }
            
            interface.setAttr("info.usdOpArgs", opArgs);
            
            FnKat::GroupAttribute masterMapping =
                    opArgs.getChildByName("masterMapping");
            if (masterMapping.isValid() && masterMapping.getNumberOfChildren())
            {
                FnGeolibServices::StaticSceneCreateOpArgsBuilder sscb(false);
                
                
                
                for (size_t i = 0, e = masterMapping.getNumberOfChildren();
                        i != e; ++i)
                {
                    std::string masterName = FnKat::DelimiterDecode(
                            masterMapping.getChildName(i));
                    
                    std::string katanaPath =  FnKat::StringAttribute(
                            masterMapping.getChildByIndex(i)
                                    ).getValue("", false);
                    
                    if (katanaPath.empty())
                    {
                        continue;
                    }
                    
                    sscb.createEmptyLocation(katanaPath, "instance source");
                    sscb.setAttrAtLocation(katanaPath,
                            "tabs.scenegraph.stopExpand", FnKat::IntAttribute(1));
                    sscb.setAttrAtLocation(katanaPath, "childPrimPath",
                            FnKat::StringAttribute(masterName));
                }
                
                interface.createChild(
                        "Masters",
                        "PxrUsdIn.MasterIntermediate",
                        FnKat::GroupBuilder()
                            .update(opArgs)
                            .set("staticScene", sscb.build())
                            .build(),
                        FnKat::GeolibCookInterface::ResetRootFalse,
                        new PxrUsdKatanaUsdInPrivateData(
                                usdInArgs->GetRootPrim(),
                                usdInArgs, privateData, useDefaultMotion,
                                &materialHierarchy),
                        PxrUsdKatanaUsdInPrivateData::Delete);
            }
        }

        bool verbose = usdInArgs->IsVerbose();

        if (!prim.IsLoaded()) {
            SdfPath pathToLoad = prim.GetPath();
            UsdStageRefPtr stage = prim.GetStage();
            readerLock.unlock();
            prim = _LoadPrim(stage, pathToLoad, verbose);
            if (!prim) {
                ERROR("load prim %s failed", pathToLoad.GetText());
                return;
            }
            readerLock.lock();
        }

        //
        // Compute and set the 'bound' attribute.
        //
        // Note, bound computation is handled here because bounding
        // box computation requires caching for optimal performance.
        // Instead of passing around a bounding box cache everywhere
        // it's needed, we use the usdInArgs data strucutre for caching.
        //

        if (PxrUsdKatanaUtils::IsBoundable(prim)) {
            interface.setAttr("bound",
                _MakeBoundsAttribute(prim, usdInArgs));
        }

        //
        // Find and execute the core op that handles the USD type.
        //

        {
            std::string opName;
            if (PxrUsdKatanaUsdInPluginRegistry::FindUsdType(
                    prim.GetTypeName(), &opName)) {
                if (!opName.empty()) {
                    interface.execOp(opName, opArgs);
                }
            }
        }

        //
        // Find and execute the site-specific op that handles the USD type.
        //

        {
            std::string opName;
            if (PxrUsdKatanaUsdInPluginRegistry::FindUsdTypeForSite(
                    prim.GetTypeName(), &opName)) {
                if (!opName.empty()) {
                    interface.execOp(opName, opArgs);
                }
            }
        }

        //
        // Find and execute the core kind op that handles the model kind.
        //

        bool execKindOp = FnKat::IntAttribute(
                interface.getOutputAttr("__UsdIn.execKindOp")).getValue(1, false);

        if (execKindOp)
        {
            TfToken kind;
            if (UsdModelAPI(prim).GetKind(&kind)) {
                std::string opName;
                if (PxrUsdKatanaUsdInPluginRegistry::FindKind(kind, &opName)) {
                    if (!opName.empty()) {
                        interface.execOp(opName, opArgs);
                    }
                }
            }
        }

        //
        // Find and execute the site-specific kind op that handles the model kind.
        //

        if (_hasSiteKinds) {
            TfToken kind;
            if (UsdModelAPI(prim).GetKind(&kind)) {
                std::string opName;
                if (PxrUsdKatanaUsdInPluginRegistry::FindKindForSite(kind, &opName)) {
                    if (!opName.empty()) {
                        interface.execOp(opName, opArgs);
                    }
                }
            }
        }

        //
        // Read blind data. This is last because blind data opinions should always win.
        //

        PxrUsdKatanaAttrMap attrs;
        PxrUsdKatanaReadBlindData(UsdKatanaBlindDataObject(prim), attrs);
        attrs.toInterface(interface);

        bool skipAllChildren = FnKat::IntAttribute(
                interface.getOutputAttr("__UsdIn.skipAllChildren")).getValue(0, false);

        if (prim.IsInstance())
        {
            UsdPrim master = prim.GetMaster();
            interface.setAttr("info.usd.masterPrimPath",
                    FnAttribute::StringAttribute(master.GetPrimPath().GetString()));
            
            
            FnAttribute::StringAttribute masterPathAttr = 
                    opArgs.getChildByName("masterMapping." +
                            FnKat::DelimiterEncode(master.GetPrimPath().GetString()));
            if (masterPathAttr.isValid())
            {
                std::string masterPath = masterPathAttr.getValue("", false);
                
                if (!masterPath.empty())
                {
                    interface.setAttr(
                            "type", FnKat::StringAttribute("instance"));
                    interface.setAttr("geometry.instanceSource",
                            FnAttribute::StringAttribute(
                                usdInArgs->GetRootLocationPath() + "/Masters/" + masterPath));
                    
                    // XXX, ConstraintGroups are still made for models that became
                    //      instances. Need to suppress creation of that stuff
                    interface.deleteChildren();
                    skipAllChildren = true;
                }
            }
            
            
        }
        
        
        // advertise available variants for UIs to choose amongst
        UsdVariantSets variantSets = prim.GetVariantSets();
        std::vector<std::string> variantNames;
        std::vector<std::string> variantValues;
        variantSets.GetNames(&variantNames);
        TF_FOR_ALL(I, variantNames)
        {
            const std::string & variantName = (*I);
            UsdVariantSet variantSet = variantSets.GetVariantSet(variantName);
            variantValues = variantSet.GetVariantNames();
            
            interface.setAttr("info.usd.variants." + variantName,
                    FnAttribute::StringAttribute(variantValues, 1));
            
            interface.setAttr("info.usd.selectedVariants." + variantName,
                    FnAttribute::StringAttribute(
                            variantSet.GetVariantSelection()));
            
        }

        // Usd Material Hierarchy:  If this is a base material,
        // splice the derived materials in as children.
        if (UsdShadeMaterial(prim)) {
            std::map<SdfPath, std::vector<SdfPath>>::const_iterator i =
                materialHierarchy->derivedMaterialPaths.find(prim.GetPath());
            if (i != materialHierarchy->derivedMaterialPaths.end()) {
                // We found some derived materials.
                const std::vector<SdfPath> &derivedMaterialPaths = i->second;
                for (const SdfPath &derivedMaterialPath: derivedMaterialPaths) {
                    UsdPrim derivedMaterial =
                        prim.GetStage()->GetPrimAtPath(derivedMaterialPath);
                    if (!TF_VERIFY(derivedMaterial)) {
                        // This shouldn't happen: we confirmed the prim
                        // exists when building the hierarchy.
                        continue;
                    }
                    const std::string& childName = derivedMaterial.GetName();
                    interface.createChild(
                            childName,
                            "",
                            opArgs,
                            FnKat::GeolibCookInterface::ResetRootFalse,
                            new PxrUsdKatanaUsdInPrivateData(
                                    derivedMaterial, usdInArgs,
                                    privateData, useDefaultMotion,
                                    &materialHierarchy),
                            PxrUsdKatanaUsdInPrivateData::Delete);
                        }
            }
        }

        if (!skipAllChildren) {

            std::set<std::string> childrenToSkip;
            FnKat::GroupAttribute childOps = interface.getOutputAttr("__UsdIn.skipChild");
            if (childOps.isValid()) {
                for (int64_t i = 0; i < childOps.getNumberOfChildren(); i++) {
                    std::string childName = childOps.getChildName(i);
                    bool shouldSkip = FnKat::IntAttribute(childOps.getChildByIndex(i)).getValue(0, false);
                    if (shouldSkip) {
                        childrenToSkip.insert(childName);
                    }
                }
            }

            // If the prim is an instance (has a valid master path)
            // we replace the current prim with the master prim before 
            // iterating on the children.
            //
            if (prim.IsInstance() && !privateData->GetMasterPath().IsEmpty())
            {
                const UsdPrim& masterPrim = prim.GetMaster();
                if (!masterPrim)
                {
                    ERROR("USD Prim is advertised as an instance but master prim cannot be found.");
                }
                else
                {
                    prim = masterPrim;
                }
            }

            // create children
            TF_FOR_ALL(childIter, prim.GetFilteredChildren(UsdPrimIsDefined && UsdPrimIsActive && !UsdPrimIsAbstract))
            {
                const UsdPrim& child = *childIter;
                const std::string& childName = child.GetName();

                if (childrenToSkip.count(childName)) {
                    continue;
                }

                // Usd Material Hierarchy:  If this child is a derived
                // material, we do not need to handle it here.  We will
                // splice it below the correct base/parent material above. 
                if (materialHierarchy->baseMaterialPath.find(child.GetPath())
                    != materialHierarchy->baseMaterialPath.end()) {
                    continue;
                }

                interface.createChild(
                        childName,
                        "",
                        opArgs,
                        FnKat::GeolibCookInterface::ResetRootFalse,
                        new PxrUsdKatanaUsdInPrivateData(
                                child, usdInArgs,
                                privateData, useDefaultMotion,
                                &materialHierarchy),
                        PxrUsdKatanaUsdInPrivateData::Delete);
            }
        }

        // keep things around if we are verbose
        if (!verbose) {
            interface.deleteAttr("__UsdIn");
        }
    }

    
    // utility to make it easier to exit earlier from InitUsdInArgs
    struct ArgsBuilder
    {
        UsdStageRefPtr stage;
        std::string rootLocation;
        std::string isolatePath;
        FnAttribute::GroupAttribute sessionAttr;
        std::string ignoreLayerRegex;
        double currentTime;
        double shutterOpen;
        double shutterClose;
        std::vector<double> motionSampleTimes;
        std::set<std::string> defaultMotionPaths;
        PxrUsdKatanaUsdInArgs::StringListMap extraAttributesOrNamespaces;
        bool verbose;
        const char * errorMessage;
        
        
        ArgsBuilder()
        : currentTime(0.0)
        , shutterOpen(0.0)
        , shutterClose(0.0)
        , verbose(false)
        , errorMessage(0)
        {
        }
        
        PxrUsdKatanaUsdInArgsRefPtr build()
        {
            return PxrUsdKatanaUsdInArgs::New(
                stage,
                rootLocation,
                isolatePath,
                sessionAttr.isValid() ? sessionAttr :
                        FnAttribute::GroupAttribute(true),
                ignoreLayerRegex,
                currentTime,
                shutterOpen,
                shutterClose,
                motionSampleTimes,
                defaultMotionPaths,
                extraAttributesOrNamespaces,
                verbose,
                errorMessage);
        }
        
        PxrUsdKatanaUsdInArgsRefPtr buildWithError(std::string errorStr)
        {
            errorMessage = errorStr.c_str();
            return build();
        }
        
    };
    

    static PxrUsdKatanaUsdInArgsRefPtr
    InitUsdInArgs(const FnKat::GeolibCookInterface &interface,
            FnKat::GroupAttribute & additionalOpArgs)
    {
        ArgsBuilder ab;
        
        FnKat::StringAttribute usdFileAttr = interface.getOpArg("fileName");
        if (!usdFileAttr.isValid()) {
            return ab.buildWithError("PxrUsdIn: USD fileName not specified.");
        }

        std::string fileName = usdFileAttr.getValue();
        
        ab.rootLocation = FnKat::StringAttribute(
                interface.getOpArg("location")).getValue(
                        interface.getRootLocationPath(), false);
        
        
        std::string sessionLocation = ab.rootLocation;
        FnKat::StringAttribute sessionLocationAttr = 
                interface.getOpArg("sessionLocation");
        if (sessionLocationAttr.isValid()) {
            sessionLocation = sessionLocationAttr.getValue();
        }
        
        FnAttribute::GroupAttribute sessionAttr = interface.getOpArg("session");


        
        
        // XXX BEGIN convert the legacy variant string to the session
        // TODO: decide how long to do this as this form has been deprecated
        //       for some time but may still be present in secondary uses
        FnAttribute::GroupBuilder legacyVariantsGb;
        
        std::string variants = FnKat::StringAttribute(
                interface.getOpArg("variants")).getValue("", false);
        std::set<std::string> selStrings = TfStringTokenizeToSet(variants);
        TF_FOR_ALL(selString, selStrings) {
            std::string errMsg;
            if (SdfPath::IsValidPathString(*selString, &errMsg)) {
                SdfPath varSelPath(*selString);
                if (varSelPath.IsPrimVariantSelectionPath()) {
                    
                    std::string entryPath = FnAttribute::DelimiterEncode(
                            sessionLocation + varSelPath.GetPrimPath().GetString());
                    std::pair<std::string, std::string> sel =
                            varSelPath.GetVariantSelection();
                    
                    legacyVariantsGb.set(entryPath + "." + sel.first,
                            FnAttribute::StringAttribute(sel.second));
                    continue;
                }
            }
            
            return ab.buildWithError(
                    TfStringPrintf("PxrUsdIn: Bad variant selection \"%s\"",
                            selString->c_str()).c_str());
        }
        
        FnAttribute::GroupAttribute legacyVariants = legacyVariantsGb.build();
        
        if (legacyVariants.getNumberOfChildren() > 0)
        {
            sessionAttr = FnAttribute::GroupBuilder()
                .set("variants", legacyVariants)
                .deepUpdate(sessionAttr)
                .build();
        }
        // XXX END

        ab.sessionAttr = sessionAttr;

        ab.ignoreLayerRegex = FnKat::StringAttribute(
                interface.getOpArg("ignoreLayerRegex")).getValue("", false);

        ab.verbose = FnKat::IntAttribute(
                interface.getOpArg("verbose")).getValue(0, false);

        FnKat::GroupAttribute systemArgs(interface.getOpArg("system"));

        ab.currentTime = 
            FnKat::FloatAttribute(systemArgs.getChildByName(
                "timeSlice.currentTime")).getValue(0, false);

        int numSamples = 
            FnKat::IntAttribute(systemArgs.getChildByName(
                "timeSlice.numSamples")).getValue(1, false);

        ab.shutterOpen =
            FnKat::FloatAttribute(systemArgs.getChildByName(
                "timeSlice.shutterOpen")).getValue(0, false);

        ab.shutterClose =
            FnKat::FloatAttribute(systemArgs.getChildByName(
                "timeSlice.shutterClose")).getValue(0, false);

        std::string motionSampleStr = FnKat::StringAttribute(
                interface.getOpArg("motionSampleTimes")).getValue("", false);

        // If motion samples was specified, convert the string of values
        // into a vector of doubles to store with the root args.
        //
        if (numSamples < 2 || motionSampleStr.empty())
        {
            ab.motionSampleTimes.push_back(0);
        }
        else
        {
            std::vector<std::string> tokens;
            pystring::split(motionSampleStr, tokens, " ");

            for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
            {
                ab.motionSampleTimes.push_back(std::stod(*it));
            }
        }

        // Build the set of paths which should use default motion sample times.
        //
        FnAttribute::GroupAttribute defaultMotionPathsAttr =
                sessionAttr.getChildByName("defaultMotionPaths");
        if (defaultMotionPathsAttr.isValid())
        {
            for (size_t i = 0, e = defaultMotionPathsAttr.getNumberOfChildren();
                    i != e; ++i)
            {
                ab.defaultMotionPaths.insert(pystring::replace(
                    FnAttribute::DelimiterDecode(defaultMotionPathsAttr.getChildName(i)),
                        sessionLocation, "", 1));
            }
        }

        ab.stage =  UsdKatanaCache::GetInstance().GetStage(
                fileName, 
                sessionAttr, sessionLocation,
                ab.ignoreLayerRegex, 
                true /* forcePopulate */);

        if (!ab.stage) {
            return ab.buildWithError("PxrUsdIn: USD Stage cannot be loaded.");
        }

        if (FnAttribute::StringAttribute(
                interface.getOpArg("instanceMode")
                        ).getValue("expanded", false) == "as sources and instances")
        {
            additionalOpArgs = FnKat::GroupAttribute("masterMapping",
                    PxrUsdKatanaUtils::BuildInstanceMasterMapping(ab.stage), true);
        }
        
        ab.isolatePath = FnKat::StringAttribute(
            interface.getOpArg("isolatePath")).getValue("", false);

        // if the specified isolatePath is not a valid prim, clear it out
        if (!ab.isolatePath.empty() && !ab.stage->GetPrimAtPath(SdfPath(ab.isolatePath)))
        {
            std::ostringstream errorBuffer;
            errorBuffer << "PxrUsdIn: Invalid isolatePath: " << ab.isolatePath << ".";
            return ab.buildWithError(errorBuffer.str());
        }

        // get extra attributes or namespaces if they exist
        //
        FnKat::StringAttribute extraAttributesOrNamespacesAttr = 
            interface.getOpArg("extraAttributesOrNamespaces");

        if (extraAttributesOrNamespacesAttr.isValid())
        {
            std::vector<std::string> tokens;

            FnKat::StringAttribute::array_type values =
                extraAttributesOrNamespacesAttr.getNearestSample(0.0f);
            
            for (FnKat::StringAttribute::array_type::const_iterator I =
                    values.begin(); I != values.end(); ++I)
            {
                std::string value(*I);
                if (value.empty())
                {
                    continue;
                }
                
                pystring::split(value, tokens, ":", 1);
                ab.extraAttributesOrNamespaces[tokens[0]].push_back(value);
            }
        }
        
        // always include userProperties if not explicitly included.
        if (ab.extraAttributesOrNamespaces.find("userProperties")
                == ab.extraAttributesOrNamespaces.end())
        {
            ab.extraAttributesOrNamespaces["userProperties"].push_back(
                    "userProperties");
        }
        else
        {
            // if it is there, enforce that it includes only the top-level attr
            std::vector<std::string> & userPropertiesNames =
                    ab.extraAttributesOrNamespaces["userProperties"];
            
            userPropertiesNames.clear();
            userPropertiesNames.push_back("userProperties");
        }
        
        return ab.build();
    }

    /*
     * Return true if the root prim path starts with any of the
     * user-specified paths that should be using default motion
     * samples.
     */
    static bool GetDefaultMotionAtRoot(PxrUsdKatanaUsdInArgsRefPtr usdInArgs)
    {
        const std::string& rootPrimPath =
                usdInArgs->GetRootPrim().GetPath().GetString();

        const std::set<std::string>& defaultMotionPaths =
                usdInArgs->GetDefaultMotionPaths();

        for (std::set<std::string>::const_iterator I = defaultMotionPaths.begin(),
                E = defaultMotionPaths.end(); I != E; ++I)
        {
            if (pystring::startswith(rootPrimPath, (*I) + "/"))
            {
                return true;
            }
        }

        return false;
    }

private:

    /*
     * Get the write lock and load the USD prim.
     */
    static UsdPrim _LoadPrim(
            const UsdStageRefPtr& stage, 
            const SdfPath& pathToLoad,
            bool verbose)
    {
        boost::unique_lock<boost::upgrade_mutex>
            writerLock(UsdKatanaGetStageLock());

        if (verbose) {
            FnLogInfo(TfStringPrintf(
                        "%s was not loaded. .. Loading.", 
                        pathToLoad.GetText()).c_str());
        }

        return stage->Load(pathToLoad);
    }

    static FnKat::DoubleAttribute
    _MakeBoundsAttribute(
            const UsdPrim& prim,
            PxrUsdKatanaUsdInArgsRefPtr usdInArgs)
    {
        if (prim.GetPath() == SdfPath::AbsoluteRootPath()) {
            // Special-case to pre-empt coding errors.
            return FnKat::DoubleAttribute();
        }
        std::vector<GfBBox3d> bounds = usdInArgs->ComputeBounds(prim);
        const std::vector<double>& motionSampleTimes = usdInArgs->GetMotionSampleTimes();

        bool hasInfiniteBounds = false;
        FnKat::DoubleAttribute boundsAttr = PxrUsdKatanaUtils::ConvertBoundsToAttribute(
                bounds, motionSampleTimes, usdInArgs->IsMotionBackward(), &hasInfiniteBounds);

        // Report infinite bounds as a warning.
        if (hasInfiniteBounds) {
            FnLogWarn("Infinite bounds found at "<<prim.GetPath().GetString());
        }

        return boundsAttr;
    }

    static bool _hasSiteKinds;
};

bool PxrUsdInOp::_hasSiteKinds = false;

//-----------------------------------------------------------------------------

/*
 * This op bootstraps the primary PxrUsdIn op in order to have GeolibPrivateData
 * available at the root op location in PxrUsdIn. Since the GeolibCookInterface
 * API does not currently have the ability to pass GeolibPrivateData via execOp,
 * and we must exec all of the registered plugins to process USD prims, we instead
 * pre-build the GeolibPrivateData for the root location to ensure it is available.
 */
class PxrUsdInBootstrapOp : public FnKat::GeolibOp
{

public:

    static void setup(FnKat::GeolibSetupInterface &interface)
    {
        interface.setThreading(
                FnKat::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(FnKat::GeolibCookInterface &interface)
    {
        interface.stopChildTraversal();

        boost::shared_lock<boost::upgrade_mutex> 
            readerLock(UsdKatanaGetStageLock());
            
        FnKat::GroupAttribute additionalOpArgs;
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs =
                PxrUsdInOp::InitUsdInArgs(interface, additionalOpArgs);
        
        if (!usdInArgs) {
            ERROR("Could not initialize PxrUsdIn usdInArgs.");
            return;
        }
        
        if (!usdInArgs->GetErrorMessage().empty())
        {
            ERROR(usdInArgs->GetErrorMessage().c_str());
            return;
        }

        FnKat::GroupAttribute opArgs = FnKat::GroupBuilder()
            .update(interface.getOpArg())
            .deepUpdate(additionalOpArgs)
            .build();
        
        // Extract the basename (string after last '/') from the location
        // the PxrUsdIn op is configured to run at such that we can create
        // that child and exec the PxrUsdIn op on it.
        //
        std::vector<std::string> tokens;
        pystring::split(usdInArgs->GetRootLocationPath(), tokens, "/");

        if (tokens.empty())
        {
            ERROR("Could not initialize PxrUsdIn op with PxrUsdIn.Bootstrap op.");
            return;
        }

        const std::string& rootName = tokens.back();

        interface.createChild(
                        rootName,
                        "PxrUsdIn",
                        opArgs,
                        FnKat::GeolibCookInterface::ResetRootTrue,
                        new PxrUsdKatanaUsdInPrivateData(
                                usdInArgs->GetRootPrim(),
                                usdInArgs,
                                NULL /* parentData */,
                                PxrUsdInOp::GetDefaultMotionAtRoot(usdInArgs)),
                        PxrUsdKatanaUsdInPrivateData::Delete);
    }

};


class PxrUsdInMasterIntermediateOp : public FnKat::GeolibOp
{
public:
    static void setup(FnKat::GeolibSetupInterface &interface)
    {
        interface.setThreading(
                FnKat::GeolibSetupInterface::ThreadModeConcurrent);
    }
    
    static void cook(FnKat::GeolibCookInterface &interface)
    {
        PxrUsdKatanaUsdInPrivateData* privateData = 
            static_cast<PxrUsdKatanaUsdInPrivateData*>(interface.getPrivateData());
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs = privateData->GetUsdInArgs();
        
        FnKat::GroupAttribute staticScene =
                interface.getOpArg("staticScene");
        
        FnKat::GroupAttribute attrsGroup = staticScene.getChildByName("a");
        
        FnKat::StringAttribute childPrimNameAttr =
                attrsGroup.getChildByName("childPrimPath");
        
        if (childPrimNameAttr.isValid())
        {
            attrsGroup = FnKat::GroupBuilder()
                .update(attrsGroup)
                .del("childPrimPath")
                .build();
            
            std::string childPrimName = childPrimNameAttr.getValue("", false);
            
            if (!childPrimName.empty())
            {
                UsdPrim prim = usdInArgs->GetStage()->GetPrimAtPath(
                        SdfPath(childPrimName));
                TF_FOR_ALL(childIter, prim.GetFilteredChildren(
                        UsdPrimIsDefined
                        && UsdPrimIsActive
                        && !UsdPrimIsAbstract))
                {
                    const UsdPrim& child = *childIter;
                    const std::string& childName = child.GetName();

                    // if (childrenToSkip.count(childName)) {
                    //     continue;
                    // }

                    interface.createChild(
                            childName,
                            "PxrUsdIn",
                            interface.getOpArg(),
                            FnKat::GeolibCookInterface::ResetRootFalse,
                            new PxrUsdKatanaUsdInPrivateData(child, usdInArgs,
                                    privateData),
                            PxrUsdKatanaUsdInPrivateData::Delete);
                }
            }
        }
        else
        {
            FnKat::GroupAttribute childrenGroup = staticScene.getChildByName("c");
            for (size_t i = 0, e = childrenGroup.getNumberOfChildren(); i != e;
                     ++i)
            {
                FnKat::GroupAttribute childGroup =
                        childrenGroup.getChildByIndex(i);
                
                if (!childGroup.isValid())
                {
                    continue;
                }
                
                interface.createChild(
                            childrenGroup.getChildName(i),
                            "",
                            FnKat::GroupBuilder()
                                .update(interface.getOpArg())
                                .set("staticScene", childGroup)
                                .build(),
                            FnKat::GeolibCookInterface::ResetRootFalse,
                            new PxrUsdKatanaUsdInPrivateData(
                                    usdInArgs->GetRootPrim(), usdInArgs,
                                    privateData),
                            PxrUsdKatanaUsdInPrivateData::Delete);
                
            }
            
        }
        
        // apply the local attrs
        for (size_t i = 0, e = attrsGroup.getNumberOfChildren(); i != e; ++i)
        {
            interface.setAttr(attrsGroup.getChildName(i),
                    attrsGroup.getChildByIndex(i));
        }
        
    }
};


//-----------------------------------------------------------------------------

DEFINE_GEOLIBOP_PLUGIN(PxrUsdInOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInBootstrapOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInMasterIntermediateOp)

void registerPlugins()
{
    REGISTER_PLUGIN(PxrUsdInOp, "PxrUsdIn", 0, 1);
    REGISTER_PLUGIN(PxrUsdInBootstrapOp, "PxrUsdIn.Bootstrap", 0, 1);
    REGISTER_PLUGIN(PxrUsdInMasterIntermediateOp, "PxrUsdIn.MasterIntermediate", 0, 1);
}
