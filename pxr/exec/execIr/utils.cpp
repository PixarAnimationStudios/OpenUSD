//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/utils.h"

#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/vdf/context.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
//
// Internal Pixar note: The following code was sourced from MfTransformablePrim
//

namespace {

// Enumerates rotation axis constants.
enum _TransformRotationAxes {
    TransformRotationAxisX = 0,
    TransformRotationAxisY = 1,
    TransformRotationAxisZ = 2
};

// Enumerates the rotation order of a transform attribute.
//
enum _TransformRotationOrder {
    TransformRotationOrderXYZ = 0x06,
    TransformRotationOrderXZY = 0x09,
    TransformRotationOrderYXZ = 0x12,
    TransformRotationOrderYZX = 0x18,
    TransformRotationOrderZXY = 0x21,
    TransformRotationOrderZYX = 0x24
};

} // anonymous namespace


// Gets the rotation axis corresponding to the given ring number.
//
// - ringNumber should be 0, 1, 2, or 3
// - 0 corresponds to twist/spin.
// - 1, 2, 3, correspond to those in the rotationOrder, as in XYZ means 1,2,3
//   are x, y, z.
static _TransformRotationAxes
_GetWhichRotationAxis(
    const _TransformRotationOrder rotationOrder,
    int ringNumber)
{
    if (ringNumber < 0 || ringNumber > 3) {
        TF_CODING_ERROR("GetWhichRotationAxis got out of range ringNumber");
    }

    // 1, 2, 3, correspond to those in the rotationOrder,
    // as in XYZ means 1,2,3 are x, y, z.
    // But 0 corresponds to spin, which is always set to the 
    // same as 3...
    if (ringNumber == 0) {
        ringNumber = 3;
    }

    const int shift = (3-ringNumber) * 2;
    return (_TransformRotationAxes) 
            ((rotationOrder & (0x03 << shift)) >> shift);
}

static GfVec3d
_GetRotationAxis(
    const _TransformRotationOrder rotationOrder,
    const int ringNumber)
{
    const _TransformRotationAxes whichAxis =
        _GetWhichRotationAxis(rotationOrder, ringNumber);
    switch(whichAxis) {
        case TransformRotationAxisX:
            return GfVec3d::XAxis();
        case TransformRotationAxisY:
            return GfVec3d::YAxis();
        case TransformRotationAxisZ:
            return GfVec3d::ZAxis();
        default:
            TF_CODING_ERROR("GetRotationAxis called with illegal argument");
            return GfVec3d::XAxis();
    }
}

