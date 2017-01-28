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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomCamera,
        TfType::Bases< UsdGeomXformable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Camera")
    // to find TfType<UsdGeomCamera>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomCamera>("Camera");
}

/* virtual */
UsdGeomCamera::~UsdGeomCamera()
{
}

/* static */
UsdGeomCamera
UsdGeomCamera::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCamera();
    }
    return UsdGeomCamera(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomCamera
UsdGeomCamera::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Camera");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCamera();
    }
    return UsdGeomCamera(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdGeomCamera::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomCamera>();
    return tfType;
}

/* static */
bool 
UsdGeomCamera::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomCamera::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomCamera::GetProjectionAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->projection);
}

UsdAttribute
UsdGeomCamera::CreateProjectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->projection,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetHorizontalApertureAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->horizontalAperture);
}

UsdAttribute
UsdGeomCamera::CreateHorizontalApertureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->horizontalAperture,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetVerticalApertureAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->verticalAperture);
}

UsdAttribute
UsdGeomCamera::CreateVerticalApertureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->verticalAperture,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetHorizontalApertureOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->horizontalApertureOffset);
}

UsdAttribute
UsdGeomCamera::CreateHorizontalApertureOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->horizontalApertureOffset,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetVerticalApertureOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->verticalApertureOffset);
}

UsdAttribute
UsdGeomCamera::CreateVerticalApertureOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->verticalApertureOffset,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetFocalLengthAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->focalLength);
}

UsdAttribute
UsdGeomCamera::CreateFocalLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->focalLength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetClippingRangeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->clippingRange);
}

UsdAttribute
UsdGeomCamera::CreateClippingRangeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->clippingRange,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetClippingPlanesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->clippingPlanes);
}

UsdAttribute
UsdGeomCamera::CreateClippingPlanesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->clippingPlanes,
                       SdfValueTypeNames->Float4Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetFStopAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->fStop);
}

UsdAttribute
UsdGeomCamera::CreateFStopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->fStop,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetFocusDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->focusDistance);
}

UsdAttribute
UsdGeomCamera::CreateFocusDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->focusDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetStereoRoleAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->stereoRole);
}

UsdAttribute
UsdGeomCamera::CreateStereoRoleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->stereoRole,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetShutterOpenAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->shutterOpen);
}

UsdAttribute
UsdGeomCamera::CreateShutterOpenAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->shutterOpen,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetShutterCloseAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->shutterClose);
}

UsdAttribute
UsdGeomCamera::CreateShutterCloseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->shutterClose,
                       SdfValueTypeNames->Double,
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
UsdGeomCamera::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->projection,
        UsdGeomTokens->horizontalAperture,
        UsdGeomTokens->verticalAperture,
        UsdGeomTokens->horizontalApertureOffset,
        UsdGeomTokens->verticalApertureOffset,
        UsdGeomTokens->focalLength,
        UsdGeomTokens->clippingRange,
        UsdGeomTokens->clippingPlanes,
        UsdGeomTokens->fStop,
        UsdGeomTokens->focusDistance,
        UsdGeomTokens->stereoRole,
        UsdGeomTokens->shutterOpen,
        UsdGeomTokens->shutterClose,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomXformable::GetSchemaAttributeNames(true),
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
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

template<class T>
static boost::optional<T> _GetValue(const UsdPrim &prim,
                                    const TfToken &name,
                                    const UsdTimeCode &time)
{
    const UsdAttribute attr = prim.GetAttribute(name);
    if (!attr) {
        TF_WARN("%s attribute on prim %s missing.",
                name.GetText(), prim.GetPath().GetText());
        return boost::none;
    }
    
    T value;
    if (!attr.Get(&value, time)) {
        TF_WARN("Failed to extract value from attribute %s at <%s>.",
                name.GetText(), attr.GetPath().GetText());
        return boost::none;
    }

    return value;
}

static
GfCamera::Projection
_TokenToProjection(const TfToken &token)
{
    if (token == UsdGeomTokens->orthographic) {
        return GfCamera::Orthographic;
    }

    if (token != UsdGeomTokens->perspective) {
        TF_WARN("Unknown projection type %s", token.GetText());
    }

    return GfCamera::Perspective;
}

static
TfToken
_ProjectionToToken(GfCamera::Projection projection)
{
    switch(projection) {
    case GfCamera::Perspective:
        return UsdGeomTokens->perspective;
    case GfCamera::Orthographic:
        return UsdGeomTokens->orthographic;
    default:
        TF_WARN("Unknown projection type %d", projection);
        return TfToken();
    }
}


static
GfRange1f
_Vec2fToRange1f(const GfVec2f &vec)
{
    return GfRange1f(vec[0], vec[1]);
}

static
GfVec2f
_Range1fToVec2f(const GfRange1f &range)
{
    return GfVec2f(range.GetMin(), range.GetMax());
}

static
std::vector<GfVec4f>
_VtArrayVec4fToVector(const VtArray<GfVec4f> &array)
{
    return std::vector<GfVec4f>(array.begin(), array.end());
}

static
VtArray<GfVec4f>
_VectorVec4fToVtArray(const std::vector<GfVec4f> &vec)
{
    VtArray<GfVec4f> result;
    result.assign(vec.begin(), vec.end());
    return result;
}

