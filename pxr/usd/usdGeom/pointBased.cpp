//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomPointBased,
        TfType::Bases< UsdGeomGprim > >();
    
}

/* virtual */
UsdGeomPointBased::~UsdGeomPointBased()
{
}

/* static */
UsdGeomPointBased
UsdGeomPointBased::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPointBased();
    }
    return UsdGeomPointBased(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdGeomPointBased::_GetSchemaKind() const {
    return UsdGeomPointBased::schemaKind;
}

/* virtual */
UsdSchemaKind UsdGeomPointBased::_GetSchemaType() const {
    return UsdGeomPointBased::schemaType;
}

/* static */
const TfType &
UsdGeomPointBased::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomPointBased>();
    return tfType;
}

/* static */
bool 
UsdGeomPointBased::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomPointBased::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomPointBased::GetPointsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->points);
}

UsdAttribute
UsdGeomPointBased::CreatePointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->points,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointBased::GetVelocitiesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->velocities);
}

UsdAttribute
UsdGeomPointBased::CreateVelocitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->velocities,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointBased::GetAccelerationsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->accelerations);
}

UsdAttribute
UsdGeomPointBased::CreateAccelerationsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->accelerations,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointBased::GetNormalsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->normals);
}

UsdAttribute
UsdGeomPointBased::CreateNormalsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->normals,
                       SdfValueTypeNames->Normal3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdGeomPointBased::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->points,
        UsdGeomTokens->velocities,
        UsdGeomTokens->accelerations,
        UsdGeomTokens->normals,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomGprim::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdGeom/samplingUtils.h"
#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/usd/usdGeom/motionAPI.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/reduce.h"

PXR_NAMESPACE_OPEN_SCOPE

TfToken 
UsdGeomPointBased::GetNormalsInterpolation() const
{
    // Because normals is a builtin, we don't need to check validity
    // of the attribute before using it
    TfToken interp;
    if (GetNormalsAttr().GetMetadata(UsdGeomTokens->interpolation, &interp)){
        return interp;
    }
    
    return UsdGeomTokens->vertex;
}

bool
UsdGeomPointBased::SetNormalsInterpolation(TfToken const &interpolation)
{
    if (UsdGeomPrimvar::IsValidInterpolation(interpolation)){
        return GetNormalsAttr().SetMetadata(UsdGeomTokens->interpolation, 
                                            interpolation);
    }

    TF_CODING_ERROR("Attempt to set invalid interpolation "
                     "\"%s\" for normals attr on prim %s",
                     interpolation.GetText(),
                     GetPrim().GetPath().GetString().c_str());
    
    return false;
}

namespace {
template <typename Reduction>
bool 
_ComputeExtentImpl(const VtVec3fArray& points, VtVec3fArray* extent,
                   Reduction&& reduction)
{
    extent->resize(2);

    // Calculate bounds
    GfRange3d bbox = WorkParallelReduceN(
        GfRange3d(),
        points.size(),
        std::forward<Reduction>(reduction),
        [](GfRange3d lhs, GfRange3d rhs){
            return GfRange3d::GetUnion(lhs, rhs);
        },
        /*grainSize=*/ 500
    );

    (*extent)[0] = GfVec3f(bbox.GetMin());
    (*extent)[1] = GfVec3f(bbox.GetMax());

    return true;
}
} // end anonymous namespace

bool
UsdGeomPointBased::ComputeExtent(const VtVec3fArray& points,
    VtVec3fArray* extent)
{
    return _ComputeExtentImpl(points, extent,
        [&points](size_t b, size_t e, GfRange3d init){
            for (size_t i = b; i != e; ++i) {
                init.UnionWith(points[i]);
            }
            return init;
        }
    );
}

bool
UsdGeomPointBased::ComputeExtent(const VtVec3fArray& points,
    const GfMatrix4d& transform, VtVec3fArray* extent)
{
    return _ComputeExtentImpl(points, extent,
        [&points, &transform](size_t b, size_t e, GfRange3d init){
            for (size_t i = b; i != e; ++i) {
                init.UnionWith(transform.Transform(points[i]));
            }
            return init;
        }
    );
}

static bool
_ComputeExtentForPointBased(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent) 
{
    const UsdGeomPointBased pointBased(boundable);
    if (!TF_VERIFY(pointBased)) {
        return false;
    }

    VtVec3fArray points;
    if (!pointBased.GetPointsAttr().Get(&points, time)) {
        return false;
    }

    if (transform) {
        return UsdGeomPointBased::ComputeExtent(points, *transform, extent);
    } else {
        return UsdGeomPointBased::ComputeExtent(points, extent);
    }
}

