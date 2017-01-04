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
#include "usdKatana/readConstraintTarget.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usdGeom/constraintTarget.h"

#include <FnAttribute/FnGroupBuilder.h>
#include <FnAttribute/FnDataBuilder.h>
#include <FnLogging/FnLogging.h>

#include <vector>

FnLogSetup("PxrUsdKatanaReadConstraintTarget");

static FnKat::Attribute
_BuildLocatorGeometryAttr( const float* color=NULL )
{
    // This data comes from Katana's own 'locator'
    float points[96] = {
        -0.0125, 0.0125, 0.0125, -0.0125, 0.5000, 0.0125, -0.0125, 0.5000,
        -0.0125, -0.0125, 0.0125, -0.0125, -0.0125, -0.0125, -0.0125,
        -0.0125, -0.5000, -0.0125, -0.0125, -0.5000, 0.0125, -0.0125,
        -0.0125, 0.0125, 0.0125, -0.0125, 0.0125, 0.0125, -0.5000, 0.0125,
        0.0125, -0.5000, -0.0125, 0.0125, -0.0125, -0.0125, 0.0125, 0.0125,
        -0.0125, 0.0125, 0.5000, -0.0125, 0.0125, 0.5000, 0.0125, 0.0125,
        0.0125, 0.0125, 0.0125, -0.0125, -0.5000, 0.0125, 0.0125, -0.5000,
        -0.0125, -0.0125, -0.5000, -0.0125, 0.0125, -0.5000, 0.0125,
        -0.0125, 0.5000, -0.0125, -0.0125, 0.5000, -0.0125, 0.0125, 0.5000,
        0.0125, 0.0125, 0.5000, 0.5000, 0.0125, 0.0125, 0.5000, 0.0125,
        -0.0125, 0.5000, -0.0125, -0.0125, 0.5000, -0.0125, 0.0125,
        -0.5000, 0.0125, -0.0125, -0.5000, -0.0125, -0.0125, -0.5000,
        -0.0125, 0.0125, -0.5000, 0.0125, 0.0125 };

    int vertices[120] = {
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12, 2, 13, 14, 1,
        6, 9, 10, 5, 12, 17, 16, 11, 19, 18, 16, 17, 4, 18, 19, 3, 11, 16,
        18, 4, 3, 19, 17, 12, 7, 21, 20, 8, 0, 22, 21, 7, 15, 23, 22, 0, 8,
        20, 23, 15, 23, 20, 21, 22, 8, 9, 6, 7, 1, 14, 15, 0, 12, 13, 2, 3,
        4, 5, 10, 11, 25, 24, 15, 12, 26, 25, 12, 11, 27, 26, 11, 8, 15,
        24, 27, 8, 25, 26, 27, 24, 31, 30, 29, 28, 28, 29, 4, 3, 29, 30, 7,
        4, 7, 30, 31, 0, 31, 28, 3, 0 };

    int startIndices[31] = {
        0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64,
        68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120 };

    FnKat::GroupBuilder geometryBuilder;

    geometryBuilder.set("point.P", FnKat::FloatAttribute(points, 96, 3));
    geometryBuilder.set("poly.vertexList", FnKat::IntAttribute(vertices, 120, 1)); 
    geometryBuilder.set("poly.startIndex", FnKat::IntAttribute(startIndices, 31, 1));

    if ( color ) {
        geometryBuilder.set("arbitrary.SPT_HwColor.inputType", FnKat::StringAttribute("color3"));
        geometryBuilder.set("arbitrary.SPT_HwColor.scope", FnKat::StringAttribute("primitive"));
        geometryBuilder.set("arbitrary.SPT_HwColor.value", FnKat::FloatAttribute(color, 3, 3));
    }

    return geometryBuilder.build();
}

FnKat::Attribute
_BuildMatrixAttr(
        const UsdGeomConstraintTarget& constraintTarget,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // Eval transform.
    UsdAttribute constraintAttr = constraintTarget.GetAttr();
    if (!constraintAttr)
        return FnKat::Attribute();

    double currentTime = data.GetUsdInArgs()->GetCurrentTime();
    const std::vector<double>& motionSampleTimes = 
        data.GetMotionSampleTimes(constraintAttr);

    const bool isMotionBackward = data.GetUsdInArgs()->IsMotionBackward();

    FnKat::DoubleBuilder matBuilder(16);
    TF_FOR_ALL(iter, motionSampleTimes) {
        double relSampleTime = *iter;
        double time = currentTime + relSampleTime;

        GfMatrix4d mat;
        if (!constraintAttr.Get(&mat, time))
            return FnKat::Attribute();

        // Convert to vector.
        const double *matArray = mat.GetArray();

        std::vector<double> &matVec = matBuilder.get(isMotionBackward ?
            PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

        matVec.resize(16);
        for (int i = 0; i < 16; ++i) {
            matVec[i] = matArray[i];
        }
    }
    
    return matBuilder.build();
}

void
PxrUsdKatanaReadConstraintTarget(
        const UsdGeomConstraintTarget& constraintTarget,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    //
    // Give constraint target locations a generic 'locator' type.
    //

    attrs.set("type",FnKat::StringAttribute("locator"));

    //
    // Build the transformation matrix for the 'xform' attribute. 
    //

    FnKat::GroupBuilder gb;
    gb.set("matrix", _BuildMatrixAttr(constraintTarget, data));
    gb.setGroupInherit(false);
    attrs.set("xform", gb.build());

    //
    // Create a default bound so the location can be targeted.
    //

    FnKat::DoubleBuilder boundBuilder;
    boundBuilder.push_back( -0.5 );
    boundBuilder.push_back(  0.5 );
    boundBuilder.push_back( -0.5 );
    boundBuilder.push_back(  0.5 );
    boundBuilder.push_back( -0.5 );
    boundBuilder.push_back(  0.5 );
    attrs.set("bound", boundBuilder.build());

    //
    // Create the visible geometry for the locator.
    //

    float color[3] = { 0.0, 1.0, 0.0 };
    attrs.set("geometry", _BuildLocatorGeometryAttr(color));

    //
    // Make the locator 'wireframe' in the viewer.
    //

    FnKat::GroupBuilder viewerBuilder;
    viewerBuilder.set("default.drawOptions.fill", FnKat::StringAttribute("wireframe"));
    attrs.set("viewer", viewerBuilder.build());
}
