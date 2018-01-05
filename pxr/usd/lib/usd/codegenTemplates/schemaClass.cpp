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

/* virtual */
{{ cls.cppClassName }}::~{{ cls.cppClassName }}()
{
}

/* static */
{{ cls.cppClassName }}
{{ cls.cppClassName }}::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return {{ cls.cppClassName }}();
    }
    return {{ cls.cppClassName }}(stage->GetPrimAtPath(path));
}

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
{% if cls.isApi %}

/* static */
{{ cls.cppClassName }}
{% if cls.isPrivateApply %}
{% if not cls.isMultipleApply %}
{{ cls.cppClassName }}::_Apply(const UsdStagePtr &stage, const SdfPath &path)
{% else %}
{{ cls.cppClassName }}::_Apply(const UsdStagePtr &stage, const SdfPath &path, const TfToken &name)
{% endif %}
{% else %}
{% if not cls.isMultipleApply %}
{{ cls.cppClassName }}::Apply(const UsdStagePtr &stage, const SdfPath &path)
{% else %}
{{ cls.cppClassName }}::Apply(const UsdStagePtr &stage, const SdfPath &path, const TfToken &name)
{% endif %}
{% endif %}
{
    // Ensure we have a valid stage, path and prim
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return {{ cls.cppClassName }}();
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        TF_CODING_ERROR("Cannot apply an api schema on the pseudoroot");
        return {{ cls.cppClassName }}();
    }

    auto prim = stage->GetPrimAtPath(path);
    if (!prim) {
        TF_CODING_ERROR("Prim at <%s> does not exist.", path.GetText());
        return {{ cls.cppClassName }}();
    }

{% if cls.isMultipleApply %}
    if (!TfIsValidIdentifier(name)) {
        TF_CODING_ERROR("Name %s is not a valid identifier.", name.GetText());
        return {{ cls.cppClassName }}();
    }

    TfToken apiName(std::string("{{ cls.primName }}") 
                    + std::string(":") 
                    + name.GetString());
{% else %}
    TfToken apiName("{{ cls.primName }}");  
{% endif %}

    // Get the current listop at the edit target
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle primSpec = editTarget.GetPrimSpecForScenePath(path);
    SdfTokenListOp listOp = primSpec->GetInfo(UsdTokens->apiSchemas)
                                    .UncheckedGet<SdfTokenListOp>();

    // Append our name to the prepend list, if it doesnt exist locally
    TfTokenVector prepends = listOp.GetPrependedItems();
    if (std::find(prepends.begin(), prepends.end(), apiName) != prepends.end()) { 
        return {{ cls.cppClassName }}();
    }

    SdfTokenListOp prependListOp;
    prepends.push_back(apiName);
    prependListOp.SetPrependedItems(prepends);
    auto result = listOp.ApplyOperations(prependListOp);
    if (!result) {
        TF_CODING_ERROR("Failed to prepend api name to current listop.");
        return {{ cls.cppClassName }}();
    }

    // Set the listop at the current edit target and return the API prim
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
    return {{ cls.cppClassName }}(prim);
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

{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName] %}
UsdAttribute
{{ cls.cppClassName }}::Get{{ Proper(attr.apiName) }}Attr() const
{
    return GetPrim().GetAttribute({{ tokensPrefix }}Tokens->{{ attr.name }});
}

UsdAttribute
{{ cls.cppClassName }}::Create{{ Proper(attr.apiName) }}Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr({{ tokensPrefix }}Tokens->{{ attr.name }},
                       {{ attr.usdType }},
                       /* custom = */ {{ "true" if attr.custom else "false" }},
                       {{ attr.variability }},
                       defaultValue,
                       writeSparsely);
}

{% endfor %}
{% for relName in cls.relOrder %}
{% set rel = cls.rels[relName] %}
UsdRelationship
{{ cls.cppClassName }}::Get{{ Proper(rel.apiName) }}Rel() const
{
    return GetPrim().GetRelationship({{ tokensPrefix }}Tokens->{{ rel.name }});
}

UsdRelationship
{{ cls.cppClassName }}::Create{{ Proper(rel.apiName) }}Rel() const
{
    return GetPrim().CreateRelationship({{ tokensPrefix }}Tokens->{{rel.name}},
                       /* custom = */ {{ "true" if rel.custom else "false" }});
}

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
        {{ tokensPrefix }}Tokens->{{ attr.name }},
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

