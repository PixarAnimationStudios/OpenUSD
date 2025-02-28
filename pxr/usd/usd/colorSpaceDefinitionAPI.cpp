//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/colorSpaceDefinitionAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdColorSpaceDefinitionAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdColorSpaceDefinitionAPI::~UsdColorSpaceDefinitionAPI()
{
}

/* static */
UsdColorSpaceDefinitionAPI
UsdColorSpaceDefinitionAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdColorSpaceDefinitionAPI();
    }
    TfToken name;
    if (!IsColorSpaceDefinitionAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid colorSpaceDefinition path <%s>.", path.GetText());
        return UsdColorSpaceDefinitionAPI();
    }
    return UsdColorSpaceDefinitionAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdColorSpaceDefinitionAPI
UsdColorSpaceDefinitionAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdColorSpaceDefinitionAPI(prim, name);
}

/* static */
std::vector<UsdColorSpaceDefinitionAPI>
UsdColorSpaceDefinitionAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdColorSpaceDefinitionAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdColorSpaceDefinitionAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Name),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_RedChroma),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_GreenChroma),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_BlueChroma),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_WhitePoint),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Gamma),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_LinearBias),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdColorSpaceDefinitionAPI::IsColorSpaceDefinitionAPIPath(
    const SdfPath &path, TfToken *name)
{
    if (!path.IsPropertyPath()) {
        return false;
    }

    std::string propertyName = path.GetName();
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(propertyName);

    // The baseName of the  path can't be one of the 
    // schema properties. We should validate this in the creation (or apply)
    // API.
    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        return false;
    }

    if (tokens.size() >= 2
        && tokens[0] == UsdTokens->colorSpaceDefinition) {
        *name = TfToken(propertyName.substr(
           UsdTokens->colorSpaceDefinition.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdColorSpaceDefinitionAPI::_GetSchemaKind() const
{
    return UsdColorSpaceDefinitionAPI::schemaKind;
}

/* static */
bool
UsdColorSpaceDefinitionAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdColorSpaceDefinitionAPI>(name, whyNot);
}

/* static */
UsdColorSpaceDefinitionAPI
UsdColorSpaceDefinitionAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdColorSpaceDefinitionAPI>(name)) {
        return UsdColorSpaceDefinitionAPI(prim, name);
    }
    return UsdColorSpaceDefinitionAPI();
}

/* static */
const TfType &
UsdColorSpaceDefinitionAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdColorSpaceDefinitionAPI>();
    return tfType;
}

/* static */
bool 
UsdColorSpaceDefinitionAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdColorSpaceDefinitionAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
static inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    return UsdSchemaRegistry::MakeMultipleApplyNameInstance(propName, instanceName);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetNameAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Name));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Name),
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetRedChromaAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_RedChroma));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateRedChromaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_RedChroma),
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetGreenChromaAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_GreenChroma));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateGreenChromaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_GreenChroma),
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetBlueChromaAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_BlueChroma));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateBlueChromaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_BlueChroma),
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetWhitePointAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_WhitePoint));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateWhitePointAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_WhitePoint),
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetGammaAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Gamma));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateGammaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Gamma),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdColorSpaceDefinitionAPI::GetLinearBiasAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_LinearBias));
}

UsdAttribute
UsdColorSpaceDefinitionAPI::CreateLinearBiasAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_LinearBias),
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
UsdColorSpaceDefinitionAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Name,
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_RedChroma,
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_GreenChroma,
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_BlueChroma,
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_WhitePoint,
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_Gamma,
        UsdTokens->colorSpaceDefinition_MultipleApplyTemplate_LinearBias,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

/*static*/
TfTokenVector
UsdColorSpaceDefinitionAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken &instanceName)
{
    const TfTokenVector &attrNames = GetSchemaAttributeNames(includeInherited);
    if (instanceName.IsEmpty()) {
        return attrNames;
    }
    TfTokenVector result;
    result.reserve(attrNames.size());
    for (const TfToken &attrName : attrNames) {
        result.push_back(
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(attrName, instanceName));
    }
    return result;
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

void UsdColorSpaceDefinitionAPI::CreateColorSpaceAttrsWithChroma(
        const GfVec2f& redChroma,
        const GfVec2f& greenChroma,
        const GfVec2f& blueChroma,
        const GfVec2f& whitePoint,
        float gamma, float linearBias)
{
    CreateRedChromaAttr(VtValue(redChroma));
    CreateGreenChromaAttr(VtValue(greenChroma));
    CreateBlueChromaAttr(VtValue(blueChroma));
    CreateWhitePointAttr(VtValue(whitePoint));
    CreateGammaAttr(VtValue(gamma));
    CreateLinearBiasAttr(VtValue(linearBias));
}

void UsdColorSpaceDefinitionAPI::CreateColorSpaceAttrsWithMatrix(
        const GfMatrix3f& rgbToXYZ,
        float gamma, float linearBias)
{
    CreateGammaAttr(VtValue(gamma));
    CreateLinearBiasAttr(VtValue(linearBias));

    GfColorSpace colorSpace(TfToken("temp"), rgbToXYZ, gamma, linearBias);

    std::tuple<GfVec2f, GfVec2f, GfVec2f, GfVec2f> primariesAndWhitePoint =
        colorSpace.GetPrimariesAndWhitePoint();
    GfVec2f redChroma   = std::get<0>(primariesAndWhitePoint);
    GfVec2f greenChroma = std::get<1>(primariesAndWhitePoint);
    GfVec2f blueChroma  = std::get<2>(primariesAndWhitePoint);
    GfVec2f whitePoint  = std::get<3>(primariesAndWhitePoint);

    CreateRedChromaAttr(VtValue(redChroma));
    CreateGreenChromaAttr(VtValue(greenChroma));
    CreateBlueChromaAttr(VtValue(blueChroma));
    CreateWhitePointAttr(VtValue(whitePoint));
}


GfColorSpace UsdColorSpaceDefinitionAPI::ComputeColorSpaceFromDefinitionAttributes() const
{
    do {
        auto r = GetRedChromaAttr();
        if (!r)
            break;
        auto g = GetGreenChromaAttr();
        if (!g)
            break;
        auto b = GetBlueChromaAttr();
        if (!b)
            break;
        auto wp = GetWhitePointAttr();
        if (!wp)
            break;
        auto gm = GetGammaAttr();
        if (!gm)
            break;
        auto lb = GetLinearBiasAttr();
        if (!lb)
            break;

        GfVec2f redChroma;
        GfVec2f greenChroma;
        GfVec2f blueChroma;
        GfVec2f whitePoint;
        float gamma;
        float linearBias;

        r.Get(&redChroma);
        g.Get(&greenChroma);
        b.Get(&blueChroma);
        wp.Get(&whitePoint);
        gm.Get(&gamma);
        lb.Get(&linearBias);

        return GfColorSpace(GetName(),
                            redChroma,
                            greenChroma,
                            blueChroma,
                            whitePoint,
                            gamma,
                            linearBias);
    } while(true);

    return GfColorSpace(GfColorSpaceNames->Raw);
}


PXR_NAMESPACE_CLOSE_SCOPE
