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
#include "usdMaya/JobArgs.h"

#include "usdMaya/shadingModeRegistry.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE



TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(PxUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBARGS_TOKENS);


JobSharedArgs::JobSharedArgs()
    :
        fileName(""),
        shadingMode(PxrUsdMayaShadingModeTokens->displayColor),
        defaultMeshScheme(UsdGeomTokens->catmullClark),
        startTime(1.0),
        endTime(1.0)
{
}

bool JobSharedArgs::parseSharedOption(const MStringArray& theOption)
{
    if (theOption[0] == MString("shadingMode")) {
        if (theOption[1]=="None") {
            shadingMode = PxrUsdMayaShadingModeTokens->none;
        } else if (theOption[1]=="GPrim Colors") {
            shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
        } else if (theOption[1]=="Material Colors") {
            shadingMode = PxrUsdMayaShadingModeTokens->displayColor;
        } else if (theOption[1]=="RfM Shaders") {
            TfToken rfmShadingMode("pxrRis");
            if (PxrUsdMayaShadingModeRegistry::GetInstance().GetExporter(rfmShadingMode)) {
                shadingMode = rfmShadingMode;
            } else {
                MGlobal::displayError(
                        TfStringPrintf("No shadingMode '%s' found.  Setting shadingMode='none'",
                                       rfmShadingMode.GetText()).c_str());
                shadingMode = PxrUsdMayaShadingModeTokens->none;
            }
        }
        return true;
    } else if (theOption[0] == MString("defaultMeshScheme")) {
        setDefaultMeshScheme(theOption[1]);
        return true;
    }
    else if (theOption[0] == MString("startTime")) {
        startTime = theOption[1].asDouble();
        return true;
    }
    else if (theOption[0] == MString("endTime")) {
        endTime = theOption[1].asDouble();
        return true;
    }
    return false;
}

void JobSharedArgs::setDefaultMeshScheme(const MString& stringVal)
{
    if (stringVal=="none" || stringVal=="Polygonal Mesh") {
        defaultMeshScheme = UsdGeomTokens->none;
    } else if (stringVal=="catmullClark" || stringVal=="CatmullClark SDiv") {
        defaultMeshScheme = UsdGeomTokens->catmullClark;
    } else if (stringVal=="loop" || stringVal=="Loop SDiv") {
        defaultMeshScheme = UsdGeomTokens->loop;
    } else if (stringVal=="bilinear" || stringVal=="Bilinear SubDiv") {
        defaultMeshScheme = UsdGeomTokens->bilinear;
    } else {
        MGlobal::displayWarning("Incorrect Default Mesh Schema: " + stringVal +
                                " defaulting to: " +
                                MString(defaultMeshScheme.GetText()));
    }
}

JobExportArgs::JobExportArgs()
    :
        exportRefsAsInstanceable(false),
        exportDisplayColor(true),
        mergeTransformAndShape(true),
        exportAnimation(false),
        excludeInvisible(false),
        exportDefaultCameras(false),
        exportMeshUVs(true),
        normalizeMeshUVs(false),
        normalizeNurbs(false),
        exportNurbsExplicitUV(true),
        nurbsExplicitUVType(PxUsdExportJobArgsTokens->Uniform),
        exportColorSets(true),
        renderLayerMode(PxUsdExportJobArgsTokens->defaultLayer),
        exportVisibility(true)
{
}

void JobExportArgs::parseSingleOption(const MStringArray& theOption)
{
    if (parseSharedOption(theOption)) return;

    if (theOption[0] == MString("exportReferencesAsInstanceable")) {
        exportRefsAsInstanceable = theOption[1].asInt();
    }
    else if (theOption[0] == MString("exportUVs")) {
        exportMeshUVs = theOption[1].asInt();
        exportNurbsExplicitUV = theOption[1].asInt();
    }
    else if (theOption[0] == MString("normalizeUVs")) {
        normalizeMeshUVs = theOption[1].asInt();
        nurbsExplicitUVType = PxUsdExportJobArgsTokens->Uniform;
    }
    else if (theOption[0] == MString("exportColorSets")) {
        exportColorSets = theOption[1].asInt();
    }
    else if (theOption[0] == MString("renderableOnly")) {
        excludeInvisible = theOption[1].asInt();
    }
    else if (theOption[0] == MString("allCameras")) {
        exportDefaultCameras = theOption[1].asInt();
    }
    else if (theOption[0] == MString("renderLayerMode")) {
        renderLayerMode = PxUsdExportJobArgsTokens->defaultLayer;

        if (theOption[1]=="Use Current Layer") {
            renderLayerMode = PxUsdExportJobArgsTokens->currentLayer;
        } else if (theOption[1]=="Modeling Variant Per Layer") {
            renderLayerMode = PxUsdExportJobArgsTokens->modelingVariant;
        }
    }
    else if (theOption[0] == MString("mergeXForm")) {
        mergeTransformAndShape = theOption[1].asInt();
    }
    else if (theOption[0] == MString("exportVisibility")) {
        exportVisibility = theOption[1].asInt();
    }
    else if (theOption[0] == MString("animation")) {
        exportAnimation = theOption[1].asInt();
    }
    else if (theOption[0] == MString("frameSample")) {
        frameSamples.insert(theOption[1].asDouble());
    }    
}


JobImportArgs::JobImportArgs()
    :
        primPath("/"),
        assemblyRep(PxUsdExportJobArgsTokens->Collapsed),
        readAnimData(false),
        useCustomFrameRange(false),
        importWithProxyShapes(false)
{
}


void JobImportArgs::parseSingleOption(const MStringArray& theOption)
{
    if (parseSharedOption(theOption)) return;

    if (theOption[0] == MString("readAnimData")) {
        readAnimData = theOption[1].asInt();
    } else if (theOption[0] == MString("assemblyRep")) {
        assemblyRep = TfToken(theOption[1].asChar());
    } else if (theOption[0] == MString("useCustomFrameRange")) {
        useCustomFrameRange = theOption[1].asInt();
    } else if (theOption[0] == MString("importWithProxyShapes")) {
        importWithProxyShapes = theOption[1].asInt();
    } else if (theOption[0] == MString("primPath")) {
        primPath = theOption[1].asChar();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

