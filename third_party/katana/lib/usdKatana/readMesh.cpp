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

static FnKat::Attribute
_GetSubdivTagsGroup(const UsdGeomMesh &mesh, bool hierarchical, double time)
{
    std::string err;

    FnKat::GroupBuilder builder;

    TfToken interpolateBoundary; 
    if (mesh.GetInterpolateBoundaryAttr().Get(&interpolateBoundary, time)){
        if (interpolateBoundary != UsdGeomTokens->none) {  // See bug/90360
            
            TF_DEBUG(USDKATANA_MESH_IMPORT).
                Msg("\tinterpolateBoundary = %s (%d)\n",
                    interpolateBoundary.GetText(),
                    UsdRiConvertToRManInterpolateBoundary(interpolateBoundary));
            builder.set("interpolateBoundary",
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

    TfToken fvInterpolateBoundary;
    // Fun prman facts: the "default behavior when no 
    // facevaryinginterpolateboundary tag is emitted" can be customized in
    // your site's rendermn.ini, so the usd fallback is unreliable.  Therefore
    // we will emit whenever the attribute has been authored.
    // Performance Note: once we have resolveInfo available, that will be more
    // efficient than doing IsAuthored() followed by Get()
    fvInterpolateBoundary = mesh.GetFaceVaryingLinearInterpolation(time);
    if (mesh.GetPrim().GetAttribute(UsdGeomTokens->faceVaryingLinearInterpolation).IsAuthored()
        or mesh.GetPrim().GetAttribute(UsdGeomTokens->faceVaryingInterpolateBoundary).IsAuthored()){
        TF_DEBUG(USDKATANA_MESH_IMPORT).
            Msg("\tfacevaryinginterpolateboundary = %s (%d)\n",
                fvInterpolateBoundary.GetText(),
                UsdRiConvertToRManFaceVaryingLinearInterpolation(fvInterpolateBoundary));
        
        builder.set("facevaryinginterpolateboundary",
            FnKat::IntAttribute(
                UsdRiConvertToRManFaceVaryingLinearInterpolation(fvInterpolateBoundary)));
    }
    else{
        TF_DEBUG(USDKATANA_MESH_IMPORT).
            Msg("\tfacevaryinginterpolateboundary SKIPPED because it was not authored\n");
    }


    // Holes
    VtIntArray holeIndices;
    if (mesh.GetHoleIndicesAttr().Get(&holeIndices, time)
        and holeIndices.size() > 0) {
        FnKat::IntBuilder holeIndicesBuilder(1);
        holeIndicesBuilder.set(std::vector<int>(holeIndices.begin(),
                                                holeIndices.end()));
        builder.set("holePolyIndices", holeIndicesBuilder.build());
    }

    // Creases
    VtIntArray creaseIndices;
    if (mesh.GetCreaseIndicesAttr().Get(&creaseIndices, time)
        and creaseIndices.size() > 0) {
        FnKat::IntBuilder creasesBuilder(1);
        creasesBuilder.set(std::vector<int>(creaseIndices.begin(),
                                            creaseIndices.end()));
        builder.set("creaseIndices", creasesBuilder.build());

        VtIntArray creaseLengths;
        if (mesh.GetCreaseLengthsAttr().Get(&creaseLengths, time)
            and creaseLengths.size() > 0) {
            FnKat::IntBuilder creaseLengthsBuilder(1);
            creaseLengthsBuilder.set(std::vector<int>(creaseLengths.begin(),
                                                      creaseLengths.end()));
            builder.set("creaseLengths", creaseLengthsBuilder.build());
        }
        VtFloatArray creaseSharpness;
        if (mesh.GetCreaseSharpnessesAttr().Get(&creaseSharpness, time)
        and creaseSharpness.size() > 0) {
            FnKat::FloatBuilder creaseSharpnessBuilder(1);
            FnKat::IntBuilder creaseSharpnessLengthsBuilder(1);
            creaseSharpnessBuilder.set(
                std::vector<float>(
                    creaseSharpness.begin(), creaseSharpness.end()));
            std::vector<int> numSharpnesses;
            if (creaseLengths.size() == creaseSharpness.size()) {
                // We have exactly 1 sharpness per crease.
                numSharpnesses.resize(creaseLengths.size());
                std::fill(numSharpnesses.begin(), numSharpnesses.end(), 1);
            } else {
                // We have N-1 sharpnesses for each crease that has N edges.
                TF_FOR_ALL(lengthItr, creaseLengths) {
                    numSharpnesses.push_back((*lengthItr) - 1);
                }
            }
            creaseSharpnessLengthsBuilder.set(numSharpnesses);

            builder.set("creaseSharpness", creaseSharpnessBuilder.build());
            builder.set("creaseSharpnessLengths",
                        creaseSharpnessLengthsBuilder.build());
        }
    }

    // Corners
    VtIntArray cornerIndices;
    if (mesh.GetCornerIndicesAttr().Get(&cornerIndices, time)
        and cornerIndices.size() > 0) {
        FnKat::IntBuilder cornersBuilder(1);
        cornersBuilder.set(std::vector<int>(cornerIndices.begin(),
                                            cornerIndices.end()));
        builder.set("cornerIndices", cornersBuilder.build());
    }
    VtFloatArray cornerSharpness;
    if (mesh.GetCornerSharpnessesAttr().Get(&cornerSharpness, time)
        and cornerSharpness.size() > 0) {
        FnKat::FloatBuilder cornerSharpnessBuilder(1);
        cornerSharpnessBuilder.set(std::vector<float>(cornerSharpness.begin(),
                                                      cornerSharpness.end()));
        builder.set("cornerSharpness", cornerSharpnessBuilder.build());
    }

    return builder.build();
}


// Promote a FloatAttribute to vertex (facevarying in usd)
FnKat::FloatAttribute 
_promoteToVertex(
        FnKat::FloatAttribute& attr,
        std::string& scope,
        FnKat::GroupAttribute& polyAttr,
        double currentTime) 
{
    // If this is already vertex, return the input
    if (scope == "vertex") {
        // aka facevarying
        return attr;
    }

    // Grab the actual array
    FnKat::FloatConstVector attrArr = attr.getNearestSample(currentTime);

    // Determine the size of our array
    FnKat::IntAttribute vertIndex = polyAttr.getChildByName("vertexList");
    FnKat::IntConstVector vertIndexArray = vertIndex.getNearestSample(currentTime);
    std::vector<float> arr;
    arr.reserve(vertIndexArray.size());

    if (scope == "point") {
        // aka varying/vertex
        for (size_t i = 0; i < vertIndexArray.size(); ++i) {
            arr.push_back(attrArr[vertIndexArray[i]]);
        }
    } else if (scope == "face") {
        // aka uniform
        // TODO: check this works
        return FnKat::FloatAttribute();
    } else if (scope == "primitive") {
        for (size_t i = 0; i < vertIndexArray.size(); ++i) {
            arr.push_back(attrArr[0]);
        }
    } else {
        // Otherwise return an empty attribute
        FnLogWarn("Cannot promote unknown scope (" << scope << ") to vertex");
        return FnKat::FloatAttribute();
    }
    FnKat::FloatBuilder vecBuilder;
    vecBuilder.set(arr);
    return vecBuilder.build();
}


// Infer ST (if it doesn't exist) from u_map1 and v_map1
FnKat::GroupAttribute _inferSt(
        FnKat::GroupAttribute& primvarGroup,
        FnKat::GroupAttribute& polyAttr,
        double currentTime) 
{
    FnKat::GroupBuilder arbBuilder;

    // Grab relevant attributes
    FnKat::Attribute st = primvarGroup.getChildByName("st");
    FnKat::Attribute s = primvarGroup.getChildByName("s");
    FnKat::Attribute t = primvarGroup.getChildByName("t");
    FnKat::GroupAttribute u = primvarGroup.getChildByName("u_map1");
    FnKat::GroupAttribute v = primvarGroup.getChildByName("v_map1");

    // If we don't have u_map1 and v_map1, don't do anything
    // If st or s,t exist, don't override what is already there
    if (!u.isValid()
            || !v.isValid()
            || st.isValid()
            || s.isValid()
            || t.isValid()) {
        return FnKat::GroupAttribute();
    }

    // Check attribute scope
    FnKat::StringAttribute uScopeAttr = u.getChildByName("scope");
    FnKat::StringAttribute vScopeAttr = v.getChildByName("scope");
    std::string uScope = uScopeAttr.getValue();
    std::string vScope = vScopeAttr.getValue();

    // Things we'll use to build st
    std::string scope;
    FnKat::StringAttribute interpolationType;
    FnKat::FloatAttribute uAttrMatched, vAttrMatched;

    // Grab the u_map1 and v_map1 values
    FnKat::FloatAttribute uAttr = u.getChildByName("value");
    FnKat::FloatAttribute vAttr = v.getChildByName("value");

    // Don't do anything for matching face or primitive scopes
    // (uniform or constant details in usd/rib)
    if ((uScope == "face" || uScope == "primitive")
            && (vScope == "face" || vScope == "primitive")) {
        return FnKat::GroupAttribute();
    }

    if (uScope == vScope) {
        // Scopes match - interleave without promoting to vertex/facevarying
        scope = uScope;
        uAttrMatched = uAttr;
        vAttrMatched = vAttr;

        // If the interpolationTypes exist and match, use those as well
        FnKat::StringAttribute uInterpAttr = u.getChildByName(
                "interpolationType");
        FnKat::StringAttribute vInterpAttr = v.getChildByName(
                "interpolationType");
        if (uInterpAttr.isValid() && vInterpAttr.isValid()
                && uInterpAttr.getValue() == vInterpAttr.getValue()) {
            interpolationType = uInterpAttr;
        }
    } else {
        // If the scopes don't match, promote everything to vertex/facevarying
        scope = "vertex";
        // This could be optimized by not creating an intermediate array,
        // but I feel that this is a little cleaner
        uAttrMatched = _promoteToVertex(uAttr, uScope, polyAttr, currentTime);
        vAttrMatched = _promoteToVertex(vAttr, vScope, polyAttr, currentTime);
    }

    bool stBuilt = false;
    if (uAttrMatched.isValid() && vAttrMatched.isValid()) {
        // If we have valid matched arrays, then interleave them
        FnKat::FloatConstVector uArray = uAttrMatched.getNearestSample(
                currentTime);
        FnKat::FloatConstVector vArray = vAttrMatched.getNearestSample(
                currentTime);
        if (uArray.size() == vArray.size()) {
            std::vector<float> stArray;
            stArray.reserve(uArray.size()*2);
            for (size_t i = 0; i < uArray.size(); ++i) {
                stArray.push_back(uArray[i]);
                stArray.push_back(vArray[i]);
            }

            // Build into ST
            FnKat::GroupBuilder attrBuilder;
            FnKat::FloatBuilder vecBuilder(/* tupleSize = */ 2);
            vecBuilder.set(stArray);
            attrBuilder.set("scope", FnKat::StringAttribute(scope));
            attrBuilder.set("inputType",
                    FnKat::StringAttribute("point2"));
            attrBuilder.set("value", vecBuilder.build());
            if (interpolationType.isValid()) {
                attrBuilder.set("interpolationType", interpolationType);
            }
            arbBuilder.set("st", attrBuilder.build());
            stBuilt = true;
        } else {
            FnLogWarn("u_map1 and v_map1 promoted to vertex, but are"
                    << "different sizes.");
        }
    }

    if (!stBuilt && u.isValid() && v.isValid()) {
        // If we couldn't promote the arrays to vertex, store S,T separately
        // as a fallback. RenderMan will combine s,t into st at render time.
        FnLogDebug("Unablo to interleave/promote to vertex: "
                << "u_map1 scope (" << uScope << ") "
                << "v_map1 scope (" << vScope << "). "
                << "Storing S, T separately.");
        arbBuilder.set("s", u);
        arbBuilder.set("t", v);
    }

    return arbBuilder.build();
}


void
PxrUsdKatanaReadMesh(
        const UsdGeomMesh& mesh,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

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

    FnKat::GroupBuilder geometryBuilder;

    geometryBuilder.set("point.P", PxrUsdKatanaGeomGetPAttr(mesh, data));

    /// Only use custom normals if the object is a polymesh.
    if (not isSubd){
        // normals
        FnKat::Attribute normalsAttr = PxrUsdKatanaGeomGetNormalAttr(mesh, data);
        if (normalsAttr.isValid())
        {
            geometryBuilder.set("point.N", normalsAttr);
        }
    }

    FnKat::GroupAttribute polyAttr = _GetPolyAttr(mesh, currentTime);
    
    geometryBuilder.set("poly", polyAttr);

    if (isSubd)
    {
        FnKat::Attribute subdivTagsGroup =
            _GetSubdivTagsGroup(mesh, /* hierarchical=*/ false, currentTime);

        if (subdivTagsGroup.isValid())
        {
            geometryBuilder.update( subdivTagsGroup );
        }
    }

    FnKat::GroupBuilder arbBuilder;

    arbBuilder.set("SPT_HwColor", 
        PxrUsdKatanaGeomGetDisplayColorAttr(mesh, data));

    FnKat::GroupAttribute primvarGroup =
        PxrUsdKatanaGeomGetPrimvarGroup(mesh, data);

    if (primvarGroup.isValid())
    {
        // Infer ST if it doesn't exist.
        // Remove this later when ST is built through in USD.
        FnKat::Attribute stAttrib = _inferSt(primvarGroup, polyAttr, currentTime);
        if (stAttrib.isValid())
        {
            arbBuilder.update(stAttrib);
        }

        arbBuilder.update(primvarGroup);
    }

    geometryBuilder.set("arbitrary", arbBuilder.build());
    attrs.set("geometry", geometryBuilder.build());

    attrs.set(
        "viewer.default.drawOptions.windingOrder",
            PxrUsdKatanaGeomGetWindingOrderAttr(mesh, data));

    // This value will be one of 'catmullClark', 'loop', 'bilinear',
    // or 'none'.  'none' means this is a polymesh, and not
    // a subdiv, so don't set this.
    if (mesh.GetSubdivisionSchemeAttr().Get(&scheme) and
            scheme != UsdGeomTokens->none)
    {
        // USD deviates from Katana only in the 'catmullClark' token.
        static char const *catclark("catmull-clark");
        char const *katScheme = 
            (scheme == UsdGeomTokens->catmullClark ? catclark : scheme.GetText());

        attrs.set(
            "prmanStatements.subdivisionMesh.scheme",
                 FnKat::StringAttribute(katScheme));
    }

    attrs.set("tabs.scenegraph.stopExpand", FnKat::IntAttribute(1));
}
