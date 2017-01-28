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
#include "pxr/imaging/glf/simpleMaterial.h"

#include <boost/python/class.hpp>

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

void wrapSimpleMaterial()
{
    typedef GlfSimpleMaterial This;

    class_<This> ("SimpleMaterial", init<>())
        .add_property("ambient",
                      make_function(
                          &This::GetAmbient,
                          return_value_policy<return_by_value>()),
                      &This::SetAmbient)
        .add_property("diffuse",
                      make_function(
                          &This::GetDiffuse,
                          return_value_policy<return_by_value>()),
                      &This::SetDiffuse)
        .add_property("specular",
                      make_function(
                          &This::GetSpecular,
                          return_value_policy<return_by_value>()),
                      &This::SetSpecular)
        .add_property("emission",
                      make_function(
                          &This::GetEmission,
                          return_value_policy<return_by_value>()),
                      &This::SetEmission)
        .add_property("shininess",
                      make_function(
                          &This::GetShininess,
                          return_value_policy<return_by_value>()),
                      &This::SetShininess)
        ;
}


PXR_NAMESPACE_CLOSE_SCOPE

