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
#ifndef USDMAYA_TRANSFORM_WRITER_H
#define USDMAYA_TRANSFORM_WRITER_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primWriter.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformOp.h"
#include <maya/MEulerRotation.h>
#include <maya/MFnTransform.h>
#include <maya/MPlugArray.h>

#include <unordered_map>

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
    // defValue should always be in "maya" space.  that is, if it's a rotation
    // it should be radians, not degrees. (This is done so we only need to do
    // conversion in one place, and so that, if we need to do euler filtering,
    // we don't do conversions, and then undo them to use MEulerRotation).
    GfVec3d defValue; 
    XFormOpType opType;
    UsdGeomXformOp::Type usdOpType;
    UsdGeomXformOp::Precision precision;
    TfToken opName;
    bool isInverse;
    UsdGeomXformOp op;
};

/// Writes transforms and serves as the base class for most shape writers.
/// Handles instancing as well as merging transforms and shapes during export.
class UsdMayaTransformWriter : public UsdMayaPrimWriter
{
public:
    typedef std::unordered_map<const TfToken, MEulerRotation, TfToken::HashFunctor> TokenRotationMap;

    PXRUSDMAYA_API
    UsdMayaTransformWriter(
            const MDagPath& iDag,
            const SdfPath& uPath,
            UsdMayaWriteJobContext& jobCtx);

    /// Main export function that runs when the traversal hits the node.
    /// This extends UsdMayaPrimWriter::Write() by exporting xform ops for
    /// UsdGeomXformable if the Maya node has transform data.
    PXRUSDMAYA_API
    void Write(const UsdTimeCode &usdTime) override;

private:
    /// Populates the AnimChannel vector with various ops based on
    /// the Maya
    /// transformation logic. If scale and/or rotate pivot are declared, creates
    /// inverse ops in the appropriate order.
    void _PushTransformStack(
            const MFnTransform& iTrans, 
            const UsdGeomXformable& usdXForm, 
            bool writeAnim);

    std::vector<AnimChannel> _animChannels;
    TokenRotationMap _previousRotates;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
