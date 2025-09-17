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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

{% if useExportAPI %}
{{ namespaceUsing }}

using namespace pxr_boost::python;

namespace {

{% else %}
using namespace pxr_boost::python;

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
                                      object defaultVal, bool writeSparsely) {
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
                                      object defaultVal, bool writeSparsely) {
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

void wrap{{ cls.cppClassName }}()
{
    typedef {{ cls.cppClassName }} This;

{% if cls.isAppliedAPISchema %}
    {{ cls.cppClassName }}_CanApplyResult::Wrap<{{ cls.cppClassName }}_CanApplyResult>(
        "_CanApplyResult", "whyNot");

{% endif %}
{% if cls.isAPISchemaBase %}
    class_< This , bases<{{ cls.parentCppClassName }}>, noncopyable> cls ("APISchemaBase", "", no_init);
{% else %}
    class_<This, bases<{{ cls.parentCppClassName }}> >
        cls("{{ cls.className }}");
{% endif %}

    cls
{% if not cls.isAPISchemaBase %}
{% if cls.isMultipleApply %}
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
{% else %}
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
{% endif %}
{% endif %}
        .def(TfTypePythonClass())

{% if not cls.isAPISchemaBase %}
{% if cls.isMultipleApply %}
        .def("Get",
            ({{ cls.cppClassName }}(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            ({{ cls.cppClassName }}(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
{% else %}
        .def("Get", &This::Get, (arg("stage"), arg("path")))
{% endif %}
        .staticmethod("Get")
{% endif %}
{% if cls.isMultipleApply %}

        .def("GetAll",
            (std::vector<{{ cls.cppClassName }}>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")
{% endif %}
{% if cls.isConcrete %}

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")
{% endif %}
{% if cls.isAppliedAPISchema and not cls.isMultipleApply %}

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")
{% endif %}
{% if cls.isAppliedAPISchema and cls.isMultipleApply %}

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")
{% endif %}
{% if cls.isAppliedAPISchema and not cls.isMultipleApply %}

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")
{% endif %}
{% if cls.isAppliedAPISchema and cls.isMultipleApply %}

        .def("Apply", &This::Apply, (arg("prim"), arg("name")))
        .staticmethod("Apply")
{% endif %}

{% if cls.isMultipleApply %}
        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             arg("includeInherited"),
             arg("instanceName"),
             return_value_policy<TfPySequenceToList>())
{% else %}
        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
{% endif %}
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

{% for attrName in cls.attrOrder -%}
{% set attr = cls.attrs[attrName] %}
{# Only emit Create/Get API if apiName is not empty string. #}
{% if attr.apiName != '' %}
        
        .def("Get{{ Proper(attr.apiName) }}Attr",
             &This::Get{{ Proper(attr.apiName) }}Attr)
        .def("Create{{ Proper(attr.apiName) }}Attr",
             &_Create{{ Proper(attr.apiName) }}Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
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
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
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
        .def("Is{{ cls.usdPrimTypeName }}Path", _WrapIs{{ cls.usdPrimTypeName }}Path)
            .staticmethod("Is{{ cls.usdPrimTypeName }}Path")
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

