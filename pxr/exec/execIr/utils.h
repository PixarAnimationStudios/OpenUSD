//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_UTILS_H
#define PXR_EXEC_EXEC_IR_UTILS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfContext;

/// Shared parameters for the Compute and Invert functions.
///
/// Clients configure one of these in order to describe how local 
/// transformations should be applied.
struct ExecIr_UtilsParams
{
    // startingSpace is the location of the joint with no local transforms
    // applied. It's where it would be if all the avars were 0.0 
    // (or 1.0 for scales)
    GfMatrix4d startingSpace = GfMatrix4d(1.0);

    // translationOrientation is the orientation in which translations 
    // will be applied, expressed in world space. The scale of the space
    // designates the units of translation. It is expected to have no 
    // translation component.
    GfMatrix4d translationOrientation = GfMatrix4d(1.0);

    // rotationOrientation is the orientation of the local rotation axes,
    // expressed in world space. 
    GfMatrix4d rotationOrientation = GfMatrix4d(1.0);
};

// Computes the starting space -- where the joint would be without 
// the effect of any local translation, rotation, or scale avars.
//
GfMatrix4d
ExecIr_UtilsComputeStandardStartingSpace(
    const VdfContext &ctx);

// Computes the orientation and scale in which local translations are 
// applied.
//
// The axes of this matrix represent the world space direction that the local
// transaltions should be applied in. The scale of this matrix changes the
// effective units of translation.  The result has no translation component.
//
GfMatrix4d 
ExecIr_UtilsComputeStandardTranslationOrientation(
    const VdfContext &ctx,
    const GfMatrix4d &startingSpace);

// Computes the orientation of the rotation.
//
// The axes of this space represent the axes of local rotation (expressed in
// world space). This is a rotation and scale matrix so it can also represent
// handedness.
// 
GfMatrix4d 
ExecIr_UtilsComputeStandardRotationOrientation(
    const VdfContext &ctx,
    const GfMatrix4d &startingSpace);

// Compute the local translation vector by evaluating the Tx,Ty,Tz inputs.
// 
GfVec3d 
ExecIr_UtilsComputeLocalTranslation(
    const VdfContext &ctx);

// Compute the local rotation by evaluating the rotation scalars,
// as well as the rotation order.
// 
GfRotation 
ExecIr_UtilsComputeLocalRotation(
    const VdfContext &ctx);

// Returns the forward-computed result space.
//
GfMatrix4d
ExecIr_UtilsCompute(
    const ExecIr_UtilsParams &params,
    const GfVec3d &localTranslation,
    const GfRotation &localRotation);

// Populates \p resultMap with inverted values.
//
void 
ExecIr_UtilsInvert(
    const VdfContext &ctx,
    const GfMatrix4d &posedSpace,
    const ExecIr_UtilsParams &params,
    ExecIrInversionResult *const resultMap);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
