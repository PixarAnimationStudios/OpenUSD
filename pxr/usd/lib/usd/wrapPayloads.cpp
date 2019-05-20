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

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdPayloads()
{
    class_<UsdPayloads>("Payloads", no_init)
        .def("AddPayload",
             (bool (UsdPayloads::*)(const SdfPayload &, UsdListPosition))
             &UsdPayloads::AddPayload,
             (arg("payload"),
              arg("position")=UsdListPositionTempDefault))
        .def("AddPayload",
             (bool (UsdPayloads::*)(const string &, const SdfPath &,
                                    const SdfLayerOffset &, UsdListPosition))
             &UsdPayloads::AddPayload,
             (arg("assetPath"), arg("primPath"),
              arg("layerOffset")=SdfLayerOffset(),
              arg("position")=UsdListPositionTempDefault))
        .def("AddPayload",(bool (UsdPayloads::*)(const string &,
                                                 const SdfLayerOffset &,
                                                 UsdListPosition))
             &UsdPayloads::AddPayload,
             (arg("assetPath"),
              arg("layerOffset")=SdfLayerOffset(),
              arg("position")=UsdListPositionTempDefault))
        .def("AddInternalPayload",
             &UsdPayloads::AddInternalPayload, 
             (arg("primPath"),
              arg("layerOffset")=SdfLayerOffset(),
              arg("position")=UsdListPositionTempDefault))

        .def("RemovePayload", &UsdPayloads::RemovePayload, arg("payload"))
        .def("ClearPayloads", &UsdPayloads::ClearPayloads)
        .def("SetPayloads", &UsdPayloads::SetPayloads)
        .def("GetPrim", (UsdPrim (UsdPayloads::*)()) &UsdPayloads::GetPrim)
        .def(!self)
        ;
}
