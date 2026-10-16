//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/iterator.h"

#include <nanobind/nanobind.h>
#include <string>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static nanobind::dict _ConvertDict(const JsObject &dictionary);

static nanobind::object
_ConvertValue(const JsValue &value)
{
    namespace nb = nanobind;
    switch (value.GetType()) {
    case JsValue::ObjectType:
        return _ConvertDict(value.GetJsObject());
    case JsValue::ArrayType: {
        nb::list result;
        for (const auto &elem : value.GetJsArray()) {
            result.append(_ConvertValue(elem));
        }
        return result;
    }
    case JsValue::BoolType:
        return nb::bool_(value.GetBool());
    case JsValue::StringType:
        return nb::cast(value.GetString());
    case JsValue::RealType:
        return nb::float_(value.GetReal());
    case JsValue::IntType:
        return value.IsUInt64() ? nb::int_(value.GetUInt64())
                                : nb::int_(value.GetInt64());
    case JsValue::NullType:
        return nb::none();
    default:
        TF_CODING_ERROR("unknown value type");
        return nb::none();
    }
}

static nanobind::dict
_ConvertDict(const JsObject &dictionary)
{
    nanobind::dict result;
    TF_FOR_ALL(i, dictionary) {
        result[i->first.c_str()] = _ConvertValue(i->second);
    }
    return result;
}

static nanobind::dict
_GetMetadata(PlugPluginPtr plugin)
{
    return _ConvertDict(plugin->GetMetadata());
}

static nanobind::dict
_GetMetadataForType(PlugPluginPtr plugin, const TfType &type)
{
    return _ConvertDict(plugin->GetMetadataForType(type));
}

} // anonymous namespace 

void wrapPlugin(nanobind::module_& m)
{
    namespace nb = nanobind;
    typedef PlugPlugin This;
    typedef PlugPluginPtr ThisPtr;

    nb::class_<This> cls(m, "Plugin",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbWeakPtr(cls);
    cls.def("Load", &This::Load)
        .def_prop_ro("isLoaded", &This::IsLoaded)
        .def_prop_ro("isPythonModule", &This::IsPythonModule)
        .def_prop_ro("isResource", &This::IsResource)
        .def_prop_ro("metadata", &_GetMetadata)
        .def_prop_ro("name", &This::GetName)
        .def_prop_ro("path", &This::GetPath)
        .def_prop_ro("resourcePath", &This::GetResourcePath)
        .def("GetMetadataForType", &_GetMetadataForType)
        .def("DeclaresType", &This::DeclaresType,
             nb::arg("type"),
             nb::arg("includeSubclasses") = false)
        .def("MakeResourcePath", &This::MakeResourcePath)
        .def("FindPluginResource", &This::FindPluginResource,
             nb::arg("path"),
             nb::arg("verify") = true)
        ;
}
