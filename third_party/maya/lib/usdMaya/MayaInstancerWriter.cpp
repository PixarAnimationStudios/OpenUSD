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
#include "usdMaya/MayaInstancerWriter.h"

#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"

#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include <maya/MAnimUtil.h>
#include <maya/MDataHandle.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

static constexpr double _EPSILON = 1e-3;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, 
    (Prototypes)
    (instancerTranslate)
);

MayaInstancerWriter::MayaInstancerWriter(const MDagPath & iDag,
    const SdfPath& uPath,
    bool instanceSource,
    usdWriteJobCtx& jobCtx)
    : MayaTransformWriter(iDag, uPath, instanceSource, jobCtx),
      _numPrototypes(0)
{
    UsdGeomPointInstancer primSchema =
            UsdGeomPointInstancer::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
    UsdModelAPI(mUsdPrim).SetKind(KindTokens->assembly);
}

/* virtual */
void
MayaInstancerWriter::write(const UsdTimeCode &usdTime)
{
    UsdGeomPointInstancer primSchema(mUsdPrim);
    writeTransformAttrs(usdTime, primSchema);
    writeInstancerAttrs(usdTime, primSchema);
}

/// Gets the transformed position of (0, 0, 0) using the transform's
/// local transformation matrix.
static bool
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

/// Returns STATIC or ANIMATED if an extra translate is needed to compensate for
/// Maya's instancer translation behavior on the given prototype DAG node.
/// (This function may return false positives, which are OK but will simply
/// contribute extra data. It should never return false negatives, which
/// would cause correctness problems.)
AnimChannelSampleType
MayaInstancerWriter::_GetInstancerTranslateSampleType(
    const MDagPath& prototypeDagPath) const
{
    // XXX: Maybe we could be smarter here and figure out if the animation
    // affects instancerTranslate?
    bool animated = getArgs().exportAnimation &&
            MAnimUtil::isAnimated(prototypeDagPath.node(), false);
    if (animated) {
        return ANIMATED;
    }

    GfVec3d origin;
    bool translated =
            _GetTransformedOriginInLocalSpace(prototypeDagPath, &origin) &&
            !GfIsClose(origin, GfVec3d(0.0), _EPSILON);
    if (translated) {
        return STATIC;
    }

    return NO_XFORM;
}

/// Exports a single prototype whose original Maya location is prototypeDagPath,
/// and whose exported prototype location is prototypeUsdPath.
/// validPrimWritersOut needs to be provided; all of the valid prim writers
/// for this prototype's hierarchy will be appended to the vector.
void
MayaInstancerWriter::_ExportPrototype(
    const MDagPath& prototypeDagPath,
    const SdfPath& prototypeUsdPath,
    std::vector<MayaPrimWriterPtr>* validPrimWritersOut)
{
    // The USD path of the prototype root if it were exported at its current
    // Maya location.
    const SdfPath prototypeComputedUsdPath =
            PxrUsdMayaUtil::MDagPathToUsdPath(prototypeDagPath, false);

    MItDag itDag(MItDag::kDepthFirst, MFn::kInvalid);
    itDag.reset(prototypeDagPath);
    for (; !itDag.isDone(); itDag.next()) {
        MDagPath curDagPath;
        itDag.getPath(curDagPath);

        if (!mWriteJobCtx.needToTraverse(curDagPath)) {
            itDag.prune();
            continue;
        }

        // The USD path of this prototype descendant prim if it were exported
        // at its current Maya location.
        const SdfPath curComputedUsdPath =
                PxrUsdMayaUtil::MDagPathToUsdPath(curDagPath, false);

        // Compute the current prim's relative path w/r/t the prototype root,
        // and use this to re-anchor it under the USD stage location where
        // we want to write out the prototype.
        const SdfPath curRelPath = curComputedUsdPath.MakeRelativePath(
                prototypeComputedUsdPath);
        const SdfPath curActualUsdPath = prototypeUsdPath
                .AppendPath(curRelPath);

        MayaPrimWriterPtr writer = mWriteJobCtx.createPrimWriter(
                curDagPath, curActualUsdPath);
        if (!writer) {
            continue;
        }

        if (writer->getPrim()) {
            validPrimWritersOut->push_back(writer);

            // The prototype root must be visible to match Maya's behavior,
            // which always vis'es the prototype root, even if it is marked
            // hidden.
            // (This check is somewhat roundabout because we might be merging
            // transforms and shapes, so it's difficult ahead-of-time to know
            // which prim writer will write the root prim.)
            if (writer->getPrim().GetPath() == prototypeUsdPath) {
                writer->setExportsVisibility(false);
            }
        }

        if (writer->shouldPruneChildren()) {
            itDag.prune();
        }
    }
}

