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
/// \file wrapMapperArgSpec.cpp


#include "pxr/usd/sdf/mapperArgSpec.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include <boost/python.hpp>

using namespace boost::python;

void wrapMapperArgSpec()
{
    typedef SdfMapperArgSpec This;

    class_<This, SdfHandle<This>, 
           bases<SdfSpec>, boost::noncopyable>
        ("MapperArgSpec", no_init)
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&This::New,
            "__init__(ownerAttributeSpec, name, value)\n"
            "ownerMapperSpec : MapperSpec\n"
            "name : string\n"
            "value : Vt.Value\n\n"
            "Create a mapper arg spec for the given ownerMapperSpec\n"
            "with the given name and value."))

        .add_property("name",
            make_function(&This::GetName,
                return_value_policy<return_by_value>()),
            &This::SetName,
            "The name for the mapper arg.")

        .add_property("value",
            &This::GetValue,
            &This::SetValue,
            "The value for the mapper arg.")

        .add_property("mapper",
            &This::GetMapper,
            "The mapper that owns this mapper arg.")
        ;

}
