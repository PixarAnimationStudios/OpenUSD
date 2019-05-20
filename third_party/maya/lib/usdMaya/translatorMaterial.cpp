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
#include "usdMaya/translatorMaterial.h"

#include "usdMaya/primReaderContext.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeImporter.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include <set>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


/* static */
MObject
UsdMayaTranslatorMaterial::Read(
        const TfToken& shadingMode,
        const UsdShadeMaterial& shadeMaterial,
        const UsdGeomGprim& boundPrim,
        UsdMayaPrimReaderContext* context)
{
    if (shadingMode == UsdMayaShadingModeTokens->none) {
        return MObject();
    }

    UsdMayaShadingModeImportContext c(shadeMaterial, boundPrim, context);

    MObject shadingEngine;

    if (c.GetCreatedObject(shadeMaterial.GetPrim(), &shadingEngine)) {
        return shadingEngine;
    }

    if (UsdMayaShadingModeImporter importer =
            UsdMayaShadingModeRegistry::GetImporter(shadingMode)) {
        shadingEngine = importer(&c);
    }

    if (!shadingEngine.isNull()) {
        c.AddCreatedObject(shadeMaterial.GetPrim(), shadingEngine);
    }

    return shadingEngine;
}

static
bool
_AssignMaterialFaceSet(
        const MObject& shadingEngine,
        const MDagPath& shapeDagPath,
        const VtIntArray& faceIndices)
{
    MStatus status;

    // Create component object using single indexed
    // components, i.e. face indices.
    MFnSingleIndexedComponent compFn;
    MObject faceComp = compFn.create(MFn::kMeshPolygonComponent, &status);
    if (!status) {
        TF_RUNTIME_ERROR("Failed to create face component.");
        return false;
    }

    MIntArray mFaces;
    TF_FOR_ALL(fIdxIt, faceIndices) {
        mFaces.append(*fIdxIt);
    }
    compFn.addElements(mFaces);

    MFnSet seFnSet(shadingEngine, &status);
    if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
        status = seFnSet.addMember(shapeDagPath, faceComp);
        if (!status) {
            TF_RUNTIME_ERROR(
                "Could not add component to shadingEngine %s.",
                seFnSet.name().asChar());
            return false;
        }
    }

    return true;
}

bool
UsdMayaTranslatorMaterial::AssignMaterial(
        const TfToken& shadingMode,
        const UsdGeomGprim& primSchema,
        MObject shapeObj,
        UsdMayaPrimReaderContext* context)
{
    // if we don't have a valid context, we make one temporarily.  This is to
    // make sure we don't duplicate shading nodes within a material.
    UsdMayaPrimReaderContext::ObjectRegistry tmpRegistry;
    UsdMayaPrimReaderContext tmpContext(&tmpRegistry);
    if (!context) {
        context = &tmpContext;
    }

    MDagPath shapeDagPath;
    MFnDagNode(shapeObj).getPath(shapeDagPath);

    MStatus status;
    const UsdShadeMaterialBindingAPI bindingAPI(primSchema.GetPrim());
    MObject shadingEngine =
        UsdMayaTranslatorMaterial::Read(shadingMode,
                                           bindingAPI.ComputeBoundMaterial(),
                                           primSchema,
                                           context);

    if (shadingEngine.isNull()) {
        status = UsdMayaUtil::GetMObjectByName("initialShadingGroup",
                                                  shadingEngine);
        if (status != MS::kSuccess) {
            return false;
        }
    }

    // If the gprim does not have a material faceSet which represents per-face
    // shader assignments, assign the shading engine to the entire gprim.
    const std::vector<UsdGeomSubset> faceSubsets =
        UsdShadeMaterialBindingAPI(
            primSchema.GetPrim()).GetMaterialBindSubsets();

    if (faceSubsets.empty()) {
        MFnSet seFnSet(shadingEngine, &status);
        if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
            status = seFnSet.addMember(shapeObj);
            if (!status) {
                TF_RUNTIME_ERROR(
                    "Could not add shadingEngine for '%s'.",
                    shapeDagPath.fullPathName().asChar());
            }
        }

        return true;
    }

    if (!faceSubsets.empty()) {

        int faceCount = 0;
        const UsdGeomMesh mesh(primSchema);
        if (mesh) {
            VtIntArray faceVertexCounts;
            mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
            faceCount = faceVertexCounts.size();
        }

        if (faceCount == 0) {
            TF_RUNTIME_ERROR(
                "Unable to get face count for gprim at path <%s>.",
                primSchema.GetPath().GetText());
            return false;
        }

        std::string reasonWhyNotPartition;

        const bool validPartition =
            UsdGeomSubset::ValidateSubsets(
                faceSubsets,
                faceCount,
                UsdGeomTokens->partition,
                &reasonWhyNotPartition);
        if (!validPartition) {
            TF_WARN("Face-subsets on <%s> don't form a valid partition: %s",
                    primSchema.GetPath().GetText(),
                    reasonWhyNotPartition.c_str());

            VtIntArray unassignedIndices =
                UsdGeomSubset::GetUnassignedIndices(faceSubsets, faceCount);
            if (!_AssignMaterialFaceSet(shadingEngine,
                                        shapeDagPath,
                                        unassignedIndices)) {
                return false;
            }
        }

        for (const auto& subset : faceSubsets) {
            const UsdShadeMaterialBindingAPI subsetBindingAPI(subset.GetPrim());
            const UsdShadeMaterial boundMaterial =
                subsetBindingAPI.ComputeBoundMaterial();
            if (boundMaterial) {
                MObject faceSubsetShadingEngine =
                    UsdMayaTranslatorMaterial::Read(
                        shadingMode,
                        boundMaterial,
                        UsdGeomGprim(),
                        context);
                if (faceSubsetShadingEngine.isNull()) {
                    status =
                        UsdMayaUtil::GetMObjectByName(
                            "initialShadingGroup",
                            faceSubsetShadingEngine);
                    if (status != MS::kSuccess) {
                        return false;
                    }
                }

                // Only transfer the first timeSample or default indices, if
                // there are no time-samples.
                VtIntArray indices;
                subset.GetIndicesAttr().Get(&indices,
                                            UsdTimeCode::EarliestTime());

                if (!_AssignMaterialFaceSet(faceSubsetShadingEngine,
                                            shapeDagPath,
                                            indices)) {
                    return false;
                }
            }
        }
    }

    return true;
}

/* static */
void
UsdMayaTranslatorMaterial::ExportShadingEngines(
        UsdMayaWriteJobContext& writeJobContext,
        const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap)
{
    const TfToken& shadingMode = writeJobContext.GetArgs().shadingMode;

    if (shadingMode == UsdMayaShadingModeTokens->none) {
        return;
    }

    if (auto exporterCreator =
            UsdMayaShadingModeRegistry::GetExporter(shadingMode)) {
        if (auto exporter = exporterCreator()) {
            exporter->DoExport(writeJobContext, dagPathToUsdMap);
        }
    }
    else {
        TF_RUNTIME_ERROR("No shadingMode '%s' found.", shadingMode.GetText());
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
