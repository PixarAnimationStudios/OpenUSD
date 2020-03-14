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
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomSphere,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Sphere")
    // to find TfType<UsdGeomSphere>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomSphere>("Sphere");
}

/* virtual */
UsdGeomSphere::~UsdGeomSphere()
{
}

/* static */
UsdGeomSphere
UsdGeomSphere::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomSphere();
    }
    return UsdGeomSphere(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomSphere
UsdGeomSphere::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Sphere");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomSphere();
    }
    return UsdGeomSphere(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdGeomSphere::_GetSchemaType() const {
    return UsdGeomSphere::schemaType;
}

/* static */
const TfType &
UsdGeomSphere::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomSphere>();
    return tfType;
}

/* static */
bool 
UsdGeomSphere::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomSphere::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomSphere::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radius);
}

UsdAttribute
UsdGeomSphere::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radius,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomSphere::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomSphere::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdGeomSphere::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->radius,
        UsdGeomTokens->extent,
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

#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdGeomSphere::ComputeExtent(double radius, VtVec3fArray* extent)
{
    // Create Sized Extent
    extent->resize(2);

    (*extent)[0] = GfVec3f(-radius);
    (*extent)[1] = GfVec3f(radius);

    return true;
}

bool
UsdGeomSphere::ComputeExtent(double radius, const GfMatrix4d& transform,
    VtVec3fArray* extent)
{
    // Create Sized Extent
    extent->resize(2);

    GfBBox3d bbox = GfBBox3d(
        GfRange3d(GfVec3d(-radius), GfVec3d(radius)), transform);
    GfRange3d range = bbox.ComputeAlignedRange();
    (*extent)[0] = GfVec3f(range.GetMin());
    (*extent)[1] = GfVec3f(range.GetMax());

    return true;
}

static bool
_ComputeExtentForSphere(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent)
{
    const UsdGeomSphere sphereSchema(boundable);
    if (!TF_VERIFY(sphereSchema)) {
        return false;
    }

    double radius;
    if (!sphereSchema.GetRadiusAttr().Get(&radius)) {
        return false;
    }

    if (transform) {
        return UsdGeomSphere::ComputeExtent(radius, *transform, extent);
    } else {
        return UsdGeomSphere::ComputeExtent(radius, extent);
    }
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomSphere>(
        _ComputeExtentForSphere);
}

PXR_NAMESPACE_CLOSE_SCOPE
