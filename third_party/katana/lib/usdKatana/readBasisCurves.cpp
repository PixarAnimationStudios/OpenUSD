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
#include "usdKatana/readBasisCurves.h"
#include "usdKatana/readGprim.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usdGeom/basisCurves.h"

#include <FnAttribute/FnDataBuilder.h>
#include <FnLogging/FnLogging.h>

FnLogSetup("PxrUsdKatanaReadBasisCurves");

static FnKat::GroupAttribute
_GetCurveAttrs(const UsdGeomBasisCurves& basisCurves, double currentTime)
{
    VtIntArray vtxCts;
    basisCurves.GetCurveVertexCountsAttr().Get(&vtxCts, currentTime);
    std::vector<int> ctsVec(vtxCts.begin(), vtxCts.end());

    FnKat::IntBuilder numVertsBuilder(1);
    std::vector<int> &numVerts = numVertsBuilder.get();
    numVerts = ctsVec;

    FnKat::GroupBuilder curveBuilder;
    curveBuilder.set("numVertices", numVertsBuilder.build());

    VtFloatArray widths;
    basisCurves.GetWidthsAttr().Get(&widths, currentTime);
    size_t numWidths = widths.size();
    if (numWidths == 1)
    {
        curveBuilder.set("constantWidth",
            FnKat::FloatAttribute(widths[0]));
    }
    else if (numWidths > 1)
    {
        FnKat::FloatBuilder widthsBuilder(1);
        widthsBuilder.set(
            std::vector<float>(widths.begin(), widths.end()));
        curveBuilder.set("point.width", widthsBuilder.build());
    }

    TfToken curveType;
    basisCurves.GetTypeAttr().Get(&curveType, currentTime);
    curveBuilder.set("degree",
        FnKat::IntAttribute(curveType == UsdGeomTokens->linear ? 1 : 3));

    return curveBuilder.build();
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

    FnKat::GroupBuilder geometryBuilder;

    FnKat::GroupAttribute curveAttr = _GetCurveAttrs(
        basisCurves, data.GetUsdInArgs()->GetCurrentTime());
    
    if (curveAttr.isValid()) {
        geometryBuilder.update(curveAttr);
    }
    
    geometryBuilder.set("point.P",
        PxrUsdKatanaGeomGetPAttr(basisCurves, data));

    // normals
    FnKat::Attribute normalsAttr = PxrUsdKatanaGeomGetNormalAttr(basisCurves, data);
    if (normalsAttr.isValid())
    {
        geometryBuilder.set("point.N", normalsAttr);
    }

    // Construct the 'geometry.arbitrary' attribute.
    
    FnKat::GroupBuilder arbBuilder;

    arbBuilder.set("SPT_HwColor",
        PxrUsdKatanaGeomGetDisplayColorAttr(basisCurves, data));

    FnKat::Attribute primvarGroup =
        PxrUsdKatanaGeomGetPrimvarGroup(basisCurves, data);
    if (primvarGroup.isValid())
    {
        arbBuilder.update(primvarGroup);
    }

    geometryBuilder.set("arbitrary", arbBuilder.build());
    attrs.set("geometry", geometryBuilder.build());  
}
