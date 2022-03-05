//
// Copyright 2018 Pixar
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
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>


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
_SetExtraDiscoveryPlugins(NdrRegistry& self, const boost::python::list& pylist)
{
    NdrDiscoveryPluginRefPtrVector plugins;
    std::vector<TfType> types;

    for (int i = 0; i < boost::python::len(pylist); ++i) {
        boost::python::extract<NdrDiscoveryPluginPtr> plugin(pylist[i]);
        if (plugin.check()) {
            NdrDiscoveryPluginPtr pluginPtr = plugin;
            if (pluginPtr) {
                plugins.push_back(pluginPtr);
            }
        } else {
            types.push_back(boost::python::extract<TfType>(pylist[i]));
        }
    }

    self.SetExtraDiscoveryPlugins(std::move(plugins));
    self.SetExtraDiscoveryPlugins(types);
}

struct ConstNodePtrToPython {
    static PyObject* convert(NdrNodeConstPtr node)
    {
        return boost::python::incref(boost::python::object(boost::python::ptr(node)).ptr());
    } 
};

void wrapRegistry()
{
    typedef NdrRegistry This;
    typedef TfWeakPtr<NdrRegistry> ThisPtr;

    boost::python::class_<std::vector<NdrDiscoveryPlugin*>>("DiscoveryPluginList")
        .def(boost::python::vector_indexing_suite<std::vector<NdrDiscoveryPlugin*>>())
        ;

    boost::python::class_<std::vector<NdrNodeConstPtr>>("NodeList")
        .def(boost::python::vector_indexing_suite<std::vector<NdrNodeConstPtr>>())
        ;

    // We need this for NodeList to convert elements.
    boost::python::to_python_converter<NdrNodeConstPtr, ConstNodePtrToPython>();

    boost::python::class_<This, ThisPtr, boost::noncopyable>("Registry", boost::python::no_init)
        .def("SetExtraDiscoveryPlugins", &_SetExtraDiscoveryPlugins)
        .def("SetExtraParserPlugins", &This::SetExtraParserPlugins)
        .def("GetSearchURIs", &This::GetSearchURIs)
        .def("GetNodeIdentifiers", &This::GetNodeIdentifiers,
            (boost::python::args("family") = TfToken(),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetNodeNames", &This::GetNodeNames,
            (boost::python::args("family") = TfToken()))
        .def("GetNodeByIdentifier", &This::GetNodeByIdentifier,
            (boost::python::args("identifier"),
             boost::python::args("typePriority") = NdrTokenVec()),
            boost::python::return_internal_reference<>())
        .def("GetNodeByIdentifierAndType", &This::GetNodeByIdentifierAndType,
            (boost::python::args("identifier"),
             boost::python::args("nodeType")),
            boost::python::return_internal_reference<>())
        .def("GetNodeByName", &This::GetNodeByName,
            (boost::python::args("name"),
             boost::python::args("typePriority") = NdrTokenVec(),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly),
            boost::python::return_internal_reference<>())
        .def("GetNodeByNameAndType", &This::GetNodeByNameAndType,
            (boost::python::args("name"),
             boost::python::args("nodeType"),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly),
            boost::python::return_internal_reference<>())

        .def("GetNodeFromAsset", &This::GetNodeFromAsset,
             (boost::python::arg("asset"), 
              boost::python::arg("metadata")=NdrTokenMap(),
              boost::python::arg("subIdentifier")=TfToken(),
              boost::python::arg("sourceType")=TfToken()),
             boost::python::return_internal_reference<>())
        .def("GetNodeFromSourceCode", &This::GetNodeFromSourceCode,
             (boost::python::arg("sourceCode"), 
              boost::python::arg("sourceType"),
              boost::python::arg("metadata")=NdrTokenMap()),
             boost::python::return_internal_reference<>())

        .def("GetNodesByIdentifier", &This::GetNodesByIdentifier,
            (boost::python::args("identifier")))
        .def("GetNodesByName", &This::GetNodesByName,
            (boost::python::args("name"),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetNodesByFamily", &This::GetNodesByFamily,
            (boost::python::args("family") = TfToken(),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetAllNodeSourceTypes", &This::GetAllNodeSourceTypes)
        ;

    // We wrap this directly under Ndr rather than under the Registry class
    // because it's not really part of the Registry, but we want to expose this
    // for testing property correctness
    boost::python::def("_ValidateProperty", _ValidateProperty);

    Ndr_ValidatePropertyAnnotatedBool::Wrap<
        Ndr_ValidatePropertyAnnotatedBool>("_AnnotatedBool", "message");
}
