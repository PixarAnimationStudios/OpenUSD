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
#include "usdMaya/usdImport.h"

#include "usdMaya/jobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/usdReadJob.h"

#include "pxr/usd/ar/resolver.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



usdImport::usdImport(const std::string& assemblyTypeName,
                     const std::string& proxyShapeTypeName) :
    mUsdReadJob(NULL),
    _assemblyTypeName(assemblyTypeName),
    _proxyShapeTypeName(proxyShapeTypeName)
{
}

usdImport::~usdImport()
{
    if (mUsdReadJob) {
        delete mUsdReadJob;
    }
}

MSyntax usdImport::createSyntax()
{
    MSyntax syntax;

    // These flags correspond to entries in
    // PxrUsdMayaJobImportArgs::GetDefaultDictionary.
    syntax.addFlag("-shd",
                   PxrUsdImportJobArgsTokens->shadingMode.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-ar",
                   PxrUsdImportJobArgsTokens->assemblyRep.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-md",
                   PxrUsdImportJobArgsTokens->metadata.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(PxrUsdImportJobArgsTokens->metadata.GetText());
    syntax.addFlag("-api",
                   PxrUsdImportJobArgsTokens->apiSchema.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(PxrUsdImportJobArgsTokens->apiSchema.GetText());
    syntax.addFlag("-epv",
                   PxrUsdImportJobArgsTokens->excludePrimvar.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(
            PxrUsdImportJobArgsTokens->excludePrimvar.GetText());

    // These are additional flags under our control.
    syntax.addFlag("-f" , "-file", MSyntax::kString);
    syntax.addFlag("-p" , "-parent", MSyntax::kString);
    syntax.addFlag("-ani", "-readAnimData", MSyntax::kBoolean);
    syntax.addFlag("-fr", "-frameRange", MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-var", "-variant", MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse("variant");

    syntax.addFlag("-v", "-verbose", MSyntax::kNoArg);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}


void* usdImport::creator(const std::string& assemblyTypeName,
                         const std::string& proxyShapeTypeName)
{
    return new usdImport(assemblyTypeName, proxyShapeTypeName);
}

MStatus usdImport::doIt(const MArgList & args)
{

    MStatus status;

    MArgDatabase argData(syntax(), args, &status);

    // Check that all flags were valid
    if (status != MS::kSuccess) {
        return status;
    }

    // Get dictionary values.
    const VtDictionary userArgs = PxrUsdMayaUtil::GetDictionaryFromArgDatabase(
            argData, PxrUsdMayaJobImportArgs::GetDefaultDictionary());
    
    std::string mFileName;
    if (argData.isFlagSet("file"))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("file", 0, tmpVal);
        mFileName = tmpVal.asChar();

        // Use the usd resolver for validation (but save the unresolved)
        if (ArGetResolver().Resolve(mFileName).empty()) {
            TF_RUNTIME_ERROR(
                    "File '%s' does not exist, or could not be resolved. "
                    "Exiting.",
                    mFileName.c_str());
            return MS::kFailure;
        }

        TF_STATUS("Importing '%s'", mFileName.c_str());
    }
    
    if (mFileName.empty()) {
        TF_RUNTIME_ERROR("Empty file specified. Exiting.");
        return MS::kFailure;
    }

    // Specify usd PrimPath.  Default will be "/<useFileBasename>"
    std::string mPrimPath;
    if (argData.isFlagSet("primPath"))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("primPath", 0, tmpVal);
        mPrimPath = tmpVal.asChar();
    }

    // Add variant (variantSet, variant).  Multi-use
    std::map<std::string,std::string> mVariants;
    for (unsigned int i=0; i < argData.numberOfFlagUses("variant"); ++i)
    {
        MArgList tmpArgList;
        status = argData.getFlagArgumentList("variant", i, tmpArgList);
        // Get the value
        MString tmpKey = tmpArgList.asString(0, &status);
        MString tmpVal = tmpArgList.asString(1, &status);
        mVariants.insert( std::pair<std::string, std::string>(tmpKey.asChar(), tmpVal.asChar()) );
    }

    bool readAnimData = true;
    if (argData.isFlagSet("readAnimData"))
    {   
        argData.getFlagArgument("readAnimData", 0, readAnimData);
    }

    GfInterval timeInterval;
    if (readAnimData) {
        if (argData.isFlagSet("frameRange")) {
            double startTime = 1.0;
            double endTime = 1.0;
            argData.getFlagArgument("frameRange", 0, startTime);
            argData.getFlagArgument("frameRange", 1, endTime);
            timeInterval = GfInterval(startTime, endTime);
        }
        else {
            timeInterval = GfInterval::GetFullInterval();
        }
    }
    else {
        timeInterval = GfInterval();
    }

    // Create the command
    if (mUsdReadJob) {
        delete mUsdReadJob;
    }

    PxrUsdMayaJobImportArgs jobArgs =
            PxrUsdMayaJobImportArgs::CreateFromDictionary(
                userArgs, /*importWithProxyShapes*/ false, timeInterval);

    // pass in assemblyTypeName and proxyShapeTypeName
    mUsdReadJob = new usdReadJob(mFileName, mPrimPath, mVariants, jobArgs,
            _assemblyTypeName, _proxyShapeTypeName);


    // Add optional command params
    if (argData.isFlagSet("parent"))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("parent", 0, tmpVal);

        if (tmpVal.length()) {
            MSelectionList selList;
            selList.add(tmpVal);
            MDagPath dagPath;
            status = selList.getDagPath(0, dagPath);
            if (status != MS::kSuccess) {
                TF_RUNTIME_ERROR(
                        "Invalid path '%s' for -parent.",
                        tmpVal.asChar());
                return MS::kFailure;
            }
            mUsdReadJob->setMayaRootDagPath( dagPath );
        }
    }

    // Execute the command
    std::vector<MDagPath> addedDagPaths;
    bool success = mUsdReadJob->doIt(&addedDagPaths);
    if (success) {
        TF_FOR_ALL(iter, addedDagPaths) {
            appendToResult(iter->fullPathName());
        }
    }
    return (success) ? MS::kSuccess : MS::kFailure;
}


MStatus usdImport::redoIt()
{
    if (!mUsdReadJob) {
        return MS::kFailure;
    }

    bool success = mUsdReadJob->redoIt();

    return (success) ? MS::kSuccess : MS::kFailure;
}


MStatus usdImport::undoIt()
{
    if (!mUsdReadJob) {
        return MS::kFailure;
    }
    
    bool success = mUsdReadJob->undoIt();

    return (success) ? MS::kSuccess : MS::kFailure;
}

PXR_NAMESPACE_CLOSE_SCOPE

