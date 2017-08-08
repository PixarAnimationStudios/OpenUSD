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
#include "pxr/usd/usdGeom/procedural.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomProcedural,
        TfType::Bases< UsdGeomBoundable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Procedural")
    // to find TfType<UsdGeomProcedural>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomProcedural>("Procedural");
}

/* virtual */
UsdGeomProcedural::~UsdGeomProcedural()
{
}

/* static */
UsdGeomProcedural
UsdGeomProcedural::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomProcedural();
    }
    return UsdGeomProcedural(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomProcedural
UsdGeomProcedural::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Procedural");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomProcedural();
    }
    return UsdGeomProcedural(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdGeomProcedural::_GetSchemaType() const {
    return UsdGeomProcedural::schemaType;
}

/* static */
const TfType &
UsdGeomProcedural::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomProcedural>();
    return tfType;
}

/* static */
bool 
UsdGeomProcedural::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomProcedural::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomProcedural::GetClassAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->proceduralClass);
}

UsdAttribute
UsdGeomProcedural::CreateClassAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->proceduralClass,
                       SdfValueTypeNames->Token,
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
UsdGeomProcedural::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->proceduralClass,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomBoundable::GetSchemaAttributeNames(true),
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

TfToken
UsdGeomProcedural::GetProceduralPropertyName(const std::string& baseName) const
{
    static const std::string prefix =
            UsdGeomTokens->procedural.GetString() + ":";
    return TfToken(prefix + baseName);
}

/* static */
UsdGeomProcedural
UsdGeomProcedural::DefineClass(
    const UsdStagePtr &stage, const SdfPath &path, const std::string& className)
{
    // Define
    static TfToken usdPrimTypeName("Procedural");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomProcedural();
    }
    UsdGeomProcedural prim(
        stage->DefinePrim(path, usdPrimTypeName));

    // Get or create the class attribute
    UsdAttribute classAttr = prim.GetClassAttr();
    if (!classAttr.IsValid()) {
        prim.CreateClassAttr();
    }

    if (!className.empty()) {
        const TfToken classToken(className);
        classAttr.Set(classToken);
    }

    return prim;
}

UsdAttribute
UsdGeomProcedural::GetProceduralAttr(const std::string& attr) const
{
    return GetPrim().GetAttribute(GetProceduralPropertyName(attr));
}

UsdAttribute
UsdGeomProcedural::CreateProceduralAttr(const std::string& attr,
                                        const SdfValueTypeName& type,
                                        VtValue const &defaultValue,
                                        bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       GetProceduralPropertyName(attr),
                       type,
                       /* custom = */ true,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

PXR_NAMESPACE_CLOSE_SCOPE


