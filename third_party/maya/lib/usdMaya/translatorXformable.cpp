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
#include "usdMaya/translatorXformable.h"

#include "usdMaya/translatorPrim.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/xformStack.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include <maya/MDagModifier.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnTransform.h>
#include <maya/MEulerRotation.h>
#include <maya/MPlug.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MFnDependencyNode.h>

#include <boost/assign/list_of.hpp>
#include <algorithm>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// This function retrieves a value for a given xformOp and given time sample. It
// knows how to deal with different type of ops and angle conversion
static bool _getXformOpAsVec3d(
        const UsdGeomXformOp &xformOp,
        GfVec3d &value,
        const UsdTimeCode &usdTime)
{
    bool retValue = false;

    const UsdGeomXformOp::Type opType = xformOp.GetOpType();

    if (opType == UsdGeomXformOp::TypeScale) {
        value = GfVec3d(1.0);
    } else {
        value = GfVec3d(0.0);
    }

    // Check whether the XformOp is a type of rotation.
    int rotAxis = -1;
    double angleMult = GfDegreesToRadians(1.0);

    switch(opType) {
        case UsdGeomXformOp::TypeRotateX:
            rotAxis = 0;
            break;
        case UsdGeomXformOp::TypeRotateY:
            rotAxis = 1;
            break;
        case UsdGeomXformOp::TypeRotateZ:
            rotAxis = 2;
            break;
        case UsdGeomXformOp::TypeRotateXYZ:
        case UsdGeomXformOp::TypeRotateXZY:
        case UsdGeomXformOp::TypeRotateYXZ:
        case UsdGeomXformOp::TypeRotateYZX:
        case UsdGeomXformOp::TypeRotateZXY:
        case UsdGeomXformOp::TypeRotateZYX:
            break;
        default:
            // This XformOp is not a rotation, so we're not converting an
            // angular value from degrees to radians.
            angleMult = 1.0;
            break;
    }

    // If we encounter a transform op, we treat it as a shear operation.
    if (opType == UsdGeomXformOp::TypeTransform) {
        // GetOpTransform() handles the inverse op case for us.
        GfMatrix4d xform = xformOp.GetOpTransform(usdTime);
        value[0] = xform[1][0]; //xyVal
        value[1] = xform[2][0]; //xzVal
        value[2] = xform[2][1]; //yzVal
        retValue = true;
    } else if (rotAxis != -1) {
        // Single Axis rotation
        double valued = 0;
        retValue = xformOp.GetAs<double>(&valued, usdTime);
        if (retValue) {
            if (xformOp.IsInverseOp()) {
                valued = -valued;
            }
            value[rotAxis] = valued * angleMult;
        }
    } else {
        GfVec3d valued;
        retValue = xformOp.GetAs<GfVec3d>(&valued, usdTime);
        if (retValue) {
            if (xformOp.IsInverseOp()) {
                valued = -valued;
            }
            value[0] = valued[0] * angleMult;
            value[1] = valued[1] * angleMult;
            value[2] = valued[2] * angleMult;
        }
    }

    return retValue;
}

// Sets the animation curve (a knot per frame) for a given plug/attribute
static void _setAnimPlugData(MPlug plg, std::vector<double> &value, MTimeArray &timeArray,
        const PxrUsdMayaPrimReaderContext* context)
{
    MStatus status;
    MFnAnimCurve animFn;
    // Make the plug keyable before attaching an anim curve
    if (!plg.isKeyable()) {
        plg.setKeyable(true);
    }
    MObject animObj = animFn.create(plg, nullptr, &status);
    if (status == MS::kSuccess ) {
        MDoubleArray valueArray( &value[0], value.size());
        animFn.addKeys(&timeArray, &valueArray);
        if (context) {
            context->RegisterNewMayaNode(animFn.name().asChar(), animObj );
        }
    } else {
        MString mayaPlgName = plg.partialName(true, true, true, false, true, true, &status);
        TF_RUNTIME_ERROR(
                "Failed to create animation object for attribute: %s",
                mayaPlgName.asChar());
    }
}

// Returns true if the array is not constant
static bool _isArrayVarying(std::vector<double> &value)
{
    bool isVarying=false;
    for (unsigned int i=1;i<value.size();i++) {
        if (!GfIsClose(value[0], value[i], 1e-9)) { isVarying=true; break; }
    }
    return isVarying;
}