// Given a \p startValue of the form twist123 (where 1,2,3 are X,Y,Z ordered in
// rotationOrder, a \p rotationOrder, a \p holdIndex specifying which element of
// startValue should remain unchanged, and \p endingRotation, returns a twist123
// rotation that produces endingRotation but also matches the values in
// startValue as closely as possible.
//
// Used when rotating things so that scalar rx/ry/rz/rspin values change in the
// most continuous manner possible.
// 
static GfVec4d 
_GetBestEndingTwist123( 
    const GfVec4d &startValue,
    const _TransformRotationOrder rotationOrder,
    const int holdIndex,
    const GfRotation &endingRotation)
{
    GfVec4d twist123 = startValue;

    if (holdIndex < 0 || holdIndex > 3) {
        TF_CODING_ERROR("Illegal holdIndex in ");
    }
    GfRotation totalRot = endingRotation;

    // Always decompose as if right handed, it's only the 
    // incoming rotation from the event that gets flipped if left-handed
    double handednessForDecomp = 1.0;

    if (holdIndex == 0) {
        // Premultiply by inverse of twist, so that twist can remain 
        // unchanged and the rest of the matrix factors out into
        // the remaining 3 angles...
        GfRotation twistRotInv = GfRotation( 
            _GetRotationAxis(rotationOrder, 0),
            -1.0 * twist123[0]);
        totalRot = twistRotInv * totalRot;
    } else if (holdIndex == 3) {
        // Postmultiply by inverse of the last rotation, so that it can remain 
        // unchanged and the rest of the matrix factors out into
        // the remaining 3 angles...
        GfRotation swingRotInv = GfRotation( 
                _GetRotationAxis(rotationOrder, 3),
                -1.0 * twist123[3]);
        totalRot = totalRot * swingRotInv;
    } else {
        // Otherwise we're holding one of the middle two rotations which means
        // we have to zero out the held value if we want to be able decompose
        // all rotations into the remaining angles.
        // 
        // XXX: It may be desirable to still hold the middle rotations without
        // zeroing them out and find the closest guess for rotations that we 
        // can't hit but I'm holding off doing that extra work unless there
        // is a desire for that behavior.
        twist123[holdIndex] = 0.0;
    }
    
    // For decompose to use the incoming angles as hints, they
    // must be expressed in radians.
    for (int i = 0; i < 4; i++) {
        twist123[i] = GfDegreesToRadians(twist123[i]);
    }

    // Get pointers to the twist angles
    double *twist123Ptrs[4] =
        {&twist123[0], &twist123[1], &twist123[2], &twist123[3]};
    // Leave the held rotation angle out of the decomposition.
    twist123Ptrs[holdIndex] = nullptr;

    // Decompose the angles
    // XXX: Need to check how rotation order will affect HoldRotation_FB and
    // HoldRotation_LR
    GfRotation::DecomposeRotation(
            GfMatrix4d(1.0).SetRotate(totalRot),
            _GetRotationAxis(rotationOrder, 0),
            _GetRotationAxis(rotationOrder, 1),
            _GetRotationAxis(rotationOrder, 2),
            handednessForDecomp,
            twist123Ptrs[0],
            twist123Ptrs[1],
            twist123Ptrs[2],
            twist123Ptrs[3],
            true, // use passed in values as hints
            nullptr // swShift
        );
    // Decompose writes radians, convert to degrees
    for (int i = 0; i < 4; i++) {
        twist123[i] = GfRadiansToDegrees(twist123[i]);
    }

    return twist123;
}

// Converts a rotation order token into the corresponding enum value.
static _TransformRotationOrder
_GetRotationOrderFromToken(const TfToken &rotationOrder)
{
    if (rotationOrder == ExecIrTokens->XYZ) {
        return TransformRotationOrderXYZ;
    } else if (rotationOrder == ExecIrTokens->XZY) {
        return TransformRotationOrderXZY;
    } else if (rotationOrder == ExecIrTokens->YXZ) {
        return TransformRotationOrderYXZ;
    } else if (rotationOrder == ExecIrTokens->YZX) {
        return TransformRotationOrderYZX;
    } else if (rotationOrder == ExecIrTokens->ZXY) {
        return TransformRotationOrderZXY;
    } else if (rotationOrder == ExecIrTokens->ZYX) {
        return TransformRotationOrderZYX;
    }

    TF_WARN("Illegal rotation order '%s'", rotationOrder.GetText());
    return TransformRotationOrderXYZ;
}


////////////////////////////////////////////////////////////////////////////////
//
// Internal Pixar note: The following code was sourced from AmUtils
//

// Computes a composed rotation from rotation angle scalars and a rotation
// order.
//
static GfRotation 
_ComputeComposedRotation(
    const double rSpin, const double rx, const double ry, const double rz,
    const _TransformRotationOrder rotationOrder)
{
    // Use the rotation order to create an array of the rx, ry, rz rotations
    // in the correct order.
    std::array<GfRotation, 3> rotations;
    for (int orderedRotIdx = 0; orderedRotIdx < 3; ++orderedRotIdx) {
        const int shift = (2-orderedRotIdx) * 2;
        GfRotation &rot = rotations[orderedRotIdx];
        switch ((rotationOrder & (0x03 << shift)) >> shift) {
        case TransformRotationAxisX:
            rot = GfRotation(GfVec3d::XAxis(), rx);
            break;
        case TransformRotationAxisY:
            rot = GfRotation(GfVec3d::YAxis(), ry);
            break;
        case TransformRotationAxisZ:
            rot = GfRotation(GfVec3d::ZAxis(), rz);
            break;
        }
    }

    // Spin rotation always comes first and uses the last rotation's axis.
    const GfRotation spinRotation(rotations.back().GetAxis(), rSpin);

    // Return the composed rotation.
    return spinRotation * rotations[0] * rotations[1] * rotations[2];
}


