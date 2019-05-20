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
#include "{{ libraryPath }}/{{ cls.GetHeaderFile() }}"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
{% if cls.isApi %}
#include "pxr/usd/usd/tokens.h"
{% endif %}

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
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("{{ cls.usdPrimTypeName }}")
    // to find TfType<{{ cls.cppClassName }}>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, {{ cls.cppClassName }}>("{{ cls.usdPrimTypeName }}");
{% endif %}
}

{% if cls.isApi %}
TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    ({{ cls.primName }})
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
    ({{ cls.propertyNamespacePrefix }})
{% endif %}
);

{% endif %}
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
    TfToken name;
    if (!Is{{ cls.usdPrimTypeName }}Path(path, &name)) {
        TF_CODING_ERROR("Invalid {{ cls.propertyNamespacePrefix }} path <%s>.", path.GetText());
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}

/* static */
bool 
{{ cls.cppClassName }}::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName] %}
        {{ tokensPrefix }}Tokens->{{ attr.name }},
{% endfor %}
{% for relName in cls.relOrder %}
{% set rel = cls.rels[relName] %}
        {{ tokensPrefix }}Tokens->{{ rel.name }},
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
        && tokens[0] == _schemaTokens->{{ cls.propertyNamespacePrefix }}) {
        *name = TfToken(propertyName.substr(
            _schemaTokens->{{ cls.propertyNamespacePrefix }}.GetString().size() + 1));
        return true;
    }

    return false;
}
{% endif %}

/* virtual */
UsdSchemaType {{ cls.cppClassName }}::_GetSchemaType() const {
    return {{ cls.cppClassName }}::schemaType;
}
{% if cls.isAppliedAPISchema %}

/* static */
{{ cls.cppClassName }}
{% if cls.isPrivateApply %}
{% if not cls.isMultipleApply %}
{{ cls.cppClassName }}::_Apply(const UsdPrim &prim)
{% else %}
{{ cls.cppClassName }}::_Apply(const UsdPrim &prim, const TfToken &name)
{% endif %}
{% else %}
{% if not cls.isMultipleApply %}
{{ cls.cppClassName }}::Apply(const UsdPrim &prim)
{% else %}
{{ cls.cppClassName }}::Apply(const UsdPrim &prim, const TfToken &name)
{% endif %}
{% endif %}
{
{% if cls.isMultipleApply %}
    return UsdAPISchemaBase::_MultipleApplyAPISchema<{{ cls.cppClassName }}>(
            prim, _schemaTokens->{{ cls.primName }}, name);
{% else %}
    return UsdAPISchemaBase::_ApplyAPISchema<{{ cls.cppClassName }}>(
            prim, _schemaTokens->{{ cls.primName }});
{% endif %}
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    TfTokenVector identifiers =
        {_schemaTokens->{{ cls.propertyNamespacePrefix }}, instanceName, propName};
    return TfToken(SdfPath::JoinIdentifier(identifiers));
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
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
{% if cls.isMultipleApply %}
_ConcatenateAttributeNames(
    const TfToken instanceName,
    const TfTokenVector& left,
    const TfTokenVector& right)
{% else %}
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{% endif %}
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
{% if cls.isMultipleApply %}

    for (const TfToken attrName : right) {
        result.push_back(
            _GetNamespacedPropertyName(instanceName, attrName));
    }
{% endif %}
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

{% endif %}
/*static*/
const TfTokenVector&
{% if cls.isMultipleApply %}
{{ cls.cppClassName }}::GetSchemaAttributeNames(
    bool includeInherited, const TfToken instanceName)
{% else %}
{{ cls.cppClassName }}::GetSchemaAttributeNames(bool includeInherited)
{% endif %}
{
{% if cls.attrOrder|length > 0 %}
    static TfTokenVector localNames = {
{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName] %}
        {{ tokensPrefix }}Tokens->{{ attr.name }},
{% endfor %}
    };
{% if cls.isMultipleApply %}
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            instanceName,
{# The schema generator has already validated whether our parent is #}
{# a multiple apply schema or UsdSchemaBaseAPI, choose the correct function #}
{# depending on the situation #}
{% if cls.parentCppClassName == "UsdAPISchemaBase" %}
            {{ cls.parentCppClassName }}::GetSchemaAttributeNames(true),
{% else %}
            {{ cls.parentCppClassName }}::GetSchemaAttributeNames(true, instanceName),
{% endif %}
            localNames);
{% else %}
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            {{ cls.parentCppClassName }}::GetSchemaAttributeNames(true),
            localNames);
{% endif %}
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

