//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "{{ libraryPath }}/{{ cls.GetHeaderFile() }}"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

{% if useExportAPI %}
{{ namespaceOpen }}

{% endif %}
// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<{{ cls.cppClassName }},
        TfType::Bases< {{ cls.parentCppClassName }} > >();
    
{% if cls.isConcrete %}
{% if cls.cppClassName != cls.usdPrimTypeName %}
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("{{ cls.usdPrimTypeName }}")
    // to find TfType<{{ cls.cppClassName }}>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, {{ cls.cppClassName }}>("{{ cls.usdPrimTypeName }}");
{% else %}
    // Skip registering an alias as it would be the same as the class name.
{% endif %}
{% endif %}
}

/* virtual */
{{ cls.cppClassName }}::~{{ cls.cppClassName }}()
{
}

{% if not cls.isAPISchemaBase %}
/* static */
{{ cls.cppClassName }}
{{ cls.cppClassName }}::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return {{ cls.cppClassName }}();
    }
{% if cls.isMultipleApply and cls.propertyNamespace %}
    TfToken name;
    if (!Is{{ cls.usdPrimTypeName }}Path(path, &name)) {
        TF_CODING_ERROR("Invalid {{ cls.propertyNamespace.prefix }} path <%s>.", path.GetText());
        return {{ cls.cppClassName }}();
    }
    return {{ cls.cppClassName }}(stage->GetPrimAtPath(path.GetPrimPath()), name);
{% else %}
    return {{ cls.cppClassName }}(stage->GetPrimAtPath(path));
{% endif %}
}

{% if cls.isMultipleApply %}
{{ cls.cppClassName }}
{{ cls.cppClassName }}::Get(const UsdPrim &prim, const TfToken &name)
{
    return {{ cls.cppClassName }}(prim, name);
}

/* static */
std::vector<{{ cls.cppClassName }}>
{{ cls.cppClassName }}::GetAll(const UsdPrim &prim)
{
    std::vector<{{ cls.cppClassName }}> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}

{% endif %}
{% endif %}
{% if cls.isConcrete %}
/* static */
{{ cls.cppClassName }}
{{ cls.cppClassName }}::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("{{ cls.usdPrimTypeName }}");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return {{ cls.cppClassName }}();
    }
    return {{ cls.cppClassName }}(
        stage->DefinePrim(path, usdPrimTypeName));
}
{% endif %}
{% if cls.isMultipleApply and cls.propertyNamespace %}

/* static */
bool 
{{ cls.cppClassName }}::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName] %}
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            {{ tokensPrefix }}Tokens->{{ attr.name }}),
{% endfor %}
{% for relName in cls.relOrder %}
{% set rel = cls.rels[relName] %}
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            {{ tokensPrefix }}Tokens->{{ rel.name }}),
{% endfor %}
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
{{ cls.cppClassName }}::Is{{ cls.usdPrimTypeName }}Path(
    const SdfPath &path, TfToken *name)
{
    if (!path.IsPropertyPath()) {
        return false;
    }

    std::string propertyName = path.GetName();
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(propertyName);

    // The baseName of the {{ cls.usdPrimTypename }} path can't be one of the 
    // schema properties. We should validate this in the creation (or apply)
    // API.
    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        return false;
    }

    if (tokens.size() >= 2
        && tokens[0] == {{ tokensPrefix }}Tokens->{{ cls.propertyNamespace.token }}) {
        *name = TfToken(propertyName.substr(
           {{ tokensPrefix }}Tokens->{{ cls.propertyNamespace.token }}.GetString().size() + 1));
        return true;
    }

    return false;
}
{% endif %}

/* virtual */
UsdSchemaKind {{ cls.cppClassName }}::_GetSchemaKind() const
{
    return {{ cls.cppClassName }}::schemaKind;
}
{% if cls.isAppliedAPISchema %}

/* static */
bool
{% if not cls.isMultipleApply %}
{{ cls.cppClassName }}::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{% else %}
{{ cls.cppClassName }}::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{% endif %}
{
{% if cls.isMultipleApply %}
    return prim.CanApplyAPI<{{ cls.cppClassName }}>(name, whyNot);
{% else %}
    return prim.CanApplyAPI<{{ cls.cppClassName }}>(whyNot);
{% endif %}
}

/* static */
{{ cls.cppClassName }}
{% if not cls.isMultipleApply %}
{{ cls.cppClassName }}::Apply(const UsdPrim &prim)
{% else %}
{{ cls.cppClassName }}::Apply(const UsdPrim &prim, const TfToken &name)
{% endif %}
{
{% if cls.isMultipleApply %}
    if (prim.ApplyAPI<{{ cls.cppClassName }}>(name)) {
        return {{ cls.cppClassName }}(prim, name);
    }
{% else %}
    if (prim.ApplyAPI<{{ cls.cppClassName }}>()) {
        return {{ cls.cppClassName }}(prim);
    }
{% endif %}
    return {{ cls.cppClassName }}();
}
{% endif %}

