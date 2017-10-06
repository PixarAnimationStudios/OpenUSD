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
#include "usdKatana/blindDataObject.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdKatanaBlindDataObject,
        TfType::Bases< UsdSchemaBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("BlindDataObject")
    // to find TfType<UsdKatanaBlindDataObject>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdKatanaBlindDataObject>("BlindDataObject");
}

/* virtual */
UsdKatanaBlindDataObject::~UsdKatanaBlindDataObject()
{
}

/* static */
UsdKatanaBlindDataObject
UsdKatanaBlindDataObject::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdKatanaBlindDataObject();
    }
    return UsdKatanaBlindDataObject(stage->GetPrimAtPath(path));
}

/* static */
UsdKatanaBlindDataObject
UsdKatanaBlindDataObject::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("BlindDataObject");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdKatanaBlindDataObject();
    }
    return UsdKatanaBlindDataObject(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdKatanaBlindDataObject::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdKatanaBlindDataObject>();
    return tfType;
}

/* static */
bool 
UsdKatanaBlindDataObject::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdKatanaBlindDataObject::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdKatanaBlindDataObject::GetTypeAttr() const
{
    return GetPrim().GetAttribute(UsdKatanaTokens->katanaType);
}

UsdAttribute
UsdKatanaBlindDataObject::CreateTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdKatanaTokens->katanaType,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdKatanaBlindDataObject::GetVisibleAttr() const
{
    return GetPrim().GetAttribute(UsdKatanaTokens->katanaVisible);
}

UsdAttribute
UsdKatanaBlindDataObject::CreateVisibleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdKatanaTokens->katanaVisible,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdKatanaBlindDataObject::GetSuppressGroupToAssemblyPromotionAttr() const
{
    return GetPrim().GetAttribute(UsdKatanaTokens->katanaSuppressGroupToAssemblyPromotion);
}

UsdAttribute
UsdKatanaBlindDataObject::CreateSuppressGroupToAssemblyPromotionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdKatanaTokens->katanaSuppressGroupToAssemblyPromotion,
                       SdfValueTypeNames->Bool,
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
UsdKatanaBlindDataObject::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdKatanaTokens->katanaType,
        UsdKatanaTokens->katanaVisible,
        UsdKatanaTokens->katanaSuppressGroupToAssemblyPromotion,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaBase::GetSchemaAttributeNames(true),
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

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // If you update this, you may need to update _KATANA_NAMESPACE_INDEX
    ((kbdNamespace, "katana:fromKlf"))
);

// This value should be set such that:
// ['katana', 'fromKlf', katanaNameSpace, 'attrName']
// katanaNamespace = prop.SplitName()[_KATANA_NAMESPACE_INDEX];
static const size_t _KATANA_NAMESPACE_INDEX = 2;

TfToken UsdKatanaBlindDataObject::GetKbdAttributeNameSpace(const UsdProperty &prop)
{
    std::vector<string> names = prop.SplitName();
    if (names.size() < _KATANA_NAMESPACE_INDEX + 2) {
        return TfToken("");
    }
    return TfToken(names[_KATANA_NAMESPACE_INDEX]);
}

std::string 
UsdKatanaBlindDataObject::GetGroupBuilderKeyForProperty(
        const UsdProperty& prop)
{
    std::vector<std::string> nameParts = prop.SplitName();
    if (nameParts.size() < _KATANA_NAMESPACE_INDEX + 2) {
        return "";
    }

    // unsanitize -- we do this inplace
    TF_FOR_ALL(namePartsIter, nameParts) {
        std::string& nameToken = *namePartsIter;
        if (nameToken.empty()) {
            nameToken = "ERROR_EMPTY_TOKEN";
        }
        else {
            if (nameToken[0] == '_' and nameToken.size() > 1 and
                    isdigit(nameToken[1])) {
                nameToken = nameToken.substr(1);
            }
        }
    }

    // we're getting the name after the katanaNamespace.
    return TfStringJoin(
            nameParts.begin()+_KATANA_NAMESPACE_INDEX+1, 
            nameParts.end(), ".");
}

static std::string
_MakeKbdAttrName(const string &katanaAttrName)
{
    // katana doesn't require everything to be valid identifiers like Sdf does.
    // This function sanitizes and GetGroupBuilderKeyForProperty un-sanitizes
    std::vector<std::string> nameTokens = TfStringSplit(katanaAttrName, ".");

    TF_FOR_ALL(nameTokenIter, nameTokens) {
        std::string& nameToken = *nameTokenIter;
        if (nameToken.empty()) {
            nameToken = "ERROR_EMPTY_TOKEN";
        }
        else {
            if (isdigit(nameToken[0])) {
                nameToken = "_" + nameToken;
            }
        }
    }

    return _tokens->kbdNamespace.GetString() + ":" + TfStringJoin(
            nameTokens, ":");
}

UsdAttribute
UsdKatanaBlindDataObject::CreateKbdAttribute(
    const std::string& katanaAttrName, 
    const SdfValueTypeName &usdType)
{
    std::string fullName = _MakeKbdAttrName(katanaAttrName);
    UsdAttribute attr = GetPrim().CreateAttribute(TfToken(fullName), usdType, 
                                                  /* custom = */ false);
    if (!TF_VERIFY(attr)) {
        return UsdAttribute();
    }
    return attr;
}

std::vector<UsdProperty>
UsdKatanaBlindDataObject::GetKbdAttributes(
    const string &nameSpace) const
{
    std::vector<UsdProperty> props = 
        GetPrim().GetPropertiesInNamespace(_tokens->kbdNamespace);
    
    std::vector<UsdProperty> validProps;
    bool requestedNameSpace = (nameSpace != "");
    TfToken nameSpaceToken(nameSpace);

    TF_FOR_ALL(propItr, props){
        UsdProperty prop = *propItr;
        if (requestedNameSpace and GetKbdAttributeNameSpace(prop) != nameSpaceToken) {
            continue;
        }
        validProps.push_back(prop);
    }
    return validProps;
}

UsdAttribute
UsdKatanaBlindDataObject::GetKbdAttribute(
    const std::string& katanaAttrName)
{
    std::string fullName = _MakeKbdAttrName(katanaAttrName);
    return GetPrim().GetAttribute(TfToken(fullName));
}

bool
UsdKatanaBlindDataObject::_IsCompatible(const UsdPrim &prim) const
{
    // HasA schemas compatible with all types for now.
    return true;
}

bool
UsdKatanaBlindDataObject::IsKbdAttribute(const UsdProperty &attr)
{
    return TfStringStartsWith(attr.GetName(), _tokens->kbdNamespace);
}

PXR_NAMESPACE_CLOSE_SCOPE
