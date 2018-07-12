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
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
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

// Returns the set of root prim paths present in the given path-set.
static
SdfPathSet
_GetRootPaths(const SdfPathSet &paths) {
    SdfPathSet result;
    for (const auto &p : paths) {
        const std::string &pathString = p.GetString();
        // Skip pseudo-root.
        if (! TF_VERIFY(pathString.size() > 1, "Invalid path '%s'", 
                          pathString.c_str())) {
            continue;
        }

        // This should be faster than calling GetPrefixes()[0].
        SdfPath rootPath(pathString.substr(0, pathString.find('/', 1)));
        result.insert(rootPath);
    }
    return result;
}

void
PxrUsdMayaShadingModeExporter::DoExport(
    const UsdStageRefPtr& stage,
    const PxrUsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap,
    const PxrUsdMayaExportParams &exportParams) 
{
    MItDependencyNodes shadingEngineIter(MFn::kShadingEngine);

    UsdPrim materialCollectionsPrim;
    const SdfPath &materialCollectionsPath = 
            exportParams.materialCollectionsPath;
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
        dagPathToUsdMap, 
        exportParams);

    PreExport(&context);

    using MaterialAssignments = std::vector<std::pair<TfToken, 
        SdfPathSet>>;
    MaterialAssignments matAssignments;

    std::vector<UsdShadeMaterial> exportedMaterials;
    for (; !shadingEngineIter.isDone(); shadingEngineIter.next()) {
        MObject shadingEngine(shadingEngineIter.thisNode());
        context.SetShadingEngine(shadingEngine);

        UsdShadeMaterial mat;
        SdfPathSet boundPrimPaths;
        Export(context, &mat, &boundPrimPaths);

        if (mat && !boundPrimPaths.empty()) {
            exportedMaterials.push_back(mat);
            matAssignments.push_back(std::make_pair(
                _GetCollectionName(mat), boundPrimPaths));
        }
    }

    context.SetShadingEngine(MObject());
    PostExport(context);

    if ((materialCollectionsPrim || exportParams.exportCollectionBasedBindings)
        && !matAssignments.empty()) 
    {
        if (!materialCollectionsPrim) {
            // Find a place to export the material collections. The collections
            // can live anywhere in the scene, but the collection-based bindings 
            // must live at or above the prims being bound.
            // 
            // This computes the first root prim below which a material has 
            // been exported.
            SdfPath rootPrimPath = 
                exportedMaterials[0].GetPath().GetPrefixes()[0];
            materialCollectionsPrim = stage->GetPrimAtPath(rootPrimPath);
            TF_VERIFY(materialCollectionsPrim, 
                "Could not get prim at path <%s>. Not exporting material "
                "collections / bindings.", rootPrimPath.GetText());
            return;
        }

        std::vector<UsdCollectionAPI> collections = 
            UsdUtilsCreateCollections(matAssignments, materialCollectionsPrim);

        if (exportParams.exportCollectionBasedBindings) 
        {
            for (size_t i = 0; i < exportedMaterials.size(); ++i) 
            {
                const UsdShadeMaterial &mat = exportedMaterials[i];
                const UsdCollectionAPI &coll = collections[i];

                // If the all the paths are under the prim with the materialBind 
                // collections, export the binding on the prim.
                const SdfPathSet &paths = matAssignments[i].second;
                if (std::all_of(paths.begin(), paths.end(), 
                        [materialCollectionsPrim](const SdfPath &p) { 
                        return p.HasPrefix(materialCollectionsPrim.GetPath());
                        })) {
                            
                    // Materials are named uniquely in maya, so we can 
                    // skip passing in the 'bindingName' param.
                    UsdShadeMaterialBindingAPI(materialCollectionsPrim).Bind(
                        coll, mat);
                    continue;
                }

                // If all the paths are not under materialCollectionsPrim, then
                // figure out the set of root paths at which to export the 
                // collection-based bindings.
                const SdfPathSet rootPaths = 
                    _GetRootPaths(matAssignments[i].second);
                for (auto &rootPath : rootPaths) {
                    auto rootPrim = stage->GetPrimAtPath(rootPath);
                    if (!TF_VERIFY(rootPrim, "Could not get prim at path <%s>", 
                        rootPath.GetText())) {
                        continue;
                    }
                    UsdShadeMaterialBindingAPI(rootPrim).Bind(coll, mat);
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
