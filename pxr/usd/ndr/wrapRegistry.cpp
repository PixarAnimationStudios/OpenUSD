//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

// Declare 'private' helper function implemented in registry.cpp
// for testing purposes.
bool NdrRegistry_ValidateProperty(
    const NdrNodeConstPtr& node,
    const NdrPropertyConstPtr& property,
    std::string* errorMessage);

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

struct Ndr_ValidatePropertyAnnotatedBool :
    public TfPyAnnotatedBoolResult<std::string>
{
    Ndr_ValidatePropertyAnnotatedBool(
        bool value, const std::string& message) :
        TfPyAnnotatedBoolResult<std::string>(value, message) {}
};

static Ndr_ValidatePropertyAnnotatedBool
_ValidateProperty(
    const NdrNode& node,
    const NdrProperty& property)
{
    std::string errorMessage;
    bool isValid = NdrRegistry_ValidateProperty(&node, &property, &errorMessage);
    return Ndr_ValidatePropertyAnnotatedBool(isValid, errorMessage);
}

static void
_SetExtraDiscoveryPlugins(NdrRegistry& self, const list& pylist)
{
    NdrDiscoveryPluginRefPtrVector plugins;
    std::vector<TfType> types;

    for (int i = 0; i < len(pylist); ++i) {
        extract<NdrDiscoveryPluginPtr> plugin(pylist[i]);
        if (plugin.check()) {
            NdrDiscoveryPluginPtr pluginPtr = plugin;
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
    static PyObject* convert(NdrNodeConstPtr node)
    {
        return incref(object(ptr(node)).ptr());
    } 
};

void wrapRegistry()
{
    typedef NdrRegistry This;
    typedef TfWeakPtr<NdrRegistry> ThisPtr;

    class_<std::vector<NdrDiscoveryPlugin*>>("DiscoveryPluginList")
        .def(vector_indexing_suite<std::vector<NdrDiscoveryPlugin*>>())
        ;

    class_<std::vector<NdrNodeConstPtr>>("NodeList")
        .def(vector_indexing_suite<std::vector<NdrNodeConstPtr>>())
        ;

    // We need this for NodeList to convert elements.
    to_python_converter<NdrNodeConstPtr, ConstNodePtrToPython>();

    class_<This, ThisPtr, boost::noncopyable>("Registry", no_init)
        .def("SetExtraDiscoveryPlugins", &_SetExtraDiscoveryPlugins)
        .def("SetExtraParserPlugins", &This::SetExtraParserPlugins)
        .def("GetSearchURIs", &This::GetSearchURIs)
        .def("GetNodeIdentifiers", &This::GetNodeIdentifiers,
            (args("family") = TfToken(),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetNodeNames", &This::GetNodeNames,
            (args("family") = TfToken()))
        .def("GetNodeByIdentifier", &This::GetNodeByIdentifier,
            (args("identifier"),
             args("typePriority") = NdrTokenVec()),
            return_internal_reference<>())
        .def("GetNodeByIdentifierAndType", &This::GetNodeByIdentifierAndType,
            (args("identifier"),
             args("nodeType")),
            return_internal_reference<>())
        .def("GetNodeByName", &This::GetNodeByName,
            (args("name"),
             args("typePriority") = NdrTokenVec(),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetNodeByNameAndType", &This::GetNodeByNameAndType,
            (args("name"),
             args("nodeType"),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())

        .def("GetNodeFromAsset", &This::GetNodeFromAsset,
             (arg("asset"), 
              arg("metadata")=NdrTokenMap(),
              arg("subIdentifier")=TfToken(),
              arg("sourceType")=TfToken()),
             return_internal_reference<>())
        .def("GetNodeFromSourceCode", &This::GetNodeFromSourceCode,
             (arg("sourceCode"), 
              arg("sourceType"),
              arg("metadata")=NdrTokenMap()),
             return_internal_reference<>())

        .def("GetNodesByIdentifier", &This::GetNodesByIdentifier,
            (args("identifier")))
        .def("GetNodesByName", &This::GetNodesByName,
            (args("name"),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetNodesByFamily", &This::GetNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetAllNodeSourceTypes", &This::GetAllNodeSourceTypes)
        ;

    // We wrap this directly under Ndr rather than under the Registry class
    // because it's not really part of the Registry, but we want to expose this
    // for testing property correctness
    def("_ValidateProperty", _ValidateProperty);

    Ndr_ValidatePropertyAnnotatedBool::Wrap<
        Ndr_ValidatePropertyAnnotatedBool>("_AnnotatedBool", "message");
}
