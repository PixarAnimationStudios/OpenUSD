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
#include "usdMaya/usdReadJob.h"

#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/stageCache.h"
#include "usdMaya/translatorLook.h"
#include "usdMaya/translatorModelAssembly.h"
#include "usdMaya/translatorXformable.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/usdUtils/stageCache.h"

#include <maya/MAnimControl.h>
#include <maya/MDagModifier.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MTime.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



// for now, we hard code this to use displayColor.  But maybe the more
// appropriate thing to do is just to leave shadingMode a lone and pass
// "displayColor" in from the UsdMayaRepresentationFull
// (usdMaya/referenceAssembly.cpp)
const static TfToken ASSEMBLY_SHADING_MODE = PxrUsdMayaShadingModeTokens->displayColor;

usdReadJob::usdReadJob(
    const std::string &iFileName,
    const std::string &iPrimPath,
    const std::map<std::string, std::string>& iVariants,
    const JobImportArgs &iArgs,
    const std::string& assemblyTypeName,
    const std::string& proxyShapeTypeName) :
    mArgs(iArgs),
    mFileName(iFileName),
    mPrimPath(iPrimPath),
    mVariants(iVariants),
    mDagModifierUndo(),
    mDagModifierSeeded(false),
    mMayaRootDagPath(),
    _assemblyTypeName(assemblyTypeName),
    _proxyShapeTypeName(proxyShapeTypeName)
{
}

usdReadJob::~usdReadJob()
{
}

bool usdReadJob::doIt(std::vector<MDagPath>* addedDagPaths)
{
    MStatus status;

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(mFileName);
    if (!rootLayer) {
        return false;
    }

    TfToken modelName = UsdUtilsGetModelNameFromRootLayer(rootLayer);

    std::vector<std::pair<std::string, std::string> > varSelsVec;
    TF_FOR_ALL(iter, mVariants) {
        const std::string& variantSetName = iter->first;
        const std::string& variantSelectionName = iter->second;
        varSelsVec.push_back(
            std::make_pair(variantSetName, variantSelectionName));
    }

    SdfLayerRefPtr sessionLayer =
        UsdUtilsStageCache::GetSessionLayerForVariantSelections(modelName,
                                                                varSelsVec);

    // Layer and Stage used to Read in the USD file
    UsdStageCacheContext stageCacheContext(UsdMayaStageCache::Get());
    UsdStageRefPtr stage = UsdStage::Open(rootLayer, sessionLayer);
    if (!stage) {
        return false;
    }

    // If readAnimData is true, we expand the Min/Max time sliders to include
    // the stage's range if necessary.
    if (mArgs.readAnimData) {
        MTime currentMinTime = MAnimControl::minTime();
        MTime currentMaxTime = MAnimControl::maxTime();

        double startTimeCode, endTimeCode;
        if (mArgs.useCustomFrameRange) {
            if (mArgs.startTime > mArgs.endTime) {
                std::string errorMsg = TfStringPrintf(
                    "Frame range start (%f) was greater than end (%f)",
                    mArgs.startTime, mArgs.endTime);
                MGlobal::displayError(errorMsg.c_str());
                return false;
            }
            startTimeCode = mArgs.startTime;
            endTimeCode = mArgs.endTime;
        } else {
            startTimeCode = stage->GetStartTimeCode();
            endTimeCode = stage->GetEndTimeCode();
        }

        if (startTimeCode < currentMinTime.value()) {
            MAnimControl::setMinTime(MTime(startTimeCode));
        }
        if (endTimeCode > currentMaxTime.value()) {
            MAnimControl::setMaxTime(MTime(endTimeCode));
        }
    }

    // Use the primPath to get the root usdNode
    UsdPrim usdRootPrim = mPrimPath.empty() ? stage->GetDefaultPrim() :
        stage->GetPrimAtPath(SdfPath(mPrimPath));
    if (!usdRootPrim && !(mPrimPath.empty() || mPrimPath == "/")) {
        usdRootPrim = stage->GetPseudoRoot();
    }

    bool isImportingPsuedoRoot = (usdRootPrim == stage->GetPseudoRoot());

    if (!usdRootPrim) {
        std::string errorMsg = TfStringPrintf(
            "No default prim found in USD file \"%s\"",
            mFileName.c_str());
        MGlobal::displayError(errorMsg.c_str());
        return false;
    }

    SdfPrimSpecHandle usdRootPrimSpec =
        SdfCreatePrimInLayer(sessionLayer, usdRootPrim.GetPrimPath());
    if (!usdRootPrimSpec) {
        return false;
    }

    // Set the variants on the usdRootPrim
    for (std::map<std::string, std::string>::iterator it=mVariants.begin(); it!=mVariants.end(); ++it) {
        usdRootPrimSpec->SetVariantSelection(it->first, it->second);
    }

    bool isSceneAssembly = mMayaRootDagPath.node().hasFn(MFn::kAssembly);
    if (isSceneAssembly) {
        mArgs.shadingMode = ASSEMBLY_SHADING_MODE;
    }

    UsdTreeIterator primIt(usdRootPrim);

    // We maintain a registry mapping SdfPaths to MObjects as we create Maya
    // nodes, so prime the registry with the root Maya node and the
    // usdRootPrim's path.
    SdfPath rootPathToRegister = usdRootPrim.GetPath();

    if (isImportingPsuedoRoot || isSceneAssembly) {
        // Skip the root prim if it is the pseudoroot, or if we are importing
        // on behalf of a scene assembly.
        ++primIt;
    } else {
        // Otherwise, associate the usdRootPrim's *parent* with the root Maya
        // node instead.
        rootPathToRegister = rootPathToRegister.GetParentPath();
    }

    mNewNodeRegistry.insert(std::make_pair(
                rootPathToRegister.GetString(),
                mMayaRootDagPath.node()));

    if (mArgs.importWithProxyShapes) {
        _DoImportWithProxies(primIt);
    } else {
        _DoImport(primIt, usdRootPrim);
    }

    SdfPathSet topImportedPaths;
    if (isImportingPsuedoRoot) {
        // get all the dag paths for the root prims
        TF_FOR_ALL(childIter, stage->GetPseudoRoot().GetChildren()) {
            topImportedPaths.insert(childIter->GetPath());
        }
    } else {
        topImportedPaths.insert(usdRootPrim.GetPath());
    }

    TF_FOR_ALL(pathsIter, topImportedPaths) {
        std::string key = pathsIter->GetString();
        MObject obj;
        if (TfMapLookup(mNewNodeRegistry, key, &obj)) {
            if (obj.hasFn(MFn::kDagNode)) {
                addedDagPaths->push_back(MDagPath::getAPathTo(obj));
            }
        }
    }

    return (status == MS::kSuccess);
}


