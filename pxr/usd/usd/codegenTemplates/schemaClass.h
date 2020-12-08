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
#ifndef {{ Upper(libraryName) }}_GENERATED_{{ Upper(cls.className) }}_H
#define {{ Upper(libraryName) }}_GENERATED_{{ Upper(cls.className) }}_H

/// \file {{ libraryName }}/{{ cls.GetHeaderFile() }}

{% if useExportAPI %}
#include "pxr/pxr.h"
#include "{{ libraryPath }}/api.h"
{% endif %}
#include "{{ cls.parentLibPath }}/{{ cls.GetParentHeaderFile() }}"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
{% if cls.tokens -%}
#include "{{ libraryPath }}/tokens.h"
{% endif %}
{% if cls.extraIncludes -%}
{{ cls.extraIncludes }}
{% endif %}

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

{% if useExportAPI %}
{{ namespaceOpen }}

{% endif %}
class SdfAssetPath;

// -------------------------------------------------------------------------- //
// {{ Upper(cls.usdPrimTypeName) }}{{' ' * (74 - cls.usdPrimTypeName|count)}} //
// -------------------------------------------------------------------------- //

/// \class {{ cls.cppClassName }}
///
{% if cls.doc -%}
/// {{ cls.doc }}
{% endif %}
{% if cls.doc and hasTokenAttrs -%}
///
{%endif%}
{% if hasTokenAttrs -%}
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref {{ tokensPrefix }}Tokens.
/// So to set an attribute to the value "rightHanded", use {{ tokensPrefix }}Tokens->rightHanded
/// as the value.
{% endif %}
///
class {{ cls.cppClassName }} : public {{ cls.parentCppClassName }}
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = {{cls.schemaKindEnumValue }};

    /// \deprecated
    /// Same as schemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    static const UsdSchemaKind schemaType = {{cls.schemaKindEnumValue }};

{% if cls.isMultipleApply %}
    /// Construct a {{ cls.cppClassName }} on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// {{ cls.cppClassName }}::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "{{ cls.propertyNamespacePrefix }}:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit {{ cls.cppClassName }}(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : {{ cls.parentCppClassName }}(prim, /*instanceName*/ name)
    { }

    /// Construct a {{ cls.cppClassName }} on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// {{ cls.cppClassName }}(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit {{ cls.cppClassName }}(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : {{ cls.parentCppClassName }}(schemaObj, /*instanceName*/ name)
    { }
{% else %}
    /// Construct a {{ cls.cppClassName }} on UsdPrim \p prim .
    /// Equivalent to {{ cls.cppClassName }}::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit {{ cls.cppClassName }}(const UsdPrim& prim=UsdPrim())
        : {{ cls.parentCppClassName }}(prim)
    {
    }

    /// Construct a {{ cls.cppClassName }} on the prim held by \p schemaObj .
    /// Should be preferred over {{ cls.cppClassName }}(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit {{ cls.cppClassName }}(const UsdSchemaBase& schemaObj)
        : {{ cls.parentCppClassName }}(schemaObj)
    {
    }
{% endif %}

    /// Destructor.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    virtual ~{{ cls.cppClassName }}() {%- if cls.isAPISchemaBase %} = 0{% endif %};

{% if cls.isMultipleApply %}
    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
{% else %}
    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
{% endif %}
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static const TfTokenVector &
{% if cls.isMultipleApply %}
    GetSchemaAttributeNames(
        bool includeInherited=true, const TfToken instanceName=TfToken());
{% else %}
    GetSchemaAttributeNames(bool includeInherited=true);
{% endif %}
{% if cls.isMultipleApply %}

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }
{% endif %}
{% if not cls.isAPISchemaBase %}

    /// Return a {{ cls.cppClassName }} holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.{{ cls.propertyNamespacePrefix }}:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// {{ cls.cppClassName }}(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
{% else %}
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// {{ cls.cppClassName }}(stage->GetPrimAtPath(path));
    /// \endcode
{% endif %}
    ///
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }}
    Get(const UsdStagePtr &stage, const SdfPath &path);
{% if cls.isMultipleApply %}

    /// Return a {{ cls.cppClassName }} with name \p name holding the
    /// prim \p prim. Shorthand for {{ cls.cppClassName }}(prim, name);
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }}
    Get(const UsdPrim &prim, const TfToken &name);
{% endif %}
{% endif %}

{% if cls.isConcrete %}
    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }}
    Define(const UsdStagePtr &stage, const SdfPath &path);
{% endif %}
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
    /// Checks if the given name \p baseName is the base name of a property
    /// of {{ cls.usdPrimTypeName }}.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// {{ cls.usdPrimTypeName }}. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static bool
    Is{{ cls.usdPrimTypeName }}Path(const SdfPath &path, TfToken *name);
{% endif %}
{% if cls.isAppliedAPISchema and not cls.isMultipleApply %}

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "{{ cls.primName }}" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid {{ cls.cppClassName }} object is returned upon success. 
    /// An invalid (or empty) {{ cls.cppClassName }} object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }} 
    Apply(const UsdPrim &prim);
{% endif %}
{% if cls.isAppliedAPISchema and cls.isMultipleApply %}

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "{{ cls.primName }}:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// '{{ cls.primName }}:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid {{ cls.cppClassName }} object is returned upon success. 
    /// An invalid (or empty) {{ cls.cppClassName }} object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }} 
    Apply(const UsdPrim &prim, const TfToken &name);
{% endif %}

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    UsdSchemaKind _GetSchemaKind() const override;

    /// \deprecated
    /// Same as _GetSchemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    UsdSchemaKind _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    const TfType &_GetTfType() const override;

{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName]%}
{# Only emit Create/Get API and doxygen if apiName is not empty string. #}
{% if attr.apiName != '' %}
public:
    // --------------------------------------------------------------------- //
    // {{ Upper(attr.apiName) }} 
    // --------------------------------------------------------------------- //
    /// {{ attr.doc }}
    ///
{% if attr.details %}
    /// | ||
    /// | -- | -- |
{% for detail in attr.details %}
    /// | {{ detail[0] }} | {{ detail[1] }} |
{% endfor %}
{% endif %}
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    UsdAttribute Get{{ Proper(attr.apiName) }}Attr() const;

    /// See Get{{ Proper(attr.apiName) }}Attr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    UsdAttribute Create{{ Proper(attr.apiName) }}Attr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

{% endif %}
{% endfor %}
{% for relName in cls.relOrder %}
{% set rel = cls.rels[relName]%}
{# Only emit Create/Get API and doxygen if apiName is not empty string. #}
{% if rel.apiName != '' %}
public:
    // --------------------------------------------------------------------- //
    // {{ Upper(rel.apiName) }} 
    // --------------------------------------------------------------------- //
    /// {{ rel.doc }}
    ///
{% for detail in rel.details %}
    /// \n  {{ detail[0] }}: {{ detail[1] }}
{% endfor %}
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    UsdRelationship Get{{ Proper(rel.apiName) }}Rel() const;

    /// See Get{{ Proper(rel.apiName) }}Rel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    UsdRelationship Create{{ Proper(rel.apiName) }}Rel() const;
{% endif %}

{% endfor %}
public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
{% if useExportAPI %}
    //  - Close the namespace with {{ namespaceClose }}
{% endif %}
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

