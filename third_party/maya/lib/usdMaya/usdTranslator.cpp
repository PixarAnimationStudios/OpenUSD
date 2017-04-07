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
/**
 * \class usdTranslator
 * \brief file translator for USD files
 */

#include "pxr/pxr.h"
#include "usdMaya/usdTranslator.h"

#include "usdMaya/JobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/usdReadJob.h"
#include "usdMaya/usdWriteJob.h"

#include <maya/MAnimControl.h>
#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE



void* usdTranslator::creator(const std::string& assemblyTypeName,
                                   const std::string& proxyShapeTypeName) {
    return new usdTranslator(assemblyTypeName, proxyShapeTypeName);
}

usdTranslator::usdTranslator(const std::string& assemblyTypeName,
                                         const std::string& proxyShapeTypeName) :
        MPxFileTranslator(),
        _assemblyTypeName(assemblyTypeName),
        _proxyShapeTypeName(proxyShapeTypeName)
{
}

usdTranslator::~usdTranslator() {
}

MStatus usdTranslator::reader(const MFileObject & file,
                         const MString &optionsString, 
                         MPxFileTranslator::FileAccessMode mode ) {
    JobImportArgs jobArgs;
    jobArgs.fileName = file.fullName().asChar();

    jobArgs.parseImportOptions(optionsString);

    std::map<std::string,std::string> variants;

    usdReadJob *mUsdReadJob = new usdReadJob(variants, jobArgs,
                                             _assemblyTypeName,
                                             _proxyShapeTypeName);
    std::vector<MDagPath> addedDagPaths;
    bool success = mUsdReadJob->doIt(&addedDagPaths);
    return (success) ? MS::kSuccess : MS::kFailure;
}


MStatus
usdTranslator::writer(const MFileObject &file,
                 const MString &optionsString,
                 MPxFileTranslator::FileAccessMode mode ) {

    JobExportArgs jobArgs;
    jobArgs.fileName = file.fullName().asChar();
    bool append=false;

    // Now resync start and end frame based on animation mode
    if (jobArgs.exportAnimation) {
        if (jobArgs.endTime<jobArgs.startTime) jobArgs.endTime=jobArgs.startTime;
    } else {
        jobArgs.startTime=MAnimControl::currentTime().value();
        jobArgs.endTime=jobArgs.startTime;
    }

    if (jobArgs.frameSamples.empty()) {
        jobArgs.frameSamples.insert(0.0);
    }

    MSelectionList objSelList;
    if(mode == MPxFileTranslator::kExportActiveAccessMode) {
        // Get selected objects
        MGlobal::getActiveSelectionList(objSelList);
    } else if(mode == MPxFileTranslator::kExportAccessMode) {
        // Get all objects at DAG root
        objSelList.add("|*", true);
    }

    // Convert selection list to jobArgs dagPaths
    for (unsigned int i=0; i < objSelList.length(); i++) {
        MDagPath dagPath;
        if (objSelList.getDagPath(i, dagPath) == MS::kSuccess) {
            jobArgs.dagPaths.insert(dagPath);
        }
    }

    if (jobArgs.dagPaths.size()) {
        MTime oldCurTime = MAnimControl::currentTime();
        usdWriteJob writeJob(jobArgs);
        if (writeJob.beginJob(append)) {
            for (double i=jobArgs.startTime;i<(jobArgs.endTime+1);i++) {
                for (double sampleTime : jobArgs.frameSamples) {
                    double actualTime = i + sampleTime;
                    MGlobal::viewFrame(actualTime);
                    writeJob.evalJob(actualTime);
                }
            }
            writeJob.endJob();
            MGlobal::viewFrame(oldCurTime);
        } else {
            return MS::kFailure;
        }
    } else {
        MGlobal::displayWarning("No DAG nodes to export. Skipping");
    }

    return MS::kSuccess;
}


MPxFileTranslator::MFileKind
usdTranslator::identifyFile(
        const MFileObject& file,
        const char* buffer,
        short size) const
{
    MFileKind retValue = kNotMyFileType;
    const MString fileName = file.fullName();
    const int lastIndex = fileName.length() - 1;

    const int periodIndex = fileName.rindex('.');
    if (periodIndex < 0 || periodIndex >= lastIndex) {
        return retValue;
    }

    const MString fileExtension = fileName.substring(periodIndex + 1, lastIndex);

    if (fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText() ||
        fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText() ||
        fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText()) {
        retValue = kIsMyFileType;
    }

    return retValue;
}

PXR_NAMESPACE_CLOSE_SCOPE