////////////////////////////////////////////////////////////////////////////////
//
// Internal Pixar note: The following code was sourced from AmTransformable.
//

GfMatrix4d
ExecIr_ComputeLocalXf(
    const double tx, 
    const double ty, 
    const double tz,
    const double rSpin, 
    const double rx, 
    const double ry, 
    const double rz,
    const TfToken &rotationOrder,
    const VdfContext &ctx)
{
    GfMatrix3d rotXf(
        _ComputeComposedRotation(
            rSpin, rx, ry, rz, _GetRotationOrderFromToken(rotationOrder)));

    // Combine rotations with translation.
    GfMatrix4d resultXf(rotXf, GfVec3d(tx, ty, tz));

    // TODO: Add support for left/right handedness.

    return resultXf;
}

GfMatrix4d
ExecIr_ComputeDefaultSpace(
    const GfMatrix4d &defaultTransRotOffsetXf,
    const GfMatrix4d &defaultScaleXf,
    const GfMatrix4d &localRestXf,
    const GfMatrix4d &parentDefaultSpace)
{
    // Compose the default translation and rotation with the rest space
    const GfMatrix4d localDefaultAndRestXf =
        defaultTransRotOffsetXf * localRestXf;

    // Compose with the scaled parent space and extract the translation
    // to determine the new origin
    const GfVec3d originPt =
        (localDefaultAndRestXf * parentDefaultSpace)
            .ExtractTranslation();

    // Compose with the unscaled parent space and extract the rotation
    // to determine the new orientation
    const GfMatrix4d orientationXf =
        (localDefaultAndRestXf * parentDefaultSpace.GetOrthonormalized())
            .GetOrthonormalized().SetTranslateOnly(GfVec3d(0.0));

    // Compose the final computed result
    return defaultScaleXf * orientationXf * GfMatrix4d().SetTranslate(originPt);
}


////////////////////////////////////////////////////////////////////////////////
//
// Internal Pixar note: The following code was sourced from IvcrUtil.
//

// Extracts scalar scale values from the given matrix.
//
// Note that this always returns 0 or positive values -- it does not capture
// flipping.
//
static GfVec3d
_ExtractScale(const GfMatrix4d &xf)
{
    GfVec3d result;
    result[0] = xf.GetRow3(0).GetLength();
    result[1] = xf.GetRow3(1).GetLength();
    result[2] = xf.GetRow3(2).GetLength();

    return result;
}

// Decomposes the given rotation matrix into scalars applied in the given
// rotation order.
//
// Note: This function only handles right-handed transforms.
//
// Uses \p hintValues to disambiguate between multiple possible solutions.
// 
// \p holdRotationAvar is used to determine which avars this writes into, and
// should be one of the tokens "Rx", "Ry", "Rz", "Rspin". The specified avar's
// return value will always be zero.
//
static GfVec4d
_DecomposeRotationMatrix(
    const GfMatrix4d &xf,
    const _TransformRotationOrder &rotationOrder,
    const GfVec4d &hintAngles)
{
    static const std::vector<GfVec3d> axes{
        GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis()};

    std::vector<int> axisOrdering(4);
    for (int i = 0; i < 4; ++i) {
        axisOrdering[i] =
            _GetWhichRotationAxis(rotationOrder, i);
    }

    // TODO: Add support for specifying the "hold avar," the avar to _not_
    // decompose into. For now, hardcode the hold avar to Rspin.
    const int holdIndex = 0;

    const GfVec4d twist123 = _GetBestEndingTwist123(
        {
            hintAngles[0],
            hintAngles[axisOrdering[1] + 1],
            hintAngles[axisOrdering[2] + 1],
            hintAngles[axisOrdering[3] + 1]
        },
        rotationOrder,
        holdIndex,
        xf.GetOrthonormalized().ExtractRotation());

    GfVec4d result;
    result[0] = twist123[0];
    result[axisOrdering[1]+1] = twist123[1];
    result[axisOrdering[2]+1] = twist123[2];
    result[axisOrdering[3]+1] = twist123[3];
    return result;
}

