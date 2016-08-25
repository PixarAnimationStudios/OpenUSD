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

#include <FnGeolib/op/FnGeolibOp.h>
#include <FnLogging/FnLogging.h>

#include "usdKatana/utils.h"

// just for info
#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/arch/pragmas.h"

#include <pystring/pystring.h>
#include <stdio.h>

#include <sstream>

FnLogSetup("PxrUsdIn")

namespace FnKat = Foundry::Katana;

// convenience macro to report an error.
#ifdef ERROR
    // from WinGDI.h
    #undef ERROR
#endif
#define ERROR(...)\
    interface.setAttr("type", Foundry::Katana::StringAttribute("error"));\
    interface.setAttr("errorMesssage", Foundry::Katana::StringAttribute(TfStringPrintf(__VA_ARGS__)));

// see overview.dox for more documentation.
class PxrUsdInOp : public FnKat::GeolibOp
{
public:
    ~PxrUsdInOp() = delete;

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

        UsdPrim prim;
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs;

        PxrUsdKatanaUsdInPrivateData* privateData = 
            static_cast<PxrUsdKatanaUsdInPrivateData*>(interface.getPrivateData());

        FnKat::GroupAttribute opArgs = interface.getOpArg();
        
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

        if (not usdInArgs) {
            ERROR("Could not initialize PxrUsdIn usdInArgs.");
            return;
        }

