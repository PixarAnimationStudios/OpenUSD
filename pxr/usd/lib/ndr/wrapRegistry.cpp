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
#include "pxr/base/tf/pySingleton.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static void
_SetExtraDiscoveryPlugins(NdrRegistry& self, const list& pylist)
{
    std::vector<TfWeakPtr<NdrDiscoveryPlugin>> result;

    for (int i = 0; i < len(pylist); ++i) {
        result.push_back(extract<TfWeakPtr<NdrDiscoveryPlugin>>(pylist[i]));
    }

    self.SetExtraDiscoveryPlugins(result);
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

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<std::vector<NdrDiscoveryPlugin*>>("DiscoveryPluginList")
        .def(vector_indexing_suite<std::vector<NdrDiscoveryPlugin*>>())
        ;

    class_<std::vector<NdrNodeConstPtr>>("NodeList")
        .def(vector_indexing_suite<std::vector<NdrNodeConstPtr>>())
        ;

    // We need this for NodeList to convert elements.
    to_python_converter<NdrNodeConstPtr, ConstNodePtrToPython>();

    class_<This, ThisPtr, boost::noncopyable>("Registry", no_init)
        .def(TfPySingleton())
        .def("SetExtraDiscoveryPlugins", &_SetExtraDiscoveryPlugins)
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
        .def("GetNodeByURI", &This::GetNodeByURI,
            return_internal_reference<>())
        .def("GetNodesByIdentifier", &This::GetNodesByIdentifier,
            (args("identifier")))
        .def("GetNodesByName", &This::GetNodesByName,
            (args("name"),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetNodesByFamily", &This::GetNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetAllNodeSourceTypes", &This::GetAllNodeSourceTypes, copyRefPolicy)
        ;
}
