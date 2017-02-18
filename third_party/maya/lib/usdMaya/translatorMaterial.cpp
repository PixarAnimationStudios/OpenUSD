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
#include "usdMaya/translatorMaterial.h"

#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MFnMeshData.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorMaterialTokens,
    PXRUSDMAYA_TRANSLATOR_MATERIAL_TOKENS);


/* static */
MObject
PxrUsdMayaTranslatorMaterial::Read(
        const TfToken& shadingMode,
        const UsdShadeMaterial& shadeMaterial,
        const UsdGeomGprim& boundPrim,
        PxrUsdMayaPrimReaderContext* context)
{
    if (shadingMode == PxrUsdMayaShadingModeTokens->none) {
        return MObject();
    }

    PxrUsdMayaShadingModeImportContext c(shadeMaterial, boundPrim, context);

    MObject shadingEngine;

    if (c.GetCreatedObject(shadeMaterial.GetPrim(), &shadingEngine)) {
        return shadingEngine;
    }

    MStatus status;

    MPlug outColorPlug;

    if (PxrUsdMayaShadingModeImporter importer = 
            PxrUsdMayaShadingModeRegistry::GetImporter(shadingMode)) {
        outColorPlug = importer(&c);
    }
    else {
        // this could spew a lot so we don't warn here.  Ideally, we did some
        // validation up front.
    }

    if (!outColorPlug.isNull()) {
        MFnSet fnSet;
        MSelectionList tmpSelList;
        shadingEngine = fnSet.create(tmpSelList, MFnSet::kRenderableOnly, &status);

        // To make sure that the shadingEngine object names do not collide with
        // the Maya transform or shape node names, we put the shadingEngine
        // objects into their own namespace.
        std::string shadingEngineName =
            PxrUsdMayaTranslatorMaterialTokens->MaterialNamespace.GetString() + std::string(":") +
            (shadeMaterial ? shadeMaterial.GetPrim() : boundPrim.GetPrim()
                ).GetName().GetString();

        if (!status) {
            MGlobal::displayError(TfStringPrintf(
                        "Failed to make shadingEngine for %s\n", 
                        shadingEngineName.c_str()).c_str());
            return shadingEngine;
        }
        fnSet.setName(MString(shadingEngineName.c_str()),
            true /* createNamespace */);

        MPlug seSurfaceShaderPlg = fnSet.findPlug("surfaceShader", &status);
        PxrUsdMayaUtil::Connect(outColorPlug, seSurfaceShaderPlg, 
                // Make sure that "surfaceShader" connection is open
                true);
    }

    return c.AddCreatedObject(shadeMaterial.GetPrim(), shadingEngine);
}

static 
bool 
_AssignMaterialFaceSet(const MObject &shadingEngine,
                   const MDagPath &shapeDagPath,
                   const VtIntArray &faceIndices)
{
    MStatus status;

    // Create component object using single indexed  
    // components, i.e. face indices.
    MFnSingleIndexedComponent compFn;
    MObject faceComp = compFn.create(
        MFn::kMeshPolygonComponent, &status);
    if (!status) {
        MGlobal::displayError("Failed to create face component.");
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
            MGlobal::displayError(TfStringPrintf("Could not"
                " add component to shadingEngine %s.", 
                seFnSet.name().asChar()).c_str());
            return false;
        }
    }

    return true;
}

