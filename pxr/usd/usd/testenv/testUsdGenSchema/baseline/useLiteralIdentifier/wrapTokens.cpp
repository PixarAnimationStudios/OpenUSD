//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdContrived/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdContrivedTokens->name.GetString(); });

void wrapUsdContrivedTokens()
{
    pxr_boost::python::class_<UsdContrivedTokensType, boost::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, libraryToken1);
    _ADD_TOKEN(cls, libraryToken2);
    _ADD_TOKEN(cls, myColorFloat);
    _ADD_TOKEN(cls, myDouble);
    _ADD_TOKEN(cls, myFloat);
    _ADD_TOKEN(cls, myNormals);
    _ADD_TOKEN(cls, myPoints);
    _ADD_TOKEN(cls, myVaryingToken);
    _ADD_TOKEN(cls, myVaryingTokenArray);
    _ADD_TOKEN(cls, myVelocities);
    _ADD_TOKEN(cls, unsignedChar);
    _ADD_TOKEN(cls, unsignedInt);
    _ADD_TOKEN(cls, unsignedInt64Array);
    _ADD_TOKEN(cls, variableTokenAllowed1);
    _ADD_TOKEN(cls, variableTokenAllowed2);
    _ADD_TOKEN(cls, variableTokenAllowed3);
    _ADD_TOKEN(cls, variableTokenArrayAllowed1);
    _ADD_TOKEN(cls, variableTokenArrayAllowed2);
    _ADD_TOKEN(cls, variableTokenArrayAllowed3);
    _ADD_TOKEN(cls, variableTokenDefault);
    _ADD_TOKEN(cls, Base);
}