// Sets the Maya Attribute values. Sets the value to the first element of the
// double arrays and then if the array is varying defines an anym curve for the
// attribute
static void _setMayaAttribute(
        MFnDagNode &depFn, 
        std::vector<double> &xVal, std::vector<double> &yVal, std::vector<double> &zVal, 
        MTimeArray &timeArray, 
        const MString& opName, 
        const MString& x, const MString& y, const MString& z,
        const PxrUsdMayaPrimReaderContext* context)
{
    MPlug plg;
    if (x!="" && !xVal.empty()) {
        plg = depFn.findPlug(opName+x);
        if ( !plg.isNull() ) {
            plg.setDouble(xVal[0]);
            if (xVal.size()>1 && _isArrayVarying(xVal)) _setAnimPlugData(plg, xVal, timeArray, context);
        }
    }
    if (y!="" && !yVal.empty()) {
        plg = depFn.findPlug(opName+y);
        if ( !plg.isNull() ) {
            plg.setDouble(yVal[0]);
            if (yVal.size()>1 && _isArrayVarying(yVal)) _setAnimPlugData(plg, yVal, timeArray, context);
        }
    }
    if (z!="" && !zVal.empty()) {
        plg = depFn.findPlug(opName+z);
        if ( !plg.isNull() ) {
            plg.setDouble(zVal[0]);
            if (zVal.size()>1 && _isArrayVarying(zVal)) _setAnimPlugData(plg, zVal, timeArray, context);
        }
    }
}

// For each xformop, we gather it's data either time sampled or not and we push
// it to the corresponding Maya xform
static bool _pushUSDXformOpToMayaXform(
        const UsdGeomXformOp& xformop, 
        const TfToken& opName,
        MFnDagNode &MdagNode,
        const PxrUsdMayaPrimReaderArgs& args,
        const PxrUsdMayaPrimReaderContext* context)
{
    std::vector<double> xValue;
    std::vector<double> yValue;
    std::vector<double> zValue;
    GfVec3d value;
    std::vector<double> timeSamples;
    if (!args.GetTimeInterval().IsEmpty()) {
        xformop.GetTimeSamplesInInterval(args.GetTimeInterval(), &timeSamples);
    }
    MTimeArray timeArray;
    if (!timeSamples.empty()) {
        timeArray.setLength(timeSamples.size());
        xValue.resize(timeSamples.size());
        yValue.resize(timeSamples.size());
        zValue.resize(timeSamples.size());
        for (unsigned int ti=0; ti < timeSamples.size(); ++ti) {
            UsdTimeCode time(timeSamples[ti]);
            if (_getXformOpAsVec3d(xformop, value, time)) {
                xValue[ti]=value[0]; yValue[ti]=value[1]; zValue[ti]=value[2];
                timeArray.set(MTime(timeSamples[ti]), ti);
            } 
            else {
                TF_RUNTIME_ERROR(
                        "Missing sampled data on xformOp: %s",
                        xformop.GetName().GetText());
            }
        }
    } 
    else {
        // pick the first available sample or default
        UsdTimeCode time=UsdTimeCode::EarliestTime();
        if (_getXformOpAsVec3d(xformop, value, time)) {
            xValue.resize(1);
            yValue.resize(1);
            zValue.resize(1);
            xValue[0]=value[0]; yValue[0]=value[1]; zValue[0]=value[2];
        } 
        else {
            TF_RUNTIME_ERROR(
                    "Missing default data on xformOp: %s",
                    xformop.GetName().GetText());
        }
    }
    if (!xValue.empty()) {
        if (opName==PxrUsdMayaXformStackTokens->shear) {
            _setMayaAttribute(MdagNode, xValue, yValue, zValue, timeArray, MString(opName.GetText()), "XY", "XZ", "YZ", context);
        } 
        else if (opName==PxrUsdMayaXformStackTokens->pivot) {
            _setMayaAttribute(MdagNode, xValue, yValue, zValue, timeArray, MString("rotatePivot"), "X", "Y", "Z", context);
            _setMayaAttribute(MdagNode, xValue, yValue, zValue, timeArray, MString("scalePivot"), "X", "Y", "Z", context);
        }
        else if (opName==PxrUsdMayaXformStackTokens->pivotTranslate) {
            _setMayaAttribute(MdagNode, xValue, yValue, zValue, timeArray, MString("rotatePivotTranslate"), "X", "Y", "Z", context);
            _setMayaAttribute(MdagNode, xValue, yValue, zValue, timeArray, MString("scalePivotTranslate"), "X", "Y", "Z", context);
        }
        else {
            if (opName==PxrUsdMayaXformStackTokens->rotate) {
                MFnTransform trans;
                if(trans.setObject(MdagNode.object()))
                {
                    auto MrotOrder =
                            PxrUsdMayaXformStack::RotateOrderFromOpType<MTransformationMatrix::RotationOrder>(
                                    xformop.GetOpType());
                    MPlug plg = MdagNode.findPlug("rotateOrder");
                    if ( !plg.isNull() ) {
                        trans.setRotationOrder(MrotOrder, /*no need to reorder*/ false);
                    }
                }
            }
            else if(opName==PxrUsdMayaXformStackTokens->rotateAxis)
            {
                // Rotate axis only accepts input in XYZ form
                // (though it's actually stored as a quaternion),
                // so we need to convert other rotation orders to XYZ
                const auto opType = xformop.GetOpType();
                if (opType != UsdGeomXformOp::TypeRotateXYZ
                        && opType != UsdGeomXformOp::TypeRotateX
                        && opType != UsdGeomXformOp::TypeRotateY
                        && opType != UsdGeomXformOp::TypeRotateZ)
                {
                    for (size_t i = 0u; i < xValue.size(); ++i)
                    {
                        auto MrotOrder =
                                PxrUsdMayaXformStack::RotateOrderFromOpType<MEulerRotation::RotationOrder>(
                                        xformop.GetOpType());
                        MEulerRotation eulerRot(xValue[i], yValue[i], zValue[i], MrotOrder);
                        eulerRot.reorderIt(MEulerRotation::kXYZ);
                        xValue[i] = eulerRot.x;
                        yValue[i] = eulerRot.y;
                        zValue[i] = eulerRot.z;
                    }
                }
            }
            _setMayaAttribute(MdagNode, xValue, yValue, zValue, timeArray, MString(opName.GetText()), "X", "Y", "Z", context);
        }
        return true;
    } 

    return false;
}

