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
#include "pxr/usd/usdAnimX/fCurve.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdAnimXFCurve,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("FCurve")
    // to find TfType<UsdAnimXFCurve>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdAnimXFCurve>("FCurve");
}

/* virtual */
UsdAnimXFCurve::~UsdAnimXFCurve()
{
}

/* static */
UsdAnimXFCurve
UsdAnimXFCurve::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdAnimXFCurve();
    }
    return UsdAnimXFCurve(stage->GetPrimAtPath(path));
}

/* static */
UsdAnimXFCurve
UsdAnimXFCurve::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("FCurve");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdAnimXFCurve();
    }
    return UsdAnimXFCurve(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdAnimXFCurve::_GetSchemaType() const {
    return UsdAnimXFCurve::schemaType;
}

/* static */
const TfType &
UsdAnimXFCurve::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdAnimXFCurve>();
    return tfType;
}

/* static */
bool 
UsdAnimXFCurve::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdAnimXFCurve::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdAnimXFCurve::GetPreInfinityTypeAttr() const
{
    return GetPrim().GetAttribute(UsdAnimXTokens->preInfinityType);
}

UsdAttribute
UsdAnimXFCurve::CreatePreInfinityTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdAnimXTokens->preInfinityType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdAnimXFCurve::GetPostInfinityTypeAttr() const
{
    return GetPrim().GetAttribute(UsdAnimXTokens->postInfinityType);
}

UsdAttribute
UsdAnimXFCurve::CreatePostInfinityTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdAnimXTokens->postInfinityType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdAnimXFCurve::GetKeyframesAttr() const
{
    return GetPrim().GetAttribute(UsdAnimXTokens->keyframes);
}

UsdAttribute
UsdAnimXFCurve::CreateKeyframesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdAnimXTokens->keyframes,
                       SdfValueTypeNames->DoubleArray,
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
UsdAnimXFCurve::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdAnimXTokens->preInfinityType,
        UsdAnimXTokens->postInfinityType,
        UsdAnimXTokens->keyframes,
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
void 
UsdAnimXFCurve::SetKeyframe(double time, double value)
{
  UsdAttribute attr = GetKeyframesAttr();
  adsk::Keyframe k;
  k.time = time;
  k.value = value;
  k.tanIn.type = adsk::TangentType::Auto;
  k.tanIn.x = 1;
  k.tanIn.y = 0;
  k.tanOut.type = adsk::TangentType::Auto;
  k.tanOut.x = 1;
  k.tanOut.y = 0;
  k.quaternionW = 1.0;
  UsdAnimXKeyframeDesc desc;
  GetKeyframeDescription(k, &desc);
  attr.Set(VtValue(desc.data), UsdTimeCode(desc.time));
}


PXR_NAMESPACE_CLOSE_SCOPE
