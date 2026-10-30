//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/sdf/siteUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/sdf/pySpec.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static nanobind::tuple
_ComputePrimChildNames( PcpPrimIndex &index )
{
    TfTokenVector nameOrder;
    PcpTokenSet prohibitedNameSet;
    index.ComputePrimChildNames(&nameOrder, &prohibitedNameSet);
    TfTokenVector prohibitedNamesVector(prohibitedNameSet.begin(),
                                        prohibitedNameSet.end());
    return nanobind::make_tuple(nameOrder, prohibitedNamesVector);
}

static TfTokenVector
_ComputePrimPropertyNames( PcpPrimIndex &index )
{
    TfTokenVector result;
    index.ComputePrimPropertyNames(&result);
    return result;
}

} // anonymous namespace 

void wrapPrimIndex(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef PcpPrimIndex This;

    nb::class_<This>(m, "PrimIndex")
        .def_prop_ro("primStack", &PcpComputePrimStackForPrimIndex)
        .def_prop_ro("rootNode", &This::GetRootNode)
        .def_prop_ro("hasAnyPayloads", &This::HasAnyPayloads)
        .def_prop_ro("localErrors", &This::GetLocalErrors)

        .def("IsValid", &This::IsValid)
        .def("IsUsd", &This::IsUsd)
        .def("IsInstanceable", &This::IsInstanceable)

        .def("ComputePrimChildNames", &_ComputePrimChildNames)
        .def("ComputePrimPropertyNames", &_ComputePrimPropertyNames)
        .def("ComposeAuthoredVariantSelections",
             &This::ComposeAuthoredVariantSelections)
        .def("GetSelectionAppliedForVariantSet",
            &This::GetSelectionAppliedForVariantSet)

        .def("GetNodeProvidingSpec", 
            (PcpNodeRef (This::*) (const SdfPrimSpecHandle&) const)
                (&This::GetNodeProvidingSpec),
            nb::arg("primSpec"))
        .def("GetNodeProvidingSpec", 
            (PcpNodeRef (This::*) (const SdfLayerHandle&, const SdfPath&) const)
                (&This::GetNodeProvidingSpec),
            nb::arg("layer"),
            nb::arg("path"))

        .def("PrintStatistics", &This::PrintStatistics)
        .def("DumpToString", &This::DumpToString,
             nb::arg("includeInheritOriginInfo") = true,
             nb::arg("includeMaps") = true)
        .def("DumpToDotGraph", &This::DumpToDotGraph,
             nb::arg("filename"),
             nb::arg("includeInheritOriginInfo") = true,
             nb::arg("includeMaps") = false)
        ;

    m.def("_CheckConsistency", &Pcp_CheckConsistency);
}
