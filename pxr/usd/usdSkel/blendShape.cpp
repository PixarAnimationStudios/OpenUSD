//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSkelBlendShape,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("BlendShape")
    // to find TfType<UsdSkelBlendShape>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdSkelBlendShape>("BlendShape");
}

/* virtual */
UsdSkelBlendShape::~UsdSkelBlendShape()
{
}

/* static */
UsdSkelBlendShape
UsdSkelBlendShape::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSkelBlendShape();
    }
    return UsdSkelBlendShape(stage->GetPrimAtPath(path));
}

/* static */
UsdSkelBlendShape
UsdSkelBlendShape::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("BlendShape");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSkelBlendShape();
    }
    return UsdSkelBlendShape(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdSkelBlendShape::_GetSchemaKind() const
{
    return UsdSkelBlendShape::schemaKind;
}

/* static */
const TfType &
UsdSkelBlendShape::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSkelBlendShape>();
    return tfType;
}

/* static */
bool 
UsdSkelBlendShape::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSkelBlendShape::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdSkelBlendShape::GetOffsetsAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->offsets);
}

UsdAttribute
UsdSkelBlendShape::CreateOffsetsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->offsets,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelBlendShape::GetNormalOffsetsAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->normalOffsets);
}

UsdAttribute
UsdSkelBlendShape::CreateNormalOffsetsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->normalOffsets,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSkelBlendShape::GetPointIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdSkelTokens->pointIndices);
}

UsdAttribute
UsdSkelBlendShape::CreatePointIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSkelTokens->pointIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
UsdSkelBlendShape::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSkelTokens->offsets,
        UsdSkelTokens->normalOffsets,
        UsdSkelTokens->pointIndices,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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

PXR_NAMESPACE_OPEN_SCOPE

UsdSkelInbetweenShape
UsdSkelBlendShape::CreateInbetween(const TfToken& name) const
{
    return UsdSkelInbetweenShape::_Create(GetPrim(), name);
}


UsdSkelInbetweenShape
UsdSkelBlendShape::GetInbetween(const TfToken& name) const
{
    return UsdSkelInbetweenShape(
        GetPrim().GetAttribute(UsdSkelInbetweenShape::_MakeNamespaced(name)));
}


bool
UsdSkelBlendShape::HasInbetween(const TfToken& name) const
{
    TfToken inbetweenName =
        UsdSkelInbetweenShape::_MakeNamespaced(name, /*quiet*/ true);
    return inbetweenName.IsEmpty() ? false :
        UsdSkelInbetweenShape::IsInbetween(
            GetPrim().GetAttribute(inbetweenName));
}


std::vector<UsdSkelInbetweenShape>
UsdSkelBlendShape::_MakeInbetweens(const std::vector<UsdProperty>& props) const
{
    std::vector<UsdSkelInbetweenShape> shapes;
    shapes.reserve(props.size());
    for(const UsdProperty& prop : props) {
        const UsdAttribute attr = prop.As<UsdAttribute>();
        // The input property list will often include properties within
        // the namespace of inbetween shapes, such as
        // 'inbetweens:shape:normalOffsets' Filter out those cases.
        if (UsdSkelInbetweenShape::IsInbetween(attr)) {
            shapes.push_back(UsdSkelInbetweenShape(attr));
        }
    }
    return shapes;
}


std::vector<UsdSkelInbetweenShape>
UsdSkelBlendShape::GetInbetweens() const
{
    const UsdPrim& prim = GetPrim();
    return _MakeInbetweens(prim ? prim.GetPropertiesInNamespace(
                               UsdSkelInbetweenShape::_GetNamespacePrefix()) :
                           std::vector<UsdProperty>());
}


std::vector<UsdSkelInbetweenShape>
UsdSkelBlendShape::GetAuthoredInbetweens() const
{
    const UsdPrim& prim = GetPrim();
    return _MakeInbetweens(prim ? prim.GetAuthoredPropertiesInNamespace(
                               UsdSkelInbetweenShape::_GetNamespacePrefix()) :
                           std::vector<UsdProperty>());
}


bool
UsdSkelBlendShape::ValidatePointIndices(TfSpan<const int> indices,
                                        size_t numPoints,
                                        std::string* reason)
{
    for (size_t i = 0; i < indices.size(); ++i) {
        const int pointIndex = indices[i];
        if (pointIndex >= 0) {
            if (ARCH_UNLIKELY(static_cast<size_t>(pointIndex) >= numPoints)) {
                if (reason) {
                    *reason = TfStringPrintf(
                        "Index [%d] at element %td >= numPoints [%zu]",
                        pointIndex, i, numPoints);
                }
                return false;
            }
        } else {
            if (reason) {
                *reason = TfStringPrintf("Index [%d] at element %td < 0",
                                         pointIndex, i);
            }
            return false;
        }
    }
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
