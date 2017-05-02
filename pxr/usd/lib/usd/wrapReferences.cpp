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
#include "pxr/usd/usd/references.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdReferences()
{
    class_<UsdReferences>("References", no_init)
        .def("AppendReference", (bool (UsdReferences::*)(const SdfReference &))
             &UsdReferences::AppendReference, arg("ref"))
        .def("AppendReference",
             (bool (UsdReferences::*)(const string &, const SdfPath &,
                                      const SdfLayerOffset &))
             &UsdReferences::AppendReference,
             (arg("assetPath"), arg("primPath"),
              arg("layerOffset")=SdfLayerOffset()))
        .def("AppendReference",(bool (UsdReferences::*)(const string &,
                                                        const SdfLayerOffset &))
             &UsdReferences::AppendReference,
             (arg("assetPath"),
              arg("layerOffset")=SdfLayerOffset()))
        .def("AppendInternalReference",
             &UsdReferences::AppendInternalReference, 
                                  (arg("primPath"),
                                   arg("layerOffset")=SdfLayerOffset()))

        .def("RemoveReference", &UsdReferences::RemoveReference, arg("ref"))
        .def("ClearReferences", &UsdReferences::ClearReferences)
        .def("SetReferences", &UsdReferences::SetReferences)
        .def("GetPrim", (UsdPrim (UsdReferences::*)()) &UsdReferences::GetPrim)
        .def(!self)
        ;
}