// Simple function that determines if the matrix is identity
// XXX Maybe there is something already in Gf but couldn't see it
static bool _isIdentityMatrix(GfMatrix4d m)
{
    bool isIdentity=true;
    for (unsigned int i=0; i<4; i++) {
        for (unsigned int j=0; j<4; j++) {
            if ((i==j && !GfIsClose(m[i][j], 1.0, 1e-9)) ||
                (i!=j && !GfIsClose(m[i][j], 0.0, 1e-9))) {
                isIdentity=false; break;
            }
        }
    }
    return isIdentity;
}

// For each xformop, we gather it's data either time sampled or not and we push it to the corresponding Maya xform
static bool _pushUSDXformToMayaXform(
        const UsdGeomXformable &xformSchema, 
        MFnDagNode &MdagNode,
        const PxrUsdMayaPrimReaderArgs& args,
        const PxrUsdMayaPrimReaderContext* context)
{
    std::vector<double> TxVal, TyVal, TzVal;
    std::vector<double> RxVal, RyVal, RzVal;
    std::vector<double> SxVal, SyVal, SzVal;
    GfVec3d xlate, rotate, scale; 
    bool resetsXformStack; 
    GfMatrix4d localXform(1.0);

    std::vector<double> tSamples;
    xformSchema.GetTimeSamplesInInterval(args.GetTimeInterval(), &tSamples);
    MTimeArray timeArray;
    if (!tSamples.empty()) {
        timeArray.setLength(tSamples.size());
        TxVal.resize(tSamples.size()); TyVal.resize(tSamples.size()); TzVal.resize(tSamples.size());
        RxVal.resize(tSamples.size()); RyVal.resize(tSamples.size()); RzVal.resize(tSamples.size());
        SxVal.resize(tSamples.size()); SyVal.resize(tSamples.size()); SzVal.resize(tSamples.size());
        for (unsigned int ti=0; ti < tSamples.size(); ++ti) {
            UsdTimeCode time(tSamples[ti]);
            if (xformSchema.GetLocalTransformation(&localXform, 
                                                   &resetsXformStack,
                                                   time)) {
                xlate=GfVec3d(0); rotate=GfVec3d(0); scale=GfVec3d(1);
                if (!_isIdentityMatrix(localXform)) {
                     PxrUsdMayaTranslatorXformable::ConvertUsdMatrixToComponents(
                             localXform, &xlate, &rotate, &scale);
                }
                TxVal[ti]=xlate [0]; TyVal[ti]=xlate [1]; TzVal[ti]=xlate [2];
                RxVal[ti]=rotate[0]; RyVal[ti]=rotate[1]; RzVal[ti]=rotate[2];
                SxVal[ti]=scale [0]; SyVal[ti]=scale [1]; SzVal[ti]=scale [2];
                timeArray.set(MTime(tSamples[ti]), ti);
            } 
            else {
                TF_RUNTIME_ERROR(
                        "Missing sampled xform data on USD prim <%s>",
                        xformSchema.GetPath().GetText());
            }
        }
    } 
    else {
        if (xformSchema.GetLocalTransformation(&localXform, &resetsXformStack)) {
            xlate=GfVec3d(0); rotate=GfVec3d(0); scale=GfVec3d(1);
            if (!_isIdentityMatrix(localXform)) {
                // XXX if we want to support the old pivotPosition, we can pass
                // it into this function..
                PxrUsdMayaTranslatorXformable::ConvertUsdMatrixToComponents(
                        localXform, &xlate, &rotate, &scale);
            }
            TxVal.resize(1); TyVal.resize(1); TzVal.resize(1);
            RxVal.resize(1); RyVal.resize(1); RzVal.resize(1);
            SxVal.resize(1); SyVal.resize(1); SzVal.resize(1);
            TxVal[0]=xlate [0]; TyVal[0]=xlate [1]; TzVal[0]=xlate [2];
            RxVal[0]=rotate[0]; RyVal[0]=rotate[1]; RzVal[0]=rotate[2];
            SxVal[0]=scale [0]; SyVal[0]=scale [1]; SzVal[0]=scale [2];
        } 
        else {
            TF_RUNTIME_ERROR(
                    "Missing default xform data on USD prim <%s>",
                    xformSchema.GetPath().GetText());
        }
    }

    // All of these vectors should have the same size and greater than 0 to set their values
    if (TxVal.size()==TyVal.size() && TxVal.size()==TzVal.size() && !TxVal.empty()) {
        _setMayaAttribute(MdagNode, TxVal, TyVal, TzVal, timeArray, MString("translate"), "X", "Y", "Z", context);
        _setMayaAttribute(MdagNode, RxVal, RyVal, RzVal, timeArray, MString("rotate"), "X", "Y", "Z", context);
        _setMayaAttribute(MdagNode, SxVal, SyVal, SzVal, timeArray, MString("scale"), "X", "Y", "Z", context);
        return true;
    } 

    return false;
}

