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

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

void wrapSimpleLight()
{
    typedef GlfSimpleLight This;

    class_<This> ("SimpleLight", init<>() )
        .add_property("transform",
                      make_function(
                          &This::GetTransform,
                          return_value_policy<return_by_value>()),
                      &This::SetTransform)
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
        .add_property("position",
                      make_function(
                          &This::GetPosition,
                          return_value_policy<return_by_value>()),
                      &This::SetPosition)
        .add_property("spotDirection",
                      make_function(
                          &This::GetSpotDirection,
                          return_value_policy<return_by_value>()),
                      &This::SetSpotDirection)
        .add_property("spotCutoff",
                      make_function(
                          &This::GetSpotCutoff,
                          return_value_policy<return_by_value>()),
                      &This::SetSpotCutoff)
        .add_property("spotFalloff",
                      make_function(
                          &This::GetSpotFalloff,
                          return_value_policy<return_by_value>()),
                      &This::SetSpotFalloff)
        .add_property("attenuation",
                      make_function(
                          &This::GetAttenuation,
                          return_value_policy<return_by_value>()),
                      &This::SetAttenuation)
        .add_property("shadowMatrix",
                      make_function(
                          &This::GetShadowMatrix,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowMatrix)
        .add_property("shadowResolution",
                      make_function(
                          &This::GetShadowResolution,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowResolution)
        .add_property("shadowBias",
                      make_function(
                          &This::GetShadowBias,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowBias)
        .add_property("shadowBlur",
                      make_function(
                          &This::GetShadowBlur,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowBlur)
        .add_property("shadowIndex",
                      make_function(
                          &This::GetShadowIndex,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowIndex)
        .add_property("hasShadow",
                      make_function(
                          &This::HasShadow,
                          return_value_policy<return_by_value>()),
                      &This::SetHasShadow)
        .add_property("isCameraSpaceLight",
                      make_function(
                          &This::IsCameraSpaceLight,
                          return_value_policy<return_by_value>()),
                      &This::SetIsCameraSpaceLight)
        .add_property("id",
                      make_function(
                          &This::GetID,
                          return_value_policy<return_by_value>()),
                      &This::SetID)
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE

