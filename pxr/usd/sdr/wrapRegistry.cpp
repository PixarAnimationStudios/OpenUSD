//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

PXR_NAMESPACE_OPEN_SCOPE

// Declare 'private' helper function implemented in registry.cpp
// for testing purposes.
bool SdrRegistry_ValidateProperty(
    const SdrShaderNodeConstPtr& node,
    const SdrShaderPropertyConstPtr& property,
    std::string* errorMessage);

PXR_NAMESPACE_CLOSE_SCOPE

struct Sdr_ValidatePropertyAnnotatedBool :
    public TfPyAnnotatedBoolResult<std::string>
{
    Sdr_ValidatePropertyAnnotatedBool(
        bool value, const std::string& message) :
        TfPyAnnotatedBoolResult<std::string>(value, message) {}
};

static Sdr_ValidatePropertyAnnotatedBool
_ValidateProperty(
    const SdrShaderNode& node,
    const SdrShaderProperty& property)
{
    std::string errorMessage;
    bool isValid = SdrRegistry_ValidateProperty(&node, &property, &errorMessage);
    return Sdr_ValidatePropertyAnnotatedBool(isValid, errorMessage);
}

static void
_SetExtraDiscoveryPlugins(SdrRegistry& self, const nb::list& pylist)
{
    SdrDiscoveryPluginRefPtrVector plugins;
    std::vector<TfType> types;

    for (size_t i = 0; i < pylist.size(); ++i) {
        nb::handle item = pylist[i];
        SdrDiscoveryPlugin* rawPlugin = nullptr;
        SdrDiscoveryPluginPtr pluginPtr;
        if (nb::try_cast<SdrDiscoveryPlugin*>(item, rawPlugin)) {
            if (rawPlugin) {
                plugins.push_back(SdrDiscoveryPluginRefPtr(rawPlugin));
            }
        } else if (nb::try_cast<SdrDiscoveryPluginPtr>(item, pluginPtr)) {
            if (pluginPtr) {
                plugins.push_back(pluginPtr);
            }
        } else {
            types.push_back(nb::cast<TfType>(item));
        }
    }

    self.SetExtraDiscoveryPlugins(std::move(plugins));
    self.SetExtraDiscoveryPlugins(types);
}

void wrapRegistry(nanobind::module_ &m)
{
    typedef SdrRegistry This;

    nb::class_<This> cls(m, "Registry",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbSingleton(cls);
    cls.def("SetExtraDiscoveryPlugins", &_SetExtraDiscoveryPlugins)
        .def("SetExtraParserPlugins", &This::SetExtraParserPlugins)
        .def("AddDiscoveryResult", 
            (void(SdrRegistry::*)(const SdrShaderNodeDiscoveryResult&))
            &This::AddDiscoveryResult)
        .def("GetSearchURIs", &This::GetSearchURIs)
        .def("GetShaderNodeIdentifiers", &This::GetShaderNodeIdentifiers,
            nb::arg("function") = TfToken(),
            nb::arg("filter") = SdrVersionFilterDefaultOnly)
        .def("GetShaderNodeNames", &This::GetShaderNodeNames,
            nb::arg("function") = TfToken())
        .def("GetShaderNodeByIdentifier", &This::GetShaderNodeByIdentifier,
            nb::arg("identifier"),
            nb::arg("typePriority") = SdrTokenVec(),
            nb::rv_policy::reference_internal)
        .def("GetShaderNodeByIdentifierAndType",
            &This::GetShaderNodeByIdentifierAndType,
            nb::arg("identifier"),
            nb::arg("nodeType"),
            nb::rv_policy::reference_internal)
        .def("GetShaderNodeByIdentifierAndSystem",
            &This::GetShaderNodeByIdentifierAndSystem,
            nb::arg("identifier"),
            nb::arg("shadingSystem"),
            nb::rv_policy::reference_internal)

        .def("GetShaderNodeFromAsset", &This::GetShaderNodeFromAsset,
             nb::arg("shaderAsset"),
             nb::arg("metadata") = SdrTokenMap(),
             nb::arg("subIdentifier") = TfToken(),
             nb::arg("sourceType") = TfToken(),
             nb::rv_policy::reference_internal)
        .def("GetShaderNodeFromSourceCode", &This::GetShaderNodeFromSourceCode,
             nb::arg("sourceCode"),
             nb::arg("sourceType"),
             nb::arg("metadata") = SdrTokenMap(),
             nb::rv_policy::reference_internal)

        .def("GetShaderNodeByName", &This::GetShaderNodeByName,
            nb::arg("name"),
            nb::arg("typePriority") = SdrTokenVec(),
            nb::arg("filter") = SdrVersionFilterDefaultOnly,
            nb::rv_policy::reference_internal)
        .def("GetShaderNodeByNameAndType", &This::GetShaderNodeByNameAndType,
            nb::arg("name"),
            nb::arg("nodeType"),
            nb::arg("filter") = SdrVersionFilterDefaultOnly,
            nb::rv_policy::reference_internal)
        .def("GetShaderNodeByNameAndSystem", &This::GetShaderNodeByNameAndSystem,
            nb::arg("name"),
            nb::arg("shadingSystem"),
            nb::arg("filter") = SdrVersionFilterDefaultOnly,
            nb::rv_policy::reference_internal)
        .def("GetShaderNodesByIdentifier", &This::GetShaderNodesByIdentifier,
            nb::arg("identifier"),
            nb::rv_policy::reference)
        .def("GetShaderNodesByName", &This::GetShaderNodesByName,
            nb::arg("name"),
            nb::arg("filter") = SdrVersionFilterDefaultOnly,
            nb::rv_policy::reference)
        .def("GetShaderNodesByFunction", &This::GetShaderNodesByFunction,
            nb::arg("function") = TfToken(),
            nb::arg("filter") = SdrVersionFilterDefaultOnly,
            nb::rv_policy::reference)
        .def("GetShaderNodesByFamily", &This::GetShaderNodesByFamily,
            nb::arg("family") = TfToken(),
            nb::arg("filter") = SdrVersionFilterDefaultOnly,
            nb::rv_policy::reference)
        .def("GetAllShaderNodeSourceTypes", &This::GetAllShaderNodeSourceTypes)
        .def("RunQuery", &This::RunQuery)
        .def("ParseAll", &This::ParseAll)
        .def("GetAllShaderNodes", &This::GetAllShaderNodes,
            nb::rv_policy::reference)
        ;

    // We wrap this directly under Sdr rather than under the Registry class
    // because it's not really part of the Registry, but we want to expose this
    // for testing property correctness
    m.def("_ValidateProperty", _ValidateProperty);

    Sdr_ValidatePropertyAnnotatedBool::WrapNb<
        Sdr_ValidatePropertyAnnotatedBool>(m, "_AnnotatedBool", "message");
}
