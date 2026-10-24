//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/expressionVariablesSource.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapExpressionVariablesSource(nanobind::module_ &m)
{
    namespace nb = nanobind;
    using This = PcpExpressionVariablesSource;

    nb::class_<This>(m, "ExpressionVariablesSource")
        .def(nb::init<>())
        .def(nb::init<
                const PcpLayerStackIdentifier&,
                const PcpLayerStackIdentifier&>(),
             nb::arg("layerStackId"), nb::arg("rootLayerStackId"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("IsRootLayerStack", &This::IsRootLayerStack)
        .def("GetLayerStackIdentifier", 
            +[](const This& s) -> nb::object { 
                const PcpLayerStackIdentifier* id = s.GetLayerStackIdentifier();
                return id ? nb::cast(*id) : nb::none();
            })
        .def("ResolveLayerStackIdentifier",
            (const PcpLayerStackIdentifier& (This::*)(
                const PcpLayerStackIdentifier&) const)
                &This::ResolveLayerStackIdentifier,
            nb::rv_policy::copy)

        .def("ResolveLayerStackIdentifier",
            (const PcpLayerStackIdentifier& (This::*)(const PcpCache&) const)
                &This::ResolveLayerStackIdentifier,
            nb::rv_policy::copy)

        .def("__repr__",
            +[](const PcpExpressionVariablesSource &s) {
                return TfStringPrintf(
                    "%sExpressionVariablesSource(%s)",
                    TF_PY_REPR_PREFIX.c_str(),
                    s.IsRootLayerStack() ? 
                    "" : TfPyRepr(*s.GetLayerStackIdentifier()).c_str());
            })
        ;

}