bool usdReadJob::_DoImport(UsdTreeIterator& primIt,
                           const UsdPrim& usdRootPrim)
{
    for(; primIt; ++primIt) {
        const UsdPrim& prim = *primIt;

        PxrUsdMayaPrimReaderArgs args(prim,
                                      mArgs.shadingMode,
                                      mArgs.defaultMeshScheme,
                                      mArgs.readAnimData,
                                      mArgs.useCustomFrameRange,
                                      mArgs.startTime,
                                      mArgs.endTime);
        PxrUsdMayaPrimReaderContext ctx(&mNewNodeRegistry);

        if (PxrUsdMayaTranslatorModelAssembly::ShouldImportAsAssembly(
                usdRootPrim,
                prim)) {
            // We use the file path of the file currently being imported and
            // the path to the prim within that file when creating the
            // reference assembly.
            const std::string refFilePath = mFileName;
            const SdfPath refPrimPath = prim.GetPath();

            // XXX: At some point, if assemblyRep == "import" we'd like
            // to import everything instead of just making an assembly.
            // Note: We may need to load the model if it isn't already.

            MObject parentNode = ctx.GetMayaNode(prim.GetPath().GetParentPath(), false);
            if (PxrUsdMayaTranslatorModelAssembly::Read(prim,
                                                        refFilePath,
                                                        refPrimPath,
                                                        parentNode,
                                                        args,
                                                        &ctx,
                                                        _assemblyTypeName,
                                                        mArgs.assemblyRep)) {
                if (ctx.GetPruneChildren()) {
                    primIt.PruneChildren();
                }
                continue;
            }
        }

        if (PxrUsdMayaPrimReaderRegistry::ReaderFn primReader
                = PxrUsdMayaPrimReaderRegistry::Find(prim.GetTypeName())) {
            primReader(args, &ctx);
            if (ctx.GetPruneChildren()) {
                primIt.PruneChildren();
            }
        }
    }

    return true;
}


bool usdReadJob::redoIt()
{
    // Undo the undo
    MStatus status = mDagModifierUndo.undoIt();
    if (status != MS::kSuccess) {
    }
    return (status == MS::kSuccess);
}


bool usdReadJob::undoIt()
{
    if (!mDagModifierSeeded) {
        mDagModifierSeeded = true;
        MStatus dagStatus;
        // Construct list of top level DAG nodes to delete and any DG nodes
        for (PathNodeMap::iterator it=mNewNodeRegistry.begin(); it!=mNewNodeRegistry.end(); ++it) {
            if (it->second != mMayaRootDagPath.node() ) { // if not the parent root node
                MFnDagNode dagFn(it->second, &dagStatus);
                if (dagStatus == MS::kSuccess) {
                    if (mMayaRootDagPath.node() != MObject::kNullObj) {
                        if (!dagFn.hasParent(mMayaRootDagPath.node() ))  { // skip if a DAG Node, but not under the root
                            continue;
                        }
                    }
                    else {
                        if (dagFn.parentCount() == 0)  { // under scene root
                            continue;
                        }
                    }
                }
                mDagModifierUndo.deleteNode(it->second);
            }
        }
    }
    MStatus status = mDagModifierUndo.doIt();
    if (status != MS::kSuccess) {
    }
    return (status == MS::kSuccess);
}


PXR_NAMESPACE_CLOSE_SCOPE

