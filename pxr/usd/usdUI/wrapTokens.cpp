//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "pxr/usd/usdUI/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nanobind::handle) { return UsdUITokens->name.GetString(); }, nanobind::rv_policy::copy);

void wrapUsdUITokens(nanobind::module_ &m)
{
    nanobind::class_<UsdUITokensType> cls(m, "Tokens");
    cls.def("__init__", [](nanobind::args, nanobind::kwargs) {
        TfPyThrowRuntimeError("This class cannot be instantiated from Python");
    });
    _ADD_TOKEN(cls, accessibility);
    _ADD_TOKEN(cls, accessibility_MultipleApplyTemplate_Description);
    _ADD_TOKEN(cls, accessibility_MultipleApplyTemplate_Label);
    _ADD_TOKEN(cls, accessibility_MultipleApplyTemplate_Priority);
    _ADD_TOKEN(cls, closed);
    _ADD_TOKEN(cls, default_);
    _ADD_TOKEN(cls, description);
    _ADD_TOKEN(cls, high);
    _ADD_TOKEN(cls, label);
    _ADD_TOKEN(cls, low);
    _ADD_TOKEN(cls, minimized);
    _ADD_TOKEN(cls, open);
    _ADD_TOKEN(cls, priority);
    _ADD_TOKEN(cls, standard);
    _ADD_TOKEN(cls, uiDescription);
    _ADD_TOKEN(cls, uiDisplayGroup);
    _ADD_TOKEN(cls, uiDisplayName);
    _ADD_TOKEN(cls, uiNodegraphNodeDisplayColor);
    _ADD_TOKEN(cls, uiNodegraphNodeDocURI);
    _ADD_TOKEN(cls, uiNodegraphNodeExpansionState);
    _ADD_TOKEN(cls, uiNodegraphNodeIcon);
    _ADD_TOKEN(cls, uiNodegraphNodePos);
    _ADD_TOKEN(cls, uiNodegraphNodeSize);
    _ADD_TOKEN(cls, uiNodegraphNodeStackingOrder);
    _ADD_TOKEN(cls, AccessibilityAPI);
    _ADD_TOKEN(cls, Backdrop);
    _ADD_TOKEN(cls, NodeGraphNodeAPI);
    _ADD_TOKEN(cls, SceneGraphPrimAPI);
}