void
PxrUsdMayaTranslatorXformable::Read(
        const UsdGeomXformable& xformSchema,
        MObject mayaNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    MStatus status;

    // == Read attrs ==
    // Read parent class attrs
    PxrUsdMayaTranslatorPrim::Read(xformSchema.GetPrim(), mayaNode, args, context);

    // Scanning Xformops to see if we have a general Maya xform or an xform
    // that conform to the commonAPI
    //
    // If fail to retrieve proper ops with proper name and order, will try to
    // decompose the xform matrix
    bool resetsXformStack= false;
    std::vector<UsdGeomXformOp> xformops = xformSchema.GetOrderedXformOps(
        &resetsXformStack);
            
    // When we find ops, we match the ops by suffix ("" will define the basic
    // translate, rotate, scale) and by order. If we find an op with a
    // different name or out of order that will miss the match, we will rely on
    // matrix decomposition

    PxrUsdMayaXformStack::OpClassList stackOps = \
            PxrUsdMayaXformStack::FirstMatchingSubstack(
                    {
                        &PxrUsdMayaXformStack::MayaStack(),
                        &PxrUsdMayaXformStack::CommonStack()
                    },
                    xformops);

    MFnDagNode MdagNode(mayaNode);
    if (!stackOps.empty()) {
        // make sure stackIndices.size() == xformops.size()
        for (unsigned int i=0; i < stackOps.size(); i++) {
            const UsdGeomXformOp& xformop(xformops[i]);
            const PxrUsdMayaXformOpClassification& opDef(stackOps[i]);
            // If we got a valid stack, we have both the members of the inverted twins..
            // ...so we can go ahead and skip the inverted twin
            if (opDef.IsInvertedTwin()) continue;

            const TfToken& opName(opDef.GetName());

            _pushUSDXformOpToMayaXform(xformop, opName, MdagNode, args, context);
        }
    } else {
        if (!_pushUSDXformToMayaXform(xformSchema, MdagNode, args, context)) {
            TF_RUNTIME_ERROR(
                    "Unable to successfully decompose matrix at USD prim <%s>",
                    xformSchema.GetPath().GetText());
        }
    }

    if (resetsXformStack) {
        MPlug plg = MdagNode.findPlug("inheritsTransform");
        if (!plg.isNull()) plg.setBool(false);
    }
}



PXR_NAMESPACE_CLOSE_SCOPE

