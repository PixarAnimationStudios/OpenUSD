//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/portalLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxPortalLight,
        TfType::Bases< UsdLuxBoundableLightBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PortalLight")
    // to find TfType<UsdLuxPortalLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxPortalLight>("PortalLight");
}

/* virtual */
UsdLuxPortalLight::~UsdLuxPortalLight()
{
}

/* static */
UsdLuxPortalLight
UsdLuxPortalLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxPortalLight();
    }
    return UsdLuxPortalLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxPortalLight
UsdLuxPortalLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PortalLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxPortalLight();
    }
    return UsdLuxPortalLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxPortalLight::_GetSchemaKind() const
{
    return UsdLuxPortalLight::schemaKind;
}

/* static */
const TfType &
UsdLuxPortalLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxPortalLight>();
    return tfType;
}

/* static */
bool 
UsdLuxPortalLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxPortalLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxPortalLight::GetWidthAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsWidth);
}

UsdAttribute
UsdLuxPortalLight::CreateWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxPortalLight::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsHeight);
}

UsdAttribute
UsdLuxPortalLight::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsHeight,
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
UsdLuxPortalLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsWidth,
        UsdLuxTokens->inputsHeight,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxBoundableLightBase::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdGeom/boundableComputeExtent.h"

PXR_NAMESPACE_OPEN_SCOPE

static bool
_ComputeLocalExtent(const float width, const float height, VtVec3fArray *extent)
{
    if (!extent) {
        return false;
    }

    extent->resize(2);
    (*extent)[1] = GfVec3f(width * 0.5f, height * 0.5f, 0.0f);
    (*extent)[0] = -(*extent)[1];
    return true;
}

static bool 
_ComputeExtent(
    const UsdGeomBoundable &boundable,
    const UsdTimeCode &time,
    const GfMatrix4d *transform,
    VtVec3fArray *extent)
{
    const UsdLuxPortalLight light(boundable);
    if (!TF_VERIFY(light)) {
        return false;
    }

    float width;
    if (!light.GetWidthAttr().Get(&width, time)) {
        return false;
    }

    float height;
    if (!light.GetHeightAttr().Get(&height, time)) {
        return false;
    }

    if (!_ComputeLocalExtent(width, height, extent)) {
        return false;
    }

    if (transform) {
        GfBBox3d bbox(GfRange3d((*extent)[0], (*extent)[1]), *transform);
        GfRange3d range = bbox.ComputeAlignedRange();
        (*extent)[0] = GfVec3f(range.GetMin());
        (*extent)[1] = GfVec3f(range.GetMax());
    }

    return true;
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdLuxPortalLight>(_ComputeExtent);
}

PXR_NAMESPACE_CLOSE_SCOPE
