//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdHydra/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdHydraTokens->name.GetString(); });

void wrapUsdHydraTokens()
{
    pxr_boost::python::class_<UsdHydraTokensType, boost::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, black);
    _ADD_TOKEN(cls, clamp);
    _ADD_TOKEN(cls, displayLookBxdf);
    _ADD_TOKEN(cls, faceIndex);
    _ADD_TOKEN(cls, faceOffset);
    _ADD_TOKEN(cls, frame);
    _ADD_TOKEN(cls, HwPrimvar_1);
    _ADD_TOKEN(cls, HwPtexTexture_1);
    _ADD_TOKEN(cls, HwUvTexture_1);
    _ADD_TOKEN(cls, hydraGenerativeProcedural);
    _ADD_TOKEN(cls, infoFilename);
    _ADD_TOKEN(cls, infoVarname);
    _ADD_TOKEN(cls, linear);
    _ADD_TOKEN(cls, linearMipmapLinear);
    _ADD_TOKEN(cls, linearMipmapNearest);
    _ADD_TOKEN(cls, magFilter);
    _ADD_TOKEN(cls, minFilter);
    _ADD_TOKEN(cls, mirror);
    _ADD_TOKEN(cls, nearest);
    _ADD_TOKEN(cls, nearestMipmapLinear);
    _ADD_TOKEN(cls, nearestMipmapNearest);
    _ADD_TOKEN(cls, primvarsHdGpProceduralType);
    _ADD_TOKEN(cls, proceduralSystem);
    _ADD_TOKEN(cls, repeat);
    _ADD_TOKEN(cls, textureMemory);
    _ADD_TOKEN(cls, useMetadata);
    _ADD_TOKEN(cls, uv);
    _ADD_TOKEN(cls, wrapS);
    _ADD_TOKEN(cls, wrapT);
    _ADD_TOKEN(cls, HydraGenerativeProceduralAPI);
}
