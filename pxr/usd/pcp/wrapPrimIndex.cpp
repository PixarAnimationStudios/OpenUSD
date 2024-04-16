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
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/siteUtils.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static SdfPrimSpecHandleVector
_GetPrimStack(const PcpPrimIndex& self)
{
    SdfPrimSpecHandleVector primStack;

    if (self.IsUsd()) {
        // Prim ranges are not cached in USD so GetPrimRange will always 
        // be empty. But since getting the primStack from prim index's prim 
        // range is python only API, we can build the prim stack that matches
        // what the prim range would be if we computed and cached it.
        const PcpNodeRange nodeRange = self.GetNodeRange();
        for (auto it = nodeRange.first; it != nodeRange.second; ++it) {
            const PcpNodeRef &node = *it;
            if (!node.CanContributeSpecs()) {
                continue;
            }
            const SdfLayerRefPtrVector &layers = 
                node.GetLayerStack()->GetLayers();
            for (const auto &layer : layers) {
                if (SdfPrimSpecHandle primSpec = 
                        layer->GetPrimAtPath(node.GetPath())) {
                    primStack.push_back(std::move(primSpec));
                }
            }
        }
    } else {
        const PcpPrimRange primRange = self.GetPrimRange();

        primStack.reserve(std::distance(primRange.first, primRange.second));
        for(const auto &path : primRange) {
            primStack.push_back(SdfGetPrimAtPath(path));
        }
    }

    return primStack;
}

static boost::python::tuple
_ComputePrimChildNames( PcpPrimIndex &index )
{
    TfTokenVector nameOrder;
    PcpTokenSet prohibitedNameSet;
    index.ComputePrimChildNames(&nameOrder, &prohibitedNameSet);
    TfTokenVector prohibitedNamesVector(prohibitedNameSet.begin(),
                                        prohibitedNameSet.end());
    return boost::python::make_tuple(nameOrder, prohibitedNamesVector);
}

static TfTokenVector
_ComputePrimPropertyNames( PcpPrimIndex &index )
{
    TfTokenVector result;
    index.ComputePrimPropertyNames(&result);
    return result;
}

} // anonymous namespace 

void wrapPrimIndex()
{
    typedef PcpPrimIndex This;

    class_<This>("PrimIndex", "", no_init)
        .add_property("primStack", 
                      make_function(&_GetPrimStack,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("rootNode", &This::GetRootNode)
        .add_property("hasAnyPayloads", &This::HasAnyPayloads)
        .add_property("localErrors", 
                      make_function(&This::GetLocalErrors,
                                    return_value_policy<TfPySequenceToList>()))

        .def("IsValid", &This::IsValid)
        .def("IsInstanceable", &This::IsInstanceable)

        .def("ComputePrimChildNames", &_ComputePrimChildNames)
        .def("ComputePrimPropertyNames",
            &_ComputePrimPropertyNames,
            return_value_policy<TfPySequenceToList>())
        .def("ComposeAuthoredVariantSelections",
             &This::ComposeAuthoredVariantSelections,
            return_value_policy<TfPyMapToDictionary>())
        .def("GetSelectionAppliedForVariantSet",
            &This::GetSelectionAppliedForVariantSet)

        .def("GetNodeProvidingSpec", 
            (PcpNodeRef (This::*) (const SdfPrimSpecHandle&) const)
                (&This::GetNodeProvidingSpec),
            args("primSpec"))
        .def("GetNodeProvidingSpec", 
            (PcpNodeRef (This::*) (const SdfLayerHandle&, const SdfPath&) const)
                (&This::GetNodeProvidingSpec),
            (args("layer"),
             args("path")))

        .def("PrintStatistics", &This::PrintStatistics)
        .def("DumpToString", &This::DumpToString,
             (args("includeInheritOriginInfo") = true,
              args("includeMaps") = true))
        .def("DumpToDotGraph", &This::DumpToDotGraph,
             (args("filename"),
              args("includeInheritOriginInfo") = true,
              args("includeMaps") = false))
        ;
}
