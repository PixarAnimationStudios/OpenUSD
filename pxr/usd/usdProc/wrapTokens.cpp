//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdProc/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdProcTokens->name.GetString(); });

void wrapUsdProcTokens()
{
    boost::python::class_<UsdProcTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _ADD_TOKEN(cls, proceduralSystem);
    _ADD_TOKEN(cls, GenerativeProcedural);
}
