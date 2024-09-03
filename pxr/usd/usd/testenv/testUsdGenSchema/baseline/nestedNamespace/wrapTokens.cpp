//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdContrived/tokens.h"

using namespace foo::bar::baz;

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdContrivedTokens->name.GetString(); });

void wrapUsdContrivedTokens()
{
    boost::python::class_<UsdContrivedTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
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
    _ADD_TOKEN(cls, testAttrOne);
    _ADD_TOKEN(cls, testAttrTwo);
    _ADD_TOKEN(cls, unsignedChar);
    _ADD_TOKEN(cls, unsignedInt);
    _ADD_TOKEN(cls, unsignedInt64Array);
    _ADD_TOKEN(cls, VariableTokenAllowed1);
    _ADD_TOKEN(cls, VariableTokenAllowed2);
    _ADD_TOKEN(cls, VariableTokenAllowed_3_);
    _ADD_TOKEN(cls, VariableTokenArrayAllowed1);
    _ADD_TOKEN(cls, VariableTokenArrayAllowed2);
    _ADD_TOKEN(cls, VariableTokenArrayAllowed_3_);
    _ADD_TOKEN(cls, VariableTokenDefault);
    _ADD_TOKEN(cls, Base);
    _ADD_TOKEN(cls, SingleApplyAPI);
}
