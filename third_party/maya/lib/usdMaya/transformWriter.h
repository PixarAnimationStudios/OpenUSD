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
#ifndef PXRUSDMAYA_TRANSFORM_WRITER_H
#define PXRUSDMAYA_TRANSFORM_WRITER_H

/// \file usdMaya/transformWriter.h

#include "usdMaya/api.h"
#include "usdMaya/primWriter.h"

#include "pxr/pxr.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformOp.h"

#include <maya/MEulerRotation.h>
#include <maya/MFnTransform.h>
#include <maya/MPlugArray.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomXformable;
class UsdTimeCode;

/// Writes transforms and serves as the base class for custom transform writers.
/// Handles the conversion of Maya transformation data into USD xformOps.
class UsdMayaTransformWriter : public UsdMayaPrimWriter
{
public:
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
    using _TokenRotationMap = std::unordered_map<
            const TfToken, MEulerRotation, TfToken::HashFunctor>;

    enum class _XformType { Translate, Rotate, Scale, Shear };
    enum class _SampleType { None, Static, Animated };

    // This may not be the best name here as it isn't necessarily animated.
    struct _AnimChannel
    {
        MPlug plug[3];
        _SampleType sampleType[3];
        // defValue should always be in "maya" space.  that is, if it's a
        // rotation it should be radians, not degrees. (This is done so we only
        // need to do conversion in one place, and so that, if we need to do
        // euler filtering, we don't do conversions, and then undo them to use
        // MEulerRotation).
        GfVec3d defValue; 
        _XformType opType;
        UsdGeomXformOp::Type usdOpType;
        UsdGeomXformOp::Precision precision;
        TfToken opName;
        bool isInverse;
        UsdGeomXformOp op;
    };

    // For a given array of _AnimChannels and time, compute the xformOp data if
    // needed and set the xformOps' values.
    static void _ComputeXformOps(
            const std::vector<_AnimChannel>& animChanList, 
            const UsdTimeCode &usdTime,
            bool eulerFilter,
            UsdMayaTransformWriter::_TokenRotationMap* previousRotates,
            UsdUtilsSparseValueWriter *valueWriter);

    // Creates an _AnimChannel from a Maya compound attribute if there is
    // meaningful data. This means we found data that is non-identity.
    // Returns true if we extracted an _AnimChannel and false otherwise (e.g.
    // the data was identity).
    static bool _GatherAnimChannel(
            _XformType opType,
            const MFnTransform& iTrans,
            const TfToken& parentName,
            const MString& xName, const MString& yName, const MString& zName,
            std::vector<_AnimChannel>* oAnimChanList, 
            bool isWritingAnimation,
            bool setOpName);

    /// Populates the AnimChannel vector with various ops based on
    /// the Maya transformation logic. If scale and/or rotate pivot are
    /// declared, creates inverse ops in the appropriate order.
    void _PushTransformStack(
            const MFnTransform& iTrans, 
            const UsdGeomXformable& usdXForm, 
            bool writeAnim);

    std::vector<_AnimChannel> _animChannels;
    _TokenRotationMap _previousRotates;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
