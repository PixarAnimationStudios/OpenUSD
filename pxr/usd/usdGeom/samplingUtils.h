//
// Copyright 2019 Pixar
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
    float* velocityScale,
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
/// \p angularVelocitiesSampleTime is not changed.
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

/// Compute the scaled time difference based on velocity scale and
/// timeCodesPerSecond
float
UsdGeom_CalculateTimeDelta(
    const float velocityScale,
    const UsdTimeCode time,
    const UsdTimeCode sampleTime,
    const double timeCodesPerSecond);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
