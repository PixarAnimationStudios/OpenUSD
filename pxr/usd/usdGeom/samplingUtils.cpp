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
#include "pxr/usd/usdGeom/samplingUtils.h"
#include "pxr/usd/usdGeom/motionAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

// Get the authored data of an attribute at the lower bracketing timesample of a
// given base time. Fails if the attribute is not authored. If baseTime is
// UsdTimeCode.Default() or the attribute has no time samples, the attribute is
// sampled at the UsdTimeCode.Default().
template<class T>
static bool
_GetAttrForTransforms(
    const UsdAttribute& attr,
    UsdTimeCode baseTime,
    UsdTimeCode* attrSampleTime,
    double* lowerTimeValue,
    double* upperTimeValue,
    bool* attrHasSamples,
    T* attrData)
{
    TRACE_FUNCTION();

    if (baseTime.IsNumeric()) {

        double sampleTimeValue = 0.0;
        double sampleUpperTimeValue = 0.0;
        bool hasSamples;
        if (!attr.GetBracketingTimeSamples(
                baseTime.GetValue(),
                &sampleTimeValue,
                &sampleUpperTimeValue,
                &hasSamples)) {
            return false;
        }

        UsdTimeCode sampleTime = UsdTimeCode::Default();
        if (hasSamples) {
            sampleTime = UsdTimeCode(sampleTimeValue);
        }

        if (!attr.Get(attrData, sampleTime)) {
            return false;
        }

        // if the basetime is exactly at a sampled value, increase basetime by an 
        // "epsilon" value based on the maximum time value and calculate bracketed 
        // time sample values again

        if (GfIsClose(
                sampleTimeValue,
                sampleUpperTimeValue,
                std::numeric_limits<double>::epsilon())) {
            double timeValueEpsilon = baseTime.GetValue() + UsdTimeCode::SafeStep();
            UsdTimeCode baseTimeEpsilon = UsdTimeCode(timeValueEpsilon);
            if (!attr.GetBracketingTimeSamples(
                baseTimeEpsilon.GetValue(),
                &sampleTimeValue,
                &sampleUpperTimeValue,
                &hasSamples)) {
                return false;
            }
        }

        *attrSampleTime = sampleTime;
        *lowerTimeValue = sampleTimeValue;
        *upperTimeValue = sampleUpperTimeValue;
        *attrHasSamples = hasSamples;

    } else {

        // baseTime is UsdTimeCode.Default()
        if (!attr.Get(attrData, baseTime)) {
            return false;
        }
        *attrSampleTime = baseTime;
        *lowerTimeValue = baseTime.GetValue();
        *upperTimeValue = baseTime.GetValue();
        *attrHasSamples = false;

    }

    return true;
}

