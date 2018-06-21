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
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/ndr/node.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapNode()
{
    typedef NdrNode This;
    typedef NdrNodePtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, ThisPtr, boost::noncopyable>("Node", no_init)
        .def("__repr__", &This::GetInfoString)
        .def("__nonzero__", &This::IsValid)
        .def("GetIdentifier", &This::GetIdentifier, copyRefPolicy)
        .def("GetVersion", &This::GetVersion)
        .def("GetName", &This::GetName, copyRefPolicy)
        .def("GetFamily", &This::GetFamily, copyRefPolicy)
        .def("GetContext", &This::GetContext, copyRefPolicy)
        .def("GetSourceType", &This::GetSourceType, copyRefPolicy)
        .def("GetSourceURI", &This::GetSourceURI, copyRefPolicy)
        .def("IsValid", &This::IsValid)
        .def("GetInfoString", &This::GetInfoString)
        .def("GetInput", &This::GetInput, return_internal_reference<>())
        .def("GetInputNames", &This::GetInputNames, copyRefPolicy)
        .def("GetOutput", &This::GetOutput, return_internal_reference<>())
        .def("GetOutputNames", &This::GetOutputNames, copyRefPolicy)
        .def("GetMetadata", &This::GetMetadata,
            return_value_policy<TfPyMapToDictionary>())
        ;
}