// Modifies matrixToConform so it matches the handedness of target.
//
// If necessary, flips the X axis.
//
static void
_ConformHandedness(
    const GfMatrix4d &target,
    GfMatrix4d *const matrixToConform)
{
    const bool handednessMatches =
        matrixToConform->GetHandedness() == target.GetHandedness();

    if (!handednessMatches) {
        *matrixToConform =
            GfMatrix4d().SetScale(GfVec3d(-1.0, 1.0, 1.0)) * *matrixToConform;
    }
}

// Computes a rotation matrix that can be post-multiplied by a given
// space to apply the local rotation around the given world pivot.
// 
static GfMatrix4d
_ComputeRotationXf(
    const GfRotation &localRotation,
    const GfVec3d &worldPivot,
    const GfMatrix4d &rotationOrientation)
{
    return
        GfMatrix4d().SetTranslate(-worldPivot) *
        rotationOrientation.GetInverse() *
        GfMatrix4d().SetRotate(localRotation) *
        rotationOrientation *
        GfMatrix4d().SetTranslate(worldPivot);
}

// Applies the specified translation to the input space.
//
// The local translation is applied relative to the given
// translationOrientation. The scale of the translationOrientation determines
// the units of the local translation.
//
static GfMatrix4d
_ApplyTranslation(
    const GfMatrix4d &inputSpace,
    const GfVec3d &localTranslation,
    const GfMatrix4d &translationOrientation)
{
    return inputSpace * GfMatrix4d().SetTranslate(
        translationOrientation.Transform(localTranslation));
}

// Applies the specified rotation to the input space.
//
// The rotation happens about the given pivot, relative to the given world
// rotation orientation.
// 
static GfMatrix4d
_ApplyRotation(
    const GfMatrix4d &inputSpace,
    const GfRotation &localRotation,
    const GfVec3d &worldPivot,
    const GfMatrix4d &rotationOrientation)
{
    return inputSpace * 
        _ComputeRotationXf(
            localRotation, worldPivot, rotationOrientation);

}

// Inverts the translation relative to the starting space.
// 
static GfVec3d
_InvertTranslation(
    const GfMatrix4d &startingSpace,
    const GfMatrix4d &posedSpace,
    const GfMatrix4d &translationOrientation)
{
    const GfVec3d localPivot(0.0, 0.0, 0.0);

    const GfVec3d worldTranslation =
        posedSpace.Transform(localPivot) -
        startingSpace.Transform(localPivot);

    return
        translationOrientation.GetInverse()
            .Transform(worldTranslation);
}

// Inverts and decomposes the rotation of the posed space relative to the
// starting space.
//
// Decomposition uses the given rotation order, but the angles are always
// returned in the order: [rSpin, rx, ry, rz]
// 
static GfVec4d
_InvertRotation(
    const GfMatrix4d &startingSpace,
    const GfMatrix4d &posedSpace,
    const GfMatrix4d &rotationOrientation,
    const _TransformRotationOrder &rotationOrder,
    const GfVec4d &hintAngles)
{
    const GfMatrix4d localRotationXf =
        rotationOrientation *
        startingSpace.GetInverse() *
        posedSpace *
        rotationOrientation.GetInverse();

    return _DecomposeRotationMatrix(
        localRotationXf, rotationOrder, hintAngles);
}

