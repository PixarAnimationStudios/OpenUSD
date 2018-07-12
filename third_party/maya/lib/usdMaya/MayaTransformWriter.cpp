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
#include "usdMaya/MayaTransformWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/usdWriteJob.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/inherits.h"

#include <maya/MFnTransform.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(transform, MayaTransformWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(transform, UsdGeomXform);

template <typename GfVec3_T>
static void
_setXformOp(const UsdGeomXformOp &op, 
        const GfVec3_T& value, 
        const UsdTimeCode &usdTime,
        UsdUtilsSparseValueWriter *valueWriter)
{
    switch(op.GetOpType()) {
        case UsdGeomXformOp::TypeRotateX:
            valueWriter->SetAttribute(op.GetAttr(), VtValue(value[0]), usdTime);
        break;
        case UsdGeomXformOp::TypeRotateY:
            valueWriter->SetAttribute(op.GetAttr(), VtValue(value[1]), usdTime);
        break;
        case UsdGeomXformOp::TypeRotateZ:
            valueWriter->SetAttribute(op.GetAttr(), VtValue(value[2]), usdTime);
        break;
        default:
            valueWriter->SetAttribute(op.GetAttr(), VtValue(value), usdTime);
    }
}

// Given an Op, value and time, set the Op value based on op type and precision
static void 
setXformOp(const UsdGeomXformOp& op,
        const GfVec3d& value,
        const UsdTimeCode& usdTime,
        UsdUtilsSparseValueWriter *valueWriter)
{
    if (!op) {
        TF_CODING_ERROR("Xform op is not valid");
        return;
    }
    
    if (op.GetOpType() == UsdGeomXformOp::TypeTransform) {
        GfMatrix4d shearXForm(1.0);
        shearXForm[1][0] = value[0]; //xyVal
        shearXForm[2][0] = value[1]; //xzVal
        shearXForm[2][1] = value[2]; //yzVal            
        valueWriter->SetAttribute(op.GetAttr(), shearXForm, usdTime);
        return;
    }

    if (UsdGeomXformOp::GetPrecisionFromValueTypeName(op.GetAttr().GetTypeName()) 
            == UsdGeomXformOp::PrecisionDouble) {
        _setXformOp<GfVec3d>(op, value, usdTime, valueWriter);
    }
    else { // float precision
        _setXformOp<GfVec3f>(op, GfVec3f(value), usdTime, valueWriter);
    }
}

// For a given GeomXForm and array of AnimChannels and time, compute the data
// from if needed and set the XFormOps values
static void 
computeXFormOps(
        const UsdGeomXformable& usdXformable, 
        const std::vector<AnimChannel>& animChanList, 
        const UsdTimeCode &usdTime,
        UsdUtilsSparseValueWriter *valueWriter)
{
    // Iterate over each AnimChannel, retrieve the default value and pull the
    // Maya data if needed. Then store it on the USD Ops
    for (unsigned int channelIdx = 0; channelIdx < animChanList.size(); ++channelIdx) {

        const AnimChannel& animChannel = animChanList[channelIdx];
        if (animChannel.isInverse) {
            continue;
        }

        GfVec3d value = animChannel.defValue;
        bool hasAnimated = false, hasStatic = false;
        for (unsigned int i = 0; i<3; i++) {
            if (animChannel.sampleType[i] == ANIMATED) {
                // NOTE the default value has already been converted to
                // radians.
                double chanVal = animChannel.plug[i].asDouble();
                value[i] = animChannel.opType == ROTATE ? 
                    GfRadiansToDegrees(chanVal) :
                    chanVal;
                hasAnimated = true;
            } 
            else if (animChannel.sampleType[i] == STATIC) {
                hasStatic = true;
            }
        }
        
        // If the channel is not animated AND has non identity value, we are
        // computing default time, then set the values.
        //
        // If the channel is animated(connected) and we are not setting default
        // time, then set the values. 
        //
        // This to make sure static channels are setting their default while
        // animating ones are actually animating
        if ((usdTime == UsdTimeCode::Default() && hasStatic && !hasAnimated) ||
            (usdTime != UsdTimeCode::Default() && hasAnimated)) {
            setXformOp(animChannel.op, value, usdTime, valueWriter);
        }
    }
}

