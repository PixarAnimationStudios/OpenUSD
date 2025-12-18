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
    _ADD_TOKEN(cls, opacities);
    _ADD_TOKEN(cls, opacitiesh);
    _ADD_TOKEN(cls, orientations);
    _ADD_TOKEN(cls, orientationsh);
    _ADD_TOKEN(cls, perspective);
    _ADD_TOKEN(cls, positions);
    _ADD_TOKEN(cls, positionsh);
    _ADD_TOKEN(cls, projectionModeHint);
    _ADD_TOKEN(cls, radianceSphericalHarmonicsCoefficients);
    _ADD_TOKEN(cls, radianceSphericalHarmonicsCoefficientsh);
    _ADD_TOKEN(cls, radianceSphericalHarmonicsDegree);
    _ADD_TOKEN(cls, scales);
    _ADD_TOKEN(cls, scalesh);
    _ADD_TOKEN(cls, sortingModeHint);
    _ADD_TOKEN(cls, sphericalBetaBeta);
    _ADD_TOKEN(cls, tangential);
    _ADD_TOKEN(cls, zDepth);
    _ADD_TOKEN(cls, LightFieldKernelBaseAPI);
    _ADD_TOKEN(cls, LightFieldKernelConstantSurfletAPI);
    _ADD_TOKEN(cls, LightFieldKernelGaussianEllipsoidAPI);
    _ADD_TOKEN(cls, LightFieldKernelGaussianSurfletAPI);
    _ADD_TOKEN(cls, LightFieldOpacityAttributeAPI);
    _ADD_TOKEN(cls, LightFieldOrientationAttributeAPI);
    _ADD_TOKEN(cls, LightFieldPositionAttributeAPI);
    _ADD_TOKEN(cls, LightFieldPositionBaseAPI);
    _ADD_TOKEN(cls, LightFieldRadianceBaseAPI);
    _ADD_TOKEN(cls, LightFieldScaleAttributeAPI);
    _ADD_TOKEN(cls, LightFieldSphericalBetaAttributeAPI);
    _ADD_TOKEN(cls, LightFieldSphericalHarmonicsAttributeAPI);
    _ADD_TOKEN(cls, ParticleField);
    _ADD_TOKEN(cls, ParticleField3DGaussianSplat);
}
