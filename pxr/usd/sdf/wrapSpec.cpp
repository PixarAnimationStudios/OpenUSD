//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/ostreamMethods.h"


#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

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
_WrapSetInfo(SdfSpec &self, const TfToken &name, nb::handle pyObj)
{
    VtValue fallback;
    if (!self.GetSchema().IsRegistered(name, &fallback)) {
        TF_CODING_ERROR("Invalid info key: %s", name.GetText());
        return;
    }

    VtValue value;
    try {
        if (fallback.IsEmpty()) {
            value = nb::cast<VtValue>(pyObj);
        }
        else {
            // We have to handle a few things as special cases to disambiguate
            // types from Python.
            if (fallback.IsHolding<SdfPath>()) {
                value = nb::cast<SdfPath>(pyObj);
            }
            else if (fallback.IsHolding<TfTokenVector>()) {
                value = nb::cast<TfTokenVector>(pyObj);
            }
            else if (fallback.IsHolding<SdfVariantSelectionMap>()) {
                value = nb::cast<SdfVariantSelectionMap>(pyObj);
            }
            else if (fallback.IsHolding<SdfRelocatesMap>()) {
                value = nb::cast<SdfRelocatesMap>(pyObj);
            }
            else if (fallback.IsHolding<SdfRelocates>()) {
                value = nb::cast<SdfRelocates>(pyObj);
            }
            else {
                value = nb::cast<VtValue>(pyObj);
                value.CastToTypeOf(fallback);
            }
        }
    } catch (const nb::cast_error &) {
        TfPyThrowTypeError("Invalid type for key");
        return;
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
    if (!self) {
        return TfPyRepr(self);
    }
    std::stringstream stream;
    self->WriteToStream(stream);
    return stream.str();
}

} // anonymous namespace 

void wrapSpec(nanobind::module_& m)
{
    typedef SdfSpec This;

    nb::class_<This>(m, "Spec")
        .def(SdfPyAbstractSpec())

        .def_prop_ro("layer", &This::GetLayer,
            "The owning layer.")
        .def_prop_ro("path", &This::GetPath,
            "The absolute scene path.")

        .def("GetAsText", &_GetAsText)

        .def("ListInfoKeys", &This::ListInfoKeys)
        .def("GetMetaDataInfoKeys", &This::GetMetaDataInfoKeys)

        .def("GetMetaDataDisplayGroup", &This::GetMetaDataDisplayGroup)

        .def("GetInfo", &_WrapGetInfo)
        .def("SetInfo", &_WrapSetInfo)
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
             nb::arg("key"))

        .def("GetTypeForInfo", &This::GetTypeForInfo,
             "GetTypeForInfo(key)\n\n"

             "key : string\n\n"

             "Returns the type of value for the given key. ")

        .def("GetFallbackForInfo", &This::GetFallbackForInfo,
             "GetFallbackForInfo(key)\n\n"

             "key : string\n\n"

             "Returns the fallback value for the given key. ")

        .def_prop_ro("isInert", &_WrapIsInertProperty,
              "Indicates whether this spec has any significant data. This is "
              "for backwards compatibility, use IsInert instead.\n\n"

              "Compatibility note: prior to presto 1.9, isInert (then isEmpty) "
              "was true for otherwise inert PrimSpecs with inert inherits, "
              "references, or variant sets. isInert is now false in such "
              "conditions.")

        .def("IsInert", &This::IsInert,
             nb::arg("ignoreChildren") = false,
             "Indicates whether this spec has any significant data. "
             "If ignoreChildren is true, child scenegraph objects will be "
             "ignored.")
       ;
}
