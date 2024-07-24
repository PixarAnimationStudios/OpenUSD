//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/simpleLight.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
        .add_property("shadowMatrices",
                      make_function(
                          &This::GetShadowMatrices,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowMatrices)
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
        .add_property("shadowIndexStart",
                      make_function(
                          &This::GetShadowIndexStart,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowIndexStart)
        .add_property("shadowIndexEnd",
                      make_function(
                          &This::GetShadowIndexEnd,
                          return_value_policy<return_by_value>()),
                      &This::SetShadowIndexEnd)
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
        .add_property("isDomeLight",
                      make_function(
                          &This::IsDomeLight,
                          return_value_policy<return_by_value>()),
                      &This::SetIsDomeLight)
        ;
}
