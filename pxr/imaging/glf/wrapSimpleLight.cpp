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
#include "pxr/imaging/glf/simpleLight.h"

#include <boost/python/class.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

void wrapSimpleLight()
{
    typedef GlfSimpleLight This;

    boost::python::class_<This> ("SimpleLight", boost::python::init<>() )
        .add_property("transform",
                      boost::python::make_function(
                          &This::GetTransform,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetTransform)
        .add_property("ambient",
                      boost::python::make_function(
                          &This::GetAmbient,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetAmbient)
        .add_property("diffuse",
                      boost::python::make_function(
                          &This::GetDiffuse,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetDiffuse)
        .add_property("specular",
                      boost::python::make_function(
                          &This::GetSpecular,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetSpecular)
        .add_property("position",
                      boost::python::make_function(
                          &This::GetPosition,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetPosition)
        .add_property("spotDirection",
                      boost::python::make_function(
                          &This::GetSpotDirection,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetSpotDirection)
        .add_property("spotCutoff",
                      boost::python::make_function(
                          &This::GetSpotCutoff,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetSpotCutoff)
        .add_property("spotFalloff",
                      boost::python::make_function(
                          &This::GetSpotFalloff,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetSpotFalloff)
        .add_property("attenuation",
                      boost::python::make_function(
                          &This::GetAttenuation,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetAttenuation)
        .add_property("shadowMatrices",
                      boost::python::make_function(
                          &This::GetShadowMatrices,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetShadowMatrices)
        .add_property("shadowResolution",
                      boost::python::make_function(
                          &This::GetShadowResolution,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetShadowResolution)
        .add_property("shadowBias",
                      boost::python::make_function(
                          &This::GetShadowBias,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetShadowBias)
        .add_property("shadowBlur",
                      boost::python::make_function(
                          &This::GetShadowBlur,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetShadowBlur)
        .add_property("shadowIndexStart",
                      boost::python::make_function(
                          &This::GetShadowIndexStart,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetShadowIndexStart)
        .add_property("shadowIndexEnd",
                      boost::python::make_function(
                          &This::GetShadowIndexEnd,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetShadowIndexEnd)
        .add_property("hasShadow",
                      boost::python::make_function(
                          &This::HasShadow,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetHasShadow)
        .add_property("isCameraSpaceLight",
                      boost::python::make_function(
                          &This::IsCameraSpaceLight,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetIsCameraSpaceLight)
        .add_property("id",
                      boost::python::make_function(
                          &This::GetID,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetID)
        .add_property("isDomeLight",
                      boost::python::make_function(
                          &This::IsDomeLight,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
                      &This::SetIsDomeLight)
        ;
}
