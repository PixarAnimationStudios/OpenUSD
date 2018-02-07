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
#include "pxr/usd/usdSkel/topology.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


tuple
_Validate(const UsdSkelTopology& self)
{
    std::string reason;
    bool success = self.Validate(&reason);
    return boost::python::make_tuple(success, reason);
}


} // namespace


void wrapUsdSkelTopology()
{
    using This = UsdSkelTopology;

    class_<This>("Topology", no_init)
        .def(init<SdfPathVector>())
        .def(init<VtIntArray>())

        .def("GetParentIndices", &This::GetParentIndices,
             return_value_policy<return_by_value>())
        
        .def("GetNumJoints", &This::GetNumJoints)

        .def("__len__", &This::GetNumJoints)

        .def("Validate", &This::Validate)
        ;
}            
