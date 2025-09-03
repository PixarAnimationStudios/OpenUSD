//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/return_internal_reference.hpp"
#include "pxr/external/boost/python/suite/indexing/vector_indexing_suite.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
_SetExtraDiscoveryPlugins(SdrRegistry& self, const list& pylist)
{
    SdrDiscoveryPluginRefPtrVector plugins;
    std::vector<TfType> types;

    for (int i = 0; i < len(pylist); ++i) {
        extract<SdrDiscoveryPluginPtr> plugin(pylist[i]);
        if (plugin.check()) {
            SdrDiscoveryPluginPtr pluginPtr = plugin;
            if (pluginPtr) {
                plugins.push_back(pluginPtr);
            }
        } else {
            types.push_back(extract<TfType>(pylist[i]));
        }
    }

    self.SetExtraDiscoveryPlugins(std::move(plugins));
    self.SetExtraDiscoveryPlugins(types);
}

struct ConstNodePtrToPython {
    static PyObject* convert(SdrShaderNodeConstPtr node)
    {
        return incref(object(ptr(node)).ptr());
    } 
};

void wrapRegistry()
{
    typedef SdrRegistry This;
    typedef TfWeakPtr<SdrRegistry> ThisPtr;

    class_<std::vector<SdrShaderNodeConstPtr>>("ShaderNodeList")
        .def(vector_indexing_suite<std::vector<SdrShaderNodeConstPtr>>())
        ;

    class_<This, ThisPtr, noncopyable>("Registry", no_init)
        .def(TfPySingleton())
        .def("SetExtraDiscoveryPlugins", &_SetExtraDiscoveryPlugins)
        .def("SetExtraParserPlugins", &This::SetExtraParserPlugins)
        .def("AddDiscoveryResult", 
            (void(SdrRegistry::*)(const SdrShaderNodeDiscoveryResult&))
            &This::AddDiscoveryResult)
        .def("GetSearchURIs", &This::GetSearchURIs)
        .def("GetShaderNodeIdentifiers", &This::GetShaderNodeIdentifiers,
            (args("family") = TfToken(),
             args("filter") = SdrVersionFilterDefaultOnly))
        .def("GetShaderNodeNames", &This::GetShaderNodeNames,
            (args("family") = TfToken()))
        .def("GetShaderNodeByIdentifier", &This::GetShaderNodeByIdentifier,
            (args("identifier"),
             args("typePriority") = SdrTokenVec()),
            return_internal_reference<>())
        .def("GetShaderNodeByIdentifierAndType",
            &This::GetShaderNodeByIdentifierAndType,
            (args("identifier"),
             args("nodeType")),
            return_internal_reference<>())

        .def("GetShaderNodeFromAsset", &This::GetShaderNodeFromAsset,
             (arg("shaderAsset"),
              arg("metadata")=SdrTokenMap(),
              arg("subIdentifier")=TfToken(),
              arg("sourceType")=TfToken()),
             return_internal_reference<>())
        .def("GetShaderNodeFromSourceCode", &This::GetShaderNodeFromSourceCode,
             (arg("sourceCode"), arg("sourceType"), 
              arg("metadata")=SdrTokenMap()),
             return_internal_reference<>())

        .def("GetShaderNodeByName", &This::GetShaderNodeByName,
            (args("name"),
             args("typePriority") = SdrTokenVec(),
             args("filter") = SdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetShaderNodeByNameAndType", &This::GetShaderNodeByNameAndType,
            (args("name"),
             args("nodeType"),
             args("filter") = SdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetShaderNodesByIdentifier", &This::GetShaderNodesByIdentifier,
            (args("identifier")))
        .def("GetShaderNodesByName", &This::GetShaderNodesByName,
            (args("name"),
             args("filter") = SdrVersionFilterDefaultOnly))
        .def("GetShaderNodesByFamily", &This::GetShaderNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = SdrVersionFilterDefaultOnly))
        .def("GetAllShaderNodeSourceTypes", &This::GetAllShaderNodeSourceTypes)
        ;

    // We wrap this directly under Sdr rather than under the Registry class
    // because it's not really part of the Registry, but we want to expose this
    // for testing property correctness
    def("_ValidateProperty", _ValidateProperty);

    Sdr_ValidatePropertyAnnotatedBool::Wrap<
        Sdr_ValidatePropertyAnnotatedBool>("_AnnotatedBool", "message");
}
