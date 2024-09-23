//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdRi/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdRiTokens->name.GetString(); });

void wrapUsdRiTokens()
{
    pxr_boost::python::class_<UsdRiTokensType, boost::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, bspline);
    _ADD_TOKEN(cls, cameraVisibility);
    _ADD_TOKEN(cls, catmullRom);
    _ADD_TOKEN(cls, collectionCameraVisibilityIncludeRoot);
    _ADD_TOKEN(cls, constant);
    _ADD_TOKEN(cls, interpolation);
    _ADD_TOKEN(cls, linear);
    _ADD_TOKEN(cls, matte);
    _ADD_TOKEN(cls, outputsRiDisplacement);
    _ADD_TOKEN(cls, outputsRiSurface);
    _ADD_TOKEN(cls, outputsRiVolume);
    _ADD_TOKEN(cls, positions);
    _ADD_TOKEN(cls, renderContext);
    _ADD_TOKEN(cls, spline);
    _ADD_TOKEN(cls, values);
    _ADD_TOKEN(cls, RiMaterialAPI);
    _ADD_TOKEN(cls, RiRenderPassAPI);
    _ADD_TOKEN(cls, RiSplineAPI);
    _ADD_TOKEN(cls, StatementsAPI);
}
