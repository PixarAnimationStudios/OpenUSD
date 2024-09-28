//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdUI/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdUITokens->name.GetString(); });

void wrapUsdUITokens()
{
    pxr_boost::python::class_<UsdUITokensType, boost::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, accessibility);
    _ADD_TOKEN(cls, accessibility_MultipleApplyTemplate_Description);
    _ADD_TOKEN(cls, accessibility_MultipleApplyTemplate_Label);
    _ADD_TOKEN(cls, accessibility_MultipleApplyTemplate_Priority);
    _ADD_TOKEN(cls, closed);
    _ADD_TOKEN(cls, default_);
    _ADD_TOKEN(cls, description);
    _ADD_TOKEN(cls, high);
    _ADD_TOKEN(cls, label);
    _ADD_TOKEN(cls, lang);
    _ADD_TOKEN(cls, localization);
    _ADD_TOKEN(cls, localization_MultipleApplyTemplate_Language);
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
    _ADD_TOKEN(cls, LocalizationAPI);
    _ADD_TOKEN(cls, NodeGraphNodeAPI);
    _ADD_TOKEN(cls, SceneGraphPrimAPI);
}
