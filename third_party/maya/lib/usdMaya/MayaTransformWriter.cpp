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


template <typename GfVec3_T>
static void
_setXformOp(UsdGeomXformOp &op, 
        const GfVec3_T& value, 
        const UsdTimeCode &usdTime)
{
    switch(op.GetOpType()) {
        case UsdGeomXformOp::TypeRotateX:
            op.Set(value[0], usdTime);
        break;
        case UsdGeomXformOp::TypeRotateY:
            op.Set(value[1], usdTime);
        break;
        case UsdGeomXformOp::TypeRotateZ:
            op.Set(value[2], usdTime);
        break;
        default:
            op.Set(value, usdTime);
    }
}

// Given an Op, value and time, set the Op value based on op type and precision
static void 
setXformOp(UsdGeomXformOp& op, const GfVec3d& value, const UsdTimeCode& usdTime)
{
    if (op.GetOpType() == UsdGeomXformOp::TypeTransform) {
        GfMatrix4d shearXForm(1.0);
        shearXForm[1][0] = value[0]; //xyVal
        shearXForm[2][0] = value[1]; //xzVal
        shearXForm[2][1] = value[2]; //yzVal            
        op.Set(shearXForm, usdTime);
        return;
    }

    if (UsdGeomXformOp::GetPrecisionFromValueTypeName(op.GetAttr().GetTypeName()) 
            == UsdGeomXformOp::PrecisionDouble) {
        _setXformOp<GfVec3d>(op, value, usdTime);
    }
    else { // float precision
        _setXformOp<GfVec3f>(op, GfVec3f(value), usdTime);
    }
}

