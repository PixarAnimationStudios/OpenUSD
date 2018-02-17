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
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = {{ "true" if cls.isConcrete else "false" }};

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = {{ "true" if cls.isTyped else "false" }};

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

    /// Destructor.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    virtual ~{{ cls.cppClassName }}();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a {{ cls.cppClassName }} holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// {{ cls.cppClassName }}(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    {% if useExportAPI -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }}
    Get(const UsdStagePtr &stage, const SdfPath &path);

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
{% if cls.isPrivateApply %}
private:
{% endif %}
{% if cls.isApi and not cls.isMultipleApply %}

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "{{ cls.primName }}" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid {{ cls.cppClassName }} object is returned upon success. 
    /// An invalid (or empty) {{ cls.cppClassName }} object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    {% if useExportAPI and not cls.isPrivateApply -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }} 
{% if cls.isPrivateApply %}
    _Apply(const UsdPrim &prim);
{% else %}
    Apply(const UsdPrim &prim);
{% endif %}
{% endif %}
{% if cls.isApi and cls.isMultipleApply %}

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
    /// failure. See \ref UsdAPISchemaBase::_MultipleApplyAPISchema() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    {% if useExportAPI and not cls.isPrivateApply -%}
    {{ Upper(libraryName) }}_API
    {% endif -%}
    static {{ cls.cppClassName }} 
{% if cls.isPrivateApply %}
    _Apply(const UsdPrim &prim, const TfToken &name);
{% else %}
    Apply(const UsdPrim &prim, const TfToken &name);
{% endif %}
{% endif %}

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
    virtual const TfType &_GetTfType() const;

{% for attrName in cls.attrOrder %}
{% set attr = cls.attrs[attrName]%}
public:
    // --------------------------------------------------------------------- //
    // {{ Upper(attr.apiName) }} 
    // --------------------------------------------------------------------- //
    /// {{ attr.doc }}
    ///
{% for detail in attr.details %}
    /// \n  {{ detail[0] }}: {{ detail[1] }}
{% endfor %}
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

{% endfor %}
{% for relName in cls.relOrder %}
{% set rel = cls.rels[relName]%}
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

