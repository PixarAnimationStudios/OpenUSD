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
#include "usdMaya/exportCommand.h"

#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJob.h"
#include "usdMaya/writeUtil.h"

#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>
#include <maya/MTime.h>

PXR_NAMESPACE_OPEN_SCOPE


UsdMayaExportCommand::UsdMayaExportCommand()
{
}

UsdMayaExportCommand::~UsdMayaExportCommand()
{
}

MSyntax UsdMayaExportCommand::createSyntax()
{
    MSyntax syntax;

    // These flags correspond to entries in
    // UsdMayaJobExportArgs::GetDefaultDictionary.
    syntax.addFlag("-mt",
                   UsdMayaJobExportArgsTokens->mergeTransformAndShape.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-ein",
                   UsdMayaJobExportArgsTokens->exportInstances.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-eri",
                   UsdMayaJobExportArgsTokens->exportRefsAsInstanceable.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-dsp",
                   UsdMayaJobExportArgsTokens->exportDisplayColor.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-shd",
                   UsdMayaJobExportArgsTokens->shadingMode.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-msn",
                   UsdMayaJobExportArgsTokens->materialsScopeName.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-uvs",
                   UsdMayaJobExportArgsTokens->exportUVs.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-mcs",
                   UsdMayaJobExportArgsTokens->exportMaterialCollections
                       .GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-mcp",
                   UsdMayaJobExportArgsTokens->materialCollectionsPath.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-cbb",
                   UsdMayaJobExportArgsTokens->exportCollectionBasedBindings
                       .GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-nnu",
                   UsdMayaJobExportArgsTokens->normalizeNurbs.GetText() ,
                   MSyntax::kBoolean);
    syntax.addFlag("-cls",
                   UsdMayaJobExportArgsTokens->exportColorSets.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-sn",
                   UsdMayaJobExportArgsTokens->stripNamespaces.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-ef" ,
                   UsdMayaJobExportArgsTokens->eulerFilter.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-dms",
                   UsdMayaJobExportArgsTokens->defaultMeshScheme.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-vis",
                   UsdMayaJobExportArgsTokens->exportVisibility.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-ero" ,
                   UsdMayaJobExportArgsTokens->exportReferenceObjects.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag("-skl",
                   UsdMayaJobExportArgsTokens->exportSkels.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-skn",
                   UsdMayaJobExportArgsTokens->exportSkin.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-psc",
                   UsdMayaJobExportArgsTokens->parentScope.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-ro",
                   UsdMayaJobExportArgsTokens->renderableOnly.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag("-dc",
                   UsdMayaJobExportArgsTokens->defaultCameras.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag("-rlm",
                   UsdMayaJobExportArgsTokens->renderLayerMode.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-k",
                   UsdMayaJobExportArgsTokens->kind.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-com",
                   UsdMayaJobExportArgsTokens->compatibility.GetText(),
                   MSyntax::kString);

    syntax.addFlag("-chr",
                   UsdMayaJobExportArgsTokens->chaser.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(UsdMayaJobExportArgsTokens->chaser.GetText());

    syntax.addFlag("-cha",
                   UsdMayaJobExportArgsTokens->chaserArgs.GetText(),
                   MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse(UsdMayaJobExportArgsTokens->chaserArgs.GetText());

    syntax.addFlag("-mfc",
                   UsdMayaJobExportArgsTokens->melPerFrameCallback.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag("-mpc",
                   UsdMayaJobExportArgsTokens->melPostCallback.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag("-pfc",
                   UsdMayaJobExportArgsTokens->pythonPerFrameCallback.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-ppc",
                   UsdMayaJobExportArgsTokens->pythonPostCallback.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-v",
                   UsdMayaJobExportArgsTokens->verbose.GetText(),
                   MSyntax::kNoArg);

    // These are additional flags under our control.
    syntax.addFlag("-fr", "-frameRange", MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-ft", "-frameStride", MSyntax::kDouble);
    syntax.addFlag("-fs", "-frameSample", MSyntax::kDouble);
    syntax.makeFlagMultiUse("-frameSample");

    syntax.addFlag("-a", "-append", MSyntax::kBoolean);
    syntax.addFlag("-f", "-file", MSyntax::kString);
    syntax.addFlag("-sl", "-selection", MSyntax::kNoArg);

    syntax.addFlag("-ft" , "-filterTypes", MSyntax::kString);
    syntax.makeFlagMultiUse("-filterTypes");

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    syntax.setObjectType(MSyntax::kSelectionList);
    syntax.setMinObjects(0);

    return syntax;
}


void* UsdMayaExportCommand::creator()
{
    return new UsdMayaExportCommand();
}


MStatus UsdMayaExportCommand::doIt(const MArgList & args)
{
try
{
    MStatus status;

    MArgDatabase argData(syntax(), args, &status);

    // Check that all flags were valid
    if (status != MS::kSuccess) {
        return status;
    }

    // Read all of the dictionary args first.
    const VtDictionary userArgs = UsdMayaUtil::GetDictionaryFromArgDatabase(
            argData, UsdMayaJobExportArgs::GetDefaultDictionary());

    // Now read all of the other args that are specific to this command.
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
    }
    else {
        TF_RUNTIME_ERROR("-file not specified.");
        return MS::kFailure;
    }

    if (fileName.empty()) {
        return MS::kFailure;
    }

    // If you provide a frame range we consider this an anim
    // export even if start and end are the same
    GfInterval timeInterval;
    if (argData.isFlagSet("frameRange")) {
        double startTime = 1;
        double endTime = 1;
        argData.getFlagArgument("frameRange", 0, startTime);
        argData.getFlagArgument("frameRange", 1, endTime);
        if (startTime > endTime) {
            // If the user accidentally set start > end, resync to the closed
            // interval with the single start point.
            timeInterval = GfInterval(startTime);
        }
        else {
            // Use the user's interval as-is.
            timeInterval = GfInterval(startTime, endTime);
        }
    } else {
        // No animation, so empty interval.
        timeInterval = GfInterval();
    }

    double frameStride = 1.0;
    if (argData.isFlagSet("frameStride")) {
        argData.getFlagArgument("frameStride", 0, frameStride);
    }

    std::set<double> frameSamples;
    unsigned int numFrameSamples = argData.numberOfFlagUses("frameSample");
    for (unsigned int i = 0; i < numFrameSamples; ++i) {
        MArgList tmpArgList;
        argData.getFlagArgumentList("frameSample", i, tmpArgList);
        frameSamples.insert(tmpArgList.asDouble(0));
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
    UsdMayaUtil::MDagPathSet dagPaths;
    for (unsigned int i=0; i < objSelList.length(); i++) {
        MDagPath dagPath;
        status = objSelList.getDagPath(i, dagPath);
        if (status == MS::kSuccess)
        {
            dagPaths.insert(dagPath);
        }
    }

    const std::vector<double> timeSamples = UsdMayaWriteUtil::GetTimeSamples(
            timeInterval, frameSamples, frameStride);
    UsdMayaJobExportArgs jobArgs = UsdMayaJobExportArgs::CreateFromDictionary(
            userArgs, dagPaths, timeSamples);

    unsigned int numFilteredTypes = argData.numberOfFlagUses("filterTypes");
    for (unsigned int i=0; i < numFilteredTypes; i++) {
        MArgList tmpArgList;
        argData.getFlagArgumentList("filterTypes", i, tmpArgList);
        jobArgs.AddFilteredTypeName(tmpArgList.asString(0));
    }

    UsdMaya_WriteJob writeJob(jobArgs);
    if (!writeJob.Write(fileName, append)) {
        return MS::kFailure;
    }

    return MS::kSuccess;
} // end of try block
catch (std::exception & e)
{
    TF_RUNTIME_ERROR("std::exception encountered: %s", e.what());
    return MS::kFailure;
}
} // end of function

PXR_NAMESPACE_CLOSE_SCOPE

