//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomBoundable,
        TfType::Bases< UsdGeomXformable > >();
    
}

/* virtual */
UsdGeomBoundable::~UsdGeomBoundable()
{
}

/* static */
UsdGeomBoundable
UsdGeomBoundable::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomBoundable();
    }
    return UsdGeomBoundable(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdGeomBoundable::_GetSchemaKind() const
{
    return UsdGeomBoundable::schemaKind;
}

/* static */
const TfType &
UsdGeomBoundable::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomBoundable>();
    return tfType;
}

/* static */
bool 
UsdGeomBoundable::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomBoundable::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomBoundable::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomBoundable::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->extent,
                       SdfValueTypeNames->Float3Array,
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
UsdGeomBoundable::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->extent,
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

#include "pxr/usd/usdGeom/debugCodes.h" 

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdGeomBoundable::ComputeExtent(const UsdTimeCode &time, 
        VtVec3fArray *extent) const
{
    UsdAttributeQuery extentAttrQuery = UsdAttributeQuery(GetExtentAttr());

    bool success = false;
    if (extentAttrQuery.HasAuthoredValue()) {
        success = extentAttrQuery.Get(extent, time);
        if (success) {
            //validate the result
            success = extent->size() == 2;
            if (!success) {
                TF_WARN("[Boundable Extent] Authored extent for <%s> is of "
                        "size %zu instead of 2.\n", 
                        GetPath().GetString().c_str(), extent->size());
            }
        }
    }

    if (!success) {
        TF_DEBUG(USDGEOM_EXTENT).Msg(
            "[Boundable Extent] WARNING: No valid extent authored for "
            "<%s>. Computing extent from source geometry data dynamically..\n", 
            GetPath().GetString().c_str());
        success = ComputeExtentFromPlugins(*this, time, extent);
        if (!success) {
            TF_DEBUG(USDGEOM_EXTENT).Msg(
                "[Boundable Extent] WARNING: Unable to compute extent for "
                "<%s>.\n", GetPath().GetString().c_str());
        }
    }

    return success;
}

PXR_NAMESPACE_CLOSE_SCOPE
