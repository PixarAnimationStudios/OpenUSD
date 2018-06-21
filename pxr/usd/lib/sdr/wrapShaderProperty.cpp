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
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

struct TfTokenPairToPythonConverter
{
    static PyObject* convert(const std::pair<TfToken, TfToken>& pair)
    {
        return incref(make_tuple(pair.first, pair.second).ptr());
    }
};

void wrapShaderProperty()
{
    typedef SdrShaderProperty This;
    typedef SdrShaderPropertyPtr ThisPtr;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "PropertyTypes", SdrPropertyTypes, SDR_PROPERTY_TYPE_TOKENS
    );

    to_python_converter<NdrOption, TfTokenPairToPythonConverter>();
    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, ThisPtr, bases<NdrProperty>,
           boost::noncopyable>("ShaderProperty", no_init)
        .def("GetLabel", &This::GetLabel, copyRefPolicy)
        .def("GetHelp", &This::GetHelp, copyRefPolicy)
        .def("GetPage", &This::GetPage, copyRefPolicy)
        .def("GetWidget", &This::GetWidget, copyRefPolicy)
        .def("GetHints", &This::GetHints,
            return_value_policy<TfPyMapToDictionary>())
        .def("GetOptions", &This::GetOptions,
            return_value_policy<TfPySequenceToList>())
        .def("GetImplementationName",
            &This::GetImplementationName, copyRefPolicy)
        .def("GetVStructMemberOf", &This::GetVStructMemberOf, copyRefPolicy)
        .def("GetVStructMemberName", &This::GetVStructMemberName, copyRefPolicy)
        .def("IsVStructMember", &This::IsVStructMember)
        .def("IsVStruct", &This::IsVStruct)
        .def("GetValidConnectionTypes", &This::GetValidConnectionTypes,
            copyRefPolicy)
        .def("IsAssetIdentifier", &This::IsAssetIdentifier)
        ;
}