// Creates an AnimChannel from a Maya compound attribute if there is meaningful
// data.  This means we found data that is non-identity.
//
// returns true if we extracted an AnimChannel and false otherwise (e.g., the
// data was identity). 
static bool 
_GatherAnimChannel(
        XFormOpType opType, 
        const MFnTransform& iTrans, 
        MString parentName, 
        MString xName, MString yName, MString zName, 
        std::vector<AnimChannel>* oAnimChanList, 
        bool isWritingAnimation,
        bool setOpName)
{
    AnimChannel chan;
    chan.opType = opType;
    chan.isInverse = false;
    if (setOpName) {
        chan.opName = parentName.asChar();
    }

    // We default to single precision (later we set the main translate op and
    // shear to double)
    chan.precision = UsdGeomXformOp::PrecisionFloat;
    
    unsigned int validComponents = 0;
    
    // this is to handle the case where there is a connection to the parent
    // plug but not to the child plugs, if the connection is there and you are
    // not forcing static, then all of the children are considered animated
    int parentSample = PxrUsdMayaUtil::getSampledType(iTrans.findPlug(parentName),false);
    
    // Determine what plug are needed based on default value & being
    // connected/animated
    MStringArray channels;
    channels.append(parentName+xName);
    channels.append(parentName+yName);
    channels.append(parentName+zName);

    GfVec3d nullValue(opType == SCALE ? 1.0 : 0.0);
    for (unsigned int i = 0; i<3; i++) {
        // Find the plug and retrieve the data as the channel default value. It
        // won't be updated if the channel is NOT ANIMATED
        chan.plug[i] = iTrans.findPlug(channels[i]);
        double plugValue = chan.plug[i].asDouble();
        chan.defValue[i] = opType == ROTATE ? GfRadiansToDegrees(plugValue) : plugValue;
        chan.sampleType[i] = NO_XFORM;
        // If we allow animation and either the parentsample or local sample is
        // not 0 then we havea ANIMATED sample else we have a scale and the
        // value is NOT 1 or if the value is NOT 0 then we have a static xform
        if ((parentSample != 0 || PxrUsdMayaUtil::getSampledType(chan.plug[i], true) != 0) && 
             isWritingAnimation) {
            chan.sampleType[i] = ANIMATED; 
            validComponents++;
        } 
        else if (!GfIsClose(chan.defValue[i], nullValue[i], 1e-7)) {
            chan.sampleType[i] = STATIC; 
            validComponents++;
        }
    }

    // If there are valid component, then we will add the animation channel.
    // Rotates with 1 component will be optimized to single axis rotation
    if (validComponents>0) {
        if (opType == SCALE) {
            chan.usdOpType = UsdGeomXformOp::TypeScale;
        } else if (opType == TRANSLATE) {
            chan.usdOpType = UsdGeomXformOp::TypeTranslate;
            // The main translate is set to double precision
            if (parentName == "translate") {
                chan.precision = UsdGeomXformOp::PrecisionDouble;
            }
        } else if (opType == ROTATE) {
            chan.usdOpType = UsdGeomXformOp::TypeRotateXYZ;
            if (validComponents == 1) {
                if (chan.sampleType[0] != NO_XFORM) chan.usdOpType = UsdGeomXformOp::TypeRotateX;
                if (chan.sampleType[1] != NO_XFORM) chan.usdOpType = UsdGeomXformOp::TypeRotateY;
                if (chan.sampleType[2] != NO_XFORM) chan.usdOpType = UsdGeomXformOp::TypeRotateZ;
            } 
            else {
                // Rotation Order ONLY applies to the "rotate" attribute
                if (parentName == "rotate") {
                    switch (iTrans.rotationOrder()) {
                        case MTransformationMatrix::kYZX:
                            chan.usdOpType = UsdGeomXformOp::TypeRotateYZX;
                        break;
                        case MTransformationMatrix::kZXY:
                            chan.usdOpType = UsdGeomXformOp::TypeRotateZXY;
                        break;
                        case MTransformationMatrix::kXZY:
                            chan.usdOpType = UsdGeomXformOp::TypeRotateXZY;
                        break;
                        case MTransformationMatrix::kYXZ:
                            chan.usdOpType = UsdGeomXformOp::TypeRotateYXZ;
                        break;
                        case MTransformationMatrix::kZYX:
                            chan.usdOpType = UsdGeomXformOp::TypeRotateZYX;
                        break;
                        default: break;
                    }
                }
            }
        } 
        else if (opType == SHEAR) {
            chan.usdOpType = UsdGeomXformOp::TypeTransform;
            chan.precision = UsdGeomXformOp::PrecisionDouble;
        } 
        else {
            return false;
        }
        oAnimChanList->push_back(chan);
        return true;
    }
    return false;
}

