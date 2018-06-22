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
    UsdGeomXformOp op;
};

/// Writes transforms and serves as the base class for most shape writers.
/// Handles instancing as well as merging transforms and shapes during export.
class MayaTransformWriter : public MayaPrimWriter
{
public:
    PXRUSDMAYA_API
    MayaTransformWriter(
            const MDagPath& iDag,
            const SdfPath& uPath,
            bool instanceSource,
            usdWriteJobCtx& jobCtx);
    
    PXRUSDMAYA_API
    void Write(const UsdTimeCode &usdTime) override;

    PXRUSDMAYA_API
    bool ExportsGprims() const override;
    
    PXRUSDMAYA_API
    bool ExportsReferences() const override;

    /// Gets the DAG path of the source transform node for this prim writer
    /// if we are merging transforms and shapes. (In that case, the Maya nodes
    /// given by GetDagPath() and GetTransformDagPath() are merged into one
    /// USD prim.) If not merging transforms and shapes, the returned DAG path
    /// is invalid.
    PXRUSDMAYA_API
    const MDagPath& GetTransformDagPath();

protected:
    /// Helper for determining whether the shape is animated, if this prim
    /// writer's source is a shape node.
    /// Always \c false if this prim writer's source is a transform.
    /// The default implementation only considers nodes with animation curve
    /// inputs as "animated". Prim writers for nodes that have time variation
    /// but no anim curves (such as particles) may want to override this.
    PXRUSDMAYA_API
    bool _IsShapeAnimated() const override;

    /// Writes the attributes that are common to all UsdGeomXformable prims,
    /// such as xformOps and xformOpOrder.
    /// Subclasses should almost always invoke _WriteXformableAttrs somewhere
    /// in their Write() function.
    PXRUSDMAYA_API
    bool _WriteXformableAttrs(
            const UsdTimeCode& usdTime, 
            UsdGeomXformable& primSchema);

private:
    /// Whether this is an instance for the purposes of USD export.
    /// May exclude some "true" Maya instances that aren't treated as such
    /// during export.
    /// Does not include instance sources.
    bool _IsInstance() const;

    /// Populates the AnimChannel vector with various ops based on
    /// the Maya
    /// transformation logic. If scale and/or rotate pivot are declared, creates
    /// inverse ops in the appropriate order.
    void _PushTransformStack(
            const MFnTransform& iTrans, 
            const UsdGeomXformable& usdXForm, 
            bool writeAnim);

    MDagPath _xformDagPath;
    std::vector<AnimChannel> _animChannels;
    bool _isShapeAnimated;
    bool _isInstanceSource;
};

typedef std::shared_ptr<MayaTransformWriter> MayaTransformWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // _usdExport_MayaTransformWriter_h_
