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
    _ADD_TOKEN(cls, kernelFalloffImplicitShapeFalloffThreshold);
    _ADD_TOKEN(cls, kernelShapeTriangleEdgeLength);
    _ADD_TOKEN(cls, opacities);
    _ADD_TOKEN(cls, opacitiesh);
    _ADD_TOKEN(cls, orientations);
    _ADD_TOKEN(cls, orientationsh);
    _ADD_TOKEN(cls, perspective);
    _ADD_TOKEN(cls, positions);
    _ADD_TOKEN(cls, positionsh);
    _ADD_TOKEN(cls, primvarsRadianceSphericalHarmonicsCoefficients);
    _ADD_TOKEN(cls, primvarsRadianceSphericalHarmonicsCoefficientsh);
    _ADD_TOKEN(cls, primvarsSphericalBetaBeta);
    _ADD_TOKEN(cls, projectionModeHint);
    _ADD_TOKEN(cls, radianceSphericalHarmonicsDegree);
    _ADD_TOKEN(cls, scales);
    _ADD_TOKEN(cls, scalesh);
    _ADD_TOKEN(cls, sortingModeHint);
    _ADD_TOKEN(cls, tangential);
    _ADD_TOKEN(cls, zDepth);
    _ADD_TOKEN(cls, GaussianFalloffFunctionAPI);
    _ADD_TOKEN(cls, GaussianShapeAPI);
    _ADD_TOKEN(cls, ImplicitShapeFalloffThresholdAPI);
    _ADD_TOKEN(cls, OpacityAttributeAPI);
    _ADD_TOKEN(cls, OrientationAttributeAPI);
    _ADD_TOKEN(cls, ParticleField);
    _ADD_TOKEN(cls, ParticleField_3DGaussianSplat);
    _ADD_TOKEN(cls, PositionAttributeAPI);
    _ADD_TOKEN(cls, ScaleAttributeAPI);
    _ADD_TOKEN(cls, SphericalBetaAttributeAPI);
    _ADD_TOKEN(cls, SphericalHarmonicsAttributeAPI);
    _ADD_TOKEN(cls, TriangleShapeAPI);
}
