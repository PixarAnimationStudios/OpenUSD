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
#include "pxr/usd/usd/editTarget.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"

#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdEditTarget()
{
    boost::python::class_<UsdEditTarget>("EditTarget")
        .def(boost::python::init<SdfLayerHandle, boost::python::optional<PcpNodeRef> >(
                 (boost::python::arg("layer"), boost::python::arg("node"))))
        .def("ForLocalDirectVariant", &UsdEditTarget::ForLocalDirectVariant,
             (boost::python::arg("layer"), boost::python::arg("varSelPath")))
        .staticmethod("ForLocalDirectVariant")
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def("IsNull", &UsdEditTarget::IsNull)
        .def("IsValid", &UsdEditTarget::IsValid)
        .def("GetLayer", &UsdEditTarget::GetLayer,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetMapFunction", &UsdEditTarget::GetMapFunction,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("MapToSpecPath", &UsdEditTarget::MapToSpecPath, boost::python::arg("scenePath"))
        .def("GetPrimSpecForScenePath",
             &UsdEditTarget::GetPrimSpecForScenePath, boost::python::arg("scenePath"))
        .def("GetPropertySpecForScenePath",
             &UsdEditTarget::GetPropertySpecForScenePath, boost::python::arg("scenePath"))
        .def("GetSpecForScenePath",
             &UsdEditTarget::GetPrimSpecForScenePath, boost::python::arg("scenePath"))
        .def("ComposeOver", &UsdEditTarget::ComposeOver, boost::python::arg("weaker"))
        ;

    // Allow passing SdLayerHandle to wrapped functions expecting UsdEditTarget.
    boost::python::implicitly_convertible<SdfLayerHandle, UsdEditTarget>();
}