bool
UsdGeomPointBased::ComputePointsAtTime(
    VtArray<GfVec3f>* points,
    const UsdTimeCode time,
    const UsdTimeCode baseTime) const
{
    std::vector<VtVec3fArray> pointsArray;
    std::vector<UsdTimeCode> times({time});
    if (!ComputePointsAtTimes(&pointsArray,
                            times,
                            baseTime)) {
        return false;
    }
    *points = pointsArray.at(0);

    return true;
}

bool
UsdGeomPointBased::ComputePointsAtTimes(
    std::vector<VtArray<GfVec3f>>* pointsArray,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime) const
{
    size_t numSamples = times.size();
    for (auto time : times) {
        if (time.IsNumeric() != baseTime.IsNumeric()) {
            TF_CODING_ERROR(
                "%s -- all sample times in times and baseTime must either all "
                "be numeric or all be default",
                GetPrim().GetPath().GetText());
        }
    }

    VtVec3fArray positions;
    VtVec3fArray velocities;
    UsdTimeCode velocitiesSampleTime;
    VtVec3fArray accelerations;
    float velocityScale;

    // by passing in 0 to expectedNumPositions we default the expected number
    // of positions to the number of points in the points attribute
    if (!UsdGeom_GetPositionsVelocitiesAndAccelerations(
            GetPointsAttr(),
            GetVelocitiesAttr(),
            GetAccelerationsAttr(),
            baseTime,
            0, 
            &positions,
            &velocities,
            &velocitiesSampleTime,
            &accelerations,
            &velocityScale,
            GetPrim())) {
        return false;
    }

    size_t numPoints = positions.size();
    if (numPoints == 0) {
        pointsArray->clear();
        pointsArray->resize(numSamples);
        return true;
    }

    UsdStageWeakPtr stage = GetPrim().GetStage();

    std::vector<VtArray<GfVec3f>> pointsArrayData;
    pointsArrayData.resize(numSamples);
    bool useInterpolated = velocities.empty();
    for (size_t i = 0; i < numSamples; i++) {

        UsdTimeCode time = times[i];
        VtArray<GfVec3f>* points = &(pointsArrayData[i]);

        // If there are no valid velocities or angular velocities, we fallback to
        // "standard" computation logic (linear interpolation between samples).
        if (useInterpolated) {

            // Try to fetch the points at the sample time. If this fails or the 
            // fetched data don't have the correct topology, we fallback to the
            // data from the base time.

            VtVec3fArray interpolatedPoints;
            if (GetPointsAttr().Get(&interpolatedPoints, time)
                    && interpolatedPoints.size() == numPoints) {
                positions = interpolatedPoints;
            }

        }

        if (!ComputePointsAtTime(
                points,
                stage,
                time,
                positions,
                velocities,
                velocitiesSampleTime,
                accelerations,
                velocityScale)) {
            return false;
        }

    }

    *pointsArray = pointsArrayData;
    return true;
}

bool
UsdGeomPointBased::ComputePointsAtTime(
    VtArray<GfVec3f>* points,
    UsdStageWeakPtr& stage,
    UsdTimeCode time,
    const VtVec3fArray& positions,
    const VtVec3fArray& velocities,
    UsdTimeCode velocitiesSampleTime,
    const VtVec3fArray& accelerations,
    float velocityScale)
{
    size_t numPoints = positions.size();

    const double timeCodesPerSecond = stage->GetTimeCodesPerSecond();
    const float velocityTimeDelta = UsdGeom_CalculateTimeDelta(
                                      velocityScale,
                                      time,
                                      velocitiesSampleTime,
                                      timeCodesPerSecond);

    points->resize(numPoints);

    const auto computePoints = [&velocityTimeDelta, 
        &positions, &velocities, &accelerations, 
        &points] (size_t start, size_t end) {
        for (size_t pointId = start ; pointId < end ; ++pointId) {
            GfVec3f translation = positions[pointId];
            if (velocities.size() != 0) {
                GfVec3f velocity = velocities[pointId];
                if (accelerations.size() != 0) {
                    velocity += velocityTimeDelta * accelerations[pointId] * 0.5;
                }
                translation += velocityTimeDelta * velocity;
            }
            (*points)[pointId] = translation;

        }
    };

    {
        WorkParallelForN(numPoints, computePoints);
    }

    return true;
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomPointBased>(
        _ComputeExtentForPointBased);
}

PXR_NAMESPACE_CLOSE_SCOPE
