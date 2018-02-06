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
#include "usdMaya/shadingModeExporter.h"

#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usdUtils/authoring.h"

#include <maya/MItDependencyNodes.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((materialNamespace, "material:"))
);

PxrUsdMayaShadingModeExporter::PxrUsdMayaShadingModeExporter() {

}

PxrUsdMayaShadingModeExporter::~PxrUsdMayaShadingModeExporter() {

}

static 
TfToken 
_GetCollectionName(const UsdShadeMaterial &mat) 
{
    return TfToken(_tokens->materialNamespace.GetString() +
                   mat.GetPrim().GetName().GetString());
}

void
PxrUsdMayaShadingModeExporter::DoExport(
    const UsdStageRefPtr& stage,
    const PxrUsdMayaUtil::ShapeSet& bindableRoots,
    bool mergeTransformAndShape,
    const SdfPath& overrideRootPath,
    const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type& dagPathToUsdMap,
    const SdfPath &materialCollectionsPath) 
{
    MItDependencyNodes shadingEngineIter(MFn::kShadingEngine);

    using MaterialAssignments = std::vector<std::pair<TfToken, 
        SdfPathSet>>;

    UsdPrim materialCollectionsPrim;
    if (!materialCollectionsPath.IsEmpty()) {
        materialCollectionsPrim = 
            stage->OverridePrim(materialCollectionsPath);
        if (!materialCollectionsPrim) {
            TF_WARN("Error: could not override prim at path <%s>. One of the "
                    "ancestors of the path must be inactive or an instance "
                    "root. Not exporting material collections!",
                    materialCollectionsPath.GetText());
        }
    }

    PxrUsdMayaShadingModeExportContext context(
        MObject(),
        stage,
        mergeTransformAndShape,
        bindableRoots,
        overrideRootPath,
        dagPathToUsdMap);

    PreExport(context);

    MaterialAssignments matAssignments;
    for (; !shadingEngineIter.isDone(); shadingEngineIter.next()) {
        MObject shadingEngine(shadingEngineIter.thisNode());
        context.SetShadingEngine(shadingEngine);

        UsdShadeMaterial mat;
        SdfPathSet boundPrimPaths;
        Export(context, &mat, &boundPrimPaths);

        if (!boundPrimPaths.empty()) {
            matAssignments.push_back(std::make_pair(
                _GetCollectionName(mat), boundPrimPaths));
        }
    }

    context.SetShadingEngine(MObject());
    PostExport(context);

    if (materialCollectionsPrim && !matAssignments.empty()) {
        std::vector<UsdCollectionAPI> collections = 
                UsdUtilsCreateCollections(matAssignments, 
                        materialCollectionsPrim);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
