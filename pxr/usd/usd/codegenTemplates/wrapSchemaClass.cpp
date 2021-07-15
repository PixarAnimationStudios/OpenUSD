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

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

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
                                      object defaultVal, bool writeSparsely) {
    return self.Create{{ Proper(attr.apiName) }}Attr(
        UsdPythonToSdfType(defaultVal, {{ attr.usdType }}), writeSparsely);
}
{% endif %}
{% endfor %}
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}

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
    std::string instanceName = self.GetName();
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
    class_< This , bases<{{ cls.parentCppClassName }}>, boost::noncopyable> cls ("APISchemaBase", "", no_init);
{% else %}
    class_<This, bases<{{ cls.parentCppClassName }}> >
        cls("{{ cls.className }}");
{% endif %}

    cls
{% if not cls.isAPISchemaBase %}
{% if cls.isMultipleApply %}
        .def(init<UsdPrim, TfToken>())
        .def(init<UsdSchemaBase const&, TfToken>())
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

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
{% if cls.isMultipleApply %}
             arg("instanceName")=TfToken(),
{% endif %}
             return_value_policy<TfPySequenceToList>())
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
{% if cls.isMultipleApply and cls.propertyNamespacePrefix %}
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

