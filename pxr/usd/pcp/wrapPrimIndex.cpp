//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/siteUtils.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static pxr_boost::python::tuple
_ComputePrimChildNames( PcpPrimIndex &index )
{
    TfTokenVector nameOrder;
    PcpTokenSet prohibitedNameSet;
    index.ComputePrimChildNames(&nameOrder, &prohibitedNameSet);
    TfTokenVector prohibitedNamesVector(prohibitedNameSet.begin(),
                                        prohibitedNameSet.end());
    return pxr_boost::python::make_tuple(nameOrder, prohibitedNamesVector);
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
                      make_function(&PcpComputePrimStackForPrimIndex,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("rootNode", &This::GetRootNode)
        .add_property("hasAnyPayloads", &This::HasAnyPayloads)
        .add_property("localErrors", 
                      make_function(&This::GetLocalErrors,
                                    return_value_policy<TfPySequenceToList>()))

        .def("IsValid", &This::IsValid)
        .def("IsUsd", &This::IsUsd)
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
