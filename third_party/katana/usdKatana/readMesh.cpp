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
#include "usdKatana/attrMap.h"
#include "usdKatana/debugCodes.h"
#include "usdKatana/readGprim.h"
#include "usdKatana/readMesh.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdRi/rmanUtilities.h"

#include <FnLogging/FnLogging.h>

#include "vtKatana/array.h"

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadMesh");

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDKATANA_MESH_IMPORT,
            "Diagnostics about mesh import");
}

static FnKat::Attribute
_GetPolyAttr(const UsdGeomMesh &mesh, double time)
{
    // Eval verts and numVerts.
    VtIntArray vertsArray, numVertsArray;
    mesh.GetFaceVertexIndicesAttr().Get(&vertsArray, time);
    mesh.GetFaceVertexCountsAttr().Get(&numVertsArray, time);
    
    // Convert to vectors.
    std::vector<int> vertsVec(vertsArray.begin(), vertsArray.end());
    std::vector<int> numVertsVec(numVertsArray.begin(), numVertsArray.end());
    std::vector<int> startVertsVec;
    PxrUsdKatanaUtils::ConvertNumVertsToStartVerts(numVertsVec, &startVertsVec);

    // Build Katana attribute.
    FnKat::GroupBuilder polyBuilder;
    {
        FnKat::IntBuilder vertsBuilder(/* tupleSize = */ 1);
        FnKat::IntBuilder startVertsBuilder(/* tupleSize = */ 1);
        vertsBuilder.set(vertsVec);
        startVertsBuilder.set(startVertsVec);
        polyBuilder.set("vertexList", vertsBuilder.build());
        polyBuilder.set("startIndex", startVertsBuilder.build());
    }
    return polyBuilder.build();
}

static void
_SetSubdivTagsGroup(PxrUsdKatanaAttrMap& attrs,
                    const UsdGeomMesh &mesh, bool hierarchical, double time)
{

    TfToken interpolateBoundary; 
    if (mesh.GetInterpolateBoundaryAttr().Get(&interpolateBoundary, time)){
        if (interpolateBoundary != UsdGeomTokens->none) {  // See bug/90360
            
            TF_DEBUG(USDKATANA_MESH_IMPORT).
                Msg("\tinterpolateBoundary = %s (%d)\n",
                    interpolateBoundary.GetText(),
                    UsdRiConvertToRManInterpolateBoundary(interpolateBoundary));
            attrs.set("geometry.interpolateBoundary",
                  FnKat::IntAttribute(
                  UsdRiConvertToRManInterpolateBoundary(interpolateBoundary)));
        }
        else {
            TF_DEBUG(USDKATANA_MESH_IMPORT).
                Msg("\tinterpolateBoundary SKIPPED because it is fallback\n");
        }
    }
    else{
        TF_DEBUG(USDKATANA_MESH_IMPORT).
            Msg("\tinterpolateBoundary SKIPPED because we failed to read it!\n");
    }

    TfToken fvLinearInterpolationTk;
    // Fun prman facts: the "default behavior when no 
    // facevaryinginterpolateboundary tag is emitted" can be customized in
    // your site's rendermn.ini, so the usd fallback is unreliable.  Therefore
    // we will emit whenever the attribute has been authored.
    // Performance Note: once we have resolveInfo available, that will be more
    // efficient than doing IsAuthored() followed by Get()
    auto fvLinearInterpolationAttr = mesh.GetFaceVaryingLinearInterpolationAttr(); 
    fvLinearInterpolationAttr.Get(&fvLinearInterpolationTk);

    if (fvLinearInterpolationAttr.IsAuthored()){
        TF_DEBUG(USDKATANA_MESH_IMPORT).
            Msg("\tfacevaryinginterpolateboundary = %s (%d)\n",
                fvLinearInterpolationTk.GetText(),
                UsdRiConvertToRManFaceVaryingLinearInterpolation(fvLinearInterpolationTk));
        
        attrs.set("geometry.facevaryinginterpolateboundary",
            FnKat::IntAttribute(
                UsdRiConvertToRManFaceVaryingLinearInterpolation(fvLinearInterpolationTk)));
    }
    else{
        TF_DEBUG(USDKATANA_MESH_IMPORT).
            Msg("\tfacevaryinginterpolateboundary SKIPPED because it was not authored\n");
    }

    TfToken triangleSubdivisionRule;
    if (mesh.GetTriangleSubdivisionRuleAttr().Get(&triangleSubdivisionRule, time)) {
        if (triangleSubdivisionRule != UsdGeomTokens->catmullClark) {
            TF_DEBUG(USDKATANA_MESH_IMPORT).
                Msg("\ttriangleSubdivisionRule = %s (%d)\n",
                    triangleSubdivisionRule.GetText(),
                    UsdRiConvertToRManTriangleSubdivisionRule(triangleSubdivisionRule));
            attrs.set("geometry.triangleSubdivisionRule",
                  FnKat::IntAttribute(
                    UsdRiConvertToRManTriangleSubdivisionRule(triangleSubdivisionRule)));
        }
        else {
            TF_DEBUG(USDKATANA_MESH_IMPORT).
                Msg("\ttriangleSubdivisionRule SKIPPED because it is default\n");
        }
    }
    else {
        TF_DEBUG(USDKATANA_MESH_IMPORT).
            Msg("\ttriangleSubdivisionRule SKIPPED because we failed to read it!\n");
    }

    // Holes
    VtIntArray holeIndices;
    if (mesh.GetHoleIndicesAttr().Get(&holeIndices, time)
        && holeIndices.size() > 0) {
        auto holeAttr = VtKatanaMapOrCopy(holeIndices);
        attrs.set("geometry.holePolyIndices", holeAttr);
    }

    // Creases
    VtIntArray creaseIndices;
    if (mesh.GetCreaseIndicesAttr().Get(&creaseIndices, time)
        && creaseIndices.size() > 0) {
        auto creaseAttr = VtKatanaMapOrCopy(creaseIndices);
        attrs.set("geometry.creaseIndices", creaseAttr);

        VtIntArray creaseLengths;
        if (mesh.GetCreaseLengthsAttr().Get(&creaseLengths, time)
            && creaseLengths.size() > 0) {
            auto creaseLengthsAttr = VtKatanaMapOrCopy(creaseLengths);
            attrs.set("geometry.creaseLengths", creaseLengthsAttr);
        }
        VtFloatArray creaseSharpness;
        if (mesh.GetCreaseSharpnessesAttr().Get(&creaseSharpness, time)
            && creaseSharpness.size() > 0) {
            auto creaseSharpnessAttr = VtKatanaMapOrCopy(creaseSharpness);
            FnKat::IntBuilder creaseSharpnessLengthsBuilder(1);
            std::vector<int> numSharpnesses;
            if (creaseLengths.size() == creaseSharpness.size()) {
                // We have exactly 1 sharpness per crease.
                numSharpnesses.resize(creaseLengths.size(), 1);
            } else {
                std::transform(creaseLengths.begin(), creaseLengths.end(),
                    std::back_inserter(numSharpnesses), [](int creaseLength){
                        // We have N-1 sharpnesses for each crease that has N edges.
                        return creaseLength - 1;
                    });
            }
            creaseSharpnessLengthsBuilder.set(numSharpnesses);

            attrs.set("geometry.creaseSharpness", creaseSharpnessAttr);
            attrs.set("geometry.creaseSharpnessLengths",
                        creaseSharpnessLengthsBuilder.build());
        }
    }

    // Corners
    VtIntArray cornerIndices;
    if (mesh.GetCornerIndicesAttr().Get(&cornerIndices, time)
        && cornerIndices.size() > 0) {
        auto cornerIndicesAttr = VtKatanaMapOrCopy(cornerIndices);
        attrs.set("geometry.cornerIndices", cornerIndicesAttr);
    }
    VtFloatArray cornerSharpness;
    if (mesh.GetCornerSharpnessesAttr().Get(&cornerSharpness, time)
        && cornerSharpness.size() > 0) {
        auto cornerSharpnessAttr = VtKatanaMapOrCopy(cornerSharpness);
        attrs.set("geometry.cornerSharpness", cornerSharpnessAttr);
    }
}

