//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/return_internal_reference.hpp"
#include "pxr/external/boost/python/suite/indexing/vector_indexing_suite.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapRegistry()
{
    typedef SdrRegistry This;
    typedef TfWeakPtr<SdrRegistry> ThisPtr;

    class_<std::vector<SdrShaderNodeConstPtr>>("ShaderNodeList")
        .def(vector_indexing_suite<std::vector<SdrShaderNodeConstPtr>>())
        ;

    class_<This, ThisPtr, bases<NdrRegistry>, noncopyable>("Registry", no_init)
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

        .def("GetShaderNodeFromAsset", &This::GetShaderNodeFromAsset,
             (arg("shaderAsset"),
              arg("metadata")=NdrTokenMap(),
              arg("subIdentifier")=TfToken(),
              arg("sourceType")=TfToken()),
             return_internal_reference<>())
        .def("GetShaderNodeFromSourceCode", &This::GetShaderNodeFromSourceCode,
             (arg("sourceCode"), arg("sourceType"), 
              arg("metadata")=NdrTokenMap()),
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
