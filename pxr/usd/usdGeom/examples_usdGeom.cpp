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
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/xformOp.h"

PXR_NAMESPACE_OPEN_SCOPE


//! [CreateMatrixWithDefault]
bool CreateMatrixWithDefault(UsdGeomXformable const &gprim, GfMatrix4d const &defValue)
{
    if (UsdGeomXformOp transform = gprim.MakeMatrixXform()){
        return transform.Set(defValue, UsdTimeCode::Default());
    } else {
        return false;
    }
}
//! [CreateMatrixWithDefault]


//! [CreateExampleSRT]
bool CreateExampleSRT(UsdGeomXformable const &gprim)
{
    // For insurance, we will make sure there aren't any ordered ops
    // before we start
    gprim.ClearXformOpOrder();

    UsdGeomXformOp s, r, t;
    
    if ( !(t = gprim.AddTranslateOp())){
        return false;
    }
    if ( !(r = gprim.AddRotateXYZOp())){
        return false;
    }
    if ( !(s = gprim.AddScaleOp())){
        return false;
    }

    return (t.Set(GfVec3d(0, 100, 0), UsdTimeCode::Default()) &&
            r.Set(GfVec3f(30, 60, 90), UsdTimeCode::Default()) &&
            s.Set(GfVec3f(2, 2, 2), UsdTimeCode::Default()));
}
//! [CreateExampleSRT]


//! [CreateSRTWithDefaults]
bool CreateSRTWithDefaults(UsdGeomXformable const &gprim, 
                           GfVec3d const &defTranslate,
                           GfVec3f const &defRotateXYZ,
                           GfVec3f const &defScale,
                           GfVec3f const &defPivot)
{
    if (UsdGeomXformCommonAPI xform = UsdGeomXformCommonAPI(gprim)){
        return xform.SetXformVectors(defTranslate, defRotateXYZ, defScale,
                                     defPivot, UsdGeomXformCommonAPI::RotationOrderXYZ,
                                     UsdTimeCode::Default());
    } else {
        return false;
    }
}
//! [CreateSRTWithDefaults]


//! [CreateAnimatedTransform]
bool CreateAnimatedTransform(UsdGeomXformable const &gprim, 
                             GfVec3d const &baseTranslate,
                             GfVec3f const &baseRotateXYZ,
                             GfVec3f const &defPivot)
{
    // Only need to do this if you're overriding an existing scene
    if (!gprim.ClearXformOpOrder()){
        return false;
    }
    
    static const TfToken  pivSuffix("pivot");
    UsdGeomXformOp    trans = gprim.AddTranslateOp();
    UsdGeomXformOp    pivot = gprim.AddTranslateOp(UsdGeomXformOp::PrecisionFloat,
                                                   pivSuffix);
    UsdGeomXformOp   rotate = gprim.AddRotateXYZOp();
    UsdGeomXformOp pivotInv = gprim.AddTranslateOp(UsdGeomXformOp::PrecisionFloat,
                                                   pivSuffix,
                                                   /* isInverseOp = */ true);
    // Now that we have created all the ops, set default values.
    // Note that we do not need to (and cannot) set the value
    // for the pivot's inverse op.
    // For didactic brevity we are eliding success return value checks,
    // but would absolutely have them in exporters!
    trans.Set(baseTranslate, UsdTimeCode::Default());
    pivot.Set(defPivot, UsdTimeCode::Default());
    rotate.Set(baseRotateXYZ, UsdTimeCode::Default());
    
    // Now animate the translation and rotation over a fixed interval with
    // cheesy linear animation.
    GfVec3d  position(baseTranslate);
    GfVec3f  rotation(baseRotateXYZ);
    
    for (double frame = 0; frame < 100.0; frame += 1.0){
        trans.Set(position, frame);
        rotate.Set(rotation, frame);
        position[0] += 5.0;
        rotation[2] += 7.0;
    }
    return true;
}
//! [CreateAnimatedTransform]

PXR_NAMESPACE_CLOSE_SCOPE

