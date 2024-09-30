//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdVol/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdVolTokens->name.GetString(); });

void wrapUsdVolTokens()
{
    pxr_boost::python::class_<UsdVolTokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, bool_);
    _ADD_TOKEN(cls, Color);
    _ADD_TOKEN(cls, double2);
    _ADD_TOKEN(cls, double3);
    _ADD_TOKEN(cls, double_);
    _ADD_TOKEN(cls, field);
    _ADD_TOKEN(cls, fieldClass);
    _ADD_TOKEN(cls, fieldDataType);
    _ADD_TOKEN(cls, fieldIndex);
    _ADD_TOKEN(cls, fieldName);
    _ADD_TOKEN(cls, fieldPurpose);
    _ADD_TOKEN(cls, filePath);
    _ADD_TOKEN(cls, float2);
    _ADD_TOKEN(cls, float3);
    _ADD_TOKEN(cls, float_);
    _ADD_TOKEN(cls, fogVolume);
    _ADD_TOKEN(cls, half);
    _ADD_TOKEN(cls, half2);
    _ADD_TOKEN(cls, half3);
    _ADD_TOKEN(cls, int2);
    _ADD_TOKEN(cls, int3);
    _ADD_TOKEN(cls, int64);
    _ADD_TOKEN(cls, int_);
    _ADD_TOKEN(cls, levelSet);
    _ADD_TOKEN(cls, mask);
    _ADD_TOKEN(cls, matrix3d);
    _ADD_TOKEN(cls, matrix4d);
    _ADD_TOKEN(cls, None_);
    _ADD_TOKEN(cls, Normal);
    _ADD_TOKEN(cls, Point);
    _ADD_TOKEN(cls, quatd);
    _ADD_TOKEN(cls, staggered);
    _ADD_TOKEN(cls, string);
    _ADD_TOKEN(cls, uint);
    _ADD_TOKEN(cls, unknown);
    _ADD_TOKEN(cls, Vector);
    _ADD_TOKEN(cls, vectorDataRoleHint);
    _ADD_TOKEN(cls, Field3DAsset);
    _ADD_TOKEN(cls, FieldAsset);
    _ADD_TOKEN(cls, FieldBase);
    _ADD_TOKEN(cls, OpenVDBAsset);
    _ADD_TOKEN(cls, Volume);
}
