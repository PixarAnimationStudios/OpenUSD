//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdr/shaderNodeQueryUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

// Python has no equivalent notion of namespaces in C++, so
// use an empty class as a namespace standin for Python.
class _SdrShaderNodeQueryUtils{};

/// Recursive function to help construct arbitrarily nested Python dicts
/// from the return value of SdrShaderNodeQueryUtils::GroupQueryResults.
nb::object
_GroupQueryResultsHelper(const VtValue& subgroup) {
    nb::dict ret;
    if (subgroup.IsHolding<SdrShaderNodeConstPtrVec>()) {
        nb::list l;
        for (auto node: subgroup.UncheckedGet<SdrShaderNodeConstPtrVec>()) {
            l.append(nb::cast(node, nb::rv_policy::reference));
        }
        return l;
    } else if (subgroup.IsHolding<VtDictionary>()) {
        const VtDictionary& innerDict = subgroup.UncheckedGet<VtDictionary>();
        for (auto& entry : innerDict) {
            ret[nb::cast(entry.first)] = _GroupQueryResultsHelper(entry.second);
        }
    } else {
        TF_VERIFY(false, "Invalid VtValue subgroup (%s) from "
                  "SdrShaderNodeQueryUtils::GroupQueryResults",
                  TfStringify(subgroup).c_str());
    }
    return ret;
}

/// Without this wrapper to call `ptr(node)`, Boost Python will attempt to
/// copy interior SdrShaderNodeConstPtr by-value as a SdrShaderNode.
nb::object
_GroupQueryResults(const SdrShaderNodeQueryResult& result) {
    VtDictionary group = SdrShaderNodeQueryUtils::GroupQueryResults(result);
    return _GroupQueryResultsHelper(VtValue(group));
}

} // anonymous namespace

void wrapShaderNodeQueryUtils(nanobind::module_ &m)
{
    nb::class_<_SdrShaderNodeQueryUtils>(m, "ShaderNodeQueryUtils")
        .def_static("GroupQueryResults", _GroupQueryResults);
}