//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyEnum.h"
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
        .def("AddDiscoveryResult", 
            (void(NdrRegistry::*)(const NdrNodeDiscoveryResult&))
            &This::AddDiscoveryResult)
        .def("AddDiscoveryResult", 
            (void(SdrRegistry::*)(const SdrShaderNodeDiscoveryResult&))
            &This::AddDiscoveryResult)
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

        .def("GetShaderNodeByName",
                (SdrShaderNodeConstPtr (SdrRegistry::*)(
                    const std::string&,
                    const SdrTokenVec&,
                    SdrVersionFilter)) &This::GetShaderNodeByName,
            (args("name"),
             args("typePriority") = SdrTokenVec(),
             args("filter") = SdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetShaderNodeByName",
                (SdrShaderNodeConstPtr (SdrRegistry::*)(
                    const std::string&,
                    const NdrTokenVec&,
                    NdrVersionFilter)) &This::GetShaderNodeByName,
            (args("name"),
             args("typePriority") = NdrTokenVec(),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())

        .def("GetShaderNodeByNameAndType",
                (SdrShaderNodeConstPtr (SdrRegistry::*)(
                    const std::string&,
                    const TfToken& nodeType,
                    SdrVersionFilter)) &This::GetShaderNodeByNameAndType,
            (args("name"),
             args("nodeType"),
             args("filter") = SdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetShaderNodeByNameAndType",
                (SdrShaderNodeConstPtr (SdrRegistry::*)(
                    const std::string&,
                    const TfToken& nodeType,
                    NdrVersionFilter)) &This::GetShaderNodeByNameAndType,
            (args("name"),
             args("nodeType"),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())

        .def("GetShaderNodesByIdentifier", &This::GetShaderNodesByIdentifier,
            (args("identifier")))

        .def("GetShaderNodesByName",
                (SdrShaderNodePtrVec (SdrRegistry::*)(
                    const std::string&,
                    SdrVersionFilter)) &This::GetShaderNodesByName,
            (args("name"),
             args("filter") = SdrVersionFilterDefaultOnly))
        .def("GetShaderNodesByName",
                (SdrShaderNodePtrVec (SdrRegistry::*)(
                    const std::string&,
                    NdrVersionFilter)) &This::GetShaderNodesByName,
            (args("name"),
             args("filter") = NdrVersionFilterDefaultOnly))

        .def("GetShaderNodesByFamily",
                (SdrShaderNodePtrVec (SdrRegistry::*)(
                    const TfToken&,
                    SdrVersionFilter)) &This::GetShaderNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = SdrVersionFilterDefaultOnly))
        .def("GetShaderNodesByFamily",
                (SdrShaderNodePtrVec (SdrRegistry::*)(
                    const TfToken&,
                    NdrVersionFilter)) &This::GetShaderNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = NdrVersionFilterDefaultOnly))

        .def("GetAllShaderNodeSourceTypes", &This::GetAllShaderNodeSourceTypes)
        ;
}
