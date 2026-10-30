//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

static TfPyObjWrapper
_WrapGetAttributeFallbackValue(const UsdPrimDefinition &self, 
                               const TfToken &attrName)
{
    VtValue result;
    self.GetAttributeFallbackValue(attrName, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapGetMetadata(const UsdPrimDefinition &self, 
                 const TfToken &key)
{
    VtValue result;
    self.GetMetadata(key, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapGetMetadataByDictKey(const UsdPrimDefinition &self, 
                          const TfToken &key, 
                          const TfToken &keyPath)
{
    VtValue result;
    self.GetMetadataByDictKey(key, keyPath, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapGetPropertyMetadata(const UsdPrimDefinition &self, 
                         const TfToken &propName, 
                         const TfToken &key)
{
    VtValue result;
    self.GetPropertyMetadata(propName, key, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapGetPropertyMetadataByDictKey(const UsdPrimDefinition &self, 
                                  const TfToken &propName, 
                                  const TfToken &key, 
                                  const TfToken &keyPath)
{
    VtValue result;
    self.GetPropertyMetadataByDictKey(propName, key, keyPath, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapPropertyGetMetadata(const UsdPrimDefinition::Property &self, 
                         const TfToken &key)
{
    VtValue result;
    self.GetMetadata(key, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapPropertyGetMetadataByDictKey(const UsdPrimDefinition::Property &self, 
                                  const TfToken &key, 
                                  const TfToken &keyPath)
{
    VtValue result;
    self.GetMetadataByDictKey(key, keyPath, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_WrapAttributeGetFallbackValue(const UsdPrimDefinition::Attribute &self)
{
    VtValue result;
    self.GetFallbackValue(&result);
    return UsdVtValueToPython(result);
}

// Override that prevents crashing if an attempt is made to call data access 
// methods on an invalid UsdPrimDefinition::Property from python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity. Note that this TfPyObjWrapper is intentionally leaked to avoid
// running Python refcount operations in its d'tor during process shutdown, 
// which is unsafe if Python has been finalized.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

static nb::object
__getattribute__(nb::object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', if the
    // object's Property is valid, or if the attribute is one of a specific
    // inclusion list.
    if ((name[0] == '_' && name[1] == '_') ||
        bool(nb::cast<UsdPrimDefinition::Property &>(selfObj)) ||
        strcmp(name, "GetName") == 0 ||
        strcmp(name, "IsAttribute") == 0 ||
        strcmp(name, "IsRelationship") == 0) {
        // Dispatch to object's __getattribute__.
        return nb::borrow<nb::object>(_object__getattribute__->ptr())(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed invalid UsdPrimDefinition.Property"));
    }
    // Unreachable.
    return nb::object();
}

void wrapUsdPrimDefinition(nb::module_ &m)
{
    typedef UsdPrimDefinition This;
    nb::class_<This> s(m, "PrimDefinition");
    s
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("GetPropertyNames", &This::GetPropertyNames,
             nb::rv_policy::copy)
        .def("GetLocallyDefinedPropertyNames", 
             &This::GetLocallyDefinedPropertyNames,
             nb::rv_policy::copy)
        .def("GetAppliedAPISchemas", &This::GetAppliedAPISchemas,
             nb::rv_policy::copy)
        .def("GetSpecType", &This::GetSpecType,
             nb::arg("propName"))

        .def("GetPropertyDefinition", &This::GetPropertyDefinition,
             nb::arg("propName"))
        .def("GetAttributeDefinition", &This::GetAttributeDefinition,
             nb::arg("attrName"))
        .def("GetRelationshipDefinition", &This::GetRelationshipDefinition,
             nb::arg("relName"))

        .def("GetSchemaPropertySpec", &This::GetSchemaPropertySpec,
             nb::arg("propName"))
        .def("GetSchemaAttributeSpec", &This::GetSchemaAttributeSpec,
             nb::arg("attrName"))
        .def("GetSchemaRelationshipSpec", &This::GetSchemaRelationshipSpec,
             nb::arg("relName"))
        .def("GetAttributeFallbackValue", &_WrapGetAttributeFallbackValue,
             nb::arg("attrName"))

        .def("ListMetadataFields", &This::ListMetadataFields)
        .def("GetMetadata", &_WrapGetMetadata,
             nb::arg("key"))
        .def("GetMetadataByDictKey", &_WrapGetMetadataByDictKey,
             nb::arg("key"), nb::arg("keyPath"))
        .def("GetDocumentation", &This::GetDocumentation)

        .def("ListPropertyMetadataFields", &This::ListPropertyMetadataFields)
        .def("GetPropertyMetadata", &_WrapGetPropertyMetadata,
             nb::arg("propName"), nb::arg("key"))
        .def("GetPropertyMetadataByDictKey", &_WrapGetPropertyMetadataByDictKey,
             nb::arg("propName"), nb::arg("key"), nb::arg("keyPath"))
        .def("GetPropertyDocumentation", &This::GetPropertyDocumentation,
             nb::arg("propName"))
        .def("FlattenTo", 
             (UsdPrim (This::*)(const UsdPrim&, 
                                SdfSpecifier) const) &This::FlattenTo,
             nb::arg("prim"), 
             nb::arg("newSpecSpecifier")=SdfSpecifierOver)
        .def("FlattenTo", 
             (UsdPrim (This::*)(const UsdPrim&, const TfToken&,
                                SdfSpecifier) const) &This::FlattenTo,
             nb::arg("parent"), 
             nb::arg("name"), 
             nb::arg("newSpecSpecifier")=SdfSpecifierOver)
        .def("FlattenTo", 
             (bool (This::*)(const SdfLayerHandle&, const SdfPath&,
                             SdfSpecifier) const) &This::FlattenTo,
             nb::arg("layer"), 
             nb::arg("path"), 
             nb::arg("newSpecSpecifier")=SdfSpecifierOver)
        ;

    {
        nb::class_<This::Property> clsObj(s, "Property");
        clsObj
            .def(nb::init<>())
            .def(!nb::self)
            .def("GetName", &This::Property::GetName,
                nb::rv_policy::copy)
            .def("IsAttribute", &This::Property::IsAttribute)
            .def("IsRelationship", &This::Property::IsRelationship)
            .def("GetSpecType", &This::Property::GetSpecType)
            .def("ListMetadataFields", &This::Property::ListMetadataFields)
            .def("GetMetadata", &_WrapPropertyGetMetadata,
                nb::arg("key"))
            .def("GetMetadataByDictKey", &_WrapPropertyGetMetadataByDictKey,
                nb::arg("key"), nb::arg("keyPath"))
            .def("GetVariability", &This::Property::GetVariability)
            .def("GetDocumentation", &This::Property::GetDocumentation)
            ;

        // Save existing __getattribute__ and replace.
        *_object__getattribute__ =
            nb::cast<TfPyObjWrapper>(clsObj.attr("__getattribute__"));
        clsObj.def("__getattribute__", __getattribute__);
    }

    nb::class_<This::Attribute, This::Property>(s, "Attribute")
        .def(nb::init<>())
        .def(nb::init<const This::Property &>(), nb::arg("property"))
        .def(!nb::self)
        .def("GetTypeName", &This::Attribute::GetTypeName)
        .def("GetTypeNameToken", &This::Attribute::GetTypeNameToken)
        .def("GetFallbackValue", &_WrapAttributeGetFallbackValue)
        ;

    nb::class_<This::Relationship, This::Property>(s, "Relationship")
        .def(nb::init<>())
        .def(nb::init<const This::Property &>(), nb::arg("property"))
        .def(!nb::self)
        ;
}
