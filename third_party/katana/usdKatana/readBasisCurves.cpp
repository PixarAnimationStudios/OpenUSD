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
#include "usdKatana/readBasisCurves.h"
#include "usdKatana/readGprim.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usdGeom/basisCurves.h"

#include <FnAPI/FnAPI.h>
#include <FnAttribute/FnDataBuilder.h>
#include <FnLogging/FnLogging.h>

#if KATANA_VERSION_MAJOR >= 3
#include "vtKatana/array.h"
#endif // KATANA_VERSION_MAJOR >= 3

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadBasisCurves");

static void
_SetCurveAttrs(PxrUsdKatanaAttrMap& attrs,
               const UsdGeomBasisCurves& basisCurves, double currentTime)
{
    VtIntArray vtxCts;
    basisCurves.GetCurveVertexCountsAttr().Get(&vtxCts, currentTime);

#if KATANA_VERSION_MAJOR >= 3
    auto countsAttr = VtKatanaMapOrCopy(vtxCts);
    attrs.set("geometry.numVertices", countsAttr);
#else
    std::vector<int> ctsVec(vtxCts.begin(), vtxCts.end());

    FnKat::IntBuilder numVertsBuilder(1);
    std::vector<int> &numVerts = numVertsBuilder.get();
    numVerts = ctsVec;

    attrs.set("geometry.numVertices", numVertsBuilder.build());
#endif // KATANA_VERSION_MAJOR >= 3

    VtFloatArray widths;
    basisCurves.GetWidthsAttr().Get(&widths, currentTime);
    size_t numWidths = widths.size();
    if (numWidths == 1)
    {
        attrs.set("geometry.constantWidth",
            FnKat::FloatAttribute(widths[0]));
    }
    else if (numWidths > 1)
    {
        const char * widthValueName = "geometry.point.width";
        TfToken interp = basisCurves.GetWidthsInterpolation();

        if (interp == UsdGeomTokens->vertex)
        {
            // vertex corresponds to "geometry.point.width"
            // everything else should go into arbitrary so that its footprint
            // can be validated
        }
        else
        {
            widthValueName = "geometry.arbitrary.width.value";
            
            if (interp == UsdGeomTokens->varying)
            {
                attrs.set("geometry.arbitrary.width.scope",
                        FnAttribute::StringAttribute("point"));

            }
            else if (interp == UsdGeomTokens->uniform)
            {
                attrs.set("geometry.arbitrary.width.scope",
                        FnAttribute::StringAttribute("face"));
            }
            else
            {
                 FnLogWarn("Unsupported width interpolation, "
                        << interp.GetString()
                        << ", in " << basisCurves.GetPath().GetString());
            }
        }


#if KATANA_VERSION_MAJOR >= 3
        auto widthsAttr = VtKatanaMapOrCopy(widths);
        attrs.set(widthValueName, widthsAttr);
#else
        FnKat::FloatBuilder widthsBuilder(1);
        widthsBuilder.set(
            std::vector<float>(widths.begin(), widths.end()));
        attrs.set(widthValueName, widthsBuilder.build());
#endif // KATANA_VERSION_MAJOR >= 3
    }

    TfToken curveType;
    basisCurves.GetTypeAttr().Get(&curveType, currentTime);
    attrs.set("geometry.degree",
        FnKat::IntAttribute(curveType == UsdGeomTokens->linear ? 1 : 3));
}

