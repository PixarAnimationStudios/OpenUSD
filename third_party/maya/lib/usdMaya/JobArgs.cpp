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

#include <maya/MDagPath.h>

#include <ostream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE



TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(PxUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBARGS_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _defaultIncludeMetadataKeys, 
    (hidden)
    (instanceable)
    (kind)
);

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
        exportSkin(false),
        autoSkelRoots(false),
        exportMeshUVs(true),
        normalizeMeshUVs(false),
        exportMaterialCollections(false),
        materialCollectionsPath(""),
        exportCollectionBasedBindings(false),
        normalizeNurbs(false),
        exportNurbsExplicitUV(true),
        exportColorSets(true),
        renderLayerMode(PxUsdExportJobArgsTokens->defaultLayer),
        defaultMeshScheme(UsdGeomTokens->catmullClark),
        exportVisibility(true),
        parentScope(SdfPath())
{
}

static
std::string
_StringifyBool(const bool value)
{
    if (value) {
        return "true";
    }

    return "false";
}

std::ostream&
operator <<(std::ostream& out, const JobExportArgs& exportArgs)
{
    out << "exportRefsAsInstanceable: " << _StringifyBool(exportArgs.exportRefsAsInstanceable) << std::endl
        << "exportDisplayColor: " << _StringifyBool(exportArgs.exportDisplayColor) << std::endl
        << "shadingMode: " << exportArgs.shadingMode << std::endl
        << "mergeTransformAndShape: " << _StringifyBool(exportArgs.mergeTransformAndShape) << std::endl
        << "exportInstances: " << _StringifyBool(exportArgs.exportInstances) << std::endl
        << "exportAnimation: " << _StringifyBool(exportArgs.exportAnimation) << std::endl
        << "excludeInvisible: " << _StringifyBool(exportArgs.excludeInvisible) << std::endl
        << "exportDefaultCameras: " << _StringifyBool(exportArgs.exportDefaultCameras) << std::endl
        << "exportSkin: " << _StringifyBool(exportArgs.exportSkin) << std::endl
        << "autoSkelRoots: " << _StringifyBool(exportArgs.autoSkelRoots) << std::endl
        << "exportMeshUVs: " << _StringifyBool(exportArgs.exportMeshUVs) << std::endl
        << "normalizeMeshUVs: " << _StringifyBool(exportArgs.normalizeMeshUVs) << std::endl
        << "exportMaterialCollections: " << _StringifyBool(exportArgs.exportMaterialCollections) << std::endl
        << "materialCollectionsPath: " << exportArgs.materialCollectionsPath << std::endl
        << "exportCollectionBasedBindings: " << _StringifyBool(exportArgs.exportCollectionBasedBindings) << std::endl
        << "normalizeNurbs: " << _StringifyBool(exportArgs.normalizeNurbs) << std::endl
        << "exportNurbsExplicitUV: " << _StringifyBool(exportArgs.exportNurbsExplicitUV) << std::endl
        << "exportColorSets: " << _StringifyBool(exportArgs.exportColorSets) << std::endl
        << "renderLayerMode: " << exportArgs.renderLayerMode << std::endl
        << "defaultMeshScheme: " << exportArgs.defaultMeshScheme << std::endl
        << "exportVisibility: " << _StringifyBool(exportArgs.exportVisibility) << std::endl
        << "parentScope: " << exportArgs.getParentScope() << std::endl;

    out << "melPerFrameCallback: " << exportArgs.melPerFrameCallback << std::endl
        << "melPostCallback: " << exportArgs.melPostCallback << std::endl
        << "pythonPerFrameCallback: " << exportArgs.pythonPerFrameCallback << std::endl
        << "pythonPostCallback: " << exportArgs.pythonPostCallback << std::endl;

    out << "dagPaths (" << exportArgs.dagPaths.size() << ")" << std::endl;
    for (const MDagPath& dagPath : exportArgs.dagPaths) {
        out << "    " << dagPath.fullPathName().asChar() << std::endl;
    }

    out << "chaserNames (" << exportArgs.chaserNames.size() << ")" << std::endl;
    for (const std::string& chaserName : exportArgs.chaserNames) {
        out << "    " << chaserName << std::endl;        
    }

    out << "allChaserArgs (" << exportArgs.allChaserArgs.size() << ")" << std::endl;
    for (const auto& chaserIter : exportArgs.allChaserArgs) {
        // Chaser name.
        out << "    " << chaserIter.first << std::endl;

        for (const auto& argIter : chaserIter.second) {
            out << "        Arg Name: " << argIter.first
                << ", Value: " << argIter.second << std::endl;
        }
    }

    out << "usdModelRootOverridePath: " << exportArgs.usdModelRootOverridePath << std::endl
        << "rootKind: " << exportArgs.rootKind << std::endl;

    return out;
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
        assemblyRep(PxrUsdMayaTranslatorTokens->Collapsed),
        readAnimData(true),
        useCustomFrameRange(false),
        startTime(1.0),
        endTime(1.0),
        importWithProxyShapes(false),
        includeMetadataKeys(
                _defaultIncludeMetadataKeys->allTokens.begin(),
                _defaultIncludeMetadataKeys->allTokens.end()),
        includeAPINames(/*empty*/)
{
}

std::ostream&
operator <<(std::ostream& out, const JobImportArgs& importArgs)
{
    out << "shadingMode: " << importArgs.shadingMode << std::endl
        << "assemblyRep: " << importArgs.assemblyRep << std::endl
        << "readAnimData: " << _StringifyBool(importArgs.readAnimData) << std::endl
        << "useCustomFrameRange: " << _StringifyBool(importArgs.useCustomFrameRange) << std::endl
        << "startTime: " << importArgs.startTime << std::endl
        << "endTime: " << importArgs.endTime << std::endl
        << "importWithProxyShapes: " << _StringifyBool(importArgs.importWithProxyShapes) << std::endl;

    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
