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

#include "pxr/pxr.h"
#include "usdKatana/blindDataObject.h"
#include "usdKatana/cache.h"
#include "usdKatana/locks.h"
#include "usdKatana/readBlindData.h"
#include "usdKatana/usdInPluginRegistry.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/motionAPI.h"

#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolib/util/Path.h>
#include <FnLogging/FnLogging.h>

#include "usdKatana/utils.h"

#include "pxr/base/tf/pathUtils.h"

#include <pystring/pystring.h>
#include <stdio.h>

#include <sstream>
#include <memory>

#include <FnAttributeFunction/plugin/FnAttributeFunctionPlugin.h>

#include <FnAPI/FnAPI.h>

FnLogSetup("PxrUsdIn")

PXR_NAMESPACE_USING_DIRECTIVE

namespace FnKat = Foundry::Katana;

// convenience macro to report an error.
#define ERROR(...)\
    interface.setAttr("type", Foundry::Katana::StringAttribute("error"));\
    interface.setAttr("errorMessage", Foundry::Katana::StringAttribute(\
        TfStringPrintf(__VA_ARGS__)));

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
            static_cast<PxrUsdKatanaUsdInPrivateData*>(
                interface.getPrivateData());

        // We may be constructing the private data locally -- in which case
        // it will not be destroyed by the Geolib runtime.
        // This won't be used directly but rather just filled if the private
        // needs to be locally built.
        std::unique_ptr<PxrUsdKatanaUsdInPrivateData> localPrivateData;

        FnKat::GroupAttribute opArgs = interface.getOpArg();
        
        // Get usdInArgs.
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs;
        if (privateData) {
            usdInArgs = privateData->GetUsdInArgs();
        } else {
            FnKat::GroupAttribute additionalOpArgs;
            usdInArgs = InitUsdInArgs(interface.getOpArg(), additionalOpArgs,
                    interface.getRootLocationPath());
            opArgs = FnKat::GroupBuilder()
                .update(opArgs)
                .deepUpdate(additionalOpArgs)
                .build();
            
            // Construct local private data if none was provided by the parent.
            // This is a legitmate case for the root of the scene -- most
            // relevant with the isolatePath pointing at a deeper scope which
            // may have meaningful type/kind ops.
            
            if (usdInArgs->GetStage())
            {
                localPrivateData.reset(new PxrUsdKatanaUsdInPrivateData(
                                   usdInArgs->GetRootPrim(),
                                   usdInArgs, privateData));
                privateData = localPrivateData.get();
            }
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

        UsdStagePtr stage = usdInArgs->GetStage();
        
        // If privateData wasn't initialized because there's no stage in
        // usdInArgs, it would have been caught before as part of the check
        // for usdInArgs->GetErrorMessage(). check again for safety.
        UsdPrim prim;
        if (privateData)
        {
            prim = privateData->GetUsdPrim();
        }
        
        // Validate usd prim.
        if (!prim) {
            ERROR("No USD prim at %s",
                  interface.getRelativeOutputLocationPath().c_str());
            return;
        }

        // Determine if we want to perform the stage-wide queries.
        FnAttribute::IntAttribute processStageWideQueries = 
            opArgs.getChildByName("processStageWideQueries");
        if (processStageWideQueries.isValid() &&
            processStageWideQueries.getValue(0, false) == 1) {
            interface.stopChildTraversal();
            // Reset processStageWideQueries for children ops.
            opArgs = FnKat::GroupBuilder()
                .update(opArgs)
                .set("processStageWideQueries", FnAttribute::IntAttribute(0))
                .build();

            const bool stageIsZup =
                (UsdGeomGetStageUpAxis(stage)==UsdGeomTokens->z);

            interface.setAttr("info.usd.stageIsZup",
                              FnKat::IntAttribute(stageIsZup));

            // Construct the global camera list at the USD scene root.
            //
            FnKat::StringBuilder cameraListBuilder;

            SdfPathVector cameraPaths = PxrUsdKatanaUtils::FindCameraPaths(stage);

            TF_FOR_ALL(cameraPathIt, cameraPaths)
            {
                const std::string path = (*cameraPathIt).GetString();

                // only add cameras to the camera list that are beneath 
                // the isolate prim path
                if (path.find(usdInArgs->GetIsolatePath()) != 
                    std::string::npos)
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

            // lightList and some globals.itemLists.
            SdfPathVector lightPaths = PxrUsdKatanaUtils::FindLightPaths(stage);
            stage->LoadAndUnload(SdfPathSet(lightPaths.begin(), lightPaths.end()),
                                 SdfPathSet());
            PxrUsdKatanaUtilsLightListEditor lightListEditor(interface,
                                                             usdInArgs);
            for (const SdfPath &p: lightPaths) {
                lightListEditor.SetPath(p);
                PxrUsdKatanaUsdInPluginRegistry::
                    ExecuteLightListFncs(lightListEditor);
            }
            lightListEditor.Build();
            
            interface.setAttr("info.usdOpArgs", opArgs);
        }
        
        if (FnAttribute::IntAttribute(
                opArgs.getChildByName("setOpArgsToInfo")).getValue(0, false))
        {
            opArgs = FnAttribute::GroupBuilder()
                .update(opArgs)
                .del("setOpArgsToInfo")
                .build();
            
            interface.setAttr("info.usdOpArgs", opArgs);
        }
        
        

        bool verbose = usdInArgs->IsVerbose();

        // The next section only makes sense to execute on non-pseudoroot prims
        if (prim.GetPath() != SdfPath::AbsoluteRootPath())
        {
            if (!prim.IsLoaded()) {
                SdfPath pathToLoad = prim.GetPath();
                readerLock.unlock();
                prim = _LoadPrim(stage, pathToLoad, verbose);
                if (!prim) {
                    ERROR("load prim %s failed", pathToLoad.GetText());
                    return;
                }
                readerLock.lock();
            }

            // When in "as sources and instances" mode, scan for instances
            // and masters at each location that contains a payload.
            if (prim.HasPayload() &&
                !usdInArgs->GetPrePopulate() &&
                FnAttribute::StringAttribute(
                    interface.getOpArg("instanceMode")
                    ).getValue("expanded", false) == 
                "as sources and instances")
            {
                FnKat::GroupAttribute masterMapping =
                    PxrUsdKatanaUtils::BuildInstanceMasterMapping(
                        prim.GetStage(), prim.GetPath());
                FnKat::StringAttribute masterParentPath(prim.GetPath().GetString());
                if (masterMapping.isValid() &&
                    masterMapping.getNumberOfChildren()) {
                    opArgs = FnKat::GroupBuilder()
                        .update(opArgs)
                        .set("masterMapping", masterMapping)
                        .set("masterParentPath", masterParentPath)
                        .build();
                } else {
                    opArgs = FnKat::GroupBuilder()
                        .update(opArgs)
                        .del("masterMapping")
                        .build();
                }
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
                                  _MakeBoundsAttribute(prim, *privateData));
            }

            //
            // Find and execute the core op that handles the USD type.
            //

            {
                std::string opName;
                if (PxrUsdKatanaUsdInPluginRegistry::FindUsdType(
                        prim.GetTypeName(), &opName)) {
                    if (!opName.empty()) {
                        
                        if (privateData)
                        {
                            // roughly equivalent to execOp except that we can
                            // locally override privateData
                            PxrUsdKatanaUsdInPluginRegistry::ExecuteOpDirectExecFnc(
                                    opName, *privateData, opArgs, interface);

                            opArgs = privateData->updateExtensionOpArgs(opArgs);
                        }
                        
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
                        if (privateData)
                        {
                            // roughly equivalent to execOp except that we can
                            // locally override privateData
                            PxrUsdKatanaUsdInPluginRegistry::ExecuteOpDirectExecFnc(
                                    opName, *privateData, opArgs, interface);
                            opArgs = privateData->updateExtensionOpArgs(opArgs);
                        }
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
                            if (privateData)
                            {
                                // roughly equivalent to execOp except that we can
                                // locally override privateData
                                PxrUsdKatanaUsdInPluginRegistry::ExecuteOpDirectExecFnc(
                                        opName, *privateData, opArgs, interface);
                                
                                opArgs = privateData->updateExtensionOpArgs(opArgs);
                            }
                        }
                    }
                }
            }

            //
            // Find and execute the site-specific kind op that handles 
            // the model kind.
            //

            if (_hasSiteKinds) {
                TfToken kind;
                if (UsdModelAPI(prim).GetKind(&kind)) {
                    std::string opName;
                    if (PxrUsdKatanaUsdInPluginRegistry::FindKindForSite(
                            kind, &opName)) {
                        if (!opName.empty()) {
                            if (privateData)
                            {
                                PxrUsdKatanaUsdInPluginRegistry::ExecuteOpDirectExecFnc(
                                        opName, *privateData, opArgs, interface);
                                opArgs = privateData->updateExtensionOpArgs(opArgs);
                            }
                        }
                    }
                }
            }

            //
            // Read blind data. This is last because blind data opinions 
            // should always win.
            //

            PxrUsdKatanaAttrMap attrs;
            PxrUsdKatanaReadBlindData(UsdKatanaBlindDataObject(prim), attrs);
            attrs.toInterface(interface);

            //
            // Execute any ops contained within the staticScene args.
            //

            FnKat::GroupAttribute opGroups = opArgs.getChildByName("staticScene.x");
            if (opGroups.isValid())
            {
                for (int childindex = 0; childindex < opGroups.getNumberOfChildren();
                        ++childindex)
                {
                    FnKat::GroupAttribute entry =
                            opGroups.getChildByIndex(childindex);

                    if (!entry.isValid())
                    {
                        continue;
                    }

                    FnKat::StringAttribute subOpType =
                            entry.getChildByName("opType");

                    FnKat::GroupAttribute subOpArgs =
                            entry.getChildByName("opArgs");

                    if (!subOpType.isValid() || !subOpArgs.isValid())
                    {
                        continue;
                    }

                    interface.execOp(subOpType.getValue("", false), subOpArgs);
                }
            }

        }   // if (prim.GetPath() != SdfPath::AbsoluteRootPath())
        
        bool skipAllChildren = FnKat::IntAttribute(
                interface.getOutputAttr("__UsdIn.skipAllChildren")).getValue(
                0, false);

        if (prim.IsMaster() and FnKat::IntAttribute(
                opArgs.getChildByName("childOfIntermediate")
                ).getValue(0, false) == 1)
        {
            interface.setAttr("type",
                    FnKat::StringAttribute("instance source"));
            interface.setAttr("tabs.scenegraph.stopExpand", 
                    FnKat::IntAttribute(1));

            // XXX masters are simple placeholders and will not get read as
            // models, so we'll need to explicitly process their Looks in a
            // manner similar to what the PxrUsdInCore_ModelOp does.
            UsdPrim lookPrim =
                    prim.GetChild(TfToken(
                    UsdKatanaTokens->katanaLooksScopeName));
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
                            privateData->GetUsdInArgs(),
                            privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
            }
        }

        if (prim.IsInstance())
        {
            UsdPrim master = prim.GetMaster();
            interface.setAttr("info.usd.masterPrimPath",
                    FnAttribute::StringAttribute(
                        master.GetPrimPath().GetString()));
            
            FnAttribute::StringAttribute masterPathAttr = 
                    opArgs.getChildByName("masterMapping." +
                            FnKat::DelimiterEncode(
                                master.GetPrimPath().GetString()));
            if (masterPathAttr.isValid())
            {
                std::string masterPath = masterPathAttr.getValue("", false);

                std::string masterParentPath = FnAttribute::StringAttribute(
                    opArgs.getChildByName("masterParentPath"))
                    .getValue("", false);
                if (masterParentPath == "/") {
                    masterParentPath = std::string();
                }

                if (!masterPath.empty())
                {
                    interface.setAttr(
                            "type", FnKat::StringAttribute("instance"));
                    interface.setAttr("geometry.instanceSource",
                            FnAttribute::StringAttribute(
                                usdInArgs->GetRootLocationPath() + 
                                masterParentPath +
                                "/Masters/" + masterPath));
                    
                    // XXX, ConstraintGroups are still made for models 
                    //      that became instances. Need to suppress creation 
                    //      of that stuff
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
            
        // Emit "Masters".
        // When prepopulating, these will be discovered and emitted under
        // the root.  Otherwise, they will be discovered incrementally
        // as each payload is loaded, and we emit them under the payload's
        // location.
        if (interface.atRoot() ||
            (prim.HasPayload() && !usdInArgs->GetPrePopulate())) {
            FnKat::GroupAttribute masterMapping =
                    opArgs.getChildByName("masterMapping");
            if (masterMapping.isValid() && masterMapping.getNumberOfChildren())
            {
                FnGeolibServices::StaticSceneCreateOpArgsBuilder sscb(false);
                
                struct usdPrimInfo
                {
                    std::vector<std::string> usdPrimPathValues;
                    std::vector<std::string> usdPrimNameValues;
                };
                
                std::map<std::string, usdPrimInfo> primInfoPerLocation;
                
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
                    
                    katanaPath = "Masters/" + katanaPath;
                    
                    std::string leafName =
                            FnGeolibUtil::Path::GetLeafName(katanaPath);
                    std::string locationParent =
                            FnGeolibUtil::Path::GetLocationParent(katanaPath);
                    
                    auto & entry = primInfoPerLocation[locationParent];
                    
                    entry.usdPrimPathValues.push_back(masterName);
                    entry.usdPrimNameValues.push_back(leafName);
                }
                
                for (const auto & I : primInfoPerLocation)
                {
                    const auto & locationParent = I.first;
                    const auto & entry = I.second;
                    
                    sscb.setAttrAtLocation(locationParent, "usdPrimPath",
                            FnKat::StringAttribute(entry.usdPrimPathValues));
                    sscb.setAttrAtLocation(locationParent, "usdPrimName",
                            FnKat::StringAttribute(entry.usdPrimNameValues));
                }
                
                FnKat::GroupAttribute childAttrs =
                    sscb.build().getChildByName("c");
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
                                usdInArgs->GetRootPrim(),
                                usdInArgs, privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
                }
            }
        }

        if (privateData)
        {
            opArgs = PxrUsdKatanaUsdInPluginRegistry::ExecuteLocationDecoratorOps(
                    *privateData, opArgs, interface);
        }

        if (!skipAllChildren) {

            std::set<std::string> childrenToSkip;
            FnKat::GroupAttribute childOps = interface.getOutputAttr(
                "__UsdIn.skipChild");
            if (childOps.isValid()) {
                for (int64_t i = 0; i < childOps.getNumberOfChildren(); i++) {
                    std::string childName = childOps.getChildName(i);
                    bool shouldSkip = FnKat::IntAttribute(
                        childOps.getChildByIndex(i)).getValue(0, false);
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
                    ERROR("USD Prim is advertised as an instance "
                        "but master prim cannot be found.");
                }
                else
                {
                    prim = masterPrim;
                }
            }

            // create children
            auto predicate = UsdPrimIsActive && !UsdPrimIsAbstract;
            if (interface.getNumInputs() == 0) {
                // Require a defining specifier on prims if there is no input.
                predicate = UsdPrimIsDefined && predicate;
            }
            TF_FOR_ALL(childIter, prim.GetFilteredChildren(predicate))
            {
                const UsdPrim& child = *childIter;
                const std::string& childName = child.GetName();

                if (childrenToSkip.count(childName)) {
                    continue;
                }

                // If we allow prims without a defining specifier then
                // also check that the prim exists in the input so we
                // have something to override.
                if (!child.HasDefiningSpecifier()) {
                    if (!interface.doesLocationExist(childName)) {
                        // Skip over with no def.
                        continue;
                    }
                }

                interface.createChild(
                        childName,
                        "",
                        FnKat::GroupBuilder()
                            .update(opArgs)
                            .set("staticScene", opArgs.getChildByName(
                                "staticScene.c." + childName))
                            .build(),
                        FnKat::GeolibCookInterface::ResetRootFalse,
                        new PxrUsdKatanaUsdInPrivateData(
                                child, usdInArgs,
                                privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
            }
        }

        // keep things around if we are verbose
        if (!verbose) {
            interface.deleteAttr("__UsdIn");
        }
        
        
        
        
    }

    

    static PxrUsdKatanaUsdInArgsRefPtr
    InitUsdInArgs(const FnKat::GroupAttribute & opArgs,
            FnKat::GroupAttribute & additionalOpArgs,
            const std::string & rootLocationPath)
    {
        ArgsBuilder ab;
        
        FnKat::StringAttribute usdFileAttr = opArgs.getChildByName("fileName");
        if (!usdFileAttr.isValid()) {
            return ab.buildWithError("PxrUsdIn: USD fileName not specified.");
        }

        std::string fileName = usdFileAttr.getValue();
        
        ab.rootLocation = FnKat::StringAttribute(
                opArgs.getChildByName("location")).getValue(
                        rootLocationPath, false);
        
        
        std::string sessionLocation = ab.rootLocation;
        FnKat::StringAttribute sessionLocationAttr = 
                opArgs.getChildByName("sessionLocation");
        if (sessionLocationAttr.isValid()) {
            sessionLocation = sessionLocationAttr.getValue();
        }
        
        FnAttribute::GroupAttribute sessionAttr = 
            opArgs.getChildByName("session");

        
        
        // XXX BEGIN convert the legacy variant string to the session
        // TODO: decide how long to do this as this form has been deprecated
        //       for some time but may still be present in secondary uses
        FnAttribute::GroupBuilder legacyVariantsGb;
        
        std::string variants = FnKat::StringAttribute(
                opArgs.getChildByName("variants")).getValue("", false);
        std::set<std::string> selStrings = TfStringTokenizeToSet(variants);
        TF_FOR_ALL(selString, selStrings) {
            std::string errMsg;
            if (SdfPath::IsValidPathString(*selString, &errMsg)) {
                SdfPath varSelPath(*selString);
                if (varSelPath.IsPrimVariantSelectionPath()) {
                    
                    std::string entryPath = FnAttribute::DelimiterEncode(
                            sessionLocation + 
                            varSelPath.GetPrimPath().GetString());
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

        ab.sessionLocation = sessionLocation;
        ab.sessionAttr = sessionAttr;

        ab.ignoreLayerRegex = FnKat::StringAttribute(
                opArgs.getChildByName("ignoreLayerRegex")).getValue("", false);

        ab.verbose = FnKat::IntAttribute(
                opArgs.getChildByName("verbose")).getValue(0, false);

        FnKat::GroupAttribute systemArgs(opArgs.getChildByName("system"));

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
                opArgs.getChildByName("motionSampleTimes")).getValue("", false);

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

            for (std::vector<std::string>::iterator it = tokens.begin(); 
                it != tokens.end(); ++it)
            {
                ab.motionSampleTimes.push_back(std::stod(*it));
            }
        }

        // Determine whether to prepopulate the USD stage.
        ab.prePopulate =
            FnKat::IntAttribute(opArgs.getChildByName("prePopulate"))
                        .getValue(1 /* default prePopulate=yes */ , false);

        ab.stage =  UsdKatanaCache::GetInstance().GetStage(
                fileName, 
                sessionAttr, sessionLocation,
                ab.ignoreLayerRegex, 
                ab.prePopulate);

        if (!ab.stage) {
            return ab.buildWithError("PxrUsdIn: USD Stage cannot be loaded.");
        }

        if (FnAttribute::StringAttribute(
                opArgs.getChildByName("instanceMode")
                    ).getValue("expanded", false) == 
                "as sources and instances")
        {
            additionalOpArgs = FnKat::GroupAttribute("masterMapping",
                PxrUsdKatanaUtils::BuildInstanceMasterMapping(ab.stage,
                                      SdfPath::AbsoluteRootPath()), true);
        }
        
        ab.isolatePath = FnKat::StringAttribute(
            opArgs.getChildByName("isolatePath")).getValue("", false);

        // if the specified isolatePath is not a valid prim, clear it out
        if (!ab.isolatePath.empty() && !ab.stage->GetPrimAtPath(
            SdfPath(ab.isolatePath)))
        {
            std::ostringstream errorBuffer;
            errorBuffer << "PxrUsdIn: Invalid isolatePath: " << 
                ab.isolatePath << ".";
            return ab.buildWithError(errorBuffer.str());
        }

        // get extra attributes or namespaces if they exist
        //
        FnKat::StringAttribute extraAttributesOrNamespacesAttr = 
            opArgs.getChildByName("extraAttributesOrNamespaces");

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
            const PxrUsdKatanaUsdInPrivateData& data)
    {
        if (prim.GetPath() == SdfPath::AbsoluteRootPath()) {
            // Special-case to pre-empt coding errors.
            return FnKat::DoubleAttribute();
        }
        const std::vector<double>& motionSampleTimes =
            data.GetMotionSampleTimes();
        std::vector<GfBBox3d> bounds =
            data.GetUsdInArgs()->ComputeBounds(prim, motionSampleTimes);

        bool hasInfiniteBounds = false;
        bool isMotionBackward = motionSampleTimes.size() > 1 &&
            motionSampleTimes.front() > motionSampleTimes.back();
        FnKat::DoubleAttribute boundsAttr = 
            PxrUsdKatanaUtils::ConvertBoundsToAttribute(
                bounds, motionSampleTimes, isMotionBackward, 
                &hasInfiniteBounds);

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
 * This op bootstraps the primary PxrUsdIn op in order to have
 * GeolibPrivateData available at the root op location in PxrUsdIn. Since the 
 * GeolibCookInterface API does not currently have the ability to pass 
 * GeolibPrivateData via execOp, and we must exec all of the registered plugins
 * to process USD prims, we instead pre-build the GeolibPrivateData for the
 * root location to ensure it is available.
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
                PxrUsdInOp::InitUsdInArgs(interface.getOpArg(),
                        additionalOpArgs, interface.getRootLocationPath());
        
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
            .set("setOpArgsToInfo", FnAttribute::IntAttribute(1))
            .build();
        
        // Extract the basename (string after last '/') from the location
        // the PxrUsdIn op is configured to run at such that we can create
        // that child and exec the PxrUsdIn op on it.
        //
        std::vector<std::string> tokens;
        pystring::split(usdInArgs->GetRootLocationPath(), tokens, "/");

        if (tokens.empty())
        {
            ERROR("Could not initialize PxrUsdIn op with "
                "PxrUsdIn.Bootstrap op.");
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
                                NULL /* parentData */),
                        PxrUsdKatanaUsdInPrivateData::Delete);
    }

};



/*
 * This op bootstraps the primary PxrUsdIn op in order to have
 * GeolibPrivateData available at the root op location in PxrUsdIn. Since the 
 * GeolibCookInterface API does not currently have the ability to pass 
 * GeolibPrivateData via execOp, and we must exec all of the registered plugins
 * to process USD prims, we instead pre-build the GeolibPrivateData for the
 * root location to ensure it is available.
 */
class PxrUsdInMaterialGroupBootstrapOp : public FnKat::GeolibOp
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
                PxrUsdInOp::InitUsdInArgs(interface.getOpArg(),
                        additionalOpArgs, interface.getRootLocationPath());
        
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

        PxrUsdKatanaUsdInPrivateData privateData(
                usdInArgs->GetRootPrim(),
                usdInArgs,
                NULL /* parentData */);

        PxrUsdKatanaUsdInPluginRegistry::ExecuteOpDirectExecFnc(
            "UsdInCore_LooksGroupOp", 
            privateData,
            opArgs, 
            interface);
    }

};


