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

#include "pxr/pxr.h"
#include "usdMaya/usdTranslatorExport.h"

#include "usdMaya/JobArgs.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/usdWriteJob.h"

#include <maya/MAnimControl.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE



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
    bool exportAnimation = false;
    GfInterval timeInterval(1.0, 1.0);
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
                TfToken shadingMode(theOption[1].asChar());
                if (!shadingMode.IsEmpty()) {
                    if (PxrUsdMayaShadingModeRegistry::GetInstance()
                            .GetExporter(shadingMode)) {
                        jobArgs.shadingMode = shadingMode;
                    }
                    else {
                        if (shadingMode != PxrUsdMayaShadingModeTokens->none) {
                            MGlobal::displayError(TfStringPrintf(
                                    "No shadingMode '%s' found. "
                                    "Setting shadingMode='none'", 
                                    shadingMode.GetText()).c_str());
                        }
                        jobArgs.shadingMode = PxrUsdMayaShadingModeTokens->none;
                    }
                }
            }
            if (theOption[0] == MString("exportDisplayColor")) {
                jobArgs.exportDisplayColor = theOption[1].asInt();
            }
            if (theOption[0] == MString("exportUVs")) {
                jobArgs.exportMeshUVs = theOption[1].asInt();
                jobArgs.exportNurbsExplicitUV = theOption[1].asInt();
            }
            if (theOption[0] == MString("exportMaterialCollections")) {
                jobArgs.exportMaterialCollections = theOption[1].asInt();
            }
            if (theOption[0] == MString("materialCollectionsPath")) {
                jobArgs.materialCollectionsPath = theOption[1].asChar();
            }
            if (theOption[0] == MString("exportCollectionBasedBindings")) {
                jobArgs.exportCollectionBasedBindings= theOption[1].asInt();
            }
            if (theOption[0] == MString("normalizeUVs")) {
                jobArgs.normalizeMeshUVs = theOption[1].asInt();
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
                const TfToken mode(theOption[1].asChar());
                if (mode != PxUsdExportJobArgsTokens->defaultLayer &&
                        mode != PxUsdExportJobArgsTokens->currentLayer &&
                        mode != PxUsdExportJobArgsTokens->modelingVariant) {
                    TF_WARN("renderLayerMode '%s' not recognized",
                            mode.GetText());
                } else {
                    jobArgs.renderLayerMode = mode;
                }
            }
            if (theOption[0] == MString("mergeXForm")) {
                jobArgs.mergeTransformAndShape = theOption[1].asInt();
            }
            if (theOption[0] == MString("exportInstances")) {
                jobArgs.exportInstances = theOption[1].asInt();
            }
            if (theOption[0] == MString("defaultMeshScheme")) {            
                const TfToken scheme(theOption[1].asChar());
                if (scheme != UsdGeomTokens->none &&
                        scheme != UsdGeomTokens->catmullClark &&
                        scheme != UsdGeomTokens->loop &&
                        scheme != UsdGeomTokens->bilinear) {
                    TF_WARN("defaultMeshScheme '%s' not recognized",
                            scheme.GetText());
                } else {
                    jobArgs.defaultMeshScheme = scheme;
                }
            }
            if (theOption[0] == MString("exportVisibility")) {
                jobArgs.exportVisibility = theOption[1].asInt();
            }
            if (theOption[0] == MString("exportSkin")) {
                const TfToken tok(theOption[1].asChar());
                if (tok == PxUsdExportJobArgsTokens->none) {
                    jobArgs.exportSkin = false;
                }
                else if (tok == PxUsdExportJobArgsTokens->auto_) {
                    jobArgs.exportSkin = true;
                    jobArgs.autoSkelRoots = true;
                }
                else if (tok == PxUsdExportJobArgsTokens->explicit_) {
                    jobArgs.exportSkin = true;
                    jobArgs.autoSkelRoots = false;
                }
                else {
                    TF_WARN("exportSkin '%s' not recognized", tok.GetText());
                }
            }
            if (theOption[0] == MString("animation")) {
                exportAnimation = theOption[1].asInt();
            }
            if (theOption[0] == MString("startTime")) {
                timeInterval.SetMin(theOption[1].asDouble());
            }
            if (theOption[0] == MString("endTime")) {
                timeInterval.SetMax(theOption[1].asDouble());
            }
            if (theOption[0] == MString("frameSample")) {
                frameSamples.insert(theOption[1].asDouble());
            }
            if (theOption[0] == MString("parentScope")) {
                jobArgs.setParentScope(theOption[1].asChar());
            }            
        }
    }


    // Now resync start and end frame based on export time interval.
    if (exportAnimation) {
        if (timeInterval.IsEmpty()) {
            // If the user accidentally set start > end, resync to the closed
            // interval with the single start point.
            jobArgs.timeInterval = GfInterval(timeInterval.GetMin());
        }
        else {
            // Use the user's interval as-is.
            jobArgs.timeInterval = timeInterval;
        }
    }
    else {
        // No animation, so empty interval.
        jobArgs.timeInterval = GfInterval();
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
        usdWriteJob writeJob(jobArgs);
        if (writeJob.beginJob(fileName, append)) {
            if (!jobArgs.timeInterval.IsEmpty()) {
                const MTime oldCurTime = MAnimControl::currentTime();
                for (double i = jobArgs.timeInterval.GetMin();
                        jobArgs.timeInterval.Contains(i);
                        i += 1.0) {
                    for (double sampleTime : frameSamples) {
                        const double actualTime = i + sampleTime;
                        MGlobal::viewFrame(actualTime);
                        writeJob.evalJob(actualTime);
                    }
                }

                // Set the time back.
                MGlobal::viewFrame(oldCurTime);
            }

            writeJob.endJob();
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
    MFileKind retValue = kNotMyFileType;
    const MString fileName = file.fullName();
    const int lastIndex = fileName.length() - 1;

    const int periodIndex = fileName.rindex('.');
    if (periodIndex < 0 || periodIndex >= lastIndex) {
        return retValue;
    }

    const MString fileExtension = fileName.substring(periodIndex + 1, lastIndex);

    if (fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText() || 
        fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText()   || 
        fileExtension == PxrUsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText()) {
        retValue = kIsMyFileType;
    }

    return retValue;
}

PXR_NAMESPACE_CLOSE_SCOPE

