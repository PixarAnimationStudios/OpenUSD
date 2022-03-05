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
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

void wrapRegistry()
{
    typedef SdrRegistry This;
    typedef TfWeakPtr<SdrRegistry> ThisPtr;

    boost::python::class_<std::vector<SdrShaderNodeConstPtr>>("ShaderNodeList")
        .def(boost::python::vector_indexing_suite<std::vector<SdrShaderNodeConstPtr>>())
        ;

    boost::python::class_<This, ThisPtr, boost::python::bases<NdrRegistry>, boost::noncopyable>("Registry", boost::python::no_init)
        .def(TfPySingleton())
        .def("GetShaderNodeByIdentifier", &This::GetShaderNodeByIdentifier,
            (boost::python::args("identifier"),
             boost::python::args("typePriority") = NdrTokenVec()),
            boost::python::return_internal_reference<>())
        .def("GetShaderNodeByIdentifierAndType",
            &This::GetShaderNodeByIdentifierAndType,
            (boost::python::args("identifier"),
             boost::python::args("nodeType")),
            boost::python::return_internal_reference<>())

        .def("GetShaderNodeFromAsset", &This::GetShaderNodeFromAsset,
             (boost::python::arg("shaderAsset"),
              boost::python::arg("metadata")=NdrTokenMap(),
              boost::python::arg("subIdentifier")=TfToken(),
              boost::python::arg("sourceType")=TfToken()),
             boost::python::return_internal_reference<>())
        .def("GetShaderNodeFromSourceCode", &This::GetShaderNodeFromSourceCode,
             (boost::python::arg("sourceCode"), boost::python::arg("sourceType"), 
              boost::python::arg("metadata")=NdrTokenMap()),
             boost::python::return_internal_reference<>())

        .def("GetShaderNodeByName", &This::GetShaderNodeByName,
            (boost::python::args("name"),
             boost::python::args("typePriority") = NdrTokenVec(),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly),
            boost::python::return_internal_reference<>())
        .def("GetShaderNodeByNameAndType",
            &This::GetShaderNodeByNameAndType,
            (boost::python::args("name"),
             boost::python::args("nodeType"),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly),
            boost::python::return_internal_reference<>())
        .def("GetShaderNodesByIdentifier", &This::GetShaderNodesByIdentifier,
            (boost::python::args("identifier")))
        .def("GetShaderNodesByName", &This::GetShaderNodesByName,
            (boost::python::args("name"),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetShaderNodesByFamily", &This::GetShaderNodesByFamily,
            (boost::python::args("family") = TfToken(),
             boost::python::args("filter") = NdrVersionFilterDefaultOnly))
        ;
}