class PxrUsdInBuildIntermediateOp : public FnKat::GeolibOp
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
            static_cast<PxrUsdKatanaUsdInPrivateData*>(
                interface.getPrivateData());
        
// If we are exec'ed from katana 2.x from an op which doesn't have
// PxrUsdKatanaUsdInPrivateData, we need to build some. We normally avoid
// this case by using the execDirect -- but some ops need to call
// PxrUsdInBuildIntermediateOp via execOp. In 3.x, they can (and are
// required to) provide the private data.
#if KATANA_VERSION_MAJOR < 3
        
        // We may be constructing the private data locally -- in which case
        // it will not be destroyed by the Geolib runtime.
        // This won't be used directly but rather just filled if the private
        // needs to be locally built.
        std::unique_ptr<PxrUsdKatanaUsdInPrivateData> localPrivateData;
        
        
        if (!privateData)
        {
            
            FnKat::GroupAttribute additionalOpArgs;
            auto usdInArgs = PxrUsdInOp::InitUsdInArgs(
                    interface.getOpArg(), additionalOpArgs,
                            interface.getRootLocationPath());
            auto opArgs = FnKat::GroupBuilder()
                .update(interface.getOpArg())
                .deepUpdate(additionalOpArgs)
                .build();
            
            // Construct local private data if none was provided by the parent.
            // This is a legitmate case for the root of the scene -- most
            // relevant with the isolatePath pointing at a deeper scope which
            // may have meaningful type/kind ops.
            
            if (usdInArgs->GetStage())
            {
                localPrivateData.reset(new PxrUsdKatanaUsdInPrivateData(
                                   usdInArgs->GetRootPrim(),
                                   usdInArgs, privateData));
                privateData = localPrivateData.get();
            }
            else
            {
                //TODO, warning
                return;
            }
            
        }
        
