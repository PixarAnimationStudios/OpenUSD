//
// Copyright 2017 Pixar
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
#include "pxrUsdTranslators/instancerWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"
#include "usdMaya/writeUtil.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/xformOp.h"

#include <maya/MAnimUtil.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MMatrix.h>

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(instancer, PxrUsdTranslators_InstancerWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(instancer, UsdGeomPointInstancer);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (Prototypes)
    (instancerTranslate)
);


namespace {

static constexpr double _EPSILON = 1e-3;

/// Determines if the second translate op encodes the exact negation of the
/// first op, across default values and time samples.
static
bool
_AreTranslateOpsOpposites(
        const UsdGeomXformOp& first,
        const UsdGeomXformOp& second)
{
    if (first.GetPrecision() != second.GetPrecision()) {
        return false;
    }

    // Are the time samples identical?
    std::vector<double> samples1;
    first.GetTimeSamples(&samples1);
    std::vector<double> samples2;
    second.GetTimeSamples(&samples2);
    if (samples1 != samples2) {
        return false;
    }

    // Also check the default-time value (if it exists).
    samples1.push_back(UsdTimeCode::Default().GetValue());
    samples2.push_back(UsdTimeCode::Default().GetValue());

    // Check through all time samples (plus the default value).
    for (const double& t : samples1) {
        GfVec3d v1(0.0);
        GfVec3d v2(0.0);
        if (first.GetAs(&v1, t) != second.GetAs(&v2, t)) {
            return false;
        }
        if (v1 != -v2) {
            return false;
        }
    }

    return true;
}

/// Edits the xform ops and xformOpOrder on xformable to remove subsequent pairs
/// of instancerTranslate and translate ops that cancel each other out.
static
void
_CleanUpRedundantTranslateOps(UsdGeomXformable& xformable)
{
    static const TfToken plainTranslateName = UsdGeomXformOp::GetOpName(
            UsdGeomXformOp::TypeTranslate);
    static const TfToken instancerTranslateName = UsdGeomXformOp::GetOpName(
            UsdGeomXformOp::TypeTranslate, _tokens->instancerTranslate);

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xformable.GetOrderedXformOps(
            &resetsXformStack);
    if (ops.size() < 2) {
        return;
    }

    // Iterate over ops in pairs, i.e., (0, 1), (1, 2), (2, 3), ...
    for (auto iter = ops.begin(); iter != ops.end() - 1; ++iter) {
        const UsdGeomXformOp& thisOp = *iter;
        const UsdGeomXformOp& nextOp = *(iter + 1);
        if (thisOp.GetOpName() == instancerTranslateName &&
                nextOp.GetOpName() == plainTranslateName) {
            if (_AreTranslateOpsOpposites(thisOp, nextOp)) {
                UsdPrim prim = xformable.GetPrim();

                // Remove scene description for xform ops.
                prim.RemoveProperty(thisOp.GetName());
                prim.RemoveProperty(nextOp.GetName());

                // If the instancerTranslate and translate were the only
                // xformOps in the xformOpOrder, then remove the xformOpOrder
                // scene description completely.
                // Otherwise, just edit the xformOpOrder to remove these ops.
                if (ops.size() == 2 && !resetsXformStack) {
                    prim.RemoveProperty(
                            xformable.GetXformOpOrderAttr().GetName());
                }
                else {
                    ops.erase(iter, iter + 2);
                    xformable.SetXformOpOrder(ops, resetsXformStack);
                }
            }
            return; // There should only be one instancerTranslate op.
        }
    }
}

/// Gets the transformed position of (0, 0, 0) using the transform's
/// local transformation matrix.
static
bool
_GetTransformedOriginInLocalSpace(
        const MDagPath& transformDagPath,
        GfVec3d* transformedOrigin)
{
    if (transformDagPath.hasFn(MFn::kTransform)) {
        MFnTransform fnXform(transformDagPath);
        MMatrix xformMat = fnXform.transformationMatrix();
        MPoint origin = MPoint::origin * xformMat;
        *transformedOrigin = GfVec3d(origin.x, origin.y, origin.z);
        return true;
    }
    return false;
}

} // anonymous namespace

PxrUsdTranslators_InstancerWriter::PxrUsdTranslators_InstancerWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaTransformWriter(depNodeFn, usdPath, jobCtx),
    _numPrototypes(0)
{
    if (!TF_VERIFY(GetDagPath().isValid())) {
        return;
    }

    UsdGeomPointInstancer primSchema =
        UsdGeomPointInstancer::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            primSchema,
            "Could not define UsdGeomPointInstancer at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }
    _usdPrim = primSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdGeomPointInstancer at path '%s'\n",
            primSchema.GetPath().GetText())) {
        return;
    }

    // Note that the instancer is a model -- it's an assembly by default, though
    // the model kind writer is allowed to "fix" this up.
    UsdModelAPI(_usdPrim).SetKind(KindTokens->assembly);
    _modelPaths.push_back(_usdPrim.GetPath());
}

