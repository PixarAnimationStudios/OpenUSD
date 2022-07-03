//
// Copyright 2021 Pixar
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
#ifndef PXR_USD_USD_PHYSICS_METRICS_H
#define PXR_USD_USD_PHYSICS_METRICS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usd/common.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \file usdPhysics/metrics.h
///
/// Helper APIs for physics related metrics operations.

/// Return *stage*'s authored *kilogramsPerUnit*, or 1.0 if unauthored.
USDPHYSICS_API
double UsdPhysicsGetStageKilogramsPerUnit(const UsdStageWeakPtr &stage);

/// Return whether *stage* has an authored *kilogramsPerUnit*.
USDPHYSICS_API
bool UsdPhysicsStageHasAuthoredKilogramsPerUnit(const UsdStageWeakPtr &stage);

/// Author *stage*'s *kilogramsPerUnit*.
///
/// \return true if kilogramsPerUnit was successfully set.  The stage's
/// UsdEditTarget must be either its root layer or session layer.
USDPHYSICS_API
bool UsdPhysicsSetStageKilogramsPerUnit(const UsdStageWeakPtr &stage,
                                       double kilogramsPerUnit);

/// Return *true* if the two given metrics are within the provided
/// relative *epsilon* of each other, when you need to know an absolute
/// metric rather than a scaling factor.  
///
/// Use like so:
/// \code
/// double stageUnits = UsdPhysicsGetStageKilogramsPerUnit(stage);
///
/// if (UsdPhysicsMassUnitsAre(stageUnits, UsdPhysicsMassUnits::kilograms))
///     // do something for kilograms
/// else if (UsdPhysicsMassUnitsAre(stageUnits, UsdPhysicsMassUnits::grams))
///     // do something for grams
/// \endcode
///
/// \return *false* if either input is zero or negative, otherwise relative
/// floating-point comparison between the two inputs.
USDPHYSICS_API
bool UsdPhysicsMassUnitsAre(double authoredUnits, double standardUnits,
                              double epsilon = 1e-5);

/// \class UsdPhysicsMassUnits
/// Container class for static double-precision symbols representing common
/// mass units of measure expressed in kilograms.
class UsdPhysicsMassUnits {
public:
    static constexpr double grams = 0.001;
    static constexpr double kilograms = 1.0;
    static constexpr double slugs = 14.5939;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PHYSICS_METRICS_H