void
PxrUsdKatanaReadMesh(
        const UsdGeomMesh& mesh,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetCurrentTime();

    //
    // Set all general attributes for a gprim type.
    //

    PxrUsdKatanaReadGprim(mesh, data, attrs);

    //
    // Set more specific Katana type.
    //

    TfToken scheme;
    mesh.GetSubdivisionSchemeAttr().Get(&scheme);
    bool isSubd = (scheme != UsdGeomTokens->none);

    attrs.set("type", FnKat::StringAttribute(isSubd ? "subdmesh" : "polymesh"));

    //
    // Construct the 'geometry' attribute.
    //

    // position
    attrs.set("geometry.point.P", PxrUsdKatanaGeomGetPAttr(mesh, data));

    /// Only use custom normals if the object is a polymesh.
    if (!isSubd){
        // normals
        FnKat::Attribute normalsAttr = PxrUsdKatanaGeomGetNormalAttr(mesh, data);
        if (normalsAttr.isValid())
        {
            // XXX RfK currently doesn't support uniform normals for polymeshes.
            TfToken interp = mesh.GetNormalsInterpolation();
            if (interp == UsdGeomTokens->varying
             || interp == UsdGeomTokens->vertex) {
                attrs.set("geometry.point.N", normalsAttr);
            }
            else if (interp == UsdGeomTokens->faceVarying) {
                attrs.set("geometry.vertex.N", normalsAttr);
            }
        }
    }

    // velocity
    FnKat::Attribute velocityAttr = PxrUsdKatanaGeomGetVelocityAttr(mesh, data);
    if (velocityAttr.isValid())
    {
        attrs.set("geometry.point.v", velocityAttr);
    }

    FnKat::GroupAttribute polyAttr = _GetPolyAttr(mesh, currentTime);
    
    attrs.set("geometry.poly", polyAttr);

    if (isSubd)
    {
        _SetSubdivTagsGroup(attrs, mesh, /* hierarchical=*/ false, currentTime);
    }

    // SPT_HwColor primvar
    attrs.set("geometry.arbitrary.SPT_HwColor", 
              PxrUsdKatanaGeomGetDisplayColorAttr(mesh, data));

    attrs.set(
        "viewer.default.drawOptions.windingOrder",
            PxrUsdKatanaGeomGetWindingOrderAttr(mesh, data));

    attrs.set("tabs.scenegraph.stopExpand", FnKat::IntAttribute(1));
}

PXR_NAMESPACE_CLOSE_SCOPE

