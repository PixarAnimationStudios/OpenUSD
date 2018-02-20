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


JobExportArgs::JobExportArgs()
    :
        exportRefsAsInstanceable(false),
        exportDisplayColor(true),
        shadingMode(PxrUsdMayaShadingModeTokens->displayColor),
        mergeTransformAndShape(true),
        exportInstances(true),
        exportAnimation(false),
        excludeInvisible(false),
        exportDefaultCameras(false),
        exportMeshUVs(true),
        normalizeMeshUVs(false),
        exportMaterialCollections(false),
        materialCollectionsPath(""),
        exportCollectionBasedBindings(false),
        normalizeNurbs(false),
        exportNurbsExplicitUV(true),
        nurbsExplicitUVType(PxUsdExportJobArgsTokens->Uniform),
        exportColorSets(true),
        renderLayerMode(PxUsdExportJobArgsTokens->defaultLayer),
        defaultMeshScheme(UsdGeomTokens->catmullClark),
        exportVisibility(true),
        parentScope(SdfPath())
{
}

void JobExportArgs::setParentScope(const std::string& ps) {
    // Otherwise this is a malformed sdfpath.
    if (!ps.empty()) {
        parentScope = ps[0] == '/' ? SdfPath(ps) : SdfPath("/" + ps);
    }
}

JobImportArgs::JobImportArgs()
    :
        shadingMode(PxrUsdMayaShadingModeTokens->displayColor),
        defaultMeshScheme(UsdGeomTokens->catmullClark),
        assemblyRep(PxrUsdMayaTranslatorTokens->Collapsed),
        readAnimData(true),
        useCustomFrameRange(false),
        startTime(1.0),
        endTime(1.0),
        importWithProxyShapes(false)
{
}

PXR_NAMESPACE_CLOSE_SCOPE

