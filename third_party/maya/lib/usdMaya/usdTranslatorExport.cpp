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
 * \class usdTranslatorExport
 * \brief file translator for USD files
 */

#include "usdMaya/usdTranslatorExport.h"

#include "usdMaya/JobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/usdWriteJob.h"

#include <maya/MAnimControl.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>


void* usdTranslatorExport::creator() {
    return new usdTranslatorExport();
}

usdTranslatorExport::usdTranslatorExport() :
        MPxFileTranslator() {
}

usdTranslatorExport::~usdTranslatorExport() {
}

MStatus
usdTranslatorExport::writer(const MFileObject &file, 
                 const MString &optionsString,
                 MPxFileTranslator::FileAccessMode mode ) {

    std::string fileName(file.fullName().asChar());
    JobExportArgs jobArgs;
    double startTime=1, endTime=1;
    std::set<double> frameSamples;
    bool append=false;
    
    // Get the options 
    if ( optionsString.length() > 0 ) {
        MStringArray optionList;
        MStringArray theOption;
        optionsString.split(';', optionList);
        for(int i=0; i<(int)optionList.length(); ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);            
            if (theOption[0] == MString("exportReferencesAsInstanceable")) {
                jobArgs.exportRefsAsInstanceable = theOption[1].asInt();
            }
            if (theOption[0] == MString("shadingMode")) {
                // Set default (most common) options
                jobArgs.exportDisplayColor = true;
                jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->none;
                
                if (theOption[1]=="None") {
                    jobArgs.exportDisplayColor = false;
                }else if (theOption[1]=="Look Colors") {
                    jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
                } else if (theOption[1]=="RfM Shaders") {
                    TfToken shadingMode("pxrRis");
                    if (PxrUsdMayaShadingModeRegistry::GetInstance().GetExporter(shadingMode)) {
                        jobArgs.shadingMode = shadingMode;
                    }
                }
            }
            if (theOption[0] == MString("exportUVs")) {
                jobArgs.exportMeshUVs = theOption[1].asInt();
                jobArgs.exportNurbsExplicitUV = theOption[1].asInt();
            }
            if (theOption[0] == MString("normalizeUVs")) {
                jobArgs.normalizeMeshUVs = theOption[1].asInt();
                jobArgs.nurbsExplicitUVType = PxUsdExportJobArgsTokens->Uniform;
            }
            if (theOption[0] == MString("exportColorSets")) {
                jobArgs.exportColorSets = theOption[1].asInt();
            }
            if (theOption[0] == MString("renderableOnly")) {
                jobArgs.excludeInvisible = theOption[1].asInt();
            }
            if (theOption[0] == MString("allCameras")) {
                jobArgs.exportDefaultCameras = theOption[1].asInt();
            }
            if (theOption[0] == MString("renderLayerMode")) {
                jobArgs.renderLayerMode = PxUsdExportJobArgsTokens->defaultLayer;

                if (theOption[1]=="Use Current Layer") {
                    jobArgs.renderLayerMode = PxUsdExportJobArgsTokens->currentLayer;
                } else if (theOption[1]=="Modeling Variant Per Layer") {
                    jobArgs.renderLayerMode = PxUsdExportJobArgsTokens->modelingVariant;
                }
            }
            if (theOption[0] == MString("mergeXForm")) {
                jobArgs.mergeTransformAndShape = theOption[1].asInt();
            }
            if (theOption[0] == MString("defaultMeshScheme")) {            
                if (theOption[1]=="Polygonal Mesh") {
                    jobArgs.defaultMeshScheme = UsdGeomTokens->none;
                } else if (theOption[1]=="Bilinear SubDiv") {
                    jobArgs.defaultMeshScheme = UsdGeomTokens->bilinear;
                } else if (theOption[1]=="CatmullClark SDiv") {
                    jobArgs.defaultMeshScheme = UsdGeomTokens->catmullClark;
                } else if (theOption[1]=="Loop SDiv") {
                    jobArgs.defaultMeshScheme = UsdGeomTokens->loop;
                }
            }
            if (theOption[0] == MString("exportVisibility")) {
                jobArgs.exportVisibility = theOption[1].asInt();
            }
            if (theOption[0] == MString("animation")) {
                jobArgs.exportAnimation = theOption[1].asInt();
            }
            if (theOption[0] == MString("startTime")) {
                startTime = theOption[1].asDouble();
            }
            if (theOption[0] == MString("endTime")) {
                endTime = theOption[1].asDouble();
            }
            if (theOption[0] == MString("frameSample")) {
                frameSamples.insert(theOption[1].asDouble());
            }
        }
        // Now resync start and end frame based on animation mode
        if (jobArgs.exportAnimation) {
            if (endTime<startTime) endTime=startTime;
        } else {
            startTime=MAnimControl::currentTime().value();
            endTime=startTime;
        }
    }

    if (frameSamples.empty()) {
        frameSamples.insert(0.0);
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
        if (writeJob.beginJob(fileName, append, startTime, endTime)) {
            for (double i=startTime;i<(endTime+1);i++) {
                for (double sampleTime : frameSamples) {
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
usdTranslatorExport::identifyFile(
        const MFileObject& file,
        const char* buffer,
        short size) const
{
    std::string fileName(file.fullName().asChar());
    if(UsdStage::IsSupportedFile(fileName))
    {
        return kIsMyFileType;
    }
    return kNotMyFileType;
}
