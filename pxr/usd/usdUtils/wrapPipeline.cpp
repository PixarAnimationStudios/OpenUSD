//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>

#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/usd/sdf/booleanExpression.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

// XXX: Temporary workaround for use by UpdateSchemaWithSdrNode(). Once
// SdfBooleanExpression is wrapped to Python we can do away with this.
static std::string
_BooleanExpressionRenameVariables(
    const std::string& expr,
    const VtDictionary& renames)
{
    auto transform = [&](const TfToken& name) {
        const auto it = renames.find(name);
        if (it == renames.end() || !it->second.IsHolding<std::string>()) {
            return name;
        }
        else {
            return TfToken(it->second.UncheckedGet<std::string>());
        }
    };

    const SdfBooleanExpression expression(expr);
    return expression.RenameVariables(transform).GetText();
}

void wrapPipeline(nanobind::module_ &m)
{
    m.def("GetAlphaAttributeNameForColor", UsdUtilsGetAlphaAttributeNameForColor, nb::arg("colorAttrName"));
    m.def("GetModelNameFromRootLayer", UsdUtilsGetModelNameFromRootLayer);
    m.def("GetRegisteredVariantSets", 
            []() { return TfNbSequenceToList(UsdUtilsGetRegisteredVariantSets()); });
    m.def("GetPrimAtPathWithForwarding", UsdUtilsGetPrimAtPathWithForwarding, 
        nb::arg("stage"), nb::arg("path"));
    m.def("UninstancePrimAtPath", UsdUtilsUninstancePrimAtPath, 
        nb::arg("stage"), nb::arg("path"));
    m.def("GetPrimaryUVSetName", UsdUtilsGetPrimaryUVSetName,
        nb::rv_policy::copy);
    m.def("GetPrefName", UsdUtilsGetPrefName,
        nb::rv_policy::copy);
    m.def(
        "GetMaterialsScopeName",
        UsdUtilsGetMaterialsScopeName,
        nb::arg("forceDefault")=false);
    m.def(
        "GetPrimaryCameraName",
        UsdUtilsGetPrimaryCameraName,
        nb::arg("forceDefault")=false);

    m.def("_BooleanExpressionRenameVariables",
        &_BooleanExpressionRenameVariables);
}