        if (interface.atRoot()) {
            interface.stopChildTraversal();

            prim = usdInArgs->GetRootPrim();

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
                                usdInArgs->GetRootPrim(), usdInArgs, privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
                
                
            }
            
        }
        else {
            prim = privateData->GetUsdPrim();
        }

        if (not prim) {
            ERROR("No prim at %s", interface.getRelativeOutputLocationPath().c_str());
            return;
        }

        bool verbose = usdInArgs->IsVerbose();

        if (not prim.IsLoaded()) {
            SdfPath pathToLoad = prim.GetPath();
            UsdStageRefPtr stage = prim.GetStage();
            readerLock.unlock();
            prim = _LoadPrim(stage, pathToLoad, verbose);
            if (not prim) {
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
                if (not opName.empty()) {
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
                if (not opName.empty()) {
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
                    if (not opName.empty()) {
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
                    if (not opName.empty()) {
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


        if (not skipAllChildren) {

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
            if (prim.IsInstance() and not privateData->GetMasterPath().IsEmpty())
            {
                const UsdPrim& masterPrim = prim.GetMaster();
                if (not masterPrim)
                {
                    ERROR("USD Prim is advertised as an instance but master prim cannot be found.");
                }
                else
                {
                    prim = masterPrim;
                }
            }

            // create children
            TF_FOR_ALL(childIter, prim.GetFilteredChildren(UsdPrimIsDefined and UsdPrimIsActive and not UsdPrimIsAbstract))
            {
                const UsdPrim& child = *childIter;
                const std::string& childName = child.GetName();

                if (childrenToSkip.count(childName)) {
                    continue;
                }

                interface.createChild(
                        childName,
                        "",
                        opArgs,
                        FnKat::GeolibCookInterface::ResetRootFalse,
                        new PxrUsdKatanaUsdInPrivateData(child, usdInArgs, privateData),
                        PxrUsdKatanaUsdInPrivateData::Delete);
            }
        }

        // keep things around if we are verbose
        if (not verbose) {
            interface.deleteAttr("__UsdIn");
        }
    }

    // TODO, consider caching this conversion
    static
    std::string GetVariantStringFromSession(
            FnAttribute::GroupAttribute sessionAttr,
                    const std::string & rootLocation)
    {
        FnAttribute::GroupAttribute variantsAttr =
                sessionAttr.getChildByName("variants");
        if (variantsAttr.getNumberOfChildren() == 0)
        {
            return "";
        }
        
        std::string rootLocationPlusSlash = rootLocation + "/";
        
        std::ostringstream buffer;
        
        for (int64_t i = 0, e = variantsAttr.getNumberOfChildren(); i != e;
                ++i)
        {
            std::string entryName = FnAttribute::DelimiterDecode(
                    variantsAttr.getChildName(i));
            
            FnAttribute::GroupAttribute entryVariantSets =
                    variantsAttr.getChildByIndex(i);
            
            if (entryVariantSets.getNumberOfChildren() == 0)
            {
                continue;
            }
            
            if (!pystring::startswith(entryName, rootLocationPlusSlash))
            {
                continue;
            }
            
            std::string primPath = pystring::slice(entryName,
                    rootLocation.size());
            
            for (int64_t i = 0, e = entryVariantSets.getNumberOfChildren();
                    i != e; ++i)
            {
                std::string variantSetName = entryVariantSets.getChildName(i);
                
                FnAttribute::StringAttribute variantValueAttr =
                        entryVariantSets.getChildByIndex(i);
                if (!variantValueAttr.isValid())
                {
                    continue;
                }
                
                buffer << ' ' << SdfPath(primPath).AppendVariantSelection(
                        variantSetName, variantValueAttr.getValue("", false)
                                ).GetString();
            }
        }
        
        return buffer.str();
    }

    static PxrUsdKatanaUsdInArgsRefPtr
    InitUsdInArgs(const FnKat::GeolibCookInterface &interface,
            FnKat::GroupAttribute & additionalOpArgs)
    {
        FnKat::StringAttribute usdFileAttr = interface.getOpArg("fileName");
        if (not usdFileAttr.isValid()) {
            FnLogInfo("Missing fileName attr.");
            return TfNullPtr;
        }

        std::string fileName = usdFileAttr.getValue();
        
        std::string rootLocation = FnKat::StringAttribute(
                interface.getOpArg("location")).getValue(
                        interface.getRootLocationPath(), false);
        
        std::string variants = FnKat::StringAttribute(
                interface.getOpArg("variants")).getValue("", false);
        
        std::string sessionLocation = rootLocation;
        FnKat::StringAttribute sessionLocationAttr = 
                interface.getOpArg("sessionLocation");
        if (sessionLocationAttr.isValid()) {
            sessionLocation = sessionLocationAttr.getValue();
        }
        
        variants += GetVariantStringFromSession(
                interface.getOpArg("session"), sessionLocation);

        SdfPathSet variantSelections;
        std::set<std::string> selStrings = TfStringTokenizeToSet(variants);
        TF_FOR_ALL(selString, selStrings) {
            std::string errMsg;
            if (SdfPath::IsValidPathString(*selString, &errMsg)) {
                SdfPath varSelPath(*selString);
                if (varSelPath.IsPrimVariantSelectionPath()) {
                    variantSelections.insert( SdfPath(*selString) );
                    continue;
                }   
            }   
            FnLogWarn(TfStringPrintf("Bad variant selection \"%s\"",
                                     selString->c_str()).c_str());
            return TfNullPtr;
        }

        std::string ignoreLayerRegex = FnKat::StringAttribute(
                interface.getOpArg("ignoreLayerRegex")).getValue("", false);

        bool verbose = FnKat::IntAttribute(
                interface.getOpArg("verbose")).getValue(0, false);

        FnKat::GroupAttribute systemArgs(interface.getOpArg("system"));

        double currentTime = 
            FnKat::FloatAttribute(systemArgs.getChildByName(
                "timeSlice.currentTime")).getValue(0, false);

        int numSamples = 
            FnKat::IntAttribute(systemArgs.getChildByName(
                "timeSlice.numSamples")).getValue(1, false);

        double shutterOpen =
            FnKat::FloatAttribute(systemArgs.getChildByName(
                "timeSlice.shutterOpen")).getValue(0, false);

        double shutterClose =
            FnKat::FloatAttribute(systemArgs.getChildByName(
                "timeSlice.shutterClose")).getValue(0, false);

        std::vector<double> motionSampleTimes;

        std::string motionSampleStr = FnKat::StringAttribute(
                interface.getOpArg("motionSampleTimes")).getValue("", false);

        // If motion samples was specified, convert the string of values
        // into a vector of doubles to store with the root args.
        //
        if (numSamples < 2 or motionSampleStr.empty())
        {
            motionSampleTimes.push_back(0);
        }
        else
        {
            std::vector<std::string> tokens;
            pystring::split(motionSampleStr, tokens, " ");

            for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
            {
                motionSampleTimes.push_back(std::stod(*it));
            }
        }

        UsdStageRefPtr stage = 
            UsdKatanaCache::GetInstance().GetStage(
                fileName, 
                variantSelections, 
                ignoreLayerRegex, 
                true /* forcePopulate */);

        if (not stage) {
            FnLogInfo("No stage.");
            return TfNullPtr;
        }

        if (FnAttribute::StringAttribute(
                interface.getOpArg("instanceMode")
                        ).getValue("expanded", false) == "as sources and instances")
        {
            additionalOpArgs = FnKat::GroupAttribute("masterMapping",
                    PxrUsdKatanaUtils::BuildInstanceMasterMapping(stage), true);
        }
        
        
        

        std::string isolatePath = FnKat::StringAttribute(
            interface.getOpArg("isolatePath")).getValue("", false);

        // if the specified isolatePath is not a valid prim, clear it out
        if (not isolatePath.empty() and not stage->GetPrimAtPath(SdfPath(isolatePath)))
        {
            FnLogWarn("Invalid isolatePath: " << isolatePath);
            isolatePath = "";
        }

        // get extra attributes or namespaces if they exist
        //
        PxrUsdKatanaUsdInArgs::StringListMap extraAttributesOrNamespaces;

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
                extraAttributesOrNamespaces[tokens[0]].push_back(value);
            }
        }
        
        // always include userProperties if not explicitly included.
        if (extraAttributesOrNamespaces.find("userProperties")
                == extraAttributesOrNamespaces.end())
        {
            extraAttributesOrNamespaces["userProperties"].push_back(
                    "userProperties");
        }
        else
        {
            // if it is there, enforce that it includes only the top-level attr
            std::vector<std::string> & userPropertiesNames =
                    extraAttributesOrNamespaces["userProperties"];
            
            userPropertiesNames.clear();
            userPropertiesNames.push_back("userProperties");
        }

        return PxrUsdKatanaUsdInArgs::New(
                stage,
                rootLocation,
                isolatePath,
                variantSelections,
                ignoreLayerRegex,
                currentTime,
                shutterOpen,
                shutterClose,
                motionSampleTimes,
                extraAttributesOrNamespaces,
                verbose);
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
                bounds, motionSampleTimes, &hasInfiniteBounds);

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
    ~PxrUsdInBootstrapOp() = delete;

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
                        new PxrUsdKatanaUsdInPrivateData(usdInArgs->GetRootPrim(), usdInArgs),
                        PxrUsdKatanaUsdInPrivateData::Delete);
    }

};

class PxrUsdInMasterIntermediateOp : public FnKat::GeolibOp
{
public:
    ~PxrUsdInMasterIntermediateOp() = delete;

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
                        and UsdPrimIsActive
                        and not UsdPrimIsAbstract))
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