bool
PxrUsdMayaTranslatorMaterial::AssignMaterial(
        const TfToken& shadingMode,
        const UsdGeomGprim& primSchema,
        MObject shapeObj,
        PxrUsdMayaPrimReaderContext* context)
{
    // if we don't have a valid context, we make one temporarily.  This is to
    // make sure we don't duplicate shading nodes within a material.
    PxrUsdMayaPrimReaderContext::ObjectRegistry tmpRegistry;
    PxrUsdMayaPrimReaderContext tmpContext(&tmpRegistry);
    if (!context) {
        context = &tmpContext;
    }

    MDagPath shapeDagPath;
    MFnDagNode(shapeObj).getPath(shapeDagPath);

    MStatus status;
    MObject shadingEngine = PxrUsdMayaTranslatorMaterial::Read(
            shadingMode,
            UsdShadeMaterial::GetBoundMaterial(primSchema.GetPrim()), 
            primSchema, 
            context);

    if (shadingEngine.isNull()) {
        status = PxrUsdMayaUtil::GetMObjectByName("initialShadingGroup",
                                                  shadingEngine);
        if (status != MS::kSuccess) {
            return false;
        }
    }

    // If the gprim does not have a material faceSet which represents per-face 
    // shader assignments, assign the shading engine to the entire gprim.
    if (!UsdShadeMaterial::HasMaterialFaceSet(primSchema.GetPrim())) {
        MFnSet seFnSet(shadingEngine, &status);
        if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
            status = seFnSet.addMember(shapeObj);
            if (!status) {
                MGlobal::displayError("Could not add shapeObj to shadingEngine.\n");
            }
        }

        return true;
    } 

    // Import per-face-set shader bindings.    
    UsdGeomFaceSetAPI materialFaceSet = UsdShadeMaterial::GetMaterialFaceSet(
        primSchema.GetPrim());

    SdfPathVector bindingTargets;
    if (!materialFaceSet.GetBindingTargets(&bindingTargets) or
        bindingTargets.empty()) {
            
        MGlobal::displayWarning(TfStringPrintf("No bindings found on material "
            "faceSet at path <%s>.", primSchema.GetPath().GetText()).c_str());
        // No bindings to export in the material faceSet.
        return false;
    }

    std::string reason;
    if (!materialFaceSet.Validate(&reason)) {
        MGlobal::displayWarning(TfStringPrintf("Invalid faceSet data "
            "found on <%s>: %s", primSchema.GetPath().GetText(), 
            reason.c_str()).c_str());
        return false;
    }

    VtIntArray faceCounts, faceIndices;
    bool isPartition = materialFaceSet.GetIsPartition();
    materialFaceSet.GetFaceCounts(&faceCounts);
    materialFaceSet.GetFaceIndices(&faceIndices);

    if (!isPartition) {
        MGlobal::displayWarning(TfStringPrintf("Invalid faceSet data "
            "found on <%s>: Not a partition.", 
            primSchema.GetPath().GetText()).c_str());
        return false;
    }

    // Check if there are faceIndices that aren't included in the material
    // face-set. 
    // Note: This won't occur if the shading was originally 
    // authored in maya and exported to the USD that we are importing, 
    // but this is supported by the USD shading model.
    UsdGeomMesh mesh(primSchema);
    if (mesh) {                
        VtIntArray faceVertexCounts;
        if (mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts))
        {
            std::set<int> assignedIndices(faceIndices.begin(),
                                            faceIndices.end());
            VtIntArray unassignedIndices;
            unassignedIndices.reserve(faceVertexCounts.size() -
                                        faceIndices.size());
            for(size_t fIdx = 0; fIdx < faceVertexCounts.size(); fIdx++) 
            {
                if (assignedIndices.count(fIdx) == 0) {
                    unassignedIndices.push_back(fIdx);
                }
            }

            // Assign the face face indices that aren't in the material
            // faceSet to the material that the mesh is bound to or 
            // to the initialShadingGroup if it doesn't have a material 
            // binding.
            if (!unassignedIndices.empty()) {
                if (!_AssignMaterialFaceSet(shadingEngine, shapeDagPath, 
                                           unassignedIndices)) {
                    return false;
                }
            }
        }
    }

    int setIndex = 0;
    int currentFaceIndex = 0;
    TF_FOR_ALL(bindingTargetsIt, bindingTargets) {
        UsdShadeMaterial material(
            primSchema.GetPrim().GetStage()->GetPrimAtPath(
                *bindingTargetsIt));

        MObject faceGroupShadingEngine = PxrUsdMayaTranslatorMaterial::Read(
            shadingMode, material, UsdGeomGprim(), context);
 
        if (faceGroupShadingEngine.isNull()) {
            status = PxrUsdMayaUtil::GetMObjectByName("initialShadingGroup",
                                                      faceGroupShadingEngine);
            if (status != MS::kSuccess) {
                return false;
            }
        }

        int numFaces = faceCounts[setIndex];
        VtIntArray faceGroupIndices;
        faceGroupIndices.reserve(numFaces);
        for (int faceIndex = currentFaceIndex; 
            faceIndex < currentFaceIndex + numFaces;
            ++faceIndex) {
            faceGroupIndices.push_back(faceIndices[faceIndex]);
        }

        ++setIndex;
        currentFaceIndex += numFaces;

        if (!_AssignMaterialFaceSet(faceGroupShadingEngine, shapeDagPath, 
                                   faceGroupIndices)) {
            return false;
        }
    }

    return true;
}

/* static */
void
PxrUsdMayaTranslatorMaterial::ExportShadingEngines(
        const UsdStageRefPtr& stage,
        const PxrUsdMayaUtil::ShapeSet& bindableRoots,
        const TfToken& shadingMode,
        bool mergeTransformAndShape,
        SdfPath overrideRootPath)
{
    if (shadingMode == PxrUsdMayaShadingModeTokens->none) {
        return;
    }

    if (PxrUsdMayaShadingModeExporter exporter =
            PxrUsdMayaShadingModeRegistry::GetExporter(shadingMode)) {
        MItDependencyNodes shadingEngineIter(MFn::kShadingEngine);
        for (; !shadingEngineIter.isDone(); shadingEngineIter.next()) {
            MObject shadingEngine(shadingEngineIter.thisNode());
            MFnDependencyNode seDepNode(shadingEngine);

            PxrUsdMayaShadingModeExportContext c(
                    shadingEngine,
                    stage,
                    mergeTransformAndShape,
                    bindableRoots,
                    overrideRootPath);

            exporter(&c);
        }
    }
    else {
        MGlobal::displayError(TfStringPrintf("No shadingMode '%s' found.",
                    shadingMode.GetText()).c_str());
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

