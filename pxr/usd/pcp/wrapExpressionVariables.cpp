//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/expressionVariables.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapExpressionVariables(nanobind::module_ &m)
{
    namespace nb = nanobind;
    using This = PcpExpressionVariables;

    nb::class_<This>(m, "ExpressionVariables")
        .def(nb::init<>())
        .def(nb::init<
            const PcpExpressionVariablesSource&, 
            const VtDictionary&>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def_static("Compute",
            +[](const PcpLayerStackIdentifier& sourceId,
                const PcpLayerStackIdentifier& rootId) {
                return PcpExpressionVariables::Compute(sourceId, rootId);
            },
            nb::arg("sourceLayerStackId"), nb::arg("rootLayerStackId"))
        .def_static("Compute",
            +[](const PcpLayerStackIdentifier& sourceId,
                const PcpLayerStackIdentifier& rootId,
                const PcpExpressionVariables& overrides) {
                return PcpExpressionVariables::Compute(
                    sourceId, rootId, &overrides);
            },
            nb::arg("sourceLayerStackId"), nb::arg("rootLayerStackId"),
            nb::arg("overrideExpressionVars"))

        .def("GetSource", 
            &This::GetSource,
            nb::rv_policy::copy)
        .def("GetVariables", &This::GetVariables,
            nb::rv_policy::copy)

        .def("__repr__",
            +[](const This& s) {
                if (s == PcpExpressionVariables()) {
                    return TfStringPrintf(
                        "%sExpressionVariables()", TF_PY_REPR_PREFIX.c_str());
                }
                return TfStringPrintf(
                    "%sExpressionVariables(%s, %s)", 
                    TF_PY_REPR_PREFIX.c_str(),
                    TfPyRepr(s.GetSource()).c_str(),
                    TfPyRepr(s.GetVariables()).c_str());
            })
        ;

}
