//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define PCP_GET_NODE_FN(nodeFn)                                         \
    static nanobind::object                                             \
    _ ## nodeFn(const PcpNodeRef& node)                                 \
    {                                                                   \
        PcpNodeRef n = node.nodeFn();                                   \
        return n ? nanobind::cast(n) : nanobind::none();                \
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
static nanobind::object
__getattribute__(nanobind::object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', or if the
    // node is valid.
    if ((name[0] == '_' && name[1] == '_') ||
        bool(nanobind::cast<PcpNodeRef &>(selfObj))){
        // Dispatch to object's __getattribute__.
        return nanobind::borrow<nanobind::object>(
            _object__getattribute__->ptr())(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Invalid access to %s", TfPyRepr(selfObj).c_str()));
    }
    // Unreachable.
    return nanobind::object();
}

void
wrapNode(nanobind::module_ &m)
{
    namespace nb = nanobind;

    m.def("_GetInvalidPcpNode", &_GetInvalidPcpNode);

    typedef PcpNodeRef This;

    nb::class_<This> clsObj(m, "NodeRef");
    clsObj
        .def_prop_ro("site", &This::GetSite)
        .def_prop_ro("path", &This::GetPath, nb::rv_policy::copy)
        .def_prop_ro("layerStack", &This::GetLayerStack, nb::rv_policy::copy)

        .def_prop_ro("parent", &_GetParentNode)
        .def_prop_ro("origin", &_GetOriginNode)
        .def_prop_ro("children", &_GetChildren)

        .def_prop_ro("arcType", &This::GetArcType)
        .def_prop_ro("mapToParent", &This::GetMapToParent, nb::rv_policy::copy)
        .def_prop_ro("mapToRoot", &This::GetMapToRoot, nb::rv_policy::copy)
        
        .def_prop_ro("siblingNumAtOrigin", &This::GetSiblingNumAtOrigin)
        .def_prop_ro("namespaceDepth", &This::GetNamespaceDepth)

        .def_prop_ro("hasSymmetry", &This::HasSymmetry)
        .def_prop_ro("hasSpecs", &This::HasSpecs)
        .def_prop_ro("isInert", &This::IsInert)
        .def_prop_ro("isCulled", &This::IsCulled)
        .def_prop_ro("isRestricted", &This::IsRestricted)
        .def_prop_ro("permission", &This::GetPermission)

        .def("GetRootNode", &_GetRootNode)
        .def("GetOriginRootNode", &_GetOriginRootNode)

        .def("IsRootNode", &This::IsRootNode)
        .def("IsDueToAncestor", &This::IsDueToAncestor)
        .def("HasTransitiveDirectDependency",
            &This::HasTransitiveDirectDependency)
        .def("HasTransitiveAncestralDependency",
            &This::HasTransitiveAncestralDependency)
        .def("GetDepthBelowIntroduction", &This::GetDepthBelowIntroduction)
        .def("GetIntroPath", &This::GetIntroPath)
        .def("GetPathAtIntroduction", &This::GetPathAtIntroduction)

        .def("CanContributeSpecs", &This::CanContributeSpecs)
        .def("GetSpecContributionRestrictedDepth",
            &This::GetSpecContributionRestrictedDepth)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(!nb::self)
        ;

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ =
        nb::cast<TfPyObjWrapper>(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", __getattribute__);
}