void
PxrUsdKatanaReadBasisCurves(
        const UsdGeomBasisCurves& basisCurves,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    //
    // Set all general attributes for a gprim type.
    //

    PxrUsdKatanaReadGprim(basisCurves, data, attrs);

    //
    // Set more specific Katana type.
    //

    attrs.set("type", FnKat::StringAttribute("curves"));

    //
    // Set 'prmanStatements' attribute.
    //

    TfToken basis;
    basisCurves.GetBasisAttr().Get(&basis);
    if (basis == UsdGeomTokens->bezier)
    {
        attrs.set("prmanStatements.basis.u",
                        FnKat::StringAttribute("bezier"));
        attrs.set("prmanStatements.basis.v",
                        FnKat::StringAttribute("bezier"));
    }
    else if (basis == UsdGeomTokens->bspline)
    {
        attrs.set("prmanStatements.basis.u",
                        FnKat::StringAttribute("b-spline"));
        attrs.set("prmanStatements.basis.v",
                        FnKat::StringAttribute("b-spline"));
    }
    else if (basis == UsdGeomTokens->catmullRom)
    {
        attrs.set("prmanStatements.basis.u",
                        FnKat::StringAttribute("catmull-rom"));
        attrs.set("prmanStatements.basis.v",
                        FnKat::StringAttribute("catmull-rom"));
    }
    else if (basis == UsdGeomTokens->hermite)
    {
        attrs.set("prmanStatements.basis.u",
                        FnKat::StringAttribute("hermite"));
        attrs.set("prmanStatements.basis.v",
                        FnKat::StringAttribute("hermite"));
    }
    else if (basis == UsdGeomTokens->power)
    {
        attrs.set("prmanStatements.basis.u",
                        FnKat::StringAttribute("power"));
        attrs.set("prmanStatements.basis.v",
                        FnKat::StringAttribute("power"));
    }
    else {
        FnLogWarn("Ignoring unsupported curve basis, " << basis.GetString()
                  << ", in " << basisCurves.GetPath().GetString());
    }

    //
    // Construct the 'geometry' attribute.
    //

    _SetCurveAttrs(attrs, basisCurves, data.GetCurrentTime());
    
    // position
    attrs.set("geometry.point.P",
        PxrUsdKatanaGeomGetPAttr(basisCurves, data));

    // normals
    FnKat::Attribute normalsAttr = PxrUsdKatanaGeomGetNormalAttr(basisCurves, data);
    if (normalsAttr.isValid())
    {
        // XXX RfK doesn't support uniform normals for curves.
        // Additionally, varying and facevarying may not be correct for
        // periodic cubic curves.
        TfToken interp = basisCurves.GetNormalsInterpolation();
        
        if (interp == UsdGeomTokens->vertex)
        {
            // non-arbitrary N is assumed to match the point length
            // ("vertex") in prman
            attrs.set("geometry.point.N", normalsAttr);
        }
        else
        {
            // otherwise, use full arbitrary declaration
            FnAttribute::StringAttribute scopeValue;

            if (interp == UsdGeomTokens->constant)
            {
                scopeValue = FnAttribute::StringAttribute("primitive");
            }
            else if (interp == UsdGeomTokens->uniform)
            {
                scopeValue = FnAttribute::StringAttribute("face");   
            }
            else if (interp == UsdGeomTokens->varying)
            {
                scopeValue = FnAttribute::StringAttribute("point");   
            }

            if (scopeValue.isValid())
            {
                attrs.set("geometry.arbitrary.N.scope", scopeValue);
                attrs.set("geometry.arbitrary.N.inputType",
                        FnAttribute::StringAttribute("normal3"));
                attrs.set("geometry.arbitrary.N.value", normalsAttr);    
            }
            else
            {
                FnLogWarn("Ignoring unsupported N interpolation, " << interp.GetString()
                        << ", in " << basisCurves.GetPath().GetString());
            }
        }
    }

    // velocity
    FnKat::Attribute velocityAttr =
        PxrUsdKatanaGeomGetVelocityAttr(basisCurves, data);
    if (velocityAttr.isValid())
    {
        attrs.set("geometry.point.v", velocityAttr);
    }

    // Add SPT_HwColor primvar
    attrs.set("geometry.arbitrary.SPT_HwColor", 
              PxrUsdKatanaGeomGetDisplayColorAttr(basisCurves, data));
}

PXR_NAMESPACE_CLOSE_SCOPE