static
std::vector<GfVec4f>
_TransformClippingPlanes(const std::vector<GfVec4f> &clippingPlanes,
                         const GfMatrix4d &inverseMatrix)
{
    std::vector<GfVec4f> result(clippingPlanes);

    for (size_t i = 0; i < clippingPlanes.size(); i++) {
        // Apply matrix to normal vectors of clipping planes
        const GfVec3f normal = inverseMatrix.TransformDir(
            GfVec3f(clippingPlanes[i][0],
                    clippingPlanes[i][1],
                    clippingPlanes[i][2]));

        result[i][0] = normal[0];
        result[i][1] = normal[1];
        result[i][2] = normal[2];
    }

    return result;
}

GfCamera
UsdGeomCamera::GetCamera(const UsdTimeCode &time, const bool isZup) const
{
    GfCamera camera;

    // If USD has legacy z-Up encoded cameras, convert to y-Up encoded
    // cameras.
    camera.SetTransform(
        isZup ?
        GfCamera::Z_UP_TO_Y_UP_MATRIX * ComputeLocalToWorldTransform(time) :
        ComputeLocalToWorldTransform(time));

    if (const boost::optional<TfToken> projection = _GetValue<TfToken>(
            GetPrim(), UsdGeomTokens->projection, time)) {
        camera.SetProjection(_TokenToProjection(*projection));
    }

    if (const boost::optional<float> horizontalAperture = _GetValue<float>(
            GetPrim(), UsdGeomTokens->horizontalAperture, time)) {
        camera.SetHorizontalAperture(*horizontalAperture);
    }

    if (const boost::optional<float> verticalAperture = _GetValue<float>(
            GetPrim(), UsdGeomTokens->verticalAperture, time)) {
        camera.SetVerticalAperture(*verticalAperture);
    }

    if (const boost::optional<float> horizontalApertureOffset =
        _GetValue<float>(
            GetPrim(), UsdGeomTokens->horizontalApertureOffset, time)) {
        camera.SetHorizontalApertureOffset(*horizontalApertureOffset);
    }

    if (const boost::optional<float> verticalApertureOffset = _GetValue<float>(
            GetPrim(), UsdGeomTokens->verticalApertureOffset, time)) {
        camera.SetVerticalApertureOffset(*verticalApertureOffset);
    }

    if (const boost::optional<float> focalLength = _GetValue<float>(
            GetPrim(), UsdGeomTokens->focalLength, time)) {
        camera.SetFocalLength(*focalLength);
    }

    if (const boost::optional<GfVec2f> clippingRange = _GetValue<GfVec2f>(
            GetPrim(), UsdGeomTokens->clippingRange, time)) {
        camera.SetClippingRange(_Vec2fToRange1f(*clippingRange));
    }

    if (const boost::optional<VtArray<GfVec4f> > clippingPlanes =
        _GetValue<VtArray<GfVec4f> >(
            GetPrim(), UsdGeomTokens->clippingPlanes, time)) {

        // If we have the clipping planes for a z-Up camera, we already
        // applied a rotation by 90 degrees to the camera matrix. For the
        // clipping planes to stay the same, we need to apply the inverse
        // matrix to their normals.
        if (isZup) {
            camera.SetClippingPlanes(
                _TransformClippingPlanes(
                    _VtArrayVec4fToVector(*clippingPlanes),
                    GfCamera::Y_UP_TO_Z_UP_MATRIX));
        } else {
            camera.SetClippingPlanes(_VtArrayVec4fToVector(*clippingPlanes));
        }
    }

    if (const boost::optional<float> fStop = _GetValue<float>(
            GetPrim(), UsdGeomTokens->fStop, time)) {
        camera.SetFStop(*fStop);
    }

    if (const boost::optional<float> focusDistance = _GetValue<float>(
            GetPrim(), UsdGeomTokens->focusDistance, time)) {
        camera.SetFocusDistance(*focusDistance);
    }
    
    return camera;
}

void
UsdGeomCamera::SetFromCamera(const GfCamera &camera, const UsdTimeCode &time)
{
    const GfMatrix4d parentToWorldInverse =
        ComputeParentToWorldTransform(time).GetInverse();

    const GfMatrix4d camMatrix = camera.GetTransform() * parentToWorldInverse;

    MakeMatrixXform().Set(camMatrix, time);
    GetProjectionAttr().Set(_ProjectionToToken(camera.GetProjection()), time);
    GetHorizontalApertureAttr().Set(camera.GetHorizontalAperture(), time);
    GetVerticalApertureAttr().Set(camera.GetVerticalAperture(), time);
    GetHorizontalApertureOffsetAttr().Set(
        camera.GetHorizontalApertureOffset(), time);
    GetVerticalApertureOffsetAttr().Set(
        camera.GetVerticalApertureOffset(), time);
    GetFocalLengthAttr().Set(camera.GetFocalLength(), time);
    GetClippingRangeAttr().Set(
        _Range1fToVec2f(camera.GetClippingRange()), time);

    GetClippingPlanesAttr().Set(
        _VectorVec4fToVtArray(camera.GetClippingPlanes()), time);

    GetFStopAttr().Set(camera.GetFStop(), time);
    GetFocusDistanceAttr().Set(camera.GetFocusDistance(), time);
}

PXR_NAMESPACE_CLOSE_SCOPE