/* virtual */
void
PxrUsdTranslators_InstancerWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaTransformWriter::Write(usdTime);

    UsdGeomPointInstancer primSchema(_usdPrim);
    writeInstancerAttrs(usdTime, primSchema);
}

/// Returns STATIC or ANIMATED if an extra translate is needed to compensate for
/// Maya's instancer translation behavior on the given prototype DAG node.
/// (This function may return false positives, which are OK but will simply
/// contribute extra data. It should never return false negatives, which
/// would cause correctness problems.)
bool
PxrUsdTranslators_InstancerWriter::_NeedsExtraInstancerTranslate(
        const MDagPath& prototypeDagPath,
        bool* instancerTranslateAnimated) const
{
    // XXX: Maybe we could be smarter here and figure out if the animation
    // affects instancerTranslate?
    bool animated = !_GetExportArgs().timeSamples.empty() &&
            MAnimUtil::isAnimated(prototypeDagPath.node(), false);
    if (animated) {
        *instancerTranslateAnimated = true;
        return true;
    }

    GfVec3d origin;
    bool translated =
            _GetTransformedOriginInLocalSpace(prototypeDagPath, &origin) &&
            !GfIsClose(origin, GfVec3d(0.0), _EPSILON);
    if (translated) {
        *instancerTranslateAnimated = false;
        return true;
    }

    return false;
}

