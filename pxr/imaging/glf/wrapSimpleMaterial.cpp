//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/simpleMaterial.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
