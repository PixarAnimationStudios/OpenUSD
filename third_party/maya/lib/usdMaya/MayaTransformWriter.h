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
#ifndef _usdExport_MayaTransformWriter_h_
#define _usdExport_MayaTransformWriter_h_

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/MayaPrimWriter.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformOp.h"
#include <maya/MFnTransform.h>
#include <maya/MPlugArray.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomXformable;
class UsdTimeCode;

enum XFormOpType { TRANSLATE, ROTATE, SCALE, SHEAR };
enum AnimChannelSampleType { NO_XFORM, STATIC, ANIMATED };

// This may not be the best name here as it isn't necessarily animated.
struct AnimChannel
{
    MPlug plug[3];
    AnimChannelSampleType sampleType[3];
    // defValue should always be in "usd" space.  that is, if it's a rotation
    // it should be a degree not radians.
    GfVec3d defValue; 
    XFormOpType opType;
    UsdGeomXformOp::Type usdOpType;
    UsdGeomXformOp::Precision precision;
    std::string opName;
    bool isInverse;
};

// Writes an MFnTransform
class MayaTransformWriter : public MayaPrimWriter
{
public:

    PXRUSDMAYA_API
    MayaTransformWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx);
    virtual ~MayaTransformWriter() {};

    PXRUSDMAYA_API
    virtual void pushTransformStack(
            const MFnTransform& iTrans, 
            const UsdGeomXformable& usdXForm, 
            bool writeAnim);
    
    PXRUSDMAYA_API
    virtual void write(const UsdTimeCode &usdTime);

    virtual bool isShapeAnimated()     const { return mIsShapeAnimated; };

    const MDagPath& getTransformDagPath() { return mXformDagPath; };

protected:
    PXRUSDMAYA_API
    bool writeTransformAttrs(
            const UsdTimeCode& usdTime, 
            UsdGeomXformable& primSchema);

private:
    bool mWriteTransformAttrs;
    MDagPath mXformDagPath;
    bool mIsShapeAnimated;
    std::vector<AnimChannel> mAnimChanList;
    bool mIsInstanceSource;

    size_t mJointOrientOpIndex[3];
    size_t mRotateOpIndex[3];
    size_t mRotateAxisOpIndex[3];

};

typedef std::shared_ptr<MayaTransformWriter> MayaTransformWriterPtr;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // _usdExport_MayaTransformWriter_h_
