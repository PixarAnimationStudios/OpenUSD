//
// Copyright 2019 Pixar
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
#include "pxr/usd/usd/payloads.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>



PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdPayloads()
{
    boost::python::class_<UsdPayloads>("Payloads", boost::python::no_init)
        .def("AddPayload",
             (bool (UsdPayloads::*)(const SdfPayload &, UsdListPosition))
             &UsdPayloads::AddPayload,
             (boost::python::arg("payload"),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))
        .def("AddPayload",
             (bool (UsdPayloads::*)(const std::string &, const SdfPath &,
                                    const SdfLayerOffset &, UsdListPosition))
             &UsdPayloads::AddPayload,
             (boost::python::arg("assetPath"), boost::python::arg("primPath"),
              boost::python::arg("layerOffset")=SdfLayerOffset(),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))
        .def("AddPayload",(bool (UsdPayloads::*)(const std::string &,
                                                 const SdfLayerOffset &,
                                                 UsdListPosition))
             &UsdPayloads::AddPayload,
             (boost::python::arg("assetPath"),
              boost::python::arg("layerOffset")=SdfLayerOffset(),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))
        .def("AddInternalPayload",
             &UsdPayloads::AddInternalPayload, 
             (boost::python::arg("primPath"),
              boost::python::arg("layerOffset")=SdfLayerOffset(),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))

        .def("RemovePayload", &UsdPayloads::RemovePayload, boost::python::arg("payload"))
        .def("ClearPayloads", &UsdPayloads::ClearPayloads)
        .def("SetPayloads", &UsdPayloads::SetPayloads)
        .def("GetPrim", (UsdPrim (UsdPayloads::*)()) &UsdPayloads::GetPrim)
        .def(!boost::python::self)
        ;
}