////////////////////////////////////////////////////////////////////////////////
//
// Internal Pixar note: The following code was sourced from IvcrFkSolver.
//

// Computes the starting space -- where the joint would be without 
// the effect of any local translation, rotation, or scale avars.
//
static GfMatrix4d
_ComputeStandardStartingSpace(
    const VdfContext &ctx,
    const GfMatrix4d &parentSpace,
    const GfMatrix4d &parentDefaultSpace)
{
    // TODO: Add support for total size avars.
    const double parentTotalSize(1.0);

    const GfMatrix4d &defaultSpace =
        ctx.GetInputValue<GfMatrix4d>(ExecIrTokens->inDefaultSpace);

    // Extract the default scales and work with a normalized matrix
    // for these calculations
    const GfMatrix4d defaultScaleXf = GfMatrix4d().SetScale(
        _ExtractScale(defaultSpace));

    // Create a space that is oriented the same as our space but 
    // located at the pivot.
    const GfMatrix4d pivotDefaultSpace = 
        defaultSpace.GetOrthonormalized();

    // TODO: Add support for constraints.

    GfMatrix4d pivotStartingSpace =
        pivotDefaultSpace * parentDefaultSpace.GetInverse() * parentSpace;

    // Since we're inheriting nonuniform scales, we want to maintain our
    // parent's nonuniform scale information but factor out its uniform scale;
    // if we aren't, we want to fully reset to identity scaling.
    //
    // TODO: Add support for _not_ inheriting non-uniform scales.
    pivotStartingSpace =
        GfMatrix4d().SetScale(GfVec3d(1/parentTotalSize)) * 
        pivotStartingSpace;

    return defaultScaleXf *  pivotStartingSpace;
}

GfMatrix4d 
ExecIr_UtilsComputeStandardTranslationOrientation(
    const VdfContext &ctx,
    const GfMatrix4d &startingSpace)
{
    // TODO: Add support for constraints.

    // Normal translation orientation is the default space relative to the
    // parent's posed space. Use orthonormalized versions of all these
    // matrices since orientation doesn't depend on scale, and combining
    // non-orthonomralized matrices and then normalizing after the fact can
    // introduce unintended rotations.
    const GfMatrix4d defaultSpaceNormalized =
        ctx.GetInputValue<GfMatrix4d>(ExecIrTokens->inDefaultSpace)
        .GetOrthonormalized();

    // TODO: Add support for _not_ inheriting non-orthogonal transforms.
    const GfMatrix4d &parentSpaceNormalized = 
        ctx.GetInputValue<GfMatrix4d>(ExecIrTokens->parentInSpace)
        .GetOrthonormalized();
    const GfMatrix4d parentDefaultSpaceNormalized =
        ctx.GetInputValue<GfMatrix4d>(
            ExecIrTokens->parentInDefaultSpace)
        .GetOrthonormalized();

    GfMatrix4d translationOrientation =
        (defaultSpaceNormalized * 
         parentDefaultSpaceNormalized.GetInverse() * 
         parentSpaceNormalized)
        .SetTranslateOnly(GfVec3d(0.0));

    _ConformHandedness(defaultSpaceNormalized, &translationOrientation);

    // TODO: Add support for translation units.

    return translationOrientation;
}

GfMatrix4d 
ExecIr_UtilsComputeStandardRotationOrientation(
    const VdfContext &ctx,
    const GfMatrix4d &startingSpace)
{
    // TODO: Add support for _not_ inheriting non-orthogonal transforms

    return GfMatrix4d(startingSpace)
        .GetOrthonormalized()
        .SetTranslateOnly(GfVec3d(0.0));
}

GfVec3d 
ExecIr_UtilsComputeLocalTranslation(
    const VdfContext &ctx)
{
    return GfVec3d(
        ctx.GetInputValue<double>(ExecIrTokens->inTx),
        ctx.GetInputValue<double>(ExecIrTokens->inTy),
        ctx.GetInputValue<double>(ExecIrTokens->inTz));
}

