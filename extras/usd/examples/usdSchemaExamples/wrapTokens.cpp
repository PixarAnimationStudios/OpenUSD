//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "./tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdSchemaExamplesTokens->name.GetString(); });

void wrapUsdSchemaExamplesTokens()
{
    pxr_boost::python::class_<UsdSchemaExamplesTokensType, boost::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, complexString);
    _ADD_TOKEN(cls, intAttr);
    _ADD_TOKEN(cls, paramsMass);
    _ADD_TOKEN(cls, paramsVelocity);
    _ADD_TOKEN(cls, paramsVolume);
    _ADD_TOKEN(cls, target);
    _ADD_TOKEN(cls, ComplexPrim);
    _ADD_TOKEN(cls, ParamsAPI);
    _ADD_TOKEN(cls, SimplePrim);
}
