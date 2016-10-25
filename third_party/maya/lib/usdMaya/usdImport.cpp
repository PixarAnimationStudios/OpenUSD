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
#include "usdMaya/usdImport.h"

#include "usdMaya/JobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/usdReadJob.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>

#include <map>
#include <string>
#include <vector>


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

    syntax.addFlag("-v"  , "-verbose", MSyntax::kNoArg);

    syntax.addFlag("-f" , "-file"               , MSyntax::kString);
    syntax.addFlag("-p" , "-parent"             , MSyntax::kString);
    syntax.addFlag("-shd" , "-shadingMode"      , MSyntax::kString);
    syntax.addFlag("-ani", "-readAnimData"      , MSyntax::kBoolean);
    syntax.addFlag("-pp", "-primPath"           , MSyntax::kString);
    syntax.addFlag("-var" , "-variant"          , MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-ar" , "-assemblyRep"      , MSyntax::kString);
    syntax.makeFlagMultiUse("variant");

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
        MGlobal::displayError("Invalid parameters detected.  Exiting.");
        return status;
    }

    JobImportArgs jobArgs;
    //bool verbose = argData.isFlagSet("verbose");
    
    std::string mFileName;
    if (argData.isFlagSet("file"))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("file", 0, tmpVal);

        // resolve the path into an absolute path
        MFileObject absoluteFile;
        absoluteFile.setRawFullName(tmpVal);
        absoluteFile.setRawFullName( absoluteFile.resolvedFullName() ); // Make sure an absolute path

        if (!absoluteFile.exists()) {
            MGlobal::displayError("File does not exist.  Exiting.");
            return MS::kFailure;
        }

        // Set the fileName
        mFileName = absoluteFile.resolvedFullName().asChar();
        MGlobal::displayInfo(MString("Importing ") + MString(mFileName.c_str()));
    }
    
    if (mFileName.empty()) {
        MString error = "Non empty file specified. Skipping...";
        MGlobal::displayError(error);
        return MS::kFailure;
    }

    if (argData.isFlagSet("shadingMode")) {
        MString stringVal;
        argData.getFlagArgument("shadingMode", 0, stringVal);
        TfToken shadingMode(stringVal.asChar());

        if (shadingMode.IsEmpty()) {
            jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
        } else if (PxrUsdMayaShadingModeRegistry::GetInstance().GetImporter(shadingMode)) {
            jobArgs.shadingMode = shadingMode;
        } else {
            if (shadingMode != PxrUsdMayaShadingModeTokens->none) {
                MGlobal::displayError(
                    TfStringPrintf("No shadingMode '%s' found. Setting shadingMode='none'",
                                   shadingMode.GetText()).c_str());
            }
            jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->none;
        }
    }

    if (argData.isFlagSet("readAnimData"))
    {   
        bool tmpBool = false;
        argData.getFlagArgument("readAnimData", 0, tmpBool);
        jobArgs.readAnimData = tmpBool;
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

    if (argData.isFlagSet("assemblyRep"))
    {
        // Get the value
        MString stringVal;
        argData.getFlagArgument("assemblyRep", 0, stringVal);
        std::string assemblyRep = stringVal.asChar();
        if (not assemblyRep.empty()) {
            jobArgs.assemblyRep = TfToken(assemblyRep);
        }
    }

    // Create the command
    if (mUsdReadJob) {
        delete mUsdReadJob;
    }


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
                std::string errorStr = TfStringPrintf(
                        "Invalid path \"%s\"for -parent.",
                        tmpVal.asChar());
                MGlobal::displayError(MString(errorStr.c_str()));
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