/* static */
const TfType &
{{ cls.cppClassName }}::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<{{ cls.cppClassName }}>();
    return tfType;
}

/* static */
bool 
{{ cls.cppClassName }}::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
{{ cls.cppClassName }}::_GetTfType() const
{
    return _GetStaticTfType();
}
{% if cls.isMultipleApply and cls.propertyNamespace %}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
static inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    return UsdSchemaRegistry::MakeMultipleApplyNameInstance(propName, instanceName);
}
{% endif %}

{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName] %}
{# Only emit Create/Get API and doxygen if apiName is not empty string. #}
{% if attr.apiName != '' %}
{% if attr.apiGet != "custom" %}
UsdAttribute
{{ cls.cppClassName }}::Get{{ Proper(attr.apiName) }}Attr() const
{
{% if cls.isMultipleApply and cls.propertyNamespace %}
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            {{ tokensPrefix }}Tokens->{{ attr.name }}));
{% else %}
    return GetPrim().GetAttribute({{ tokensPrefix }}Tokens->{{ attr.name }});
{% endif %}
}
{% endif %}

UsdAttribute
{{ cls.cppClassName }}::Create{{ Proper(attr.apiName) }}Attr(VtValue const &defaultValue, bool writeSparsely) const
{
{% if cls.isMultipleApply and cls.propertyNamespace %}
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           {{ tokensPrefix }}Tokens->{{ attr.name }}),
{% else %}
    return UsdSchemaBase::_CreateAttr({{ tokensPrefix }}Tokens->{{ attr.name }},
{% endif %}
                       {{ attr.usdType }},
                       /* custom = */ {{ "true" if attr.custom else "false" }},
                       {{ attr.variability }},
                       defaultValue,
                       writeSparsely);
}

{% endif %}
{% endfor %}
{% for relName in cls.relOrder %}
{% set rel = cls.rels[relName] %}
{# Only emit Create/Get API and doxygen if apiName is not empty string. #}
{% if rel.apiName != '' %}
{% if rel.apiGet != "custom" %}
UsdRelationship
{{ cls.cppClassName }}::Get{{ Proper(rel.apiName) }}Rel() const
{
{% if cls.isMultipleApply and cls.propertyNamespace %}
    return GetPrim().GetRelationship(
        _GetNamespacedPropertyName(
            GetName(),
            {{ tokensPrefix }}Tokens->{{ rel.name }}));
{% else %}
    return GetPrim().GetRelationship({{ tokensPrefix }}Tokens->{{ rel.name }});
{% endif %}
}
{% endif %}

UsdRelationship
{{ cls.cppClassName }}::Create{{ Proper(rel.apiName) }}Rel() const
{
{% if cls.isMultipleApply and cls.propertyNamespace %}
    return GetPrim().CreateRelationship(
                       _GetNamespacedPropertyName(
                           GetName(),
                           {{ tokensPrefix }}Tokens->{{ rel.name }}),
{% else %}
    return GetPrim().CreateRelationship({{ tokensPrefix }}Tokens->{{rel.name}},
{% endif %}
                       /* custom = */ {{ "true" if rel.custom else "false" }});
}

{% endif %}
{% endfor %}
{% if cls.attrOrder|length > 0 %}
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

{% endif %}
/*static*/
const TfTokenVector&
{{ cls.cppClassName }}::GetSchemaAttributeNames(bool includeInherited)
{
{% if cls.attrOrder|length > 0 %}
    static TfTokenVector localNames = {
{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName] %}
{% if attr.apiName != '' %}
        {{ tokensPrefix }}Tokens->{{ attr.name }},
{% endif %}
{% endfor %}
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            {{ cls.parentCppClassName }}::GetSchemaAttributeNames(true),
            localNames);
{% else %}
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        {{ cls.parentCppClassName }}::GetSchemaAttributeNames(true);
{% endif %}

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

{% if cls.isMultipleApply %}
/*static*/
TfTokenVector
{{ cls.cppClassName }}::GetSchemaAttributeNames(
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

{% endif %}
{% if useExportAPI %}
{{ namespaceClose }}

{% endif %}
// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
{% if useExportAPI %}
//
// Just remember to wrap code in the appropriate delimiters:
// '{{ namespaceOpen }}', '{{ namespaceClose }}'.
{% endif %}
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

