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
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/siteUtils.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

static SdfPrimSpecHandleVector
_GetPrimStack(const PcpPrimIndex& self)
{
    const PcpPrimRange primRange = self.GetPrimRange();

    SdfPrimSpecHandleVector primStack;
    primStack.reserve(std::distance(primRange.first, primRange.second));
    TF_FOR_ALL(it, primRange) {
        primStack.push_back(SdfGetPrimAtPath(*it));
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

void wrapPrimIndex()
{
    typedef PcpPrimIndex This;

    class_<This>("PrimIndex", "", no_init)
        .add_property("primStack", 
                      make_function(&_GetPrimStack,
                                    return_value_policy<TfPySequenceToList>()))
        .add_property("rootNode", &This::GetRootNode)
        .add_property("hasPayload", &This::HasPayload)
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
