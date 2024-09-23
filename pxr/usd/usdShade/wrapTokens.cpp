//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdShade/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdShadeTokens->name.GetString(); });

void wrapUsdShadeTokens()
{
    pxr_boost::python::class_<UsdShadeTokensType, boost::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, allPurpose);
    _ADD_TOKEN(cls, bindMaterialAs);
    _ADD_TOKEN(cls, coordSys);
    _ADD_TOKEN(cls, coordSys_MultipleApplyTemplate_Binding);
    _ADD_TOKEN(cls, displacement);
    _ADD_TOKEN(cls, fallbackStrength);
    _ADD_TOKEN(cls, full);
    _ADD_TOKEN(cls, id);
    _ADD_TOKEN(cls, infoId);
    _ADD_TOKEN(cls, infoImplementationSource);
    _ADD_TOKEN(cls, inputs);
    _ADD_TOKEN(cls, interfaceOnly);
    _ADD_TOKEN(cls, materialBind);
    _ADD_TOKEN(cls, materialBinding);
    _ADD_TOKEN(cls, materialBindingCollection);
    _ADD_TOKEN(cls, materialVariant);
    _ADD_TOKEN(cls, outputs);
    _ADD_TOKEN(cls, outputsDisplacement);
    _ADD_TOKEN(cls, outputsSurface);
    _ADD_TOKEN(cls, outputsVolume);
    _ADD_TOKEN(cls, preview);
    _ADD_TOKEN(cls, sdrMetadata);
    _ADD_TOKEN(cls, sourceAsset);
    _ADD_TOKEN(cls, sourceCode);
    _ADD_TOKEN(cls, strongerThanDescendants);
    _ADD_TOKEN(cls, subIdentifier);
    _ADD_TOKEN(cls, surface);
    _ADD_TOKEN(cls, universalRenderContext);
    _ADD_TOKEN(cls, universalSourceType);
    _ADD_TOKEN(cls, volume);
    _ADD_TOKEN(cls, weakerThanDescendants);
    _ADD_TOKEN(cls, ConnectableAPI);
    _ADD_TOKEN(cls, CoordSysAPI);
    _ADD_TOKEN(cls, Material);
    _ADD_TOKEN(cls, MaterialBindingAPI);
    _ADD_TOKEN(cls, NodeDefAPI);
    _ADD_TOKEN(cls, NodeGraph);
    _ADD_TOKEN(cls, Shader);
}
