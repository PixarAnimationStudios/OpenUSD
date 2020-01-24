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
#include "pxr/pxr.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

// We override __getattribute__ for UsdSchemaBase to check object validity
// and raise an exception instead of crashing from Python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdSchemaBase.
static object
__getattribute__(object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim is valid. Also add explicit exceptions for every
    // method on this base class. The real purpose here is to protect against
    // invalid calls in subclasses which will try to actually manipulate the
    // underlying (invalid) prim and likely crash.
    if ((name[0] == '_' && name[1] == '_') ||
        extract<UsdSchemaBase &>(selfObj)().GetPrim().IsValid() ||
        strcmp(name, "GetPrim") == 0 ||
        strcmp(name, "GetPath") == 0 ||
        strcmp(name, "GetSchemaClassPrimDefinition") == 0 ||
        strcmp(name, "GetSchemaAttributeNames") == 0 ||
        strcmp(name, "GetSchemaType") == 0 ||
        strcmp(name, "IsAPISchema") == 0 ||
        strcmp(name, "IsConcrete") == 0 ||
        strcmp(name, "IsTyped") == 0 ||
        strcmp(name, "IsAppliedAPISchema") == 0 ||
        strcmp(name, "IsMultipleApplyAPISchema") == 0) {
        // Dispatch to object's __getattribute__.
        return (*_object__getattribute__)(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed schema on invalid prim"));
    }
    // Unreachable.
    return object();
}

void wrapUsdSchemaBase()
{
    class_<UsdSchemaBase> cls("SchemaBase");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("otherSchema")))
        .def(TfTypePythonClass())

        .def("GetPrim", &UsdSchemaBase::GetPrim)
        .def("GetPath", &UsdSchemaBase::GetPath)
        .def("GetSchemaClassPrimDefinition",
             &UsdSchemaBase::GetSchemaClassPrimDefinition)
        .def("GetSchemaAttributeNames", &UsdSchemaBase::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("IsAPISchema", &UsdSchemaBase::IsAPISchema)
        .def("IsConcrete", &UsdSchemaBase::IsConcrete) 
        .def("IsTyped", &UsdSchemaBase::IsTyped) 
        .def("IsAppliedAPISchema", &UsdSchemaBase::IsAppliedAPISchema) 
        .def("IsMultipleApplyAPISchema", &UsdSchemaBase::IsMultipleApplyAPISchema) 

        .def("GetSchemaType", &UsdSchemaBase::GetSchemaType)

        .def(!self)

        ;

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = object(cls.attr("__getattribute__"));
    cls.def("__getattribute__", __getattribute__);
}
