//
// Copyright 2020 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

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
static object
__getattribute__Impl(object selfObj, const char *name, const object &getattribute) {
    // Allow attribute lookups if the attribute name starts with '__', if the
    // object's Property is valid, or if the attribute is one of a specific
    // inclusion list.
    if ((name[0] == '_' && name[1] == '_') ||
        extract<UsdPrimDefinition::Property &>(selfObj)() ||
        strcmp(name, "GetName") == 0 ||
        strcmp(name, "IsAttribute") == 0 ||
        strcmp(name, "IsRelationship") == 0) {
        // Dispatch to object's __getattribute__.
        return getattribute(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed invalid UsdPrimDefinition.Property"));
    }
    // Unreachable.
    return object();
}

void wrapUsdPrimDefinition()
{
    typedef UsdPrimDefinition This;
    scope s = class_<This, boost::noncopyable>("PrimDefinition", no_init)
        .def("GetPropertyNames", &This::GetPropertyNames,
             return_value_policy<TfPySequenceToList>())
        .def("GetAppliedAPISchemas", &This::GetAppliedAPISchemas,
             return_value_policy<TfPySequenceToList>())
        .def("GetSpecType", &This::GetSpecType,
             (arg("propName")))

        .def("GetPropertyDefinition", &This::GetPropertyDefinition,
             (arg("propName")))
        .def("GetAttributeDefinition", &This::GetAttributeDefinition,
             (arg("attrName")))
        .def("GetRelationshipDefinition", &This::GetRelationshipDefinition,
             (arg("relName")))

        .def("GetSchemaPropertySpec", &This::GetSchemaPropertySpec,
             (arg("propName")))
        .def("GetSchemaAttributeSpec", &This::GetSchemaAttributeSpec,
             (arg("attrName")))
        .def("GetSchemaRelationshipSpec", &This::GetSchemaRelationshipSpec,
             (arg("relName")))
        .def("GetAttributeFallbackValue", &_WrapGetAttributeFallbackValue,
             (arg("attrName")))

        .def("ListMetadataFields", &This::ListMetadataFields,
             return_value_policy<TfPySequenceToList>())
        .def("GetMetadata", &_WrapGetMetadata,
             (arg("key")))
        .def("GetMetadataByDictKey", &_WrapGetMetadataByDictKey,
             (arg("key"), arg("keyPath")))
        .def("GetDocumentation", &This::GetDocumentation)

        .def("ListPropertyMetadataFields", &This::ListPropertyMetadataFields,
             return_value_policy<TfPySequenceToList>())
        .def("GetPropertyMetadata", &_WrapGetPropertyMetadata,
             (arg("propName"), arg("key")))
        .def("GetPropertyMetadataByDictKey", &_WrapGetPropertyMetadataByDictKey,
             (arg("propName"), arg("key"), arg("keyPath")))
        .def("GetPropertyDocumentation", &This::GetPropertyDocumentation,
             (arg("propName")))
        .def("FlattenTo", 
             (UsdPrim (This::*)(const UsdPrim&, 
                                SdfSpecifier) const) &This::FlattenTo,
              (arg("prim"), 
               arg("newSpecSpecifier")=SdfSpecifierOver))
        .def("FlattenTo", 
             (UsdPrim (This::*)(const UsdPrim&, const TfToken&,
                                SdfSpecifier) const) &This::FlattenTo,
              (arg("parent"), 
               arg("name"), 
               arg("newSpecSpecifier")=SdfSpecifierOver))
        .def("FlattenTo", 
             (bool (This::*)(const SdfLayerHandle&, const SdfPath&,
                             SdfSpecifier) const) &This::FlattenTo,
              (arg("layer"), 
               arg("path"), 
               arg("newSpecSpecifier")=SdfSpecifierOver))
        ;

    {
        class_<This::Property> clsObj("Property");
        clsObj
            .def(!self)
            .def("GetName", &This::Property::GetName,
                return_value_policy<return_by_value>())
            .def("IsAttribute", &This::Property::IsAttribute)
            .def("IsRelationship", &This::Property::IsRelationship)
            .def("GetSpecType", &This::Property::GetSpecType)
            .def("ListMetadataFields", &This::Property::ListMetadataFields,
                return_value_policy<TfPySequenceToList>())
            .def("GetMetadata", &_WrapPropertyGetMetadata,
                (arg("key")))
            .def("GetMetadataByDictKey", &_WrapPropertyGetMetadataByDictKey,
                (arg("key"), arg("keyPath")))
            .def("GetVariability", &This::Property::GetVariability)
            .def("GetDocumentation", &This::Property::GetDocumentation)
            ;
        // Override __getattribute__ to prevent crashing if an attempt is made
        // to call data access methods on an invalid UsdPrimDefinition::Property
        // from python.
        static object __getattribute__ = object(clsObj.attr("__getattribute__"));
        clsObj.def("__getattribute__", 
            +[](object selfObj, const char *name) {
                return __getattribute__Impl(
                    selfObj, name, __getattribute__);
            });
    }

    class_<This::Attribute, bases<This::Property>>("Attribute")
        .def(init<const This::Property &>(arg("property")))
        .def(!self)
        .def("GetTypeName", &This::Attribute::GetTypeName)
        .def("GetTypeNameToken", &This::Attribute::GetTypeNameToken)
        .def("GetFallbackValue", &_WrapAttributeGetFallbackValue)
        ;

    class_<This::Relationship, bases<This::Property>>("Relationship")
        .def(init<const This::Property &>(arg("property")))
        .def(!self)
        ;

}