bool
PxrUsdTranslators_InstancerWriter::writeInstancerAttrs(
        const UsdTimeCode& usdTime,
        const UsdGeomPointInstancer& instancer)
{
    MStatus status = MS::kSuccess;
    MFnDagNode dagNode(GetDagPath(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Note: In this function, we don't read instances using the provided
    // MFnInstancer API. One reason is that it breaks up prototypes into their
    // constituent shapes, and there's no way to figure out which hierarchy
    // they came from. Another reason is that it only provides computed matrices
    // and not separate position, rotation, scale attrs.

    const SdfPath prototypesGroupPath =
            instancer.GetPrim().GetPath().AppendChild(_tokens->Prototypes);

    // At the default time, setup all the prototype instances.
    if (usdTime.IsDefault()) {
        const MPlug inputHierarchy = dagNode.findPlug("inputHierarchy", true,
                &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        // Note that the "Prototypes" prim needs to be a model group to ensure
        // contiguous model hierarchy.
        const UsdPrim prototypesGroupPrim = GetUsdStage()->DefinePrim(
                prototypesGroupPath);
        UsdModelAPI(prototypesGroupPrim).SetKind(KindTokens->group);
        _modelPaths.push_back(prototypesGroupPath);

        UsdRelationship prototypesRel = instancer.CreatePrototypesRel();

        const unsigned int numElements = inputHierarchy.numElements();
        for (unsigned int i = 0; i < numElements; ++i) {
            const MPlug plug = inputHierarchy[i];
            const MPlug source(UsdMayaUtil::GetConnected(plug));
            if (source.isNull()) {
                TF_WARN("Cannot read prototype: the source plug %s was null",
                        plug.name().asChar());
                return false;
            }

            MFnDagNode sourceNode(source.node(), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);

            MDagPath prototypeDagPath;
            sourceNode.getPath(prototypeDagPath);

            // Prototype names are guaranteed unique by virtue of having a
            // unique numerical suffix _# indicating the prototype index.
            const TfToken prototypeName(
                    TfStringPrintf("%s_%d", sourceNode.name().asChar(), i));
            const SdfPath prototypeUsdPath = prototypesGroupPrim.GetPath()
                    .AppendChild(prototypeName);
            UsdPrim prototypePrim = GetUsdStage()->DefinePrim(
                    prototypeUsdPath);
            _modelPaths.push_back(prototypeUsdPath);

            // Try to be conservative and only create an intermediary xformOp
            // with the instancerTranslate if we can ensure that we don't need
            // to compensate for the translation on the prototype root.
            //
            // XXX: instancerTranslate does not behave well when added to a
            // reference that has an existing transform on the far side of the
            // reference. However, its behavior at least matches the
            // behavior in UsdMayaTranslatorModelAssembly. If we fix the
            // behavior there, we need to make sure that this is also
            // fixed to match.
            bool instancerTranslateAnimated = false;
            if (_NeedsExtraInstancerTranslate(
                    prototypeDagPath, &instancerTranslateAnimated)) {
                UsdGeomXformable xformable(prototypePrim);
                UsdGeomXformOp newOp = xformable.AddTranslateOp(
                        UsdGeomXformOp::PrecisionDouble,
                        _tokens->instancerTranslate);
                _instancerTranslateOps.push_back(
                        {prototypeDagPath, newOp, instancerTranslateAnimated});
            }

            // Two notes:
            // (1) We don't un-instance here, because it's OK for the prototype
            // to just be a reference to an instance master if the prototype
            // participates in Maya native instancing.
            // (2) The prototype root must be visible to match Maya's behavior,
            // which always vis'es the prototype root, even if it is marked
            // hidden.
            _writeJobCtx.CreatePrimWriterHierarchy(
                    prototypeDagPath,
                    prototypeUsdPath,
                    /*forceUninstance*/ false,
                    /*exportRootVisibility*/ false,
                    &_prototypeWriters);
            prototypesRel.AddTarget(prototypeUsdPath);
        }

        _numPrototypes = numElements;
    }

    // If there aren't any prototypes, fail and don't export on subsequent
    // time-sampled exports.
    if (_numPrototypes == 0) {
        return false;
    }

    // Actual write of prototypes (@ both default time and animated time).
    for (UsdMayaPrimWriterSharedPtr& writer : _prototypeWriters) {
        writer->Write(usdTime);

        if (usdTime.IsDefault()) {
            // Prototype roots should have kind component or derived.
            // Calling Write() above may have populated kinds, so don't stomp
            // over existing component-derived kinds.
            // (Note that ModelKindWriter's fix-up stage might change this.)
            if (writer->GetUsdPath().GetParentPath() == prototypesGroupPath) {
                if (const UsdPrim writerPrim = writer->GetUsdPrim()) {
                    UsdModelAPI primModelAPI(writerPrim);
                    TfToken kind;
                    primModelAPI.GetKind(&kind);
                    if (!KindRegistry::IsA(kind, KindTokens->component)) {
                        primModelAPI.SetKind(KindTokens->component);
                    }
                }
            }
        }
    }

    // Write the instancerTranslate xformOp for all prims that need it.
    // (This should happen @ default time or animated time depending on whether
    // the xform is animated.)
    for (const _TranslateOpData& opData : _instancerTranslateOps) {
        if (opData.isAnimated != usdTime.IsDefault()) {
            GfVec3d origin;
            if (_GetTransformedOriginInLocalSpace(opData.mayaPath, &origin)) {
                UsdGeomXformOp translateOp = opData.op;
                _SetAttribute(translateOp.GetAttr(), -origin, usdTime);
            }
        }
    }

    // Grab the inputPoints data from the source plug.
    // (This attribute's value must come from a source plug; it isn't
    // directly writeable. Thus reading it directly may not give the right
    // value depending on Maya's execution behavior.)
    MPlug inputPointsDest = dagNode.findPlug("inputPoints", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug inputPointsSrc = UsdMayaUtil::GetConnected(inputPointsDest);
    if (inputPointsSrc.isNull()) {
        TF_WARN("inputPoints not connected on instancer '%s'",
                GetDagPath().fullPathName().asChar());
        return false;
    }

    auto holder = UsdMayaUtil::GetPlugDataHandle(inputPointsSrc);
    if (!holder) {
        TF_WARN("Unable to read inputPoints data handle for instancer '%s'",
                GetDagPath().fullPathName().asChar());
        return false;
    }

    MFnArrayAttrsData inputPointsData(holder->GetDataHandle().data(),
            &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (!UsdMayaWriteUtil::WriteArrayAttrsToInstancer(
            inputPointsData, instancer, _numPrototypes, usdTime,
            _GetSparseValueWriter())) {
        return false;
    }

    // Load the completed point instancer to compute and set its extent.
    instancer.GetPrim().GetStage()->Load(instancer.GetPath());
    VtArray<GfVec3f> extent(2);
    if (instancer.ComputeExtentAtTime(&extent, usdTime, usdTime)) {
        _SetAttribute(instancer.CreateExtentAttr(), &extent, usdTime);
    }

    return true;
}

/* virtual */
void
PxrUsdTranslators_InstancerWriter::PostExport()
{
    for (UsdMayaPrimWriterSharedPtr& writer : _prototypeWriters) {
        writer->PostExport();
    }

    // Try to clean up matching instancerTranslate/translate pairs on the
    // prototypes.
    for (const _TranslateOpData& opData : _instancerTranslateOps) {
        UsdPrim prim = opData.op.GetAttr().GetPrim();
        if (UsdGeomXformable xformable = UsdGeomXformable(prim)) {
            _CleanUpRedundantTranslateOps(xformable);
        }
    }
}

/* virtual */
bool
PxrUsdTranslators_InstancerWriter::ShouldPruneChildren() const
{
    return true;
}

/* virtual */
const SdfPathVector&
PxrUsdTranslators_InstancerWriter::GetModelPaths() const
{
    return _modelPaths;
}


PXR_NAMESPACE_CLOSE_SCOPE
