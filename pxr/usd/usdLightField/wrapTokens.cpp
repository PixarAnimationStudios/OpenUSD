//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdLightField/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdLightFieldTokens->name.GetString(); });

void wrapUsdLightFieldTokens()
{
    pxr_boost::python::class_<UsdLightFieldTokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, cameraDistance);
    _ADD_TOKEN(cls, elipsoid);
    _ADD_TOKEN(cls, gaussianShape);
    _ADD_TOKEN(cls, orientations);
    _ADD_TOKEN(cls, orientationsf);
    _ADD_TOKEN(cls, perspective);
    _ADD_TOKEN(cls, plane);
    _ADD_TOKEN(cls, primvarsSphericalHarmonics);
    _ADD_TOKEN(cls, primvarsSphericalHarmonicsf);
    _ADD_TOKEN(cls, projectionModeHint);
    _ADD_TOKEN(cls, scales);
    _ADD_TOKEN(cls, scalesf);
    _ADD_TOKEN(cls, sortingModeHint);
    _ADD_TOKEN(cls, sphericalHarmonicsColorSpace);
    _ADD_TOKEN(cls, sRGB);
    _ADD_TOKEN(cls, tangential);
    _ADD_TOKEN(cls, triangle);
    _ADD_TOKEN(cls, zDepth);
    _ADD_TOKEN(cls, GaussiansAPI);
    _ADD_TOKEN(cls, SphericalHarmonicsAPI);
}
