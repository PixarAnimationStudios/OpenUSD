//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/validator.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace
{

    UsdValidatorMetadata *
    _NewMetadata(
        const TfToken &name,
        const PlugPluginPtr &plugin,
        const TfTokenVector &keywords,
        const TfToken &doc,
        const TfTokenVector &schemaTypes,
        bool isSuite)
    {
        return new UsdValidatorMetadata{name, plugin, keywords, doc, schemaTypes, isSuite};
    }

} // anonymous namespace

void wrapUsdValidator()
{
    class_<UsdValidatorMetadata>("ValidatorMetadata", no_init)
        .def("__init__", make_constructor(&_NewMetadata, default_call_policies(),
                                          (arg("name") = TfToken(),
                                           arg("plugin") = PlugPluginPtr(),
                                           arg("keywords") = TfTokenVector(),
                                           arg("doc") = TfToken(),
                                           arg("schemaTypes") = TfTokenVector(),
                                           arg("isSuite") = false)))
        .add_property("name", make_getter(
            &UsdValidatorMetadata::name, return_value_policy<return_by_value>()))
        .add_property("plugin", make_getter(
            &UsdValidatorMetadata::pluginPtr, return_value_policy<return_by_value>()))
        .add_property("keywords", make_getter(
            &UsdValidatorMetadata::keywords, return_value_policy<TfPySequenceToList>()))
        .def_readonly("doc", &UsdValidatorMetadata::doc)
        .add_property("schemaTypes", make_getter(
            &UsdValidatorMetadata::schemaTypes, return_value_policy<TfPySequenceToList>()))
        .def_readonly("isSuite", &UsdValidatorMetadata::isSuite);
}
