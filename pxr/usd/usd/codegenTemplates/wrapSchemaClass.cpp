//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "{{ libraryPath }}/{{ cls.GetHeaderFile() }}"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
{% if cls.isAppliedAPISchema %}
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
{% endif %}
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

#include <string>

{% if useExportAPI %}
{{ namespaceUsing }}

namespace {

{% endif %}
#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

{% for attrName in cls.attrOrder -%}
{% set attr = cls.attrs[attrName] %}
{# Only emit Create/Get API if apiName is not empty string. #}
{% if attr.apiName != '' %}
        
static UsdAttribute
_Create{{ Proper(attr.apiName) }}Attr({{ cls.cppClassName }} &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.Create{{ Proper(attr.apiName) }}Attr(
        UsdPythonToSdfType(defaultVal, {{ attr.usdType }}), writeSparsely);
}
{% endif %}
{% endfor %}
{% if cls.isMultipleApply and cls.propertyNamespace %}

static bool _WrapIs{{ cls.usdPrimTypeName }}Path(const SdfPath &path) {
    TfToken collectionName;
    return {{ cls.cppClassName }}::Is{{ cls.usdPrimTypeName }}Path(
        path, &collectionName);
}
{% endif %}
{% if not cls.isAPISchemaBase %}

static std::string
_Repr(const {{ cls.cppClassName }} &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
{% if cls.isMultipleApply %}
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "{{ libraryName[0]|upper }}{{ libraryName[1:] }}.{{ cls.className }}(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
{% else %}
    return TfStringPrintf(
        "{{ libraryName[0]|upper }}{{ libraryName[1:] }}.{{ cls.className }}(%s)",
        primRepr.c_str());
{% endif %}
}
{% endif %}
{% if cls.isAppliedAPISchema %}

struct {{ cls.cppClassName }}_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    {{ cls.cppClassName }}_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

{% if cls.isMultipleApply %}
static {{ cls.cppClassName }}_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = {{ cls.cppClassName }}::CanApply(prim, name, &whyNot);
    return {{ cls.cppClassName }}_CanApplyResult(result, whyNot);
}
{% else %}
static {{ cls.cppClassName }}_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = {{ cls.cppClassName }}::CanApply(prim, &whyNot);
    return {{ cls.cppClassName }}_CanApplyResult(result, whyNot);
}
{% endif %}
{% endif %}
{% if useExportAPI %}

{% for schema in appliedSchemas %}
{% set schemaCls = classes[schema]%}
{% for attrName in schemaCls.attrOrder -%}
{% set attr = schemaCls.attrs[attrName] %}
{# Only emit Create/Get API if apiName is not empty string. #}
{% if attr.apiName != '' %}
        
static UsdAttribute
_Create{{ Proper(attr.apiName) }}Attr({{ cls.cppClassName }} &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.Create{{ Proper(attr.apiName) }}Attr(
        UsdPythonToSdfType(defaultVal, {{ attr.usdType }}), writeSparsely);
}
{% endif %}
{% endfor %}
{% if schemaCls.isMultipleApply and schemaCls.propertyNamespace %}

static bool _WrapIs{{ schemaCls.usdPrimTypeName }}Path(const SdfPath &path) {
    TfToken collectionName;
    return {{ schemaCls.cppClassName }}::Is{{ cls.usdPrimTypeName }}Path(
        path, &collectionName);
}
{% endif %}
{% endfor %}
} // anonymous namespace
{% endif %}