// For a given GeomXForm and array of AnimChannels and time, compute the data
// from if needed and set the XFormOps values
static void 
computeXFormOps(
        const UsdGeomXformable& usdXformable, 
        const std::vector<AnimChannel>& animChanList, 
        const UsdTimeCode &usdTime)
{
    bool resetsXformStack=false;
    std::vector<UsdGeomXformOp> xformops = usdXformable.GetOrderedXformOps(
        &resetsXformStack);
    if (xformops.size() != animChanList.size()) {
        MString errorMsg(TfStringPrintf(
            "ERROR in MayaTransformWriter::computeXFormOps "
            "for scope:%s USDOptSize:%lu ChannelListSize:%lu SKIPPING...", 
            usdXformable.GetPath().GetText(), xformops.size(), animChanList.size()).c_str());
        MGlobal::displayError(errorMsg);
        return;
    }

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
            setXformOp(xformops[channelIdx], value, usdTime);
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

// Populate the AnimationChannel vector with various ops based on the Maya
// transformation logic If scale and/or rotate pivot are declared, create
// inverse ops in the appropriate order
void MayaTransformWriter::pushTransformStack(
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
    _GatherAnimChannel(TRANSLATE, iTrans, "translate", "X", "Y", "Z", &mAnimChanList, writeAnim, false);

    // inspect the rotate pivot translate
    if (_GatherAnimChannel(TRANSLATE, iTrans, "rotatePivotTranslate", "X", "Y", "Z", &mAnimChanList, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // inspect the rotate pivot
    bool hasRotatePivot = _GatherAnimChannel(TRANSLATE, iTrans, "rotatePivot", "X", "Y", "Z", &mAnimChanList, writeAnim, true);
    if (hasRotatePivot) {
        rotPivotIdx = mAnimChanList.size()-1;
    }

    // inspect the rotate, no suffix to be closer compatibility with common API
    _GatherAnimChannel(ROTATE, iTrans, "rotate", "X", "Y", "Z", &mAnimChanList, writeAnim, false);

    // inspect the rotateAxis/orientation
    if (_GatherAnimChannel(ROTATE, iTrans, "rotateAxis", "X", "Y", "Z", &mAnimChanList, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // invert the rotate pivot
    if (hasRotatePivot) {
        AnimChannel chan;
        chan.usdOpType = UsdGeomXformOp::TypeTranslate;
        chan.precision = UsdGeomXformOp::PrecisionFloat;
        chan.opName = "rotatePivot";
        chan.isInverse = true;
        mAnimChanList.push_back(chan);
        rotPivotINVIdx = mAnimChanList.size()-1;
    }

    // inspect the scale pivot translation
    if (_GatherAnimChannel(TRANSLATE, iTrans, "scalePivotTranslate", "X", "Y", "Z", &mAnimChanList, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // inspect the scale pivot point
    bool hasScalePivot = _GatherAnimChannel(TRANSLATE, iTrans, "scalePivot", "X", "Y", "Z", &mAnimChanList, writeAnim, true);
    if (hasScalePivot) {
        scalePivotIdx = mAnimChanList.size()-1;
    }

    // inspect the shear. Even if we have one xform on the xform list, it represents a share so we should name it
    if (_GatherAnimChannel(SHEAR, iTrans, "shear", "XY", "XZ", "YZ", &mAnimChanList, writeAnim, true)) {
        conformsToCommonAPI = false;
    }

    // add the scale. no suffix to be closer compatibility with common API
    _GatherAnimChannel(SCALE, iTrans, "scale", "X", "Y", "Z", &mAnimChanList, writeAnim, false);

    // inverse the scale pivot point
    if (hasScalePivot) {
        AnimChannel chan;
        chan.usdOpType = UsdGeomXformOp::TypeTranslate;
        chan.precision = UsdGeomXformOp::PrecisionFloat;
        chan.opName = "scalePivot";
        chan.isInverse = true;
        mAnimChanList.push_back(chan);
        scalePivotINVIdx = mAnimChanList.size()-1;
    }
    
    // If still potential common API, check if the pivots are the same and NOT animated/connected
    if (hasRotatePivot != hasScalePivot) {
        conformsToCommonAPI = false;
    }

    if (conformsToCommonAPI && hasRotatePivot && hasScalePivot) {
        AnimChannel rotPivChan, scalePivChan;
        rotPivChan = mAnimChanList[rotPivotIdx];
        scalePivChan = mAnimChanList[scalePivotIdx];
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
            mAnimChanList[rotPivotIdx].opName = "pivot";
            mAnimChanList[scalePivotINVIdx].opName = "pivot";
            mAnimChanList.erase(mAnimChanList.begin()+scalePivotIdx);
            mAnimChanList.erase(mAnimChanList.begin()+rotPivotINVIdx);
        }
    }
    
    // Loop over anim channel vector and create corresponding XFormOps
    // including the inverse ones if needed
    TF_FOR_ALL(iter, mAnimChanList) {
        const AnimChannel& animChan = *iter;
        usdXformable.AddXformOp(
                animChan.usdOpType, animChan.precision,
                TfToken(animChan.opName),
                animChan.isInverse);
    }
}

MayaTransformWriter::MayaTransformWriter(
        const MDagPath& iDag,
        const SdfPath& uPath,
        bool instanceSource,
        usdWriteJobCtx& jobCtx) :
    MayaPrimWriter(iDag, uPath, jobCtx),
    mXformDagPath(iDag),
    mIsShapeAnimated(false),
    mIsInstanceSource(instanceSource)
{
    auto isInstance = false;
    auto hasTransform = iDag.hasFn(MFn::kTransform);
    auto hasOnlyOneShapeBelow = [&jobCtx] (const MDagPath& path) {
        auto numberOfShapesDirectlyBelow = 0u;
        path.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
        if (numberOfShapesDirectlyBelow != 1) {
            return false;
        }
        const auto childCount = path.childCount();
        if (childCount == 1) {
            return true;
        }
        // Make sure that the other objects are exportable - ie, still want
        // to collapse if it has two shapes below, but one of them is an
        // intermediateObject shape
        MDagPath childDag(path);
        auto numExportableChildren = 0u;
        for (auto i = 0u; i < childCount; ++i) {
            childDag.push(path.child(i));
            if (jobCtx.needToTraverse(childDag)) {
                ++numExportableChildren;
                if (numExportableChildren > 1) {
                    return false;
                }
            }
            childDag.pop();
        }
        return (numExportableChildren == 1);
    };

    auto setup_merged_shape = [this, &isInstance, &iDag, &hasTransform, &hasOnlyOneShapeBelow] () {
        // Use the parent transform if there is only a single shape under the shape's xform
        this->mXformDagPath.pop();
        if (hasOnlyOneShapeBelow(mXformDagPath)) {
            // Use the parent path (xform) instead of the shape path
            this->setUsdPath( getUsdPath().GetParentPath() );
            hasTransform = true;
        } else if (isInstance) {
            this->mXformDagPath = iDag;
        } else {
            this->mXformDagPath = MDagPath(); // make path invalid
        }
    };

    auto invalidate_transform = [this] () {
        this->setValid(false); // no need to iterate over this Writer, as not writing it out
        this->mXformDagPath = MDagPath(); // make path invalid
    };

    // it's more straightforward to separate code
    if (mIsInstanceSource) {
        if (!hasTransform) {
            mXformDagPath = MDagPath();
        }
    } else if (getArgs().exportInstances) {
        if (hasTransform) {
            if (hasOnlyOneShapeBelow(iDag)) {
                auto copyDag = iDag;
                copyDag.extendToShapeDirectlyBelow(0);
                if (copyDag.isInstanced()) {
                    invalidate_transform();
                } else if (getArgs().mergeTransformAndShape) {
                    invalidate_transform();
                }
            }
        } else {
            if (iDag.isInstanced()) {
                isInstance = true;
                setup_merged_shape();
            } else if (getArgs().mergeTransformAndShape) {
                setup_merged_shape();
            } else {
                mXformDagPath = MDagPath();
            }
        }
    } else {
        // Merge shape and transform
        // If is a transform, then do not write
        // Return if has a single shape directly below xform
        if (getArgs().mergeTransformAndShape) {
            if (hasTransform) { // if is an actual transform
                if (hasOnlyOneShapeBelow(iDag)) {
                    invalidate_transform();
                }
            } else { // must be a shape then
                setup_merged_shape();
            }
        } else {
            if (!hasTransform) { // if is NOT an actual transform
                mXformDagPath = MDagPath(); // make path invalid
            }
        }
    }

    // Determine if transform is animated
    if (mXformDagPath.isValid()) {
        UsdGeomXform primSchema = UsdGeomXform::Define(getUsdStage(), getUsdPath());
        mUsdPrim = primSchema.GetPrim();
        if (!mUsdPrim.IsValid()) {
            setValid(false);
            return;
        }
        if (!mIsInstanceSource) {
            if (hasTransform) {
                MFnTransform transFn(mXformDagPath);
                // Create a vector of AnimChannels based on the Maya transformation
                // ordering
                pushTransformStack(transFn, primSchema, getArgs().exportAnimation);
            }

            if (isInstance) {
                const auto masterPath = mWriteJobCtx.getMasterPath(getDagPath());
                if (!masterPath.IsEmpty()){
                    mUsdPrim.GetInherits().AppendInherit(masterPath);
                    mUsdPrim.SetInstanceable(true);
                }
            }
        }
    }

    // Determine if shape is animated
    // note that we can't use hasTransform, because we need to test the original
    // dag, not the transform (if mergeTransformAndShape is on)!
    if (!getDagPath().hasFn(MFn::kTransform)) { // if is a shape
        MObject obj = getDagPath().node();
        if (getArgs().exportAnimation) {
            mIsShapeAnimated = PxrUsdMayaUtil::isAnimated(obj);
        }
    }
}

//virtual 
void MayaTransformWriter::write(const UsdTimeCode &usdTime)
{
    if (!mIsInstanceSource) {
        UsdGeomXform primSchema(mUsdPrim);
        // Set attrs
        writeTransformAttrs(usdTime, primSchema);
    }
}

bool MayaTransformWriter::writeTransformAttrs(
        const UsdTimeCode &usdTime, UsdGeomXformable &xformSchema)
{
    // Write parent class attrs
    writePrimAttrs(mXformDagPath, usdTime, xformSchema); // for the shape

    // can this use xformSchema instead?  do we even need _usdXform?
    computeXFormOps(xformSchema, mAnimChanList, usdTime);
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