// Check if the bracketing time samples for two different attributes are 
// aligned. Fail if they are not aligned or if the number of time samples
// do not match. Also check if the size of the array is correct.
static bool
_CheckSampleAlignment(
    bool attribute1HasSamples,
    double attribute1LowerTimeValue,
    double attribute1UpperTimeValue,
    UsdTimeCode attribute1SampleTime,
    double attribute2LowerTimeValue,
    double attribute2UpperTimeValue,
    UsdTimeCode attribute2SampleTime,
    VtValue attrData,
    size_t correctAttrDataLength,
    bool* alignmentValid,
    bool* attrCorrectLength)
{
    // boolean value indicating whether or not the bracketing time samples for
    // position and velocity are equivalent
    bool bracketingTimeSamplesAligned = attribute1HasSamples && GfIsClose(
            attribute1LowerTimeValue, 
            attribute2LowerTimeValue, 
            std::numeric_limits<double>::epsilon()) 
            && GfIsClose(
            attribute1UpperTimeValue,
            attribute2UpperTimeValue,
            std::numeric_limits<double>::epsilon());

    *alignmentValid = true;
    *attrCorrectLength = true;

    if (!bracketingTimeSamplesAligned || !GfIsClose(
            attribute1SampleTime.GetValue(),
            attribute2SampleTime.GetValue(),
            std::numeric_limits<double>::epsilon())) {
        *alignmentValid = false;
    }

    if (attrData.GetArraySize() != correctAttrDataLength) {
        *attrCorrectLength = false;
    }

    if (!(*alignmentValid) || !(*attrCorrectLength)) {
      return false;
    }
    return true;
}

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
    UsdPrim const &prim)
{
    // Get positions attribute and check array size

    UsdTimeCode positionsSampleTime;
    double positionsLowerTimeValue = 0.0;
    double positionsUpperTimeValue = 0.0;
    bool positionsHasSamples;

    if (!_GetAttrForTransforms<VtVec3fArray>(
            positionsAttr,
            baseTime,
            &positionsSampleTime,
            &positionsLowerTimeValue,
            &positionsUpperTimeValue,
            &positionsHasSamples,
            positions)) {
        TF_WARN("%s -- no positions", prim.GetPath().GetText());
        return false;
    }

    size_t correctAttrDataLength = positions->size();

    if (expectedNumPositions != 0) {
        if (positions->size() != expectedNumPositions) {
            TF_WARN("%s -- found [%zu] positions, but expected [%zu]",
                prim.GetPath().GetText(),
                positions->size(),
                expectedNumPositions);
            return false;
        }
    }

    // Get velocities attribute and check sample alignment with positions
    // attribute and array size

    double velocitiesLowerTimeValue = 0.0;
    double velocitiesUpperTimeValue = 0.0;
    bool velocitiesHasSamples = true;
    bool velocitiesAlignmentValid;
    bool velocitiesCorrectLength;

    if (!positionsHasSamples || !_GetAttrForTransforms<VtVec3fArray>(
            velocitiesAttr,
            baseTime,
            velocitiesSampleTime,
            &velocitiesLowerTimeValue,
            &velocitiesUpperTimeValue,
            &velocitiesHasSamples,
            velocities)) {
        velocities->clear();
    }
    if (!_CheckSampleAlignment(
            velocitiesHasSamples,
            positionsLowerTimeValue,
            positionsUpperTimeValue,
            positionsSampleTime,
            velocitiesLowerTimeValue,
            velocitiesUpperTimeValue,
            *velocitiesSampleTime,
            VtValue(*velocities),
            correctAttrDataLength,
            &velocitiesAlignmentValid,
            &velocitiesCorrectLength)) {
        if (!velocities->empty() && !velocitiesAlignmentValid) {
            TF_WARN("%s -- velocity samples are not aligned with position samples",
                prim.GetPath().GetText());
        }
        if (!velocities->empty() && 
                velocitiesAlignmentValid && 
                !velocitiesCorrectLength) {
            TF_WARN("%s -- found [%zu] velocities, but expected [%zu]",
                prim.GetPath().GetText(),
                velocities->size(),
                correctAttrDataLength);
        }
        velocities->clear();
    }


    // Get accelerations attribute and check sample alignment with velocities
    // attribute and array size

    UsdTimeCode accelerationsSampleTime;
    double accelerationsLowerTimeValue = 0.0;
    double accelerationsUpperTimeValue = 0.0;
    bool accelerationsHasSamples = true;
    bool accelerationsAlignmentValid;
    bool accelerationsCorrectLength;

    if (!velocitiesHasSamples || (velocities->size() == 0) 
            || !_GetAttrForTransforms<VtVec3fArray>(
                accelerationsAttr,
                baseTime,
                &accelerationsSampleTime,
                &accelerationsLowerTimeValue,
                &accelerationsUpperTimeValue,
                &accelerationsHasSamples,
                accelerations)) {
        accelerations->clear();
    }
    if (!_CheckSampleAlignment(
            accelerationsHasSamples,
            velocitiesLowerTimeValue,
            velocitiesUpperTimeValue,
            *velocitiesSampleTime,
            accelerationsLowerTimeValue,
            accelerationsUpperTimeValue,
            accelerationsSampleTime,
            VtValue(*accelerations),
            correctAttrDataLength,
            &accelerationsAlignmentValid,
            &accelerationsCorrectLength)) {
        if (!accelerations->empty() && !accelerationsAlignmentValid) {
            TF_WARN("%s -- acceleration samples are not aligned with velocity samples",
                prim.GetPath().GetText());
        }
        if (!accelerations->empty() && 
                accelerationsAlignmentValid && 
                !accelerationsCorrectLength) {
            TF_WARN("%s -- found [%zu] accelerations, but expected [%zu]",
                prim.GetPath().GetText(),
                accelerations->size(),
                correctAttrDataLength);
        }
        accelerations->clear();
    }

    *velocityScale = UsdGeomMotionAPI(prim).ComputeVelocityScale(
        baseTime); 

    return true;
}

