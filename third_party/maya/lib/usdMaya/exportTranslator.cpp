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
#include "usdMaya/exportTranslator.h"

#include "usdMaya/jobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/writeJob.h"
#include "usdMaya/writeUtil.h"

#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE



void* UsdMayaExportTranslator::creator() {
    return new UsdMayaExportTranslator();
}

UsdMayaExportTranslator::UsdMayaExportTranslator() :
        MPxFileTranslator() {
}

UsdMayaExportTranslator::~UsdMayaExportTranslator() {
}

MStatus
UsdMayaExportTranslator::writer(const MFileObject &file, 
                 const MString &optionsString,
                 MPxFileTranslator::FileAccessMode mode ) {

    std::string fileName(file.fullName().asChar());
    VtDictionary userArgs;
    bool exportAnimation = false;
    GfInterval timeInterval(1.0, 1.0);
    double frameStride = 1.0;
    bool append=false;
    
    MStringArray filteredTypes;
    // Get the options 
    if ( optionsString.length() > 0 ) {
        MStringArray optionList;
        MStringArray theOption;
        optionsString.split(';', optionList);
        for(int i=0; i<(int)optionList.length(); ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);
            if (theOption.length() != 2) {
                continue;
            }

            std::string argName(theOption[0].asChar());
            if (argName == "animation") {
                exportAnimation = (theOption[1].asInt() != 0);
            }
            else if (argName == "startTime") {
                timeInterval.SetMin(theOption[1].asDouble());
            }
            else if (argName == "endTime") {
                timeInterval.SetMax(theOption[1].asDouble());
            }
            else if (argName == "frameStride") {
                frameStride = theOption[1].asDouble();
            }
            else if (argName == "filterTypes") {
                theOption[1].split(',', filteredTypes);
            }
            else {
                userArgs[argName] = UsdMayaUtil::ParseArgumentValue(
                    argName, theOption[1].asChar(),
                    UsdMayaJobExportArgs::GetDefaultDictionary());
            }
        }
    }


    // Now resync start and end frame based on export time interval.
    if (exportAnimation) {
        if (timeInterval.IsEmpty()) {
            // If the user accidentally set start > end, resync to the closed
            // interval with the single start point.
            timeInterval = GfInterval(timeInterval.GetMin());
        }
    }
    else {
        // No animation, so empty interval.
        timeInterval = GfInterval();
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
    UsdMayaUtil::MDagPathSet dagPaths;
    for (unsigned int i=0; i < objSelList.length(); i++) {
        MDagPath dagPath;
        if (objSelList.getDagPath(i, dagPath) == MS::kSuccess) {
            dagPaths.insert(dagPath);
        }
    }
    
    if (dagPaths.empty()) {
        TF_WARN("No DAG nodes to export. Skipping.");
        return MS::kSuccess;
    }

    const std::vector<double> timeSamples = UsdMayaWriteUtil::GetTimeSamples(
            timeInterval, std::set<double>(), frameStride);
    UsdMayaJobExportArgs jobArgs = UsdMayaJobExportArgs::CreateFromDictionary(
            userArgs, dagPaths, timeSamples);
    for (unsigned int i=0; i < filteredTypes.length(); ++i) {
        jobArgs.AddFilteredTypeName(filteredTypes[i].asChar());
    }

    UsdMaya_WriteJob writeJob(jobArgs);
    if (!writeJob.Write(fileName, append)) {
        return MS::kFailure;
    }
    
    return MS::kSuccess;
}

MPxFileTranslator::MFileKind
UsdMayaExportTranslator::identifyFile(
        const MFileObject& file,
        const char*  /*buffer*/,
        short  /*size*/) const
{
    MFileKind retValue = kNotMyFileType;
    const MString fileName = file.fullName();
    const int lastIndex = fileName.length() - 1;

    const int periodIndex = fileName.rindex('.');
    if (periodIndex < 0 || periodIndex >= lastIndex) {
        return retValue;
    }

    const MString fileExtension = fileName.substring(periodIndex + 1, lastIndex);

    if (fileExtension ==
            UsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText() || 
        fileExtension ==
            UsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText() || 
        fileExtension ==
            UsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText() ||
        fileExtension ==
            UsdMayaTranslatorTokens->UsdFileExtensionPackage.GetText()) {
        retValue = kIsMyFileType;
    }

    return retValue;
}

/* static */
const std::string&
UsdMayaExportTranslator::GetDefaultOptions()
{
    static std::string defaultOptions;
    static std::once_flag once;
    std::call_once(once, []() {
        std::vector<std::string> entries;
        for (const std::pair<std::string, VtValue> keyValue :
                UsdMayaJobExportArgs::GetDefaultDictionary()) {
            if (keyValue.second.IsHolding<bool>()) {
                entries.push_back(TfStringPrintf("%s=%d",
                        keyValue.first.c_str(),
                        static_cast<int>(keyValue.second.Get<bool>())));
            }
            else if (keyValue.second.IsHolding<std::string>()) {
                entries.push_back(TfStringPrintf("%s=%s",
                        keyValue.first.c_str(),
                        keyValue.second.Get<std::string>().c_str()));
            }
        }
        entries.push_back("animation=0");
        entries.push_back("startTime=1");
        entries.push_back("endTime=1");
        entries.push_back("frameStride=1.0");
        defaultOptions = TfStringJoin(entries, ";");
    });

    return defaultOptions;
}

PXR_NAMESPACE_CLOSE_SCOPE