#endif 
        
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs = privateData->GetUsdInArgs();

        FnKat::GroupAttribute staticScene =
                interface.getOpArg("staticScene");

        FnKat::GroupAttribute attrsGroup = staticScene.getChildByName("a");

        FnKat::StringAttribute primPathAttr =
                attrsGroup.getChildByName("usdPrimPath");
        FnKat::StringAttribute primNameAttr =
                attrsGroup.getChildByName("usdPrimName");

        
        std::set<std::string> createdChildren;
        
        // If prim attrs are present, use them to build out the usd prim.
        // Otherwise, build out a katana group.
        //
        if (primPathAttr.isValid())
        {
            attrsGroup = FnKat::GroupBuilder()
                .update(attrsGroup)
                .del("usdPrimPath")
                .del("usdPrimName")
                .build();

            
            auto usdPrimPathValues = primPathAttr.getNearestSample(0);
            
            
            
            for (size_t i = 0; i < usdPrimPathValues.size(); ++i)
            {
                std::string primPath(usdPrimPathValues[i]);
                if (usdPrimPathValues.empty())
                {
                    continue;
                }
                
                // Get the usd prim at the given source path.
                //
                UsdPrim prim = usdInArgs->GetStage()->GetPrimAtPath(
                        SdfPath(primPath));

                // Get the desired name for the usd prim; if one isn't provided,
                // ask the prim directly.
                //
                std::string nameToUse = prim.GetName();
                if (primNameAttr.getNumberOfValues() > static_cast<int64_t>(i))
                {
                    auto primNameAttrValues = primNameAttr.getNearestSample(0);
                    
                    std::string primName = primNameAttrValues[i];
                    if (!primName.empty())
                    {
                        nameToUse = primName;
                    }
                }

                // XXX In order for the prim's material hierarchy to get built
                // out correctly via the PxrUsdInCore_LooksGroupOp, we'll need
                // to override the original 'rootLocation' and 'isolatePath'
                // UsdIn args.
                //
                ArgsBuilder ab;
                ab.update(usdInArgs);
                ab.rootLocation =
                        interface.getOutputLocationPath() + "/" + nameToUse;
                ab.isolatePath = primPath;

                // If the child we are making has intermediate children,
                // send those along. This currently happens with point
                // instancer prototypes and the children of Looks groups.
                //
                FnKat::GroupAttribute childrenGroup =
                        staticScene.getChildByName("c." + nameToUse);

                createdChildren.insert(nameToUse);
                // Build the prim using PxrUsdIn.
                //
                interface.createChild(
                        nameToUse,
                        "PxrUsdIn",
                        FnKat::GroupBuilder()
                            .update(interface.getOpArg())
                            .set("childOfIntermediate", FnKat::IntAttribute(1))
                            .set("staticScene", childrenGroup)
                            .build(),
                        FnKat::GeolibCookInterface::ResetRootFalse,
                        new PxrUsdKatanaUsdInPrivateData(prim, ab.build(),
                                privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
            }
        }
        
        FnKat::GroupAttribute childrenGroup =
            staticScene.getChildByName("c");
        for (size_t i = 0, e = childrenGroup.getNumberOfChildren(); i != e;
                 ++i)
        {
            FnKat::GroupAttribute childGroup =
                    childrenGroup.getChildByIndex(i);

            if (!childGroup.isValid())
            {
                continue;
            }

            std::string childName = childrenGroup.getChildName(i);
            
            if (createdChildren.find(childName) != createdChildren.end())
            {
                continue;
            }
            
            // Build the intermediate group using the same op.
            //
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
        

        // Apply local attrs.
        //
        for (size_t i = 0, e = attrsGroup.getNumberOfChildren(); i != e; ++i)
        {
            interface.setAttr(attrsGroup.getChildName(i),
                    attrsGroup.getChildByIndex(i));
        }
        
    }
};




class PxrUsdInAddViewerProxyOp : public FnKat::GeolibOp
{
public:
    static void setup(FnKat::GeolibSetupInterface &interface)
    {
        interface.setThreading(
                FnKat::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(FnKat::GeolibCookInterface &interface)
    {
        interface.setAttr("proxies", 
        PxrUsdKatanaUtils::GetViewerProxyAttr(
            FnKat::DoubleAttribute(interface.getOpArg("currentTime")
                    ).getValue(0.0, false),
            FnKat::StringAttribute(interface.getOpArg("fileName")
                    ).getValue("", false),
            
            FnKat::StringAttribute(interface.getOpArg("isolatePath")
                    ).getValue("", false),
            
            FnKat::StringAttribute(interface.getOpArg("rootLocation")
                    ).getValue("", false),
            interface.getOpArg("session"),
            FnKat::StringAttribute(interface.getOpArg("ignoreLayerRegex")
                    ).getValue("", false)
        ));
    }
};





class FlushStageFnc : public Foundry::Katana::AttributeFunction
{
public:
    static FnAttribute::Attribute run(FnAttribute::Attribute args)
    {
        boost::upgrade_lock<boost::upgrade_mutex>
                readerLock(UsdKatanaGetStageLock());
        
        FnKat::GroupAttribute additionalOpArgs;
        auto usdInArgs = PxrUsdInOp::InitUsdInArgs(args, additionalOpArgs,
                "/root");
        
        if (usdInArgs)
        {
            boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                    writerLock(readerLock);
            UsdKatanaCache::GetInstance().FlushStage(usdInArgs->GetStage());
        }
        
        
        return FnAttribute::Attribute();
    }

};



//-----------------------------------------------------------------------------

DEFINE_GEOLIBOP_PLUGIN(PxrUsdInOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInBootstrapOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInMaterialGroupBootstrapOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInBuildIntermediateOp)
DEFINE_GEOLIBOP_PLUGIN(PxrUsdInAddViewerProxyOp)
DEFINE_ATTRIBUTEFUNCTION_PLUGIN(FlushStageFnc);

void registerPlugins()
{
    REGISTER_PLUGIN(PxrUsdInOp, "PxrUsdIn", 0, 1);
    REGISTER_PLUGIN(PxrUsdInBootstrapOp, "PxrUsdIn.Bootstrap", 0, 1);
    REGISTER_PLUGIN(PxrUsdInMaterialGroupBootstrapOp, 
        "PxrUsdIn.BootstrapMaterialGroup", 0, 1);
    REGISTER_PLUGIN(PxrUsdInBuildIntermediateOp,
        "PxrUsdIn.BuildIntermediate", 0, 1);    
    REGISTER_PLUGIN(PxrUsdInAddViewerProxyOp,
        "PxrUsdIn.AddViewerProxy", 0, 1);    
    REGISTER_PLUGIN(FlushStageFnc,
        "PxrUsdIn.FlushStage", 0, 1);
    
    
}
