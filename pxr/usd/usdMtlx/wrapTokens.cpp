//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdMtlx/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdMtlxTokens->name.GetString(); });

void wrapUsdMtlxTokens()
{
    pxr_boost::python::class_<UsdMtlxTokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, configMtlxVersion);
    _ADD_TOKEN(cls, DefaultOutputName);
    _ADD_TOKEN(cls, MaterialXConfigAPI);
}
