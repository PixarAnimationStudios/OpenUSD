//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
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

/* virtual */
UsdSchemaKind UsdGeomCamera::_GetSchemaKind() const
{
    return UsdGeomCamera::schemaKind;
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

UsdAttribute
UsdGeomCamera::GetExposureAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->exposure);
}

UsdAttribute
UsdGeomCamera::CreateExposureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->exposure,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetExposureIsoAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->exposureIso);
}

UsdAttribute
UsdGeomCamera::CreateExposureIsoAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->exposureIso,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetExposureTimeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->exposureTime);
}

UsdAttribute
UsdGeomCamera::CreateExposureTimeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->exposureTime,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetExposureFStopAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->exposureFStop);
}

UsdAttribute
UsdGeomCamera::CreateExposureFStopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->exposureFStop,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCamera::GetExposureResponsivityAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->exposureResponsivity);
}

UsdAttribute
UsdGeomCamera::CreateExposureResponsivityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->exposureResponsivity,
                       SdfValueTypeNames->Float,
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
        UsdGeomTokens->exposure,
        UsdGeomTokens->exposureIso,
        UsdGeomTokens->exposureTime,
        UsdGeomTokens->exposureFStop,
        UsdGeomTokens->exposureResponsivity,
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
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

template<class T>
static std::optional<T> _GetValue(const UsdPrim &prim,
                                  const TfToken &name,
                                  const UsdTimeCode &time)
{
    const UsdAttribute attr = prim.GetAttribute(name);
    if (!attr) {
        TF_WARN("%s attribute on prim %s missing.",
                name.GetText(), prim.GetPath().GetText());
        return {};
    }
    
    T value;
    if (!attr.Get(&value, time)) {
        TF_WARN("Failed to extract value from attribute %s at <%s>.",
                name.GetText(), attr.GetPath().GetText());
        return {};
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

GfCamera
UsdGeomCamera::GetCamera(const UsdTimeCode &time) const
{
    GfCamera camera;

    camera.SetTransform(
        ComputeLocalToWorldTransform(time));

    if (const std::optional<TfToken> projection = _GetValue<TfToken>(
            GetPrim(), UsdGeomTokens->projection, time)) {
        camera.SetProjection(_TokenToProjection(*projection));
    }

    if (const std::optional<float> horizontalAperture = _GetValue<float>(
            GetPrim(), UsdGeomTokens->horizontalAperture, time)) {
        camera.SetHorizontalAperture(*horizontalAperture);
    }

    if (const std::optional<float> verticalAperture = _GetValue<float>(
            GetPrim(), UsdGeomTokens->verticalAperture, time)) {
        camera.SetVerticalAperture(*verticalAperture);
    }

    if (const std::optional<float> horizontalApertureOffset =
        _GetValue<float>(
            GetPrim(), UsdGeomTokens->horizontalApertureOffset, time)) {
        camera.SetHorizontalApertureOffset(*horizontalApertureOffset);
    }

    if (const std::optional<float> verticalApertureOffset = _GetValue<float>(
            GetPrim(), UsdGeomTokens->verticalApertureOffset, time)) {
        camera.SetVerticalApertureOffset(*verticalApertureOffset);
    }

    if (const std::optional<float> focalLength = _GetValue<float>(
            GetPrim(), UsdGeomTokens->focalLength, time)) {
        camera.SetFocalLength(*focalLength);
    }

    if (const std::optional<GfVec2f> clippingRange = _GetValue<GfVec2f>(
            GetPrim(), UsdGeomTokens->clippingRange, time)) {
        camera.SetClippingRange(_Vec2fToRange1f(*clippingRange));
    }

    if (const std::optional<VtArray<GfVec4f> > clippingPlanes =
        _GetValue<VtArray<GfVec4f> >(
            GetPrim(), UsdGeomTokens->clippingPlanes, time)) {

        camera.SetClippingPlanes(_VtArrayVec4fToVector(*clippingPlanes));
    }

    if (const std::optional<float> fStop = _GetValue<float>(
            GetPrim(), UsdGeomTokens->fStop, time)) {
        camera.SetFStop(*fStop);
    }

    if (const std::optional<float> focusDistance = _GetValue<float>(
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

    UsdGeomXformOp xformOp = MakeMatrixXform();
    if (!xformOp) {
        // The returned xformOp may be invalid if there are xform op
        // opinions in the composed layer stack stronger than that of
        // the current edit target.
        return;
    }
    xformOp.Set(camMatrix, time);

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

float
UsdGeomCamera::ComputeLinearExposureScale(UsdTimeCode time) const 
{   
    float exposureTime = 1.0f;
    float exposureIso = 100.0f;
    float exposureFStop = 1.0f;
    float exposureResponsivity = 1.0f;
    float exposureExponent = 0.0f;

    GetExposureTimeAttr().Get(&exposureTime, time);
    GetExposureIsoAttr().Get(&exposureIso, time);
    GetExposureFStopAttr().Get(&exposureFStop, time);
    GetExposureResponsivityAttr().Get(&exposureResponsivity, time);
    GetExposureAttr().Get(&exposureExponent, time);

    return (exposureTime * exposureIso * powf(2.0f, exposureExponent) * 
            exposureResponsivity) / (100.0f * exposureFStop * exposureFStop);
}

PXR_NAMESPACE_CLOSE_SCOPE