void wrap{{ cls.cppClassName }}(nb::module_ &m)
{
    typedef {{ cls.cppClassName }} This;

{% if cls.isAppliedAPISchema %}
    {{ cls.cppClassName }}_CanApplyResult::Wrap<{{ cls.cppClassName }}_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

{% endif %}
{% if cls.isAPISchemaBase %}
    nb::class_<This, {{ cls.parentCppClassName }}> cls(m, "APISchemaBase", "");
{% else %}
    nb::class_<This, {{ cls.parentCppClassName }}>
        cls(m, "{{ cls.className }}");
{% endif %}

    cls
{% if not cls.isAPISchemaBase %}
{% if cls.isMultipleApply %}
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
{% else %}
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
{% endif %}
{% endif %}
        .def(TfTypePythonClass())

{% if not cls.isAPISchemaBase %}
{% if cls.isMultipleApply %}
        .def_static("Get",
            ({{ cls.cppClassName }}(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            ({{ cls.cppClassName }}(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))
{% else %}
        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))
{% endif %}
{% endif %}
{% if cls.isMultipleApply %}

        .def_static("GetAll",
            (std::vector<{{ cls.cppClassName }}>(*)(const UsdPrim &prim))
                &This::GetAll,
            nb::arg("prim"),
            nb::rv_policy::copy)
{% endif %}
{% if cls.isConcrete %}

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))
{% endif %}
{% if cls.isAppliedAPISchema and not cls.isMultipleApply %}

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))
{% endif %}
{% if cls.isAppliedAPISchema and cls.isMultipleApply %}

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"), nb::arg("name"))
{% endif %}
{% if cls.isAppliedAPISchema and not cls.isMultipleApply %}

        .def_static("Apply", &This::Apply, nb::arg("prim"))
{% endif %}
{% if cls.isAppliedAPISchema and cls.isMultipleApply %}

        .def_static("Apply", &This::Apply, nb::arg("prim"), nb::arg("name"))
{% endif %}

{% if cls.isMultipleApply %}
        .def_static("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)
        .def_static("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             nb::arg("includeInherited"),
             nb::arg("instanceName"),
             nb::rv_policy::copy)
{% else %}
        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)
{% endif %}

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

{% for attrName in cls.attrOrder -%}
{% set attr = cls.attrs[attrName] %}
{# Only emit Create/Get API if apiName is not empty string. #}
{% if attr.apiName != '' %}
        
        .def("Get{{ Proper(attr.apiName) }}Attr",
             &This::Get{{ Proper(attr.apiName) }}Attr)
        .def("Create{{ Proper(attr.apiName) }}Attr",
             &_Create{{ Proper(attr.apiName) }}Attr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
{% endif %}
{% endfor %}

{% for relName in cls.relOrder -%}
{# Only emit Create/Get API and doxygen if apiName is not empty string. #}
{% set rel = cls.rels[relName] %}
{% if rel.apiName != '' %}
        
        .def("Get{{ Proper(rel.apiName) }}Rel",
             &This::Get{{ Proper(rel.apiName) }}Rel)
        .def("Create{{ Proper(rel.apiName) }}Rel",
             &This::Create{{ Proper(rel.apiName) }}Rel)
{% endif %}
{% endfor %}
{% for schema in appliedSchemas %}
{% set schemaCls = classes[schema]%}
{% for attrName in schemaCls.attrOrder -%}
{% set attr = schemaCls.attrs[attrName] %}
{# Only emit Create/Get API if apiName is not empty string. #}
{% if attr.apiName != '' %}
        
        .def("Get{{ Proper(attr.apiName) }}Attr",
             &This::Get{{ Proper(attr.apiName) }}Attr)
        .def("Create{{ Proper(attr.apiName) }}Attr",
             &_Create{{ Proper(attr.apiName) }}Attr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
{% endif %}
{% endfor %}

{% for relName in schemaCls.relOrder -%}
{# Only emit Create/Get API and doxygen if apiName is not empty string. #}
{% set rel = schemaCls.rels[relName] %}
{% if rel.apiName != '' %}
        .def("Get{{ Proper(rel.apiName) }}Rel",
             &This::Get{{ Proper(rel.apiName) }}Rel)
        .def("Create{{ Proper(rel.apiName) }}Rel",
             &This::Create{{ Proper(rel.apiName) }}Rel)

{% endif %}
{% endfor %}
        .def("{{ schema }}", &This::{{ schema }})
{% endfor %}
{% if cls.isMultipleApply and cls.propertyNamespace %}
        .def_static("Is{{ cls.usdPrimTypeName }}Path", _WrapIs{{ cls.usdPrimTypeName }}Path)
{% endif %}
{% if not cls.isAPISchemaBase %}
        .def("__repr__", ::_Repr)
{% endif %}
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
{% if useExportAPI %}
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
{% endif %}
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

