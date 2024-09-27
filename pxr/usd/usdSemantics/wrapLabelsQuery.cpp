//
// Copyright 2024 Pixar
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

#include "pxr/usd/usdSemantics/labelsQuery.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/noncopyable.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {    

std::string _Repr(const UsdSemanticsLabelsQuery& query) {
     auto timeRepr = std::visit(
          [](auto&& queryTime) -> std::string { return TfPyRepr(queryTime); },
          query.GetTime());
     return TfStringPrintf(
          "%sLabelsQuery('%s', %s)",
          TF_PY_REPR_PREFIX.c_str(),
          query.GetTaxonomy().GetText(),
          timeRepr.c_str()
     );
}

}

void wrapUsdSemanticsLabelsQuery()
{
    using This = UsdSemanticsLabelsQuery;
    class_<This, boost::noncopyable>("LabelsQuery", no_init)
        .def(init<const TfToken&, UsdTimeCode>(
             (arg("taxonomy"), arg("timeCode"))))
        .def(init<const TfToken&, const GfInterval&>(
             (arg("taxonomy"), arg("timeInterval"))))
        .def("__repr__", &::_Repr)
        .def("ComputeUniqueDirectLabels", &This::ComputeUniqueDirectLabels)
        .def("ComputeUniqueInheritedLabels",
             &This::ComputeUniqueInheritedLabels)
        .def("HasDirectLabel", &This::HasDirectLabel)
        .def("HasInheritedLabel", &This::HasInheritedLabel)
        .def("GetTaxonomy", &This::GetTaxonomy,
             return_value_policy<copy_const_reference>());
}