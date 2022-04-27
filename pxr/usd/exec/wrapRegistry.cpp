//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/exec/registry.h"
#include "pxr/usd/exec/execNode.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapRegistry()
{
    typedef ExecRegistry This;
    typedef TfWeakPtr<ExecRegistry> ThisPtr;

    class_<std::vector<ExecNodeConstPtr>>("ExecNodeList")
        .def(vector_indexing_suite<std::vector<ExecNodeConstPtr>>())
        ;

    class_<This, ThisPtr, bases<NdrRegistry>, boost::noncopyable>("Registry", no_init)
        .def(TfPySingleton())
        .def("GetExecNodeByIdentifier", &This::GetExecNodeByIdentifier,
            (args("identifier"),
             args("typePriority") = NdrTokenVec()),
            return_internal_reference<>())
        .def("GetExecNodeByIdentifierAndType",
            &This::GetExecNodeByIdentifierAndType,
            (args("identifier"),
             args("nodeType")),
            return_internal_reference<>())

        .def("GetExecNodeFromAsset", &This::GetExecNodeFromAsset,
             (arg("execAsset"),
              arg("metadata")=NdrTokenMap(),
              arg("subIdentifier")=TfToken(),
              arg("sourceType")=TfToken()),
             return_internal_reference<>())
        .def("GetExecNodeFromSourceCode", &This::GetExecNodeFromSourceCode,
             (arg("sourceCode"), arg("sourceType"), 
              arg("metadata")=NdrTokenMap()),
             return_internal_reference<>())

        .def("GetExecNodeByName", &This::GetExecNodeByName,
            (args("name"),
             args("typePriority") = NdrTokenVec(),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetExecNodeByNameAndType",
            &This::GetExecNodeByNameAndType,
            (args("name"),
             args("nodeType"),
             args("filter") = NdrVersionFilterDefaultOnly),
            return_internal_reference<>())
        .def("GetExecNodesByIdentifier", &This::GetExecNodesByIdentifier,
            (args("identifier")))
        .def("GetExecNodesByName", &This::GetExecNodesByName,
            (args("name"),
             args("filter") = NdrVersionFilterDefaultOnly))
        .def("GetExecNodesByFamily", &This::GetExecNodesByFamily,
            (args("family") = TfToken(),
             args("filter") = NdrVersionFilterDefaultOnly))
        ;
}