/* virtual */
bool
MayaInstancerWriter::writeInstancerAttrs(
    const UsdTimeCode& usdTime, const UsdGeomPointInstancer& instancer)
{
    MStatus status = MS::kSuccess;
    MFnDagNode dagNode(getDagPath(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Note: In this function, we don't read instances using the provided
    // MFnInstancer API. One reason is that it breaks up prototypes into their
    // constituent shapes, and there's no way to figure out which hierarchy
    // they came from. Another reason is that it only provides computed matrices
    // and not separate position, rotation, scale attrs.

    // At the default time, setup all the prototype instances.
    if (usdTime.IsDefault()) {
        const MPlug inputHierarchy = dagNode.findPlug("inputHierarchy", true,
                &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        const UsdPrim prototypesGroupPrim = getUsdStage()->DefinePrim(
                instancer.GetPrim().GetPath().AppendChild(_tokens->Prototypes));
        UsdModelAPI(prototypesGroupPrim).SetKind(KindTokens->group);
        UsdRelationship prototypesRel = instancer.CreatePrototypesRel();

        const unsigned int numElements = inputHierarchy.numElements();
        for (unsigned int i = 0; i < numElements; ++i) {
            const MPlug plug = inputHierarchy[i];
            const MPlug source(PxrUsdMayaUtil::GetConnected(plug));
            if (source.isNull()) {
                TF_WARN("Cannot read prototype: the source plug %s was null",
                        plug.name().asChar());
                return false;
            }

            MFnDagNode sourceNode(source.node(), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);

            MDagPath prototypeDagPath;
            sourceNode.getPath(prototypeDagPath);

            const TfToken prototypeName(TfStringPrintf("prototype_%d", i));
            const SdfPath prototypeUsdPath = prototypesGroupPrim.GetPath()
                    .AppendChild(prototypeName);
            UsdPrim prototypePrim = getUsdStage()->DefinePrim(
                    prototypeUsdPath);

            // Try to be conservative and only create an intermediary xformOp
            // with the instancerTranslate if we can ensure that we don't need
            // to compensate for the translation on the prototype root.
            //
            // XXX: instancerTranslate does not behave well when added to a
            // reference that has an existing transform on the far side of the
            // reference. However, its behavior at least matches the
            // behavior in PxrUsdMayaTranslatorModelAssembly. If we fix the
            // behavior there, we need to make sure that this is also
            // fixed to match.
            const AnimChannelSampleType instancerTranslateSampleType =
                    _GetInstancerTranslateSampleType(prototypeDagPath);
            if (instancerTranslateSampleType != NO_XFORM) {
                UsdGeomXformable xformable(prototypePrim);
                UsdGeomXformOp newOp = xformable.AddTranslateOp(
                        UsdGeomXformOp::PrecisionDouble,
                        _tokens->instancerTranslate);
                _instancerTranslateOps.emplace_back(
                        prototypeDagPath, newOp, instancerTranslateSampleType);
            }

            _ExportPrototype(
                    prototypeDagPath, prototypeUsdPath, &_prototypeWriters);
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
    for (MayaPrimWriterPtr& writer : _prototypeWriters) {
        writer->write(usdTime);

        if (usdTime.IsDefault()) {
            // Prototypes should have kind component or derived (don't stomp
            // over existing component-derived kinds).
            // (Note that ModelKindWriter's fix-up stage might change this.)
            if (const UsdPrim writerPrim = writer->getPrim()) {
                UsdModelAPI primModelAPI(writerPrim);
                TfToken kind;
                primModelAPI.GetKind(&kind);
                if (!KindRegistry::IsA(kind, KindTokens->component)) {
                    primModelAPI.SetKind(KindTokens->component);
                }
            }
        }
    }

    // Write the instancerTranslate xformOp for all prims that need it.
    // (This should happen @ default time or animated time depending on whether
    // the xform is animated.)
    for (const MayaInstancerWriter_TranslateOpData& opData :
                _instancerTranslateOps) {
        if ((opData.sampleType == STATIC && usdTime.IsDefault()) ||
                (opData.sampleType == ANIMATED && !usdTime.IsDefault())) {
            GfVec3d origin;
            if (_GetTransformedOriginInLocalSpace(opData.mayaPath, &origin)) {
                UsdGeomXformOp translateOp = opData.op;
                translateOp.Set(-origin, usdTime);
            }
        }
    }

    // Grab the inputPoints data from the source plug.
    // (This attribute's value must come from a source plug; it isn't
    // directly writeable. Thus reading it directly may not give the right
    // value depending on Maya's execution behavior.)
    MPlug inputPointsDest = dagNode.findPlug("inputPoints", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug inputPointsSrc = PxrUsdMayaUtil::GetConnected(inputPointsDest);
    if (inputPointsSrc.isNull()) {
        TF_WARN("inputPoints not connected on instancer '%s'",
                getDagPath().fullPathName().asChar());
        return false;
    }

    auto holder = PxrUsdMayaUtil::GetPlugDataHandle(inputPointsSrc);
    if (!holder) {
        TF_WARN("Unable to read inputPoints data handle for instancer '%s'",
                getDagPath().fullPathName().asChar());
        return false;
    }

    MFnArrayAttrsData inputPointsData(holder->GetDataHandle().data(),
            &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (!PxrUsdMayaWriteUtil::WriteArrayAttrsToInstancer(
            inputPointsData, instancer, _numPrototypes, usdTime)) {
        return false;
    }

    // Load the completed point instancer to compute and set its extent.
    instancer.GetPrim().GetStage()->Load(instancer.GetPath());
    VtArray<GfVec3f> extent(2);
    if (instancer.ComputeExtentAtTime(&extent, usdTime, usdTime)) {
        instancer.CreateExtentAttr().Set(extent, usdTime);
    }

    return true;
}

void
MayaInstancerWriter::postExport()
{
    for (MayaPrimWriterPtr& writer : _prototypeWriters) {
        writer->postExport();
    }
}

bool
MayaInstancerWriter::exportsReferences() const
{
    return true;
}

bool
MayaInstancerWriter::shouldPruneChildren() const
{
    return true;
}

bool
MayaInstancerWriter::getAllAuthoredUsdPaths(SdfPathVector* outPaths) const
{
    bool hasPrims = MayaPrimWriter::getAllAuthoredUsdPaths(outPaths);
    SdfPath protosPath = getUsdPath().AppendChild(_tokens->Prototypes);
    if (getUsdStage()->GetPrimAtPath(protosPath)) {
        outPaths->push_back(protosPath);
        hasPrims = true;
    }
    for (const MayaPrimWriterPtr primWriter : _prototypeWriters) {
        if (primWriter->getAllAuthoredUsdPaths(outPaths)) {
            hasPrims = true;
        }
    }
    return hasPrims;
}

PXR_NAMESPACE_CLOSE_SCOPE
