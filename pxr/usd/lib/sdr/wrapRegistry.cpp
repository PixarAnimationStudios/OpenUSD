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

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapRegistry()
{
    typedef SdrRegistry This;
    typedef TfWeakPtr<SdrRegistry> ThisPtr;

    class_<std::vector<SdrShaderNodeConstPtr>>("ShaderNodeList")
        .def(vector_indexing_suite<std::vector<SdrShaderNodeConstPtr>>())
        ;

    class_<This, ThisPtr, bases<NdrRegistry>, boost::noncopyable>("Registry", no_init)
        .def(TfPySingleton())
        .def("GetShaderNodeByIdentifier", &This::GetShaderNodeByIdentifier,
            (args("identifier"),
             args("typePriority") = NdrTokenVec()),
            return_internal_reference<>())
        .def("GetShaderNodeByIdentifierAndType",
            &This::GetShaderNodeByIdentifierAndType,
            (args("identifier"),
             args("nodeType")),
            return_internal_reference<>())
        .def("GetShaderNodeByName", &This::GetShaderNodeByName,
            (args("name"),
             args("typePriority") = NdrTokenVec(),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetShaderNodeByNameAndType",
            &This::GetShaderNodeByNameAndType,
            (args("name"),
             args("nodeType"),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetShaderNodeByURI", &This::GetShaderNodeByURI,
            return_internal_reference<>())
        .def("GetShaderNodesByIdentifier", &This::GetShaderNodesByIdentifier,
            (args("identifier")))
        .def("GetShaderNodesByName", &This::GetShaderNodesByName,
            (args("name"),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetShaderNodesByFamily", &This::GetShaderNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = NdrVersionFilterDefaultOnly))
        ;
}
