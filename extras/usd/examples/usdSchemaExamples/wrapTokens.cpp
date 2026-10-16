//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "./tokens.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nb::handle) { return UsdSchemaExamplesTokens->name.GetString(); });

void wrapUsdSchemaExamplesTokens(nb::module_ &m)
{
    nb::class_<UsdSchemaExamplesTokensType> cls(m, "Tokens");
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
