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
 * \class usdTranslatorImport
 * \brief file translator for USD files
 */

#include "usdMaya/usdTranslatorImport.h"

#include "usdMaya/JobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/usdReadJob.h"
#include "usdMaya/usdWriteJob.h"

#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MString.h>

#include <map>
#include <string>


void* usdTranslatorImport::creator(const std::string& assemblyTypeName,
                                   const std::string& proxyShapeTypeName) {
    return new usdTranslatorImport(assemblyTypeName, proxyShapeTypeName);
}

usdTranslatorImport::usdTranslatorImport(const std::string& assemblyTypeName,
                                         const std::string& proxyShapeTypeName) :
        MPxFileTranslator(),
        _assemblyTypeName(assemblyTypeName),
        _proxyShapeTypeName(proxyShapeTypeName)
{
}

usdTranslatorImport::~usdTranslatorImport() {
}

MStatus usdTranslatorImport::reader(const MFileObject & file,
                         const MString &optionsString, 
                         MPxFileTranslator::FileAccessMode mode ) {
    std::string fileName(file.fullName().asChar());
    std::string primPath("/");
    std::map<std::string,std::string> variants;
    JobImportArgs jobArgs;
        
    int i;

    if ( optionsString.length() > 0 ) {
        MStringArray optionList;
        MStringArray theOption;
        optionsString.split(';', optionList);
        for(i=0; i<(int)optionList.length(); ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);
            if (theOption[0] == MString("shadingMode")) {
                if (theOption[1]=="None") {
                    jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->none;
                } else if (theOption[1]=="GPrim Colors") {
                    jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
                } else if (theOption[1]=="Look Colors") {
                    jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
                } else if (theOption[1]=="RfM Shaders") {
                    TfToken shadingMode("pxrRis");
                    if (PxrUsdMayaShadingModeRegistry::GetInstance().GetExporter(shadingMode)) {
                        jobArgs.shadingMode = shadingMode;
                    } else {
                        MGlobal::displayError(
                        TfStringPrintf("No shadingMode '%s' found.  Setting shadingMode='none'", 
                                        shadingMode.GetText()).c_str());
                        jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->none;
                    }
                }
            } else if (theOption[0] == MString("readAnimData")) {
                jobArgs.readAnimData = theOption[1].asInt();
            } else if (theOption[0] == MString("assemblyRep")) {
                jobArgs.assemblyRep = TfToken(theOption[1].asChar());
            } else if (theOption[0] == MString("startTime")) {
                jobArgs.startTime = theOption[1].asDouble();
            } else if (theOption[0] == MString("endTime")) {
                jobArgs.endTime = theOption[1].asDouble();
            } else if (theOption[0] == MString("useCustomFrameRange")) {
                jobArgs.useCustomFrameRange = theOption[1].asInt();
            }
        }
    }

    usdReadJob *mUsdReadJob = new usdReadJob(fileName, primPath, variants, jobArgs,
            _assemblyTypeName, _proxyShapeTypeName);
    std::vector<MDagPath> addedDagPaths;
    bool success = mUsdReadJob->doIt(&addedDagPaths);
    return (success) ? MS::kSuccess : MS::kFailure;
}

MPxFileTranslator::MFileKind
usdTranslatorImport::identifyFile(
        const MFileObject& file,
        const char* buffer,
        short size) const
{
    MFileKind retValue = kNotMyFileType;
    const MString fileName = file.fullName();
    const int lastIndex = fileName.length() - 1;

    const int periodIndex = fileName.rindex('.');
    if (periodIndex < 0 or periodIndex >= lastIndex) {
        return retValue;
    }

    const MString fileExtension = fileName.substring(periodIndex + 1, lastIndex);

    if (fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText() or
        fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText() or
        fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText()) {
        retValue = kIsMyFileType;
    }

    return retValue;
}
