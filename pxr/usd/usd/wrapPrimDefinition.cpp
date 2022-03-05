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

void wrapUsdPrimDefinition()
{
    typedef UsdPrimDefinition This;
    boost::python::class_<This, boost::noncopyable>("PrimDefinition", boost::python::no_init)
        .def("GetPropertyNames", &This::GetPropertyNames,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAppliedAPISchemas", &This::GetAppliedAPISchemas,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetSchemaPropertySpec", &This::GetSchemaPropertySpec,
             (boost::python::arg("propName")))
        .def("GetSchemaAttributeSpec", &This::GetSchemaAttributeSpec,
             (boost::python::arg("attrName")))
        .def("GetSchemaRelationshipSpec", &This::GetSchemaRelationshipSpec,
             (boost::python::arg("relName")))
        .def("GetAttributeFallbackValue", &_WrapGetAttributeFallbackValue,
             (boost::python::arg("attrName"), boost::python::arg("key")))

        .def("ListMetadataFields", &This::ListMetadataFields,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetMetadata", &_WrapGetMetadata,
             (boost::python::arg("key")))
        .def("GetMetadataByDictKey", &_WrapGetMetadataByDictKey,
             (boost::python::arg("key"), boost::python::arg("keyPath")))
        .def("GetDocumentation", &This::GetDocumentation)

        .def("ListPropertyMetadataFields", &This::ListPropertyMetadataFields,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetPropertyMetadata", &_WrapGetPropertyMetadata,
             (boost::python::arg("propName"), boost::python::arg("key")))
        .def("GetPropertyMetadataByDictKey", &_WrapGetPropertyMetadataByDictKey,
             (boost::python::arg("propName"), boost::python::arg("key"), boost::python::arg("keyPath")))
        .def("GetPropertyDocumentation", &This::GetPropertyDocumentation,
             (boost::python::arg("propName")))
        .def("FlattenTo", 
             (UsdPrim (This::*)(const UsdPrim&, 
                                SdfSpecifier) const) &This::FlattenTo,
              (boost::python::arg("prim"), 
               boost::python::arg("newSpecSpecifier")=SdfSpecifierOver))
        .def("FlattenTo", 
             (UsdPrim (This::*)(const UsdPrim&, const TfToken&,
                                SdfSpecifier) const) &This::FlattenTo,
              (boost::python::arg("parent"), 
               boost::python::arg("name"), 
               boost::python::arg("newSpecSpecifier")=SdfSpecifierOver))
        .def("FlattenTo", 
             (bool (This::*)(const SdfLayerHandle&, const SdfPath&,
                             SdfSpecifier) const) &This::FlattenTo,
              (boost::python::arg("layer"), 
               boost::python::arg("path"), 
               boost::python::arg("newSpecSpecifier")=SdfSpecifierOver))
        ;
}
