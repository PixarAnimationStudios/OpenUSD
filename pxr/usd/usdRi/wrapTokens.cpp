//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usdRi/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, +[](nb::object) { return UsdRiTokens->name.GetString(); });

void wrapUsdRiTokens(nb::module_ &m)
{
    nb::class_<UsdRiTokensType> cls(m, "Tokens");
    _ADD_TOKEN(cls, bspline);
    _ADD_TOKEN(cls, cameraVisibility);
    _ADD_TOKEN(cls, catmullRom);
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
    _ADD_TOKEN(cls, RiSplineAPI);
    _ADD_TOKEN(cls, StatementsAPI);
}
