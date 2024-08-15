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

#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/object.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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

    TfToken
    _GetMetadataName(const UsdValidatorMetadata &metadata)
    {
        return metadata.name;
    }

    void
    _SetMetadataName(UsdValidatorMetadata &metadata, const TfToken &name)
    {
        metadata.name = name;
    }

    TfTokenVector
    _GetMetadataKeywords(const UsdValidatorMetadata &metadata)
    {
        return metadata.keywords;
    }

    void
    _SetMetadataKeywords(UsdValidatorMetadata &metadata, const TfTokenVector &keywords)
    {
        metadata.keywords = keywords;
    }

    TfTokenVector
    _GetMetadataSchemaTypes(const UsdValidatorMetadata &metadata)
    {
        return metadata.schemaTypes;
    }

    void
    _SetMetadataSchemaTypes(UsdValidatorMetadata &metadata, const TfTokenVector &schemaTypes)
    {
        metadata.schemaTypes = schemaTypes;
    }

    PlugPluginPtr
    _GetMetadataPlugin(const UsdValidatorMetadata &metadata)
    {
        return metadata.pluginPtr;
    }

    void
    _SetMetadataPlugin(UsdValidatorMetadata &metadata, const PlugPluginPtr &plugin)
    {
        metadata.pluginPtr = plugin;
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
        .add_property("name", &_GetMetadataName, &_SetMetadataName)
        .add_property("plugin", &_GetMetadataPlugin, &_SetMetadataPlugin)
        .add_property("keywords", make_function(
            &_GetMetadataKeywords, return_value_policy<TfPySequenceToList>()), _SetMetadataKeywords)
        .def_readwrite("doc", &UsdValidatorMetadata::doc)
        .add_property("schemaTypes", make_function(
            &_GetMetadataSchemaTypes, return_value_policy<TfPySequenceToList>()), _SetMetadataSchemaTypes)
        .def_readwrite("isSuite", &UsdValidatorMetadata::isSuite);
}
