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
/// \file wrapSpec.cpp
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <boost/python/class.hpp>

using namespace boost::python;

static
VtValue
_WrapGetInfo(SdfSpec &self, const TfToken &name)
{
    return self.GetInfo(name);
}

static
bool
_WrapIsInertProperty(SdfSpec &self)
{
    return self.IsInert();
}

static
void
_WrapSetInfo(SdfSpec &self, const TfToken &name, const object& pyObj)
{
    VtValue fallback;
    if (not self.GetSchema().IsRegistered(name, &fallback)) {
        TF_CODING_ERROR("Invalid info key: %s", name.GetText());
        return;
    }

    VtValue value;
    if (fallback.IsEmpty()) {
        value = extract<VtValue>(pyObj)();
    }
    else {
        // We have to handle a few things as special cases to disambiguate
        // types from Python.
        if (fallback.IsHolding<SdfPath>()) {
            value = extract<SdfPath>(pyObj)();
        }
        else if (fallback.IsHolding<TfTokenVector>()) {
            value = extract<TfTokenVector>(pyObj)();
        }
        else if (fallback.IsHolding<SdfVariantSelectionMap>()) {
            value = extract<SdfVariantSelectionMap>(pyObj)();
        }
        else {
            value = extract<VtValue>(pyObj)();
            value.CastToTypeOf(fallback);
        }
    }
    
    if (value.IsEmpty()) {
        TfPyThrowTypeError("Invalid type for key");
        return;
    }

    self.SetInfo(name, value);
}

static
std::string
_GetAsText(const SdfSpecHandle &self)
{
    if (not self) {
        return TfPyRepr(self);
    }
    std::stringstream stream;
    self->WriteToStream(stream);
    return stream.str();
}

void
wrapSpec()
{
    typedef SdfSpec This;

    class_<This, SdfHandle<This>, boost::noncopyable>("Spec", no_init)
        .def(SdfPyAbstractSpec())

        .add_property("layer", &This::GetLayer,
            "The owning layer.")
        .add_property("path", &This::GetPath,
            "The absolute scene path.")

        .def("GetAsText", &::_GetAsText)

        .def("ListInfoKeys", &This::ListInfoKeys,
            return_value_policy<TfPySequenceToList>())
        .def("GetMetaDataInfoKeys", &This::GetMetaDataInfoKeys,
            return_value_policy<TfPySequenceToList>())

        .def("GetMetaDataDisplayGroup", &This::GetMetaDataDisplayGroup)

        .def("GetInfo", &::_WrapGetInfo)
        .def("SetInfo", &::_WrapSetInfo)
        .def("SetInfoDictionaryValue", &This::SetInfoDictionaryValue)
        .def("HasInfo", &This::HasInfo,
             "HasInfo(key) -> bool\n\n"

             "key : string\n\n"

             "Returns whether there is a setting for the scene spec "
             "info with the given key.\n\n"
            
             "When asked for a value for one of its scene spec info, a "
             "valid value will always be returned. But if this API returns "
             "false for a scene spec info, the value of that info will be "
             "the defined default value. \n\n"
             "(XXX: This may change such that it is an error to "
             "ask for a value when there is none).\n\n"
             
             "When dealing with a composedLayer, it is not necessary to worry "
             "about whether a scene spec info 'has a value' because the "
             "composed layer will always have a valid value, even if it is the "
             "default.\n\n"
            
             "A spec may or may not have an expressed value for "
             "some of its scene spec info.")

        .def("ClearInfo", &This::ClearInfo,
             "ClearInfo(key)\n\n"

             "key : string\nn"

             "Clears the value for scene spec info with the given key. "
             "After calling this, HasInfo() will return false. "
             "To make HasInfo() return true, set a value for that scene "
             "spec info.",
             (arg("key")))

        .def("GetTypeForInfo", &This::GetTypeForInfo,
             "GetTypeForInfo(key)\n\n"

             "key : string\n\n"

             "Returns the type of value for the given key. ")

        .def("GetFallbackForInfo",
              make_function(&This::GetFallbackForInfo,
                  return_value_policy<return_by_value>()),
             "GetFallbackForInfo(key)\n\n"

             "key : string\n\n"

             "Returns the fallback value for the given key. ")

        .add_property("isInert", &_WrapIsInertProperty,
              "Indicates whether this spec has any significant data. This is "
              "for backwards compatibility, use IsInert instead.\n\n"

              "Compatibility note: prior to presto 1.9, isInert (then isEmpty) "
              "was true for otherwise inert PrimSpecs with inert inherits, "
              "references, or variant sets. isInert is now false in such "
              "conditions.")

        .def("IsInert", &This::IsInert,
             (arg("ignoreChildren") = false),
             
             "Indicates whether this spec has any significant data. "
             "If ignoreChildren is true, child scenegraph objects will be "
             "ignored.")
       ;
}
