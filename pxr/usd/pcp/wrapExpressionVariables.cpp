//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/expressionVariables.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapExpressionVariables()
{
    using This = PcpExpressionVariables;

    class_<This>("ExpressionVariables")
        .def(init<>())
        .def(init<
            const PcpExpressionVariablesSource&, 
            const VtDictionary&>())

        .def(self == self)
        .def(self != self)

        .def("Compute",
            +[](const PcpLayerStackIdentifier& sourceId,
                const PcpLayerStackIdentifier& rootId) {
                return PcpExpressionVariables::Compute(sourceId, rootId);
            },
            (arg("sourceLayerStackId"), arg("rootLayerStackId")))
        .def("Compute",
            +[](const PcpLayerStackIdentifier& sourceId,
                const PcpLayerStackIdentifier& rootId,
                const PcpExpressionVariables& overrides) {
                return PcpExpressionVariables::Compute(
                    sourceId, rootId, &overrides);
            },
            (arg("sourceLayerStackId"), arg("rootLayerStackId"),
             arg("overrideExpressionVars")))
        .staticmethod("Compute")

        .def("GetSource", 
            &This::GetSource,
            return_value_policy<return_by_value>())
        .def("GetVariables", &This::GetVariables,
            return_value_policy<return_by_value>())

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
