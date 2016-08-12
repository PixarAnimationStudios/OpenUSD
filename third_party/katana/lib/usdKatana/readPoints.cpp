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
#include "usdKatana/readGprim.h"
#include "usdKatana/readPoints.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/points.h"

static FnKat::Attribute
_GetVelocityAttr(
    const UsdGeomPointBased& points,
    double currentTime)
{
    VtVec3fArray velocities;
    if (not points.GetVelocitiesAttr().Get(&velocities, currentTime))
    {
        return FnKat::Attribute();
    }

    // float attribute list with a width of 3
    FnKat::FloatBuilder velocitiesBuilder(3);
    std::vector<float> velVec;
    PxrUsdKatanaUtils::ConvertArrayToVector(velocities, &velVec);
    velocitiesBuilder.set(velVec);

    return velocitiesBuilder.build();
}

static FnKat::Attribute
_GetWidthAttr(const UsdGeomPoints& points, double currentTime)
{
    VtFloatArray widths;
    if (not points.GetWidthsAttr().Get(&widths, currentTime))
    {
        return FnKat::Attribute();
    }

    FnKat::FloatBuilder widthsBuilder(1);
    widthsBuilder.set(std::vector<float>(widths.begin(), widths.end()));


    return widthsBuilder.build();
}

void
PxrUsdKatanaReadPoints(
        const UsdGeomPoints& points,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetUsdInArgs()->GetCurrentTimeD();

    //
    // Set all general attributes for a gprim type.
    //

    PxrUsdKatanaReadGprim(points, data, attrs);

    //
    // Set more specific Katana type.
    //

    attrs.set("type", FnKat::StringAttribute("pointcloud"));

    //
    // Construct the 'geometry' attribute.
    //

    FnKat::GroupBuilder geometryBuilder;

    // point
    geometryBuilder.set("point.P", PxrUsdKatanaGeomGetPAttr(points, data));

    // velocity
    FnKat::Attribute velocitiesAttr = _GetVelocityAttr(points, currentTime);
    if (velocitiesAttr.isValid())
    {
        geometryBuilder.set("point.v", velocitiesAttr);
    }

    // normals
    FnKat::Attribute normalsAttr = PxrUsdKatanaGeomGetNormalAttr(points, data);
    if (normalsAttr.isValid())
    {
        geometryBuilder.set("point.N", normalsAttr);
    }

    // width
    FnKat::Attribute widthsAttr = _GetWidthAttr(points, currentTime);
    if (widthsAttr.isValid())
    {
        geometryBuilder.set("point.width", widthsAttr);
    }

    
    FnKat::GroupBuilder arbBuilder;

    // other primvars
    FnKat::Attribute primvarGroup = PxrUsdKatanaGeomGetPrimvarGroup(points, data);
    if (primvarGroup.isValid())
    {
        arbBuilder.update(primvarGroup);
    }

    geometryBuilder.set("arbitrary", arbBuilder.build());
    attrs.set("geometry", geometryBuilder.build());
}
