//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_GEOM_SAMPLING_UTILS_H
#define PXR_USD_USD_GEOM_SAMPLING_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;
class UsdPrim;
class UsdTimeCode;


/// Get the authored positions (or points), velocities, and accelerations.
/// Also fetches the velocity scale. This method fails if the positions can't 
/// be fetched. If velocities can't be fetched or positions are not time-varying, 
/// then \p velocities is cleared and \p velocitiesSampleTime is not changed. 
/// If accelerations can't be fetched then \p accelerations is cleared. 
bool 
UsdGeom_GetPositionsVelocitiesAndAccelerations(
    const UsdAttribute& positionsAttr,
    const UsdAttribute& velocitiesAttr,
    const UsdAttribute& accelerationsAttr,
    UsdTimeCode baseTime,
    size_t expectedNumPositions,
    VtVec3fArray* positions,
    VtVec3fArray* velocities,
    UsdTimeCode* velocitiesSampleTime,
    VtVec3fArray* accelerations,
    UsdPrim const &prim);

/// Get the authored scales. This method fails if the scales can't be fetched. 
bool
UsdGeom_GetScales(
    const UsdAttribute& scalesAttr,
    const UsdTimeCode baseTime,
    size_t expectedNumScales,
    VtVec3fArray* scales,
    UsdPrim const &prim);


/// Get the authored orientations and angular velocities for instance
/// transform computation. This method fails if the orientations can't be
/// fetched. If angular velocities can't be fetched or orientations are not
/// time-varying, then \p angularVelocities is cleared and
/// \p angularVelocitiesSampleTime is not changed. This method supports half
/// precision rotations
bool
UsdGeom_GetOrientationsAndAngularVelocities(
    const UsdAttribute& orientationsAttr,
    const UsdAttribute& angularVelocitiesAttr,
    UsdTimeCode baseTime,
    size_t expectedNumOrientations,
    VtQuathArray* orientations,
    VtVec3fArray* angularVelocities,
    UsdTimeCode* angularVelocitiesSampleTime,
    UsdPrim const &prim);

/// \overload UsdGeom_GetOrientationsAndAngularVelocities to handle single
/// precision rotations
bool
UsdGeom_GetOrientationsAndAngularVelocities(
    const UsdAttribute& orientationsAttr,
    const UsdAttribute& angularVelocitiesAttr,
    UsdTimeCode baseTime,
    size_t expectedNumOrientations,
    VtQuatfArray* orientations,
    VtVec3fArray* angularVelocities,
    UsdTimeCode* angularVelocitiesSampleTime,
    UsdPrim const &prim);

/// Compute the scaled time difference based on timeCodesPerSecond
double
UsdGeom_CalculateTimeDelta(
    const UsdTimeCode time,
    const UsdTimeCode sampleTime,
    const double timeCodesPerSecond);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