GfRotation 
ExecIr_UtilsComputeLocalRotation(
    const VdfContext &ctx)
{
    const _TransformRotationOrder rotationOrder =
        _GetRotationOrderFromToken(
            ctx.GetInputValue<TfToken>(
                ExecIrTokens->inRotationOrder));

    return _ComputeComposedRotation(
        ctx.GetInputValue<double>(ExecIrTokens->inRspin),
        ctx.GetInputValue<double>(ExecIrTokens->inRx),
        ctx.GetInputValue<double>(ExecIrTokens->inRy),
        ctx.GetInputValue<double>(ExecIrTokens->inRz),
        rotationOrder);
}

GfMatrix4d
ExecIr_UtilsCompute(
    const ExecIr_UtilsParams &params,
    const GfVec3d &localTranslation,
    const GfRotation &localRotation)
{
    // Translation:
    const GfMatrix4d translatedSpace = 
        _ApplyTranslation(
            params.startingSpace, 
            localTranslation, 
            params.translationOrientation);

    // Rotation:
    const GfVec3d worldRotatePivot = 
        translatedSpace.Transform(/* localPivot */ GfVec3d(0.0, 0.0, 0.0));

    const GfMatrix4d rotatedSpace = 
        _ApplyRotation(
            translatedSpace,
            localRotation,
            worldRotatePivot,
            params.rotationOrientation);

    // TODO: Add support for scale avars.
    // TODO: Add support for _not_ inheriting non-orthogonal transforms.

    return rotatedSpace;
}

void 
ExecIr_UtilsInvert(
    const VdfContext &ctx,
    const GfMatrix4d &posedSpace,
    const ExecIr_UtilsParams &params,
    ExecIrResult *const resultMap)
{
    const GfVec3d localTranslation = 
        _InvertTranslation(
            params.startingSpace,
            posedSpace,
            params.translationOrientation);

    (*resultMap)[ExecIrTokens->inTx] = localTranslation[0];
    (*resultMap)[ExecIrTokens->inTy] = localTranslation[1];
    (*resultMap)[ExecIrTokens->inTz] = localTranslation[2];

    // TODO: Inversion hints. For now, we choose a rotation parameterization as
    // close to (0, 0, 0, 0) as possible.
    const GfVec4d forwardAvarValues(0.0, 0.0, 0.0, 0.0);

    const _TransformRotationOrder rotationOrder =
        _GetRotationOrderFromToken(
            ctx.GetInputValue<TfToken>(
                ExecIrTokens->inRotationOrder));

    const GfVec4d angles = _InvertRotation(
        params.startingSpace,
        posedSpace,
        params.rotationOrientation,
        rotationOrder,
        forwardAvarValues);

    // TODO: Add support for scale and squetch avars.

    (*resultMap)[ExecIrTokens->inRspin] = angles[0];
    (*resultMap)[ExecIrTokens->inRx] = angles[1];
    (*resultMap)[ExecIrTokens->inRy] = angles[2];
    (*resultMap)[ExecIrTokens->inRz] = angles[3];
}

////////////////////////////////////////////////////////////////////////////////
//
// Internal Pixar note: The following code was sourced from IvcrFkController.
//

ExecIr_UtilsParams
ExecIr_ComputeFkParams(const VdfContext &ctx)
{
    const GfMatrix4d startingSpace = _ComputeStandardStartingSpace(
        ctx,
        ctx.GetInputValue<GfMatrix4d>(ExecIrTokens->parentInSpace),
        ctx.GetInputValue<GfMatrix4d>(
            ExecIrTokens->parentInDefaultSpace));

    return {
        startingSpace,
        ExecIr_UtilsComputeStandardTranslationOrientation(ctx, startingSpace),
        ExecIr_UtilsComputeStandardRotationOrientation(ctx, startingSpace)
    };
}

PXR_NAMESPACE_CLOSE_SCOPE
