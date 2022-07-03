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

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/metrics.h"
#include "pxr/usd/usdPhysics/tokens.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include <cfloat>

PXR_NAMESPACE_OPEN_SCOPE

constexpr double UsdPhysicsMassUnits::grams;
constexpr double UsdPhysicsMassUnits::kilograms;
constexpr double UsdPhysicsMassUnits::slugs;

double
UsdPhysicsGetStageKilogramsPerUnit(const UsdStageWeakPtr &stage)
{
    double units = UsdPhysicsMassUnits::kilograms;
    if (!stage) {
        TF_CODING_ERROR("Invalid UsdStage");
        return units;
    }

    stage->GetMetadata(UsdPhysicsTokens->kilogramsPerUnit, &units);
    return units;
}

bool
UsdPhysicsStageHasAuthoredKilogramsPerUnit(const UsdStageWeakPtr &stage)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid UsdStage");
        return false;
    }

    return stage->HasAuthoredMetadata(UsdPhysicsTokens->kilogramsPerUnit);
}

bool
UsdPhysicsSetStageKilogramsPerUnit(const UsdStageWeakPtr &stage,
                                  double kilogramsPerUnit)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid UsdStage");
        return false;
    }

    return stage->SetMetadata(UsdPhysicsTokens->kilogramsPerUnit, 
            kilogramsPerUnit);
}

bool
UsdPhysicsMassUnitsAre(double authoredUnits, double standardUnits,
                         double epsilon)
{
    if (authoredUnits <= 0 || standardUnits <= 0) {
        return false;
    }

    const double diff = GfAbs(authoredUnits - standardUnits);
    return (diff / authoredUnits < epsilon) && (diff / standardUnits < epsilon);
}

PXR_NAMESPACE_CLOSE_SCOPE
