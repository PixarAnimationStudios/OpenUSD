//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
