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
#include "usdMaya/usdExport.h"

#include "usdMaya/usdWriteJob.h"
#include "usdMaya/util.h"
#include "usdMaya/shadingModeRegistry.h"

#include <maya/MFileObject.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MAnimControl.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MComputation.h>
#include <maya/MObjectArray.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>
#include <maya/MTime.h>

#include "pxr/usd/usdGeom/tokens.h"

usdExport::usdExport()
{
}

usdExport::~usdExport()
{
}

MSyntax usdExport::createSyntax()
{
    MSyntax syntax;

    syntax.addFlag("-v"  , "-verbose", MSyntax::kNoArg);

    syntax.addFlag("-mt" , "-mergeTransformAndShape", MSyntax::kBoolean);
    syntax.addFlag("-eri", "-exportRefsAsInstanceable", MSyntax::kBoolean);
    syntax.addFlag("-dsp" , "-exportDisplayColor", MSyntax::kBoolean);
    syntax.addFlag("-shd" , "-shadingMode" , MSyntax::kString);
    syntax.addFlag("-uvs" , "-exportUVs", MSyntax::kBoolean);
    syntax.addFlag("-nuv" , "-normalizeMeshUVs" , MSyntax::kBoolean);
    syntax.addFlag("-nnu" , "-normalizeNurbs" , MSyntax::kBoolean);
    syntax.addFlag("-euv" , "-nurbsExplicitUVType" , MSyntax::kString);
    syntax.addFlag("-cls" , "-exportColorSets", MSyntax::kBoolean);
    syntax.addFlag("-dms" , "-defaultMeshScheme", MSyntax::kString);
    syntax.addFlag("-vis" , "-exportVisibility", MSyntax::kBoolean);

    syntax.addFlag("-fr" , "-frameRange"   , MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-pr" , "-preRoll"   , MSyntax::kDouble);

    syntax.addFlag("-ro"  , "-renderableOnly", MSyntax::kNoArg);
    syntax.addFlag("-sl"  , "-selection", MSyntax::kNoArg);
    syntax.addFlag("-dc"  , "-defaultCameras", MSyntax::kNoArg);
    syntax.addFlag("-rlm" , "-renderLayerMode" , MSyntax::kString);

    syntax.addFlag("-a" , "-append" , MSyntax::kBoolean);
    syntax.addFlag("-f" , "-file" , MSyntax::kString);

    syntax.addFlag("-atp" , "-attrprefix", MSyntax::kString);
    syntax.makeFlagMultiUse("-attrprefix");

    syntax.addFlag("-chr" , "-chaser", MSyntax::kString);
    syntax.makeFlagMultiUse("-chaser");

    syntax.addFlag("-cha" , "-chaserArgs", MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse("-chaserArgs");

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}


void* usdExport::creator()
{
    return new usdExport();
}


MStatus usdExport::doIt(const MArgList & args)
{
try
{
    MStatus status;

    MArgDatabase argData(syntax(), args, &status);

    // Check that all flags were valid
    if (status != MS::kSuccess) {
        MGlobal::displayError("Invalid parameters detected.  Exiting.");
        return status;
    }

    bool verbose = argData.isFlagSet("verbose");

    JobExportArgs jobArgs;

    if (argData.isFlagSet("mergeTransformAndShape")) {
        bool tmpBool = true;
        argData.getFlagArgument("mergeTransformAndShape", 0, tmpBool);
        jobArgs.mergeTransformAndShape = tmpBool;
    }

    if (argData.isFlagSet("exportRefsAsInstanceable")) {
        bool tmpBool = false;
        argData.getFlagArgument("exportRefsAsInstanceable", 0, tmpBool);
        jobArgs.exportRefsAsInstanceable = tmpBool;
    }

    if (argData.isFlagSet("exportDisplayColor")) {
        bool tmpBool = true;
        argData.getFlagArgument("exportDisplayColor", 0, tmpBool);
        jobArgs.exportDisplayColor = tmpBool;
    }
    
    if (argData.isFlagSet("shadingMode")) {
        MString stringVal;
        argData.getFlagArgument("shadingMode", 0, stringVal);
        TfToken shadingMode(stringVal.asChar());

        if (shadingMode.IsEmpty()) {
            jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
        }
        else {
            if (PxrUsdMayaShadingModeRegistry::GetInstance().GetExporter(shadingMode)) {
                jobArgs.shadingMode = shadingMode;
            }
            else {
                if (shadingMode != PxrUsdMayaShadingModeTokens->none) {
                    MGlobal::displayError(TfStringPrintf("No shadingMode '%s' found.  Setting shadingMode='none'", 
                                shadingMode.GetText()).c_str());
                }
                jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->none;
            }
        }
    }

    if (argData.isFlagSet("exportUVs")) {
        bool tmpBool = true;
        argData.getFlagArgument("exportUVs", 0, tmpBool);
        jobArgs.exportMeshUVs = tmpBool;
        jobArgs.exportNurbsExplicitUV = tmpBool;
    }

    if (argData.isFlagSet("normalizeMeshUVs")) {
        bool tmpBool = false;
        argData.getFlagArgument("normalizeMeshUVs", 0, tmpBool);
        jobArgs.normalizeMeshUVs = tmpBool;
    }

    if (argData.isFlagSet("normalizeNurbs")) {
        bool tmpBool = false;
        argData.getFlagArgument("normalizeNurbs", 0, tmpBool);
        jobArgs.normalizeNurbs = tmpBool;
    }

    if (argData.isFlagSet("nurbsExplicitUVType")) {
        MString stringVal;
        argData.getFlagArgument("nurbsExplicitUVType", 0, stringVal);
        if (stringVal=="uniform") {
            jobArgs.nurbsExplicitUVType = PxUsdExportJobArgsTokens->Uniform;
        }
    }

    if (argData.isFlagSet("exportColorSets")) {
        bool tmpBool = true;
        argData.getFlagArgument("exportColorSets", 0, tmpBool);
        jobArgs.exportColorSets = tmpBool;
    }

    if (argData.isFlagSet("defaultMeshScheme")) {
        MString stringVal;

        argData.getFlagArgument("defaultMeshScheme", 0, stringVal);
        if (stringVal=="none") {
            jobArgs.defaultMeshScheme = UsdGeomTokens->none;
        } else if (stringVal=="catmullClark") {
            jobArgs.defaultMeshScheme = UsdGeomTokens->catmullClark;
        } else if (stringVal=="loop") {
            jobArgs.defaultMeshScheme = UsdGeomTokens->loop;
        } else if (stringVal=="bilinear") {
            jobArgs.defaultMeshScheme = UsdGeomTokens->bilinear;
        } else {
            MGlobal::displayWarning("Incorrect Default Mesh Schema: " + stringVal +
            " defaulting to: " + MString(jobArgs.defaultMeshScheme.GetText()));
        }
    }

    if (argData.isFlagSet("exportVisibility")) {
        bool tmpBool = true;
        argData.getFlagArgument("exportVisibility", 0, tmpBool);
        jobArgs.exportVisibility = tmpBool;
    }

    bool append = false;
    std::string fileName;

    if (argData.isFlagSet("append")) {
        argData.getFlagArgument("append", 0, append);
    }

    if (argData.isFlagSet("file"))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("file", 0, tmpVal);

        // resolve the path into an absolute path
        MFileObject absoluteFile;
        absoluteFile.setRawFullName(tmpVal);
        absoluteFile.setRawFullName( absoluteFile.resolvedFullName() ); // Make sure an absolute path
        fileName = absoluteFile.resolvedFullName().asChar();

        if (fileName.empty()) {
            fileName = tmpVal.asChar();
        }
        MGlobal::displayInfo(MString("Saving as ") + MString(fileName.c_str()));
    }
    else {
        MString error = "-file not specified.";
        MGlobal::displayError(error);
        return MS::kFailure;
    }

    if (fileName.empty()) {
        return MS::kFailure;
    }

    double startTime=1;
    double endTime=1;
    double preRoll=0;
    
    // If you provide a frame range we consider this an anim
    // export even if start and end are the same
    if (argData.isFlagSet("frameRange")) {
        argData.getFlagArgument("frameRange", 0, startTime);
        argData.getFlagArgument("frameRange", 1, endTime);
        jobArgs.exportAnimation=true;
    } else {
        jobArgs.exportAnimation=false;
    }

    if (argData.isFlagSet("preRoll")) {
        argData.getFlagArgument("preRoll", 0, preRoll);
    }
    
    jobArgs.excludeInvisible = argData.isFlagSet("renderableOnly");
    jobArgs.exportDefaultCameras = argData.isFlagSet("defaultCameras");

    if (argData.isFlagSet("renderLayerMode")) {
        MString stringVal;
        argData.getFlagArgument("renderLayerMode", 0, stringVal);
        TfToken renderLayerMode(stringVal.asChar());

        if (renderLayerMode.IsEmpty()) {
            jobArgs.renderLayerMode = PxUsdExportJobArgsTokens->defaultLayer;
        } else if (renderLayerMode != PxUsdExportJobArgsTokens->defaultLayer &&
                   renderLayerMode != PxUsdExportJobArgsTokens->currentLayer &&
                   renderLayerMode != PxUsdExportJobArgsTokens->modelingVariant) {
            MGlobal::displayError(TfStringPrintf("Invalid renderLayerMode '%s'.  Setting renderLayerMode='defaultLayer'", 
                                renderLayerMode.GetText()).c_str());
            jobArgs.renderLayerMode = PxUsdExportJobArgsTokens->defaultLayer;
        } else {
            jobArgs.renderLayerMode = renderLayerMode;
        }
    }

    if (argData.isFlagSet("melPerFrameCallback"))
    {
        MString tmpVal;
        argData.getFlagArgument("melPerFrameCallback", 0, tmpVal);
        jobArgs.melPerFrameCallback.assign ( tmpVal.asUTF8() );
    }

    if (argData.isFlagSet("pythonPerFrameCallback"))
    {
        MString tmpVal;
        argData.getFlagArgument("pythonPerFrameCallback", 0, tmpVal);
        jobArgs.pythonPerFrameCallback.assign ( tmpVal.asUTF8() );
    }

    if (argData.isFlagSet("melPostJobCallback"))
    {
        MString tmpVal;
        argData.getFlagArgument("melPostJobCallback", 0, tmpVal);
        jobArgs.melPostCallback.assign ( tmpVal.asUTF8() );
    }

    if (argData.isFlagSet("pythonPostJobCallback"))
    {
        MString tmpVal;
        argData.getFlagArgument("pythonPostJobCallback", 0, tmpVal);
        jobArgs.pythonPostCallback.assign ( tmpVal.asUTF8() );
    }

    unsigned int numChasers = argData.numberOfFlagUses("chaser");
    for (unsigned int i=0; i < numChasers; i++) {
        MArgList tmpArgList;
        argData.getFlagArgumentList("chaser", i, tmpArgList);
        std::string argValue = std::string(tmpArgList.asString(0).asChar());
        jobArgs.chaserNames.push_back(argValue);
    }

    unsigned int numChaserArgs = argData.numberOfFlagUses("chaserArgs");
    for (unsigned int i=0; i < numChaserArgs; i++) {
        MArgList tmpArgList;
        argData.getFlagArgumentList("chaserArgs", i, tmpArgList);
        std::string chaserName = std::string(tmpArgList.asString(0).asChar());
        std::string argName = std::string(tmpArgList.asString(1).asChar());
        std::string argValue = std::string(tmpArgList.asString(2).asChar());

        if (std::find(jobArgs.chaserNames.begin(), 
                    jobArgs.chaserNames.end(), 
                    chaserName)
                != jobArgs.chaserNames.end()) {
            jobArgs.allChaserArgs[chaserName][argName] = argValue;
        }
        else {
            std::string warning = TfStringPrintf("Bad chaserArg for unknown chaser: '%s'",
                    chaserName.c_str());
            MGlobal::displayWarning(MString(warning.c_str()));
        }
    }


    // Get the objects to export as a MSelectionList
    MSelectionList objSelList;
    if (argData.isFlagSet("selection")) {
        MGlobal::getActiveSelectionList(objSelList);
    }
    else {
        argData.getObjects(objSelList);

        // If no objects specified, then get all objects at DAG root
        if (objSelList.isEmpty()) {
            objSelList.add("|*", true);
        }
    }

    // Convert selection list to jobArgs dagPaths
    for (unsigned int i=0; i < objSelList.length(); i++) {
        MDagPath dagPath;
        status = objSelList.getDagPath(i, dagPath);
        if (status == MS::kSuccess)
        {
            jobArgs.dagPaths.insert(dagPath);
        }
    }

    // Create WriteJob object
    usdWriteJob usdWriteJob(jobArgs);

    MComputation computation;
    computation.beginComputation();

    // Create stage and process static data
    if (usdWriteJob.beginJob(fileName, append, startTime, endTime)) {
        if (jobArgs.exportAnimation) {
            MTime oldCurTime = MAnimControl::currentTime();
            for (double i=startTime;i<(endTime+1);i++) {
                if (verbose) {
                    MString info;
                    info = i;
                    MGlobal::displayInfo(info);
                }
                MGlobal::viewFrame(i);
                // Process per frame data
                usdWriteJob.evalJob(i);
                if (computation.isInterruptRequested()) {
                    break;
                }
            }
            // set the time back
            MGlobal::viewFrame(oldCurTime);
        }

        // Finalize the export, close the stage
        usdWriteJob.endJob();
    }

    computation.endComputation();

    return MS::kSuccess;
} // end of try block
catch (std::exception & e)
{
    MString theError("std::exception encountered: ");
    theError += e.what();
    MGlobal::displayError(theError);
    return MS::kFailure;
}
} // end of function
