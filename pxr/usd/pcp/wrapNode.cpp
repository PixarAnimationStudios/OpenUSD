//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define PCP_GET_NODE_FN(nodeFn)                                         \
    static boost::python::object                                        \
    _ ## nodeFn(const PcpNodeRef& node)                                 \
    {                                                                   \
        PcpNodeRef n = node.nodeFn();                                   \
        return n ? boost::python::object(n) : boost::python::object();  \
    }

PCP_GET_NODE_FN(GetParentNode);
PCP_GET_NODE_FN(GetOriginNode);
PCP_GET_NODE_FN(GetRootNode);
PCP_GET_NODE_FN(GetOriginRootNode);

static PcpNodeRefVector
_GetChildren(const PcpNodeRef& node)
{
    return Pcp_GetChildren(node);
}

// Test function to retrieve an invalid PcpNodeRef in Python
static PcpNodeRef
_GetInvalidPcpNode()
{
    return {};
}

} // anonymous namespace 

// We override __getattribute__ for PcpNode to check object validity and raise
// an exception instead of crashing from Python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on PcpNodeRef.
static object
__getattribute__(object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', or if the
    // node is valid.
    if ((name[0] == '_' && name[1] == '_') ||
        bool(extract<PcpNodeRef &>(selfObj)())){
        // Dispatch to object's __getattribute__.
        return (*_object__getattribute__)(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Invalid access to %s", TfPyRepr(selfObj).c_str()));
    }
    // Unreachable.
    return object();
}

void
wrapNode()
{

    def("_GetInvalidPcpNode", &_GetInvalidPcpNode);

    typedef PcpNodeRef This;

    class_<This> clsObj("NodeRef", no_init);
    clsObj
        .add_property("site", &This::GetSite)
        .add_property("path", 
                      make_function(&This::GetPath, 
                                    return_value_policy<return_by_value>()))
        .add_property("layerStack", 
                      make_function(&This::GetLayerStack, 
                                    return_value_policy<return_by_value>()))

        .add_property("parent", &_GetParentNode)
        .add_property("origin", &_GetOriginNode)
        .add_property("children", 
                      make_function(&_GetChildren, 
                                    return_value_policy<TfPySequenceToList>()))

        .add_property("arcType", &This::GetArcType)
        .add_property("mapToParent", 
                      make_function(&This::GetMapToParent,
                                    return_value_policy<return_by_value>()))
        .add_property("mapToRoot", 
                      make_function(&This::GetMapToRoot,
                                    return_value_policy<return_by_value>()))
        
        .add_property("siblingNumAtOrigin", &This::GetSiblingNumAtOrigin)
        .add_property("namespaceDepth", &This::GetNamespaceDepth)

        .add_property("hasSymmetry", &This::HasSymmetry)
        .add_property("hasSpecs", &This::HasSpecs)
        .add_property("isInert", &This::IsInert)
        .add_property("isCulled", &This::IsCulled)
        .add_property("isRestricted", &This::IsRestricted)
        .add_property("permission", &This::GetPermission)

        .def("GetRootNode", &_GetRootNode)
        .def("GetOriginRootNode", &_GetOriginRootNode)

        .def("IsRootNode", &This::IsRootNode)
        .def("IsDueToAncestor", &This::IsDueToAncestor)
        .def("GetDepthBelowIntroduction", &This::GetDepthBelowIntroduction)
        .def("GetIntroPath", &This::GetIntroPath)
        .def("GetPathAtIntroduction", &This::GetPathAtIntroduction)

        .def("CanContributeSpecs", &This::CanContributeSpecs)
        .def("GetSpecContributionRestrictedDepth",
            &This::GetSpecContributionRestrictedDepth)

        .def(self == self)
        .def(self != self)
        .def(!self)
        ;

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = object(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", __getattribute__);
}