bool
UsdGeom_GetOrientationsAndAngularVelocities(
    const UsdAttribute& orientationsAttr,
    const UsdAttribute& angularVelocitiesAttr,
    UsdTimeCode baseTime,
    size_t expectedNumOrientations,
    VtQuathArray* orientations,
    VtVec3fArray* angularVelocities,
    UsdTimeCode* angularVelocitiesSampleTime,
    UsdPrim const &prim)
{
    // Get orientations attribute and check array size

    UsdTimeCode orientationsSampleTime;
    double orientationsLowerTimeValue = 0.0;
    double orientationsUpperTimeValue = 0.0;
    bool orientationsHasSamples = true;

    if (!_GetAttrForTransforms<VtQuathArray>(
            orientationsAttr,
            baseTime,
            &orientationsSampleTime,
            &orientationsLowerTimeValue,
            &orientationsUpperTimeValue,
            &orientationsHasSamples,
            orientations)) {
        return false;
    }

    size_t correctAttrDataLength = orientations->size();

    if (expectedNumOrientations != 0) {
        if (orientations->size() != expectedNumOrientations) {
            TF_WARN("%s -- found [%zu] orientations, but expected [%zu]",
                prim.GetPath().GetText(),
                orientations->size(),
                expectedNumOrientations);
            return false;
        }
    }

    // Get angular velocities attribute and check sample alignment
    // with orientations attribute and array size

    double angularVelocitiesLowerTimeValue = 0.0;
    double angularVelocitiesUpperTimeValue = 0.0;
    bool angularVelocitiesHasSamples = true;
    bool angularVelocitiesAlignmentValid;
    bool angularVelocitiesCorrectLength;

    if (!orientationsHasSamples || !_GetAttrForTransforms<VtVec3fArray>(
            angularVelocitiesAttr,
            baseTime,
            angularVelocitiesSampleTime,
            &angularVelocitiesLowerTimeValue,
            &angularVelocitiesUpperTimeValue,
            &angularVelocitiesHasSamples,
            angularVelocities)) {
        angularVelocities->clear();
    }
    if (!_CheckSampleAlignment(
            angularVelocitiesHasSamples,
            orientationsLowerTimeValue,
            orientationsUpperTimeValue,
            orientationsSampleTime,
            angularVelocitiesLowerTimeValue,
            angularVelocitiesUpperTimeValue,
            *angularVelocitiesSampleTime,
            VtValue(*angularVelocities),
            correctAttrDataLength,
            &angularVelocitiesAlignmentValid,
            &angularVelocitiesCorrectLength)) {
        if (!angularVelocities->empty() && !angularVelocitiesAlignmentValid) {
            TF_WARN("%s -- angular velocity samples are not aligned with orientation samples",
                prim.GetPath().GetText());
        }
        if (!angularVelocities->empty() && 
                angularVelocitiesAlignmentValid && 
                !angularVelocitiesCorrectLength) {
            TF_WARN("%s -- found [%zu] angular velocities, but expected [%zu]",
                prim.GetPath().GetText(),
                angularVelocities->size(),
                correctAttrDataLength);
        }
        angularVelocities->clear();
    }

    return true;
}


bool
UsdGeom_GetScales(
    const UsdAttribute& scalesAttr,
    const UsdTimeCode baseTime,
    size_t expectedScales,
    VtVec3fArray* scales,
    UsdPrim const &prim)
{
    TRACE_FUNCTION();

    // We don't currently support an attribute which linearly changes the
    // scale (as velocity does for position). Instead, we lock the scale to
    // the last authored value without performing any interpolation.

    // Get scales attribute and check array size

    UsdTimeCode scalesSampleTime;
    bool scalesHasSamples;
    // dummy values for passing into _GetAttrForTransforms
    // when we do not need to use the resulting bracketing time values
    double dummyLowerTimeValue = 0.0;
    double dummyUpperTimeValue = 0.0;

    if (!_GetAttrForTransforms<VtVec3fArray>(
            scalesAttr,
            baseTime,
            &scalesSampleTime,
            &dummyLowerTimeValue,
            &dummyUpperTimeValue,
            &scalesHasSamples,
            scales)) {
        return false;
    }

    if (scales->size() != expectedScales) {
        TF_WARN("%s -- found [%zu] scales, but expected [%zu]",
            prim.GetPath().GetText(),
            scales->size(),
            expectedScales);
    }

    return true;
}

float
UsdGeom_CalculateTimeDelta(
    const float velocityScale,
    const UsdTimeCode time,
    const UsdTimeCode sampleTime,
    const double timeCodesPerSecond)
{
    return velocityScale * static_cast<float>(
            (time.GetValue() - sampleTime.GetValue())
            / timeCodesPerSecond);
}

PXR_NAMESPACE_CLOSE_SCOPE
