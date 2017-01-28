//
// Copyright 2016 Pixar
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
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

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

void
wrapNode()
{
    typedef PcpNodeRef This;

    scope s = class_<This>
        ("NodeRef", no_init)

        .add_property("site", &This::GetSite)
        .add_property("path", 
                      make_function(&This::GetPath, 
                                    return_value_policy<return_by_value>()))
        .add_property("layerStack", 
                      make_function(&This::GetLayerStack, 
                                    return_value_policy<return_by_value>()))

        .add_property("parent", &::_GetParentNode)
        .add_property("origin", &::_GetOriginNode)
        .add_property("children", 
                      make_function(&::_GetChildren, 
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

        .def("GetRootNode", &::_GetRootNode)
        .def("GetOriginRootNode", &::_GetOriginRootNode)

        .def("IsDirect", &This::IsDirect)
        .def("IsDueToAncestor", &This::IsDueToAncestor)
        .def("GetDepthBelowIntroduction", &This::GetDepthBelowIntroduction)
        .def("GetIntroPath", &This::GetIntroPath)

        .def("CanContributeSpecs", &This::CanContributeSpecs)

        .def(self == self)
        .def(self != self)
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
