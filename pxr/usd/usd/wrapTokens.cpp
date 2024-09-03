//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usd/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdTokens->name.GetString(); });

void wrapUsdTokens()
{
    boost::python::class_<UsdTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _ADD_TOKEN(cls, apiSchemas);
    _ADD_TOKEN(cls, clips);
    _ADD_TOKEN(cls, clipSets);
    _ADD_TOKEN(cls, collection);
    _ADD_TOKEN(cls, collection_MultipleApplyTemplate_);
    _ADD_TOKEN(cls, collection_MultipleApplyTemplate_Excludes);
    _ADD_TOKEN(cls, collection_MultipleApplyTemplate_ExpansionRule);
    _ADD_TOKEN(cls, collection_MultipleApplyTemplate_IncludeRoot);
    _ADD_TOKEN(cls, collection_MultipleApplyTemplate_Includes);
    _ADD_TOKEN(cls, collection_MultipleApplyTemplate_MembershipExpression);
    _ADD_TOKEN(cls, exclude);
    _ADD_TOKEN(cls, expandPrims);
    _ADD_TOKEN(cls, expandPrimsAndProperties);
    _ADD_TOKEN(cls, explicitOnly);
    _ADD_TOKEN(cls, fallbackPrimTypes);
    _ADD_TOKEN(cls, APISchemaBase);
    _ADD_TOKEN(cls, ClipsAPI);
    _ADD_TOKEN(cls, CollectionAPI);
    _ADD_TOKEN(cls, ModelAPI);
    _ADD_TOKEN(cls, Typed);
}
