//
// Copyright 2018 Pixar
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
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/ndr/property.h"
#include "pxr/usd/sdf/types.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapProperty()
{
    typedef NdrProperty This;
    typedef NdrPropertyPtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, ThisPtr, boost::noncopyable>("Property", no_init)
        .def("__repr__", &This::GetInfoString)
        .def("GetName", &This::GetName, copyRefPolicy)
        .def("GetType", &This::GetType, copyRefPolicy)
        .def("GetDefaultValue", &This::GetDefaultValue, copyRefPolicy)
        .def("IsOutput", &This::IsOutput)
        .def("IsArray", &This::IsArray)
        .def("IsDynamicArray", &This::IsDynamicArray)
        .def("GetArraySize", &This::GetArraySize)
        .def("GetInfoString", &This::GetInfoString)
        .def("GetMetadata", &This::GetMetadata,
            return_value_policy<TfPyMapToDictionary>())
        .def("IsConnectable", &This::IsConnectable)
        .def("CanConnectTo", &This::CanConnectTo)
        .def("GetTypeAsSdfType", &This::GetTypeAsSdfType,
            return_value_policy<TfPyPairToTuple>())
        ;
}