void MayaTransformWriter::_PushTransformStack(
        const MFnTransform& iTrans, 
        const UsdGeomXformable& usdXformable, 
        bool writeAnim)
{   
    // NOTE: I think this logic and the logic in MayaTransformReader
    // should be merged so the concept of "CommonAPI" stays centralized.
    //
    // By default we assume that the xform conforms to the common API
    // (xlate,pivot,rotate,scale,pivotINVERTED) As soon as we encounter any
    // additional xform (compensation translates for pivots, rotateAxis or
    // shear) we are not conforming anymore 
    bool conformsToCommonAPI = true;

    // Keep track of where we have rotate and scale Pivots and their inverse so
    // that we can combine them later if possible
    unsigned int rotPivotIdx = -1, rotPivotINVIdx = -1, scalePivotIdx = -1, scalePivotINVIdx = -1;
    
    // Check if the Maya prim inheritTransform
    MPlug inheritPlug = iTrans.findPlug("inheritsTransform");
    if (!inheritPlug.isNull()) {
        if(!inheritPlug.asBool()) {
            usdXformable.SetResetXformStack(true);
        }
    }
            
    // inspect the translate, no suffix to be closer compatibility with common API
    _GatherAnimChannel(TRANSLATE, iTrans, "translate", "X", "Y", "Z", &_animChannels, writeAnim, false);

    // inspect the rotate pivot translate
    if (_GatherAnimChannel(TRANSLATE, iTrans, "rotatePivotTranslate", "X", "Y", "Z", &_animChannels, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // inspect the rotate pivot
    bool hasRotatePivot = _GatherAnimChannel(TRANSLATE, iTrans, "rotatePivot", "X", "Y", "Z", &_animChannels, writeAnim, true);
    if (hasRotatePivot) {
        rotPivotIdx = _animChannels.size()-1;
    }

    // inspect the rotate, no suffix to be closer compatibility with common API
    _GatherAnimChannel(ROTATE, iTrans, "rotate", "X", "Y", "Z", &_animChannels, writeAnim, false);

    // inspect the rotateAxis/orientation
    if (_GatherAnimChannel(ROTATE, iTrans, "rotateAxis", "X", "Y", "Z", &_animChannels, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // invert the rotate pivot
    if (hasRotatePivot) {
        AnimChannel chan;
        chan.usdOpType = UsdGeomXformOp::TypeTranslate;
        chan.precision = UsdGeomXformOp::PrecisionFloat;
        chan.opName = "rotatePivot";
        chan.isInverse = true;
        _animChannels.push_back(chan);
        rotPivotINVIdx = _animChannels.size()-1;
    }

    // inspect the scale pivot translation
    if (_GatherAnimChannel(TRANSLATE, iTrans, "scalePivotTranslate", "X", "Y", "Z", &_animChannels, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // inspect the scale pivot point
    bool hasScalePivot = _GatherAnimChannel(TRANSLATE, iTrans, "scalePivot", "X", "Y", "Z", &_animChannels, writeAnim, true);
    if (hasScalePivot) {
        scalePivotIdx = _animChannels.size()-1;
    }

    // inspect the shear. Even if we have one xform on the xform list, it represents a share so we should name it
    if (_GatherAnimChannel(SHEAR, iTrans, "shear", "XY", "XZ", "YZ", &_animChannels, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // add the scale. no suffix to be closer compatibility with common API
    _GatherAnimChannel(SCALE, iTrans, "scale", "X", "Y", "Z", &_animChannels, writeAnim, false);

    // inverse the scale pivot point
    if (hasScalePivot) {
        AnimChannel chan;
        chan.usdOpType = UsdGeomXformOp::TypeTranslate;
        chan.precision = UsdGeomXformOp::PrecisionFloat;
        chan.opName = "scalePivot";
        chan.isInverse = true;
        _animChannels.push_back(chan);
        scalePivotINVIdx = _animChannels.size()-1;
    }
    
    // If still potential common API, check if the pivots are the same and NOT animated/connected
    if (hasRotatePivot != hasScalePivot) {
        conformsToCommonAPI = false;
    }

    if (conformsToCommonAPI && hasRotatePivot && hasScalePivot) {
        AnimChannel rotPivChan, scalePivChan;
        rotPivChan = _animChannels[rotPivotIdx];
        scalePivChan = _animChannels[scalePivotIdx];
        // If they have different sampleType or are ANIMATED, does not conformsToCommonAPI anymore
        for (unsigned int i = 0;i<3;i++) {
            if (rotPivChan.sampleType[i] != scalePivChan.sampleType[i] ||
                    rotPivChan.sampleType[i] == ANIMATED) {
                conformsToCommonAPI = false;
            }
        }

        // If The defaultValue is not the same, does not conformsToCommonAPI anymore
        if (!GfIsClose(rotPivChan.defValue, scalePivChan.defValue, 1e-9)) {
            conformsToCommonAPI = false;
        }

        // If opType, usdType or precision are not the same, does not conformsToCommonAPI anymore
        if (rotPivChan.opType != scalePivChan.opType           || 
                rotPivChan.usdOpType != scalePivChan.usdOpType || 
                rotPivChan.precision != scalePivChan.precision) {
            conformsToCommonAPI = false;
        }

        if (conformsToCommonAPI) {
            // To Merge, we first rename rotatePivot and the scalePivot inverse
            // to pivot. Then we remove the scalePivot and the inverse of the
            // rotatePivot.
            //
            // This means that pivot and its inverse will wrap rotate and scale
            // since no other ops have been found
            //
            // NOTE: scalePivotIdx > rotPivotINVIdx
            _animChannels[rotPivotIdx].opName = "pivot";
            _animChannels[scalePivotINVIdx].opName = "pivot";
            _animChannels.erase(_animChannels.begin()+scalePivotIdx);
            _animChannels.erase(_animChannels.begin()+rotPivotINVIdx);
        }
    }
    
    // Loop over anim channel vector and create corresponding XFormOps
    // including the inverse ones if needed
    TF_FOR_ALL(iter, _animChannels) {
        AnimChannel& animChan = *iter;
        animChan.op = usdXformable.AddXformOp(
            animChan.usdOpType, animChan.precision,
            TfToken(animChan.opName),
            animChan.isInverse);
        if (!animChan.op) {
            TF_CODING_ERROR("Could not add xform op");
            animChan.op = UsdGeomXformOp();
        }
    }
}

MayaTransformWriter::MayaTransformWriter(
        const MDagPath& iDag,
        const SdfPath& uPath,
        usdWriteJobCtx& jobCtx)
        : MayaPrimWriter(iDag, uPath, jobCtx)
{
    // Even though we define an Xform here, it's OK for subclassers to
    // re-define the prim as another type.
    UsdGeomXform primSchema = UsdGeomXform::Define(GetUsdStage(), GetUsdPath());
    _usdPrim = primSchema.GetPrim();
    TF_VERIFY(_usdPrim);

    // There are special cases where you might subclass MayaTransformWriter
    // without actually having a transform (e.g. the internal
    // FunctorPrimWriter), so accomodate those here.
    if (iDag.hasFn(MFn::kTransform)) {
        MFnTransform transFn(iDag);
        // Create a vector of AnimChannels based on the Maya transformation
        // ordering
        _PushTransformStack(transFn, primSchema,
                !_GetExportArgs().timeInterval.IsEmpty());
    }
}

void MayaTransformWriter::Write(const UsdTimeCode& usdTime)
{
    MayaPrimWriter::Write(usdTime);

    // There are special cases where you might subclass MayaTransformWriter
    // without actually having a transform (e.g. the internal
    // FunctorPrimWriter), so accomodate those here.
    if (GetDagPath().hasFn(MFn::kTransform)) {
        if (UsdGeomXformable xformSchema = UsdGeomXformable(_usdPrim)) {
            computeXFormOps(xformSchema, _animChannels, usdTime,
                    _GetSparseValueWriter());
        }
        else {
            TF_CODING_ERROR("'%s' has a transform, but <%s> is not Xformable",
                    GetDagPath().fullPathName().asChar(),
                    _usdPrim.GetPath().GetText());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